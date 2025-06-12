#ifndef HISTOGRAM1D_H
#define HISTOGRAM1D_H

#include <vector>
#include <iostream>
#include <iomanip>
#include <stdexcept>

class Histogram1D {
public:
    Histogram1D(double min, double max, size_t bins)
        : min_(min), max_(max), bins_(bins), counts_(bins, 0.0) 
    {
        if (max <= min || bins == 0) {
            throw std::invalid_argument("Invalid histogram range or bin count.");
        }
        bin_width_ = (max - min) / bins;
    }

    void fill(double value, double weight = 1.0) {
        if (value < min_ || value >= max_) return; // ignore out-of-range
        size_t bin = static_cast<size_t>((value - min_) / bin_width_);
        counts_[bin] += weight;
    }

    double get_bin_center(size_t i) const {
        if (i >= bins_) throw std::out_of_range("Invalid bin index");
        return min_ + (i + 0.5) * bin_width_;
    }

    double get_bin_count(size_t i) const {
        if (i >= bins_) throw std::out_of_range("Invalid bin index");
        return counts_[i];
    }

    size_t num_bins() const { return bins_; }

    void print(std::ostream& out = std::cout) const {
        out << std::fixed << std::setprecision(4);
        for (size_t i = 0; i < bins_; ++i) {
            out << get_bin_center(i) << "\t" << counts_[i] << "\n";
        }
    }

    void merge(const Histogram1D& other) {
        if (bins_ != other.bins_ || min_ != other.min_ || max_ != other.max_) {
            throw std::runtime_error("Cannot merge histograms with different binning.");
        }
        for (size_t i = 0; i < bins_; ++i) {
            counts_[i] += other.counts_[i];
        }
    }

private:
    double min_, max_, bin_width_;
    size_t bins_;
    std::vector<double> counts_;
};

#endif // HISTOGRAM1D_H
