#include "analysis.h"
#include <cmath>
#include <string>
#include <unordered_set>
#include <variant>

#include "analysisregister.h"

class MidRapidityCounts : public Analysis {
public:
    MidRapidityCounts()
        : y_min(-0.5), y_max(0.5)  // set your rapidity window here
    {
        // Particles to consider (positive PDGs). Add charge conjugates except neutral self-conjugates.
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

        // Name the root counts node (optional but tidy)
        auto& countsNode = data["Counts"];
        countsNode.name = "Counts";

        // Initialize event counter node (optional)
        auto& ev_node = data["n_events"];
        ev_node.name = "n_events";
        if (!std::holds_alternative<int>(ev_node.value)) ev_node.value = 0;
    }

    void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) override {
        for (size_t i = 0; i < block.npart; ++i) {
            int pdg = accessor.get_int("pdg", block, i);
            if (!selected_pdgs.count(pdg)) continue;

            double E  = accessor.get_double("p0", block, i);
            double pz = accessor.get_double("pz", block, i);
            if (!std::isfinite(E) || !std::isfinite(pz)) continue;

            // Avoid invalid log: requires E > |pz|
            if (E <= std::abs(pz)) continue;

            double y = 0.5 * std::log((E + pz) / (E - pz));
            if (!std::isfinite(y) || y < y_min || y >= y_max) continue;

            // Passed all cuts → increment count for this PDG
            auto& group = data["Counts"].subdata[std::to_string(pdg)];
            group.name = std::to_string(pdg);
            if (!std::holds_alternative<int>(group.value)) group.value = 0;
            std::get<int>(group.value) += 1;
        }

        // Count this event (adjust if analyze_particle_block is called multiple times per event in your framework)
        auto& ev_node = data["n_events"];
        if (!std::holds_alternative<int>(ev_node.value)) ev_node.value = 0;
        std::get<int>(ev_node.value) += 1;
    }

    void finalize() override {
        // No post-processing needed for simple counts, but you can compute derived quantities here.
    }

    void save(const std::string& save_dir_path) override {
        // No-op: base handles serialization
        (void)save_dir_path;
    }

private:
    std::unordered_set<int> selected_pdgs;
    double y_min;
    double y_max;
};

REGISTER_ANALYSIS("MidRapidityCounts", MidRapidityCounts);
