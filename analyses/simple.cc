#include "analysis.h"
#include "analysisregister.h"
#include "histogram1d.h"

#include <iostream>
#include <fstream>     // Needed for std::ofstream
#include <cmath>       // Needed for std::log and std::abs

class Rapidity : public Analysis {
public:
    Rapidity() : hist_(-5.0, 5.0, 100) {}

    void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) override {
        for (size_t i = 0; i < block.npart; ++i) {
            double pz = accessor.get_double("pz", block, i);
            double e  = accessor.get_double("p0", block, i);
            if (e <= std::abs(pz)) continue; // Avoid division by zero or domain error
            double y = 0.5 * std::log((e + pz) / (e - pz));
            hist_.fill(y);
        }
    }

    void finalize() override {}

    void save(const std::string& save_dir_path) override {
        std::ofstream out(save_dir_path + "/rap.dat");
        if (!out) {
            throw std::runtime_error("Could not open file: " + save_dir_path + "/rap.dat");
        }
        hist_.print(out);
    }

    void print_result_to(std::ostream& os) const override {
        hist_.print(os);
    }

private:
    Histogram1D hist_;
};

// Register this analysis with the registry
REGISTER_ANALYSIS("simple", Rapidity)
