#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "binaryreader.h"
#include "histogram1d.h"
#include <memory>
#include <vector>
#include <variant>
#include <filesystem>
#include <unordered_map>

#include <string> // for std::string
#include <yaml-cpp/yaml.h>

using MergeKeyValue = std::variant<int, double, std::string>;
using MergeKey = std::unordered_map<std::string, MergeKeyValue>;

struct MergeKeyHash {
    std::size_t operator()(const MergeKey& key) const;
};

using Data = std::variant<int, double, std::vector<int>, std::vector<double>, Histogram1D>;

void merge_data(Data& a, const Data& b);
void print_data(std::ostream& os, const Data& d);
void to_yaml(YAML::Emitter& out, const Data& d);
void to_yaml(YAML::Emitter& out, const MergeKeyValue& v);

class Analysis {
protected:
    MergeKey keys;
    std::unordered_map<std::string, Data> data;
    std::string smash_version;

public:
    virtual ~Analysis() = default;
    Analysis& operator+=(const Analysis& other);

    void set_merge_keys(MergeKey k);
    const MergeKey& get_merge_keys() const;

    void on_header(Header& header);
    void save_as_yaml(const std::string& filename) const;

    const std::unordered_map<std::string, Data>& get_data() const { return data; }
    const std::string& get_smash_version() const { return smash_version; }
    virtual void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) = 0;
    virtual void finalize() = 0;
    virtual void save(const std::string& save_dir_path) = 0;
    virtual void print_result_to(std::ostream& os) const {}
};

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
