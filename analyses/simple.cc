#include "analysis.h"
#include "analysisregister.h"
#include <iostream>

#include "histogram1d.h"


class Rapidity : public Analysis {
public:
    Rapidity() : hist_(-5.0, 5.0, 100) {}

    void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) override {
        for (size_t i = 0; i < block.npart; ++i) {
            double pz = accessor.get_double("pz", block, i);
            double e  = accessor.get_double("p0", block, i);
            if (e <= std::abs(pz)) continue;
            double y = 0.5 * std::log((e + pz) / (e - pz));
            hist_.fill(y);
        }
    }
    void finalize() override {}; 

void save(const std::string& save_dir_path) override {
    std::ofstream out(save_dir_path + "/rap.dat");
    if (!out) {
        throw std::runtime_error("Could not open file: " + save_dir_path + "/rap.dat");
    }
    hist_.print(out);
}

private:
    Histogram1D hist_;
};
// Register this analysis using your macro
REGISTER_ANALYSIS("simple", Rapidity)
