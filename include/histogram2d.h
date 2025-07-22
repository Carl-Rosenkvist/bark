#ifndef HISTOGRAM2D_H
#define HISTOGRAM2D_H

#include <vector>
#include <iostream>
#include <iomanip>
#include <stdexcept>

class Histogram2D {
public:
    Histogram2D(double x_min, double x_max, size_t x_bins,
                double y_min, double y_max, size_t y_bins)
        : x_min_(x_min), x_max_(x_max), x_bins_(x_bins),
          y_min_(y_min), y_max_(y_max), y_bins_(y_bins),
          counts_(x_bins * y_bins, 0.0)
    {
        if (x_max <= x_min || y_max <= y_min || x_bins == 0 || y_bins == 0) {
            throw std::invalid_argument("Invalid histogram range or bin count.");
        }
        x_bin_width_ = (x_max - x_min) / x_bins;
        y_bin_width_ = (y_max - y_min) / y_bins;
    }

    void fill(double x, double y, double weight = 1.0) {
        if (x < x_min_ || x >= x_max_ || y < y_min_ || y >= y_max_) return;
        size_t x_bin = static_cast<size_t>((x - x_min_) / x_bin_width_);
        size_t y_bin = static_cast<size_t>((y - y_min_) / y_bin_width_);
        counts_[x_bin * y_bins_ + y_bin] += weight;
    }

    double x_bin_center(size_t i) const {
        if (i >= x_bins_) throw std::out_of_range("Invalid x bin index");
        return x_min_ + (i + 0.5) * x_bin_width_;
    }

    double y_bin_center(size_t j) const {
        if (j >= y_bins_) throw std::out_of_range("Invalid y bin index");
        return y_min_ + (j + 0.5) * y_bin_width_;
    }

    double get_bin_count(size_t i, size_t j) const {
        if (i >= x_bins_ || j >= y_bins_) throw std::out_of_range("Invalid bin index");
        return counts_[i * y_bins_ + j];
    }

    size_t num_x_bins() const { return x_bins_; }
    size_t num_y_bins() const { return y_bins_; }

    void print(std::ostream& out = std::cout) const {
        out << std::fixed << std::setprecision(4);
        for (size_t i = 0; i < x_bins_; ++i) {
            for (size_t j = 0; j < y_bins_; ++j) {
                out << x_bin_center(i) << "\t"
                    << y_bin_center(j) << "\t"
                    << get_bin_count(i, j) << "\n";
            }
        }
    }

    void scale(double factor) {
        for (double& count : counts_) {
            count *= factor;
        }
    }

    Histogram2D& operator+=(const Histogram2D& other) {
        if (x_bins_ != other.x_bins_ || y_bins_ != other.y_bins_ ||
            x_min_ != other.x_min_ || x_max_ != other.x_max_ ||
            y_min_ != other.y_min_ || y_max_ != other.y_max_) {
            throw std::runtime_error("Cannot add histograms with different binning.");
        }
        for (size_t i = 0; i < counts_.size(); ++i) {
            counts_[i] += other.counts_[i];
        }
        return *this;
    }

private:
    double x_min_, x_max_, x_bin_width_;
    double y_min_, y_max_, y_bin_width_;
    size_t x_bins_, y_bins_;
    std::vector<double> counts_;  // stored in row-major: [x][y]
};

#endif // HISTOGRAM2D_H
