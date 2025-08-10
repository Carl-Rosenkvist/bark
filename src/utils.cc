#include "utils.h"

#include "yaml-cpp/yaml.h"
#include <sstream>
#include <fstream>
#include <string>
#include <filesystem>
#include <variant>
#include <iostream>
#include "analysis.h"

// your code...
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




