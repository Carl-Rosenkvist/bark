#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "binaryreader.h"
#include "histogram1d.h"
#include <memory>
#include <vector>
#include <variant>
#include <filesystem>  // At the top of your file if not already
using MergeKeyValue = std::variant<int, double, std::string>;
using MergeKey = std::unordered_map<std::string, MergeKeyValue>;


struct MergeKeyHash {
    std::size_t operator()(const MergeKey& key) const {
        std::size_t h = 0;
        for (const auto& [k, v] : key) {
            h ^= std::hash<std::string>{}(k) ^ (std::hash<MergeKeyValue>{}(v) << 1);
        }
        return h;
    }
};
// Define the data type used in the analysis framework
using Data = std::variant<int, double, std::vector<int>, std::vector<double>, Histogram1D>;



inline void merge_data(Data& a, const Data& b) {
    if (a.index() != b.index()) {
        throw std::runtime_error("Mismatched types in Data merge");
    }

    std::visit([&](auto& lhs, const auto& rhs) {
        using L = std::decay_t<decltype(lhs)>;
        using R = std::decay_t<decltype(rhs)>;

        // Only run the merging logic if types actually match
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
// Pretty-print a Data value
inline void print_data(std::ostream& os, const Data& d) {
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
            val.print(os);  // assumes Histogram1D has print(std::ostream&)
        } else {
            static_assert(sizeof(T) == 0, "Unsupported type in Data for print");
        }
    }, d);
}
class Analysis {
protected:
    MergeKey keys;
    std::unordered_map<std::string,Data> data;
    std::string smash_version;


public:
    Analysis& operator+=(const Analysis& other) {
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

    virtual ~Analysis() = default;
    void set_merge_keys(MergeKey k) { keys = std::move(k); }
    const MergeKey& get_merge_keys() const { return keys; }
    void on_header(Header& header){smash_version = header.smash_version;}
    virtual void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) = 0;
    virtual void finalize() = 0;
    virtual void save(const std::string& save_dir_path) = 0;
    virtual void print_result_to(std::ostream& os) const {}
};


// Dispatching accessor that forwards particle blocks to registered analyses
class DispatchingAccessor : public Accessor {
public:
    void register_analysis(std::shared_ptr<Analysis> analysis) {
        analyses.push_back(std::move(analysis));
    }

    void on_particle_block(const ParticleBlock& block) override {
        for (auto& a : analyses) {
            a->analyze_particle_block(block, *this);
        }
    }

    void on_end_block(const EndBlock& block) override {
        // Optional: per-event finalization
    }
    void on_header(Header& header) override{
        for(auto& a : analyses){
            a->on_header(header);
        }
    }
  
private:
    std::vector<std::shared_ptr<Analysis>> analyses;
};

#endif // ANALYSIS_H
