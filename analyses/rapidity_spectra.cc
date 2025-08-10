#include "analysis.h"
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <unordered_set>

#include "analysisregister.h"

class RapidityHistogramAnalysis : public Analysis {
public:
    RapidityHistogramAnalysis()
        : y_min(-4.0), y_max(4.0), n_bins(30) {
        const std::vector<int> positive_pdgs = {
            111, 211, 311, 321, 310, 130,
            3122, 3222, 3212, 3112, 3322, 3312, 3334,
            2212
        };

        for (int pdg : positive_pdgs) {
            selected_pdgs.insert(pdg);
            if (pdg != 111 && pdg != 310 && pdg != 130) {
                selected_pdgs.insert(-pdg); // charge conjugates
            }
        }
    }

   

void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) override {
    int wounded = 0;
    for (size_t i = 0; i < block.npart; ++i) {
        int pdg = accessor.get_int("pdg", block, i);
        int ncoll = accessor.get_int("ncoll", block, i);
        if ((pdg == 2212 || pdg == 2112) && ncoll > 0) {
            ++wounded;
        }
    }

    if (wounded == 0) return;

    std::string wounded_key = std::to_string(wounded);
    auto& group = data["wounded"].subdata[wounded_key];

    for (size_t i = 0; i < block.npart; ++i) {
        int pdg = accessor.get_int("pdg", block, i);
        if (!selected_pdgs.count(pdg)) continue;

        double E  = accessor.get_double("p0", block, i);
        double pz = accessor.get_double("pz", block, i);
        if (!std::isfinite(E) || !std::isfinite(pz)) continue;
        if (E <= std::abs(pz)) continue;

        double y = 0.5 * std::log((E + pz) / (E - pz));
        if (!std::isfinite(y) || y < y_min || y >= y_max) continue;

        std::string key = "rapidity_pdg_" + std::to_string(pdg);
        auto& hist_node = group.subdata[key];
        hist_node.name = key;

        if (!std::holds_alternative<Histogram1D>(hist_node.value)) {
            hist_node.value = Histogram1D(y_min, y_max, n_bins);
        }

        std::get<Histogram1D>(hist_node.value).fill(y);
    }

    auto& ev_node = group.subdata["n_events"];
    ev_node.name = "n_events";

    if (!std::holds_alternative<int>(ev_node.value)) {
        ev_node.value = 0;
    }

    std::get<int>(ev_node.value) += 1;
}
    
void finalize() override {
  
}
    void save(const std::string& save_dir_path) override {
        // No-op: base handles serialization
    }

private:
    std::unordered_set<int> selected_pdgs;
    double y_min;
    double y_max;
    int n_bins;
};

REGISTER_ANALYSIS("Rapidity", RapidityHistogramAnalysis);
