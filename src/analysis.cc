#include "analysis.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <type_traits>

// MergeKey hash
std::size_t MergeKeyHash::operator()(const MergeKey& key) const {
    std::size_t h = 0;
    for (const auto& [k, v] : key) {
        h ^= std::hash<std::string>{}(k) ^ (std::hash<MergeKeyValue>{}(v) << 1);
    }
    return h;
}





void merge_values(Data& a_val, const Data& b_val, const std::string& context) {
    
if (std::holds_alternative<std::monostate>(a_val)) {
    a_val = b_val;
    return;
}
    if (std::holds_alternative<std::monostate>(b_val)) {
        return;
    }

    if (a_val.index() != b_val.index()) {
        return;
    }

    std::visit([&](auto& lhs_val) {
        using T = std::decay_t<decltype(lhs_val)>;

        const auto& rhs_val = std::get<T>(b_val);

        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
            lhs_val += rhs_val;

        } else if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<double>>) {
            lhs_val.insert(lhs_val.end(), rhs_val.begin(), rhs_val.end());

        } else if constexpr (std::is_same_v<T, Histogram1D>) {
            lhs_val += rhs_val;

        } else {
            // Only merge if exactly equal
            if (lhs_val != rhs_val) {
                std::cerr << "❌ merge_values: Conflicting non-leaf values in '" << context << "'\n";
            }
        }
    }, a_val);
}



void merge_data(DataNode& a, const DataNode& b) {
    if (a.name.empty()) a.name = b.name;
    else if (!b.name.empty() && a.name != b.name) {
        std::cerr << "⚠️ merge_data: name mismatch: '" << a.name << "' vs '" << b.name << "'\n";
    }

    // Promote empty node
    if (std::holds_alternative<std::monostate>(a.value) && a.subdata.empty()) {
        a = b;
        return;
    }

    const bool a_leaf = a.is_leaf();
    const bool b_leaf = b.is_leaf();

    if (a_leaf && b_leaf) {
        merge_values(a.value, b.value, a.name);
    } else if (!a_leaf && !b_leaf) {
        if (!std::holds_alternative<std::monostate>(a.value) &&
            !std::holds_alternative<std::monostate>(b.value) &&
            a.value != b.value) {
            std::cerr << "❌ merge_data: root value mismatch in '" << a.name << "'\n";
        }
    } else if (a_leaf != b_leaf) {
        std::cerr << "❌ merge_data: mixing leaf/non-leaf nodes in '" << a.name << "'\n";
    }

    for (const auto& [k, v_sub] : b.subdata) {
        merge_data(a.subdata[k], v_sub);
    }
}
// Print logic
void print_data(std::ostream& os, const Data& d) {
    std::visit([&](const auto& val) {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
            os << val;
        } else if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<double>>) {
            os << "[";
            for (size_t i = 0; i < val.size(); ++i) {
                os << val[i];
                if (i + 1 < val.size()) os << ", ";
            }
            os << "]";
        } else if constexpr (std::is_same_v<T, Histogram1D>) {
            val.print(os);
        }
    }, d);
}

// YAML serialization
void to_yaml(YAML::Emitter& out, const MergeKeyValue& v) {
    std::visit([&](const auto& val) {
        out << val;
    }, v);
}

void to_yaml(YAML::Emitter& out, const Data& d) {
    std::visit([&](const auto& val) {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, int> || std::is_same_v<T, double>) {
            out << val;

        } else if constexpr (std::is_same_v<T, std::vector<int>> ||
                             std::is_same_v<T, std::vector<double>>) {
            out << YAML::BeginSeq;
            for (const auto& item : val) out << item;
            out << YAML::EndSeq;

        } else if constexpr (std::is_same_v<T, Histogram1D>) {
            out << YAML::BeginMap;

            std::vector<double> edges;   edges.reserve(val.num_bins() + 1);
            std::vector<double> counts;  counts.reserve(val.num_bins());

            for (size_t i = 0; i < val.num_bins(); ++i) {
                edges.push_back(val.bin_edge(i));         // left edge of bin i
                counts.push_back(val.get_bin_count(i));   // count of bin i
            }
            if (val.num_bins() > 0) {
                edges.push_back(val.bin_edge(val.num_bins())); // rightmost edge
            }

            out << YAML::Key << "bins"   << YAML::Value << edges;   // length = N+1
            out << YAML::Key << "values" << YAML::Value << counts;  // length = N
            out << YAML::EndMap;

        } else {
            throw std::runtime_error("Unsupported Data type for YAML");
        }
    }, d);
}



void to_yaml(YAML::Emitter& out, const DataNode& node) {
    out << YAML::BeginMap;

    // If the node has a value (i.e. is a leaf), emit it directly under "value"
    if (!std::holds_alternative<std::monostate>(node.value)) {
        out << YAML::Key << "value" << YAML::Value;
        to_yaml(out, node.value);
    }

    // Emit all child nodes directly (no "subdata" nesting)
    for (const auto& [k, subnode] : node.subdata) {
        out << YAML::Key << k << YAML::Value;
        to_yaml(out, subnode);
    }

    out << YAML::EndMap;
}


// Analysis methods
Analysis& Analysis::operator+=(const Analysis& other) {
    if (this->keys != other.get_merge_keys()) {
        throw std::runtime_error("Cannot merge Analysis objects: MergeKey mismatch.");
    }

    for (const auto& [k, v] : other.data) {
        if (data.count(k)) {
            merge_data(data[k], v);
        } else {
            data[k] = v;
        }
    }
    return *this;
}

void Analysis::set_merge_keys(MergeKey k) {
    keys = std::move(k);
}

const MergeKey& Analysis::get_merge_keys() const {
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

    out << YAML::Key << "data" << YAML::Value << YAML::BeginMap;
    for (const auto& [k, v] : data) {
        out << YAML::Key << k << YAML::Value;
        to_yaml(out, v);
    }
    out << YAML::EndMap;

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
