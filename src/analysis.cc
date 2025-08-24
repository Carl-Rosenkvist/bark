#include "analysis.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include "analysisregister.h"

// YAML serialization
void to_yaml(YAML::Emitter& out, const MergeKeyValue& v) {
    std::visit([&](const auto& val) {
        out << val;
    }, v);
}

// Analysis methods
Analysis& Analysis::operator+=(const Analysis& other) {
    if (this->keys != other.get_merge_keys()) {
        throw std::runtime_error("Cannot merge Analysis objects: MergeKey mismatch.");
    }

    this->dataNode += other.dataNode;
    return *this;
}

void Analysis::set_merge_keys(MergeKeySet k) {
    keys = std::move(k);
}

const MergeKeySet& Analysis::get_merge_keys() const {
    return keys;
}

void Analysis::on_header(Header& header) {
    smash_version = header.smash_version;
}

void Analysis::save_as_yaml(const std::string& filename) const {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "smash_version" << YAML::Value << smash_version;

    out << YAML::Key << "merge_keys" << YAML::Value << YAML::BeginMap;
    for (const auto& [k, v] : keys) {
        out << YAML::Key << k << YAML::Value;
        to_yaml(out, v);
    }
    out << YAML::EndMap;

    out << YAML::Key << "data" << YAML::Value;
    if (dataNode.empty()) {
        out << YAML::BeginMap << YAML::EndMap;
    } else {
        out << YAML::BeginMap;
        for (const auto& [k, v] : dataNode.children()) {
            out << YAML::Key << YAML::DoubleQuoted << k << YAML::Value;
            to_yaml(out, v);
        }
        out << YAML::EndMap;
    }

    out << YAML::EndMap;

    std::ofstream fout(filename);
    fout << out.c_str();
}

// DispatchingAccessor methods
void DispatchingAccessor::register_analysis(std::shared_ptr<Analysis> analysis) {
    analyses.push_back(std::move(analysis));
}

void DispatchingAccessor::on_particle_block(const ParticleBlock& block) {
    for (auto& a : analyses) {
        a->analyze_particle_block(block, *this);
    }
}

void DispatchingAccessor::on_end_block(const EndBlock&) {
    // optional
}

void DispatchingAccessor::on_header(Header& header) {
    for (auto& a : analyses) {
        a->on_header(header);
    }
}

void run_analysis(const std::vector<std::pair<std::string, std::string>>& file_and_meta,
                  const std::string& analysis_name,
                  const std::vector<std::string>& quantities,
                  bool save_output,
                  bool print_output,
                  const std::string& output_folder)
{
    if (quantities.empty()) throw std::runtime_error("No quantities provided");

    if (save_output) {
        std::error_code ec;
        std::filesystem::create_directories(output_folder, ec);
        if (ec) throw std::runtime_error("create_directories failed: " + ec.message());
    }

    std::vector<std::pair<std::string, MergeKeySet>> input_files;
    input_files.reserve(file_and_meta.size());
    for (const auto& [file, meta] : file_and_meta) {
        MergeKeySet ks = parse_merge_key(meta);
        sort_keyset(ks);
        input_files.emplace_back(file, std::move(ks));
    }

    std::vector<Entry> results;
    results.reserve(input_files.size());

    auto find_or_insert = [&](MergeKeySet k) -> std::shared_ptr<Analysis>& {
        auto it = std::lower_bound(results.begin(), results.end(), k,
            [](Entry const& e, MergeKeySet const& x){ return e.key < x; });
        if (it == results.end() || it->key < k || k < it->key) {
            it = results.insert(it, Entry{std::move(k), nullptr});
        }
        return it->analysis;
    };

    for (auto& [path, key] : input_files) {
        auto analysis = AnalysisRegistry::instance().create(analysis_name);
        if (!analysis) throw std::runtime_error("Unknown analysis: " + analysis_name);

        analysis->set_merge_keys(key);

        auto dispatcher = std::make_shared<DispatchingAccessor>();
        dispatcher->register_analysis(analysis);

        BinaryReader reader(path, quantities, dispatcher);
        reader.read();

        auto& slot = find_or_insert(std::move(key));
        if (slot) {
            *slot += *analysis;
        } else {
            slot = std::move(analysis);
        }
    }

    for (auto& e : results) {
        e.analysis->finalize();
        if (print_output) {
            const std::string label = label_from_keyset(e.key);
            std::cout << "=== Result for " << (label.empty() ? "(no key)" : label) << " ===\n";
            e.analysis->print_result_to(std::cout);
        }
    }

    if (save_output) {
        std::filesystem::path out = std::filesystem::path(output_folder) / (analysis_name + ".yaml");
        save_all_to_yaml(out.string(), results);
    }
}

MergeKeySet parse_merge_key(const std::string& meta) {
    MergeKeySet ks;
    if (meta.empty()) return ks;

    std::stringstream ss(meta);
    std::string item;
    while (std::getline(ss, item, ',')) {
        auto eq = item.find('=');
        if (eq == std::string::npos) continue;
        std::string name = item.substr(0, eq);
        std::string val  = item.substr(eq + 1);

        try {
            if (val.find('.') != std::string::npos) {
                ks.emplace_back(name, std::stod(val));
            } else {
                try {
                    ks.emplace_back(name, std::stoi(val));
                } catch (...) {
                    ks.emplace_back(name, std::stod(val));
                }
            }
        } catch (...) {
            ks.emplace_back(name, val);
        }
    }
    sort_keyset(ks);
    return ks;
}

void sort_keyset(MergeKeySet& k) {
    std::sort(k.begin(), k.end(), [](auto const& a, auto const& b){
        if (a.name != b.name) return a.name < b.name;
        return a.value < b.value;
    });
}

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string label_from_keyset(const MergeKeySet& ks) {
    std::ostringstream oss;
    for (size_t i = 0; i < ks.size(); ++i) {
        if (i) oss << " | ";
        oss << ks[i].name << "=";
        std::visit([&](auto const& x){ oss << x; }, ks[i].value);
    }
    return oss.str();
}

void save_all_to_yaml(const std::string& filename,
                      const std::vector<Entry>& results)
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "results" << YAML::Value << YAML::BeginSeq;

    for (const auto& e : results) {
        out << YAML::BeginMap;

        out << YAML::Key << "merge_keys" << YAML::Value << YAML::BeginMap;
        for (const auto& kv : e.key) {
            out << YAML::Key << kv.name << YAML::Value;
            std::visit([&](auto const& x){ out << x; }, kv.value);
        }
        out << YAML::EndMap;

        out << YAML::Key << "smash_version" << YAML::Value
            << e.analysis->get_smash_version();

        out << YAML::Key << "data" << YAML::Value;
        if (e.analysis->get_data().empty()) {
            out << YAML::BeginMap << YAML::EndMap;
        } else {
            out << YAML::BeginMap;
            for (const auto& [k, v] : e.analysis->get_data().children()) {
                to_yaml(out, v);
            }
            out << YAML::EndMap;
        }

        out << YAML::EndMap;
    }

    out << YAML::EndSeq;
    out << YAML::EndMap;

    std::ofstream fout(filename);
    if (!fout) throw std::runtime_error("Failed to open " + filename);
    fout << out.c_str();
}
