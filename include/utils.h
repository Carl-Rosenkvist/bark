
#pragma once

#include <string>
#include <unordered_map>
#include <memory>
#include "analysis.h"

// Parses key=value,... into a MergeKey
MergeKey parse_merge_key(const std::string& meta);

// Turns a MergeKey into a label string like key_val_key_val
std::string label_from_key(const MergeKey& key);

// Saves a map of analyses into a single YAML file
void save_all_to_yaml(const std::string& filename,
                      const std::unordered_map<MergeKey, std::shared_ptr<Analysis>, MergeKeyHash>& analysis_map);

// Checks if 'str' ends with 'suffix'
bool ends_with(const std::string& str, const std::string& suffix);
