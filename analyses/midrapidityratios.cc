#include "analysis.h"
#include "analysisregister.h"
#include <iostream>
#include <fstream>
#include <cmath>
#include <unordered_map>

class MidrapiditySpeciesCounter : public Analysis {
public:
    MidrapiditySpeciesCounter() = default;

    void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) override {
        for (size_t i = 0; i < block.npart; ++i) {
            double pz  = accessor.get_double("pz", block, i);
            double e   = accessor.get_double("p0", block, i);
            int pdg    = accessor.get_int("pdg", block, i);

            if (e <= std::abs(pz)) continue; // avoid invalid log
            double y = 0.5 * std::log((e + pz) / (e - pz));

            if (std::abs(y) < 0.5) {
                counts_[pdg]++;
            }
        }
    }

    void finalize() override {}

    void save(const std::string& save_dir_path) override {
        std::ofstream out(save_dir_path + "/midrapidity_counts.dat");
        if (!out) {
            throw std::runtime_error("Could not open file: " + save_dir_path + "/midrapidity_counts.dat");
        }
        for (const auto& [pdg, count] : counts_) {
            out << pdg << " " << count << "\n";
        }
    }

    void print_result_to(std::ostream& os) const override {
        for (const auto& [pdg, count] : counts_) {
            os << pdg << " " << count << "\n";
        }
    }

private:
    std::unordered_map<int, int> counts_;
};

REGISTER_ANALYSIS("midrapidity_counts", MidrapiditySpeciesCounter)
