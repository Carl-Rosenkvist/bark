# BARK

A simple and extensible C++/Python library for reading and analyzing binary particle output files.

## Features

- C++ header-based binary file reader for particle data
- Plugin-style extensible analysis system (via registry macros)
- Optional Python bindings via `pybind11`
- Histogramming utilities
- No external dependencies (except optionally `pybind11`)
- Devloped primarly for SMASH
## Structure

```
bark/
├── include/
│   ├── binaryreader.h
│   ├── analysis.h
│   ├── analysisregister.h
│   └── histogram1d.h
├── src/
│   ├── binaryreader.cc
│   ├── analysisregister.cc
│   └── main.cc
├── analyses/
│   └── simple.cc  # example analysis
├── bindings.cpp   # optional pybind11 bindings
├── CMakeLists.txt
```

## Build Instructions

```bash
mkdir build
cd build
cmake ..
make
```
``
This reads the binary file and dispatches particle blocks to any registered analysis.

## Writing Analyses

You can create a custom analysis by subclassing `Analysis`:

```cpp
class MyAnalysis : public Analysis {
public:
    void analyze_particle_block(const ParticleBlock& block, const Accessor& accessor) override {
        // your logic here
    }
    void finalize() override {}
    void save(const std::string& path) override {}
};
```

Then register it in the same file:

```cpp
REGISTER_ANALYSIS("my_analysis", MyAnalysis)
```
## Example Analysis

```cpp
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

private:
    std::vector<int> selected_pdgs;
    double y_min;
    double y_max;
    int n_bins;

};

REGISTER_ANALYSIS("Rapidity", RapidityHistogramAnalysis);
```
## How Analyses Work

Each analysis plugin in BARK subclasses the `Analysis` interface and is responsible for processing particle blocks and storing results.

### Merging by Metadata

When you run over multiple binary files, BARK uses user-supplied metadata (like `sqrt_s`, `projectile`, `target`) to associate results with a **merge key**. All files with the same merge key are automatically merged:
- **Scalars** (`int`, `double`) are summed.
- **Vectors** are concatenated.
- **Histograms** are added bin-by-bin.

You define metadata like this:

```bash
./binary_reader file1.bin:sqrt_s=5.02,target=Pb file2.bin:sqrt_s=5.02,target=Pb simple pdg pz p0
```

### YAML Output

Each analysis writes a human-readable YAML file named after the analysis, e.g., `simple.yaml`, which contains:
- The `smash_version`
- The `merge_keys`
- The `data` block with all computed quantities

Example output:
```yaml
sqrt_s_5.02_target_Pb:
  smash_version: "2.0"
  merge_keys:
    sqrt_s: 5.02
    target: "Pb"
  data:
    dndy_pion:
      bins: [-1.0, 0.0, 1.0]
      values: [2.3, 5.1, 2.9]
    mean_pt: 0.493
```

This structure is suitable for post-processing in Python, ROOT, or YAML-aware tools.


## Python usage
```py 
from bark import BinaryReader, CollectorAccessor
import numpy as np
accessor = CollectorAccessor()
reader = BinaryReader("particles_binary.bin", ["pdg", "pz", "p0"], accessor)
reader.read()

pz = accessor.get_double_array("pz")
e  = accessor.get_double_array("p0")
pdg = accessor.get_int_array("pdg")

y = 0.5 * np.log((e + pz) / (e - pz))


```
