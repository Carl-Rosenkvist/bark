#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "binaryreader.h"
#include "histogram1d.h"
#include <memory>
#include <vector>
#include <variant>
#include <string>
#include <map>
#include <unordered_map>
#include <filesystem>
#include <yaml-cpp/yaml.h>

// Merge keys
using MergeKeyValue = std::variant<int, double, std::string>;
using MergeKey = std::unordered_map<std::string, MergeKeyValue>;
struct MergeKeyHash {
    std::size_t operator()(const MergeKey& key) const;
};

// Flat data variant
using Data = std::variant<
    std::monostate, // Empty/default
    int,
    double,
    std::vector<int>,
    std::vector<double>,
    Histogram1D>;



// Node structure
struct DataNode {
    std::string name;
    Data value;
    std::map<std::string, DataNode> subdata;

    DataNode() = default;
    DataNode(std::string n) : name(std::move(n)) {}
    DataNode(std::string n, Data v) : name(std::move(n)), value(std::move(v)) {}

    bool is_leaf() const { return std::holds_alternative<std::monostate>(value) == false; }
};


template <typename T>
DataNode make_node(std::string name, T val) {
    return DataNode{.name = std::move(name), .value = std::move(val)};
}


void merge_values(Data& a_val, const Data& b_val, const std::string& context = "");
void merge_data(DataNode& a, const DataNode& b);

void to_yaml(YAML::Emitter& out, const DataNode& node);
void to_yaml(YAML::Emitter& out, const MergeKeyValue& v);

// Base class
class Analysis {
protected:
    MergeKey keys;
    std::string smash_version;
    std::map<std::string, DataNode> data;

public:
    virtual ~Analysis() = default;
    Analysis& operator+=(const Analysis& other);

    void set_merge_keys(MergeKey k);
    const MergeKey& get_merge_keys() const;

    void on_header(Header& header);
    void save_as_yaml(const std::string& filename) const;

    const std::map<std::string, DataNode>& get_data() const { return data; }
    const std::string& get_smash_version() const { return smash_version; }

    virtual void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) = 0;
    virtual void finalize() = 0;
    virtual void save(const std::string& save_dir_path) = 0;
    virtual void print_result_to(std::ostream& os) const {}
};

// Dispatcher
class DispatchingAccessor : public Accessor {
public:
    void register_analysis(std::shared_ptr<Analysis> analysis);
    void on_particle_block(const ParticleBlock& block) override;
    void on_end_block(const EndBlock& block) override;
    void on_header(Header& header) override;

private:
    std::vector<std::shared_ptr<Analysis>> analyses;
};

#endif // ANALYSIS_H
