#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "binaryreader.h"
#include <memory>
#include <vector>
// Base class for all analyses
class Analysis {
public:
    virtual ~Analysis() = default;
    virtual void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) = 0;
    virtual void finalize() = 0;
    virtual void save(const std::string& save_dir_path) = 0;
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
  
private:
    std::vector<std::shared_ptr<Analysis>> analyses;
};

#endif // ANALYSIS_H
