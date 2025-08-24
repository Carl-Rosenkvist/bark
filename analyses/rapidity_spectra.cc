#include "analysis.h"
#include "analysisregister.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_set>

class RapidityAndPtHistogramAnalysis : public Analysis {
public:
    RapidityAndPtHistogramAnalysis()
        : y_min_(-4.0), y_max_(4.0), y_bins_(30),
          pt_min_(0.0), pt_max_(3.0), pt_bins_(30),
          wounded_bin_width_(10), wounded_min_(0), wounded_max_(416),
          y_hist_(y_min_, y_max_, y_bins_),
          pt_hist_(pt_min_, pt_max_, pt_bins_),
          wounded_node_(dataNode.add_child("wounded"))
    {
        const std::vector<int> positive_pdgs = {
            111, 211, 311, 321, 310, 130,
            3122, 3222, 3212, 3112, 3322, 3312, 3334,
            2212
        };
        for (int pdg : positive_pdgs) {
            selected_pdgs_.insert(pdg);
            if (pdg != 111 && pdg != 310 && pdg != 130) {
                selected_pdgs_.insert(-pdg);
            }
        }

    }

    void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) override {
        int wounded = 0;
        for (size_t i = 0; i < block.npart; ++i) {
            int pdg = accessor.get_int("pdg", block, i);
            int ncoll = accessor.get_int("ncoll", block, i);
            if ((pdg == 2212 || pdg == 2112) && ncoll > 0) ++wounded;
        }
        if (wounded <= 0) return;

        DataNode& group = group_for_wounded_(wounded);

        for (size_t i = 0; i < block.npart; ++i) {
            int pdg = accessor.get_int("pdg", block, i);
            if (!selected_pdgs_.count(pdg)) continue;

            double E  = accessor.get_double("p0", block, i);
            double pz = accessor.get_double("pz", block, i);
            double px = accessor.get_double("px", block, i);
            double py = accessor.get_double("py", block, i);
            double y = 0.5 * std::log((E + pz) / (E - pz));
            if (std::isfinite(E) && std::isfinite(pz) && E > std::abs(pz)) {
                if (std::isfinite(y) && y >= y_min_ && y < y_max_) {
                    auto& ydata = get_or_make_histogram_(group, "rapidity_pdg_" + std::to_string(pdg), y_hist_);
                    std::get<Histogram1D>(ydata).fill(y);
                }
            }

            if (std::isfinite(px) && std::isfinite(py)) {
                double pt = std::hypot(px, py);
                if (std::isfinite(pt) && pt >= pt_min_ && pt < pt_max_ && std::abs(y) < 0.5) {
            
                    auto& ptdata = get_or_make_histogram_(group, "p_perp_pdg_" + std::to_string(pdg), pt_hist_);
                    std::get<Histogram1D>(ptdata).fill(pt);
                }
            }
        }

        DataNode& ev_node = group.add_child("n_events");
        Data& ev_data = ev_node.get_data();
        if (!std::holds_alternative<int>(ev_data)) ev_data = 0;
        std::get<int>(ev_data) += 1;
    }

    void finalize() override {

        write_binning_metadata_();

  }
    void save(const std::string&) override {}

private:
    void write_binning_metadata_() {
        DataNode& meta = dataNode.add_child("meta").add_child("histogram_binning");

        auto add_hist = [&](const std::string& name, double min, double max, int bins) {
            DataNode& node = meta.add_child(name);
            node.add_child("min").get_data()       = min;
            node.add_child("max").get_data()       = max;
            node.add_child("bin_width").get_data() = (max - min) / std::max(1, bins);
        };

        add_hist("rapidity", y_min_, y_max_, y_bins_);
        add_hist("p_perp", pt_min_, pt_max_, pt_bins_);
    }

    int clamp_wounded_(int w) const {
        return std::clamp(w, wounded_min_, wounded_max_);
    }

    std::pair<int,int> bin_bounds_(int w) const {
        int cw = clamp_wounded_(w);
        int idx = (cw - wounded_min_) / wounded_bin_width_;
        int start = wounded_min_ + idx * wounded_bin_width_;
        int end = std::min(start + wounded_bin_width_ - 1, wounded_max_);
        return {start, end};
    }

    static std::string zpad3_(int x) {
        std::ostringstream o; o << std::setw(3) << std::setfill('0') << x; return o.str();
    }

    std::string wounded_range_label_(int start, int end) const {
        return "w" + zpad3_(start) + "-" + zpad3_(end);
    }

    DataNode& group_for_wounded_(int wounded) {
        auto [start, end] = bin_bounds_(wounded);
        return wounded_node_.add_child(wounded_range_label_(start, end));
    }

    Data& get_or_make_histogram_(DataNode& group, const std::string& key, const Histogram1D& ref) {
        DataNode& node = group.add_child(key);
        Data& d = node.get_data();
        if (!std::holds_alternative<Histogram1D>(d)) d = ref;
        return d;
    }

private:
    double y_min_, y_max_;
    int    y_bins_;
    double pt_min_, pt_max_;
    int    pt_bins_;
    int    wounded_bin_width_, wounded_min_, wounded_max_;

    Histogram1D y_hist_;
    Histogram1D pt_hist_;

    DataNode& wounded_node_;
    std::unordered_set<int> selected_pdgs_;
};

REGISTER_ANALYSIS("Rapidity", RapidityAndPtHistogramAnalysis);
