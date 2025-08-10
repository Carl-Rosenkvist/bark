#ifndef BARYON_STOPPING_ANALYSIS_H
#define BARYON_STOPPING_ANALYSIS_H

#include "analysis.h"
#include "analysisregister.h"

#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <iostream>
/*
class BaryonStoppingAnalysis : public Analysis {
public:
    BaryonStoppingAnalysis()
        : y_min(-6.0), y_max(6.0), y_bins(120),
          xF_min(0.0), xF_max(0.95), xF_bins(120),
          beam_energy(17.3/2.0),  // per nucleon in GeV; 60 GeV CM energy => 30 + 30
          proton_mass(0.938)
    {
        pz_beam = std::sqrt(beam_energy * beam_energy - proton_mass * proton_mass);

        std::vector<int> positive_pdgs = {
            111, 211, 311, 321, 310, 130,
            3122, 3222, 3212, 3112, 3322, 3312, 3334,
            2212
        };

        for (int pdg : positive_pdgs) {
            selected_pdgs.push_back(pdg);
            if (pdg != 111 && pdg != 310 && pdg != 130) {
                selected_pdgs.push_back(-pdg);
            }
        }

        for (int pdg : selected_pdgs) {
            data["rapidity_pdg_" + std::to_string(pdg)] = Histogram1D(y_min, y_max, y_bins);
            data["xf_pdg_" + std::to_string(pdg)] = Histogram1D(xF_min, xF_max, xF_bins);
        }

        data["n_events"] = 0;
    }

    void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) override {
        std::get<int>(data["n_events"]) += 1;

        for (size_t i = 0; i < block.npart; ++i) {
            int pdg = accessor.get_int("pdg", block, i);
            if (std::find(selected_pdgs.begin(), selected_pdgs.end(), pdg) == selected_pdgs.end()) continue;

            double E = accessor.get_double("p0", block, i);
            double pz = accessor.get_double("pz", block, i);

            if (E <= std::abs(pz)) continue;  // unphysical

            // Rapidity
            double y = 0.5 * std::log((E + pz) / (E - pz));
            std::string key_y = "rapidity_pdg_" + std::to_string(pdg);
            std::get<Histogram1D>(data.at(key_y)).fill(y);

            // Feynman x
            double xF = std::abs(pz / pz_beam);
            if (std::abs(xF) <= 1.5) {  // just to be safe
                std::string key_xf = "xf_pdg_" + std::to_string(pdg);
                std::get<Histogram1D>(data.at(key_xf)).fill(xF);
            }
        }
    }

    void finalize() override {
        int n_events = std::get<int>(data.at("n_events"));
        if (n_events == 0) return;

        for (int pdg : selected_pdgs) {
            for (const std::string& prefix : {"rapidity_pdg_", "xf_pdg_"}) {
                std::string key = prefix + std::to_string(pdg);
                auto& hist = std::get<Histogram1D>(data.at(key));
                double norm = 1.0 / (n_events * hist.bin_width());
                hist.scale(norm);
            }
        }
    }

    void save(const std::string& dir_path) override {
        std::filesystem::create_directories(dir_path);  // Ensure output directory exists
        int n_events = std::get<int>(data.at("n_events"));

        for (int pdg : selected_pdgs) {
            for (const std::string& prefix : {"rapidity_pdg_", "xf_pdg_"}) {
                std::string key = prefix + std::to_string(pdg);
                const auto& hist = std::get<Histogram1D>(data.at(key));

                std::filesystem::path file_path = std::filesystem::path(dir_path) / (key + ".csv");
                std::ofstream out(file_path);

                if (!out) {
                    std::cerr << "Error: Failed to write to " << file_path << "\n";
                    continue;
                }

                out << "#" << smash_version << "\n";
                out << (prefix[0] == 'x' ? "xF" : "y") << ",dN/dx,error\n";

                for (int i = 0; i < hist.num_bins(); ++i) {
                    double x = hist.bin_center(i);
                    double count = hist.raw_bin_content(i);
                    double val = hist.bin_content(i);
                    double err = (count > 0) ? std::sqrt(count) / (hist.bin_width() * n_events) : 0.0;

                    out << std::fixed << std::setprecision(6)
                        << x << "," << val << "," << err << "\n";
                }

                std::cout << "Saved: " << file_path << "\n";
            }
        }
    }

private:
    std::vector<int> selected_pdgs;
    double y_min, y_max;
    int y_bins;
    double xF_min, xF_max;
    int xF_bins;

    double beam_energy;  // GeV per nucleon
    double proton_mass;
    double pz_beam;
};

REGISTER_ANALYSIS("BaryonStopping", BaryonStoppingAnalysis);
*/
#endif // BARYON_STOPPING_ANALYSIS_H
