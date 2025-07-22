#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <sstream>
#include <variant>
#include "binaryreader.h"
#include "analysis.h"
#include "analysisregister.h"

MergeKey parse_merge_key(const std::string& meta) {
    MergeKey key;
    std::stringstream ss(meta);
    std::string item;
    while (std::getline(ss, item, ',')) {
        auto eq = item.find('=');
        if (eq == std::string::npos) continue;
        std::string k = item.substr(0, eq);
        std::string v = item.substr(eq + 1);

        try {
            if (v.find('.') != std::string::npos)
                key[k] = std::stod(v);
            else
                key[k] = std::stoi(v);
        } catch (...) {
            key[k] = v;
        }
    }
    return key;
}

bool ends_with(const std::string& str, const std::string& suffix) {
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string label_from_key(const MergeKey& key) {
    std::string label;
    for (const auto& [k, v] : key) {
        label += k + "_";
        std::visit([&](const auto& val) {
            using T = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<T, std::string>)
                label += val;
            else
                label += std::to_string(val);
        }, v);
        label += "_";
    }
    if (!label.empty() && label.back() == '_') label.pop_back();
    return label;
}

void save_all_to_yaml(const std::string& filename,
                      const std::unordered_map<MergeKey, std::shared_ptr<Analysis>, MergeKeyHash>& analysis_map) {
    YAML::Emitter out;
    out << YAML::BeginMap;

    for (const auto& [key, analysis] : analysis_map) {
        std::string label = label_from_key(key);
        out << YAML::Key << label;
        out << YAML::Value << YAML::BeginMap;

        out << YAML::Key << "smash_version" << YAML::Value << analysis->get_smash_version();

        out << YAML::Key << "merge_keys" << YAML::Value << YAML::BeginMap;
        for (const auto& [k, v] : key) {
            out << YAML::Key << k << YAML::Value;
            to_yaml(out, v);
        }
        out << YAML::EndMap;

        out << YAML::Key << "data" << YAML::Value << YAML::BeginMap;
        for (const auto& [k, v] : analysis->get_data()) {
            out << YAML::Key << k << YAML::Value;
            to_yaml(out, v);
        }
        out << YAML::EndMap;

        out << YAML::EndMap; // end of one analysis block
    }

    out << YAML::EndMap;

    std::ofstream fout(filename);
    fout << out.c_str();
}
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <file[:key=val,...]>... <analysis> <quantities...>"
                  << " [--no-save] [--no-print] [--output-folder <path>]\n";
        return 1;
    }

    std::vector<std::pair<std::string, MergeKey>> input_files;
    std::string analysis_name;
    std::vector<std::string> quantities;
    bool save_output = true, print_output = true;
    std::filesystem::path output_folder = ".";

    int i = 1;
    // Parse input files with optional :key=val
    for (; i < argc; ++i) {
        std::string arg = argv[i];
        if (ends_with(arg, ".bin") || arg.find(':') != std::string::npos) {
            auto pos = arg.find(':');
            if (pos == std::string::npos) {
                input_files.emplace_back(arg, MergeKey{});
            } else {
                std::string file = arg.substr(0, pos);
                MergeKey key = parse_merge_key(arg.substr(pos + 1));
                input_files.emplace_back(file, key);
            }
        } else {
            break;
        }
    }

    if (i >= argc) {
        std::cerr << "Error: No analysis specified.\n";
        return 1;
    }

    analysis_name = argv[i++];

    for (; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-save") {
            save_output = false;
        } else if (arg == "--no-print") {
            print_output = false;
        } else if (arg == "--output-folder") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --output-folder requires a path argument.\n";
                return 1;
            }
            output_folder = argv[++i];
        } else {
            quantities.push_back(arg);
        }
    }

    if (quantities.empty()) {
        std::cerr << "Error: No quantities provided.\n";
        return 1;
    }

    if (save_output && !std::filesystem::exists(output_folder)) {
        std::filesystem::create_directories(output_folder);
    }

    std::unordered_map<MergeKey, std::shared_ptr<Analysis>, MergeKeyHash> analysis_map;

    for (const auto& [path, key] : input_files) {
        auto analysis = AnalysisRegistry::instance().create(analysis_name);
        if (!analysis) {
            std::cerr << "Unknown analysis: " << analysis_name << "\n";
            return 1;
        }
        analysis->set_merge_keys(key);

        auto dispatcher = std::make_shared<DispatchingAccessor>();
        dispatcher->register_analysis(analysis);

        BinaryReader reader(path, quantities, dispatcher);
        reader.read();

        auto& existing = analysis_map[key];
        if (existing) {
            *existing += *analysis;
        } else {
            analysis_map[key] = std::move(analysis);
        }
    }

    for (const auto& [key, analysis] : analysis_map) {
        analysis->finalize();
        std::string label = label_from_key(key);

   
        if (save_output) {
            std::filesystem::path filename = output_folder / (analysis_name + ".yaml");
            
            save_all_to_yaml(filename.string(), analysis_map);
        }
        if (print_output) {
            std::cout << "=== Result for " << (label.empty() ? "(no key)" : label) << " ===\n";
            analysis->print_result_to(std::cout);
        }
    }

    return 0;
}
