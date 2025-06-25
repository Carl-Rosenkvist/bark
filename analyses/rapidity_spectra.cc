#include "analysis.h"
#include <cmath>
#include <string>
#include <sstream>
#include <fstream>
#include <iomanip>

#include "analysisregister.h"
class RapidityHistogramAnalysis : public Analysis {
public:
    RapidityHistogramAnalysis()
        : y_min(-4.0), y_max(4.0), n_bins(80)
    {
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
            data["rapidity_pdg_" + std::to_string(pdg)] = Histogram1D(y_min, y_max, n_bins);
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
            if (E <= std::abs(pz)) continue;

            double y = 0.5 * std::log((E + pz) / (E - pz));
            std::string key = "rapidity_pdg_" + std::to_string(pdg);
            std::get<Histogram1D>(data.at(key)).fill(y);
        }
    }

    void finalize() override {
        int n_events = std::get<int>(data.at("n_events"));
        if (n_events == 0) return;

        for (int pdg : selected_pdgs) {
            std::string key = "rapidity_pdg_" + std::to_string(pdg);
            auto& hist = std::get<Histogram1D>(data.at(key));
            double norm = 1.0 / (n_events * hist.bin_width());
            hist.scale(norm);
        }
    }

   

void save(const std::string& dir_path) override {
    std::filesystem::create_directories(dir_path);  // Ensure output directory exists

    int n_events = std::get<int>(data.at("n_events"));
    for (int pdg : selected_pdgs) {
        std::string key = "rapidity_pdg_" + std::to_string(pdg);
        const auto& hist = std::get<Histogram1D>(data.at(key));

        std::filesystem::path file_path = std::filesystem::path(dir_path) / (key + ".csv");
        std::ofstream out(file_path);

        if (!out) {
            std::cerr << "Error: Failed to write to " << file_path << "\n";
            continue;
        }
        out << "#"+smash_version << "\n";
        out << "y,dN/dy,error\n";
        for (int i = 0; i < hist.num_bins(); ++i) {
            double y_center = hist.bin_center(i);
            double count = hist.raw_bin_content(i);
            double val = hist.bin_content(i);
            double err = (count > 0) ? std::sqrt(count) / (hist.bin_width() * n_events) : 0.0;

            out << std::fixed << std::setprecision(6)
                << y_center << "," << val << "," << err << "\n";
        }

        std::cout << "Saved: " << file_path << "\n";
    }
}
private:
    std::vector<int> selected_pdgs;
    double y_min;
    double y_max;
    int n_bins;

};

REGISTER_ANALYSIS("Rapidity", RapidityHistogramAnalysis);
