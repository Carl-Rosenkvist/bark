#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <cmath>                 // std::round (used in MergeKey ctor)
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include <yaml-cpp/yaml.h>

#include "binaryreader.h"
#include "histogram1d.h"
#include "datatree.h"
// ---------- Merge keys (vector of name/value pairs) ----------
using MergeKeyValue = std::variant<int, double, std::string>;

struct MergeKey {
    std::string name;
    std::variant<int, double, std::string> value;

    explicit MergeKey(std::string n, std::variant<int,double,std::string> v)
      : name(std::move(n)),
        value(std::visit([](auto x)->decltype(value){
            using T = std::decay_t<decltype(x)>;
            if constexpr (std::is_same_v<T,double>) {
                double s = 1000.0; // 3 decimals
                return std::round(x*s)/s;
            } else {
                return x;
            }
        }, v)) {}
};

inline bool operator<(MergeKey const& a, MergeKey const& b) {
    if (a.name != b.name) return a.name < b.name;
    return a.value < b.value; // variant orders by index, then value
}
inline bool operator==(MergeKey const& a, MergeKey const& b) {
    return a.name == b.name && a.value == b.value;
}

using MergeKeySet = std::vector<MergeKey>;

inline bool operator<(MergeKeySet const& A, MergeKeySet const& B) {
    return std::lexicographical_compare(A.begin(), A.end(), B.begin(), B.end());
}
inline bool operator==(MergeKeySet const& A, MergeKeySet const& B) {
    return A.size() == B.size() && std::equal(A.begin(), A.end(), B.begin());
}

void to_yaml(YAML::Emitter& out, const MergeKeyValue& v);

// Helpers (you can also keep these in utils.h if you prefer)
MergeKeySet parse_merge_key(const std::string& meta);
void sort_keyset(MergeKeySet& k);
bool ends_with(const std::string& str, const std::string& suffix);
std::string label_from_keyset(const MergeKeySet& ks);

// ---------- Analysis base ----------
class Analysis {
protected:
    MergeKeySet keys;
    std::string smash_version;
    DataNode dataNode;

public:
    virtual ~Analysis() = default;
    Analysis& operator+=(const Analysis& other);

    void set_merge_keys(MergeKeySet k);
    const MergeKeySet& get_merge_keys() const;

    void on_header(Header& header);
    void save_as_yaml(const std::string& filename) const;

    const DataNode& get_data() const { return dataNode; }
    DataNode& get_data(){ return dataNode; }


    const std::string& get_smash_version() const { return smash_version; }

    virtual void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) = 0;
    virtual void finalize() = 0;
    virtual void save(const std::string& save_dir_path) = 0;
    virtual void print_result_to(std::ostream& os) const {}
};

// ---------- Dispatcher ----------
class DispatchingAccessor : public Accessor {
public:
    void register_analysis(std::shared_ptr<Analysis> analysis);
    void on_particle_block(const ParticleBlock& block) override;
    void on_end_block(const EndBlock& block) override;
    void on_header(Header& header) override;

private:
    std::vector<std::shared_ptr<Analysis>> analyses;
}; 

// ---------- Result entry + run ----------
struct Entry {
    MergeKeySet key;
    std::shared_ptr<Analysis> analysis;
};

// Write one YAML with all entries (deterministic order)
void save_all_to_yaml(const std::string& filename,
                      const std::vector<Entry>& results);

void run_analysis(const std::vector<std::pair<std::string, std::string>>& file_and_meta,
                  const std::string& analysis_name,
                  const std::vector<std::string>& quantities,
                  bool save_output = true,
                  bool print_output = true,
                  const std::string& output_folder = ".");

#endif // ANALYSIS_H
