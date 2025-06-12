# smash-binary-reader

A simple and extensible C++/Python library for reading and analyzing SMASH binary output files.

## Features

- C++ header-based binary file reader for SMASH particle data
- Plugin-style extensible analysis system (via registry macros)
- Optional Python bindings via `pybind11`
- Histogramming utilities
- No external dependencies (except optionally `pybind11`)

## Structure

```
smash-binary-reader/
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

```bash
./binary_reader path/to/file.dat
```

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


## Current status of main
Currently the project has not yet implemented a proper main file but this is work in progress

```cpp
int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <binary file path>\n";
        return 1;
    }

    const std::string filename = argv[1];
    
    std::vector<std::string> selected = {
        "p0", "px", "py", "pz", "pdg", "charge"
    };

    auto dispatcher = std::make_shared<DispatchingAccessor>();
    auto analysis = AnalysisRegistry::instance().create("simple"); 
    dispatcher->register_analysis(analysis);
    BinaryReader reader(filename, selected, dispatcher);
    reader.read();
    
    analysis->save(""); 
    return 0;
}

```
