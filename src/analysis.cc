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

// Merge logic
void merge_data(Data& a, const Data& b) {
    if (a.index() != b.index()) {
        throw std::runtime_error("Mismatched types in Data merge");
    }

    std::visit([&](auto& lhs, const auto& rhs) {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;

        if constexpr (std::is_same_v<L, R>) {
            if constexpr (std::is_same_v<L, int> || std::is_same_v<L, double>) {
                lhs += rhs;
            } else if constexpr (std::is_same_v<L, std::vector<int>> || std::is_same_v<L, std::vector<double>>) {
                lhs.insert(lhs.end(), rhs.begin(), rhs.end());
            } else if constexpr (std::is_same_v<L, Histogram1D>) {
                lhs += rhs;
            } else {
                throw std::runtime_error("Unsupported type in Data variant");
            }
        } else {
            throw std::runtime_error("Mismatched internal types in Data variant");
        }
    }, a, b);
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

        } else if constexpr (std::is_same_v<T, std::vector<int>> || std::is_same_v<T, std::vector<double>>) {
            out << YAML::BeginSeq;
            for (const auto& item : val) out << item;
            out << YAML::EndSeq;

        } else if constexpr (std::is_same_v<T, Histogram1D>) {
            out << YAML::BeginMap;
            std::vector<double> bins, values;
            for (size_t i = 0; i < val.num_bins(); ++i) {
                bins.push_back(val.bin_center(i));
                values.push_back(val.get_bin_count(i));
            }
            out << YAML::Key << "bins" << YAML::Value << bins;
            out << YAML::Key << "values" << YAML::Value << values;
            out << YAML::EndMap;

        } else {
            throw std::runtime_error("Unsupported Data type for YAML");
        }
    }, d);
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
