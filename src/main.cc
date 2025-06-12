#include <iostream>
#include <memory>
#include "binaryreader.h"
#include "analysis.h"

#include <dlfcn.h>  // for Unix/macOS
#include <filesystem>
#include "analysisregister.h"



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

