#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include <sstream>
#include <variant>
#include "binaryreader.h"
#include "analysis.h"
#include "analysisregister.h"
#include "utils.h"
int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0]
                  << " <file[:key=val,...]>... <analysis> <quantities...>"
                  << " [--no-save] [--no-print] [--output-folder <path>]\n";
        return 1;
    }

    std::vector<std::pair<std::string, MergeKey>> input_files;
    std::string analysis_name;
    std::vector<std::string> quantities;
    bool save_output = true, print_output = true;
    std::filesystem::path output_folder = ".";

    int i = 1;
    // Parse input files with optional :key=val
    for (; i < argc; ++i) {
        std::string arg = argv[i];
        if (ends_with(arg, ".bin") || arg.find(':') != std::string::npos) {
            auto pos = arg.find(':');
            if (pos == std::string::npos) {
                input_files.emplace_back(arg, MergeKey{});
            } else {
                std::string file = arg.substr(0, pos);
                MergeKey key = parse_merge_key(arg.substr(pos + 1));
                input_files.emplace_back(file, key);
            }
        } else {
            break;
        }
    }

    if (i >= argc) {
        std::cerr << "Error: No analysis specified.\n";
        return 1;
    }

    analysis_name = argv[i++];

    for (; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--no-save") {
            save_output = false;
        } else if (arg == "--no-print") {
            print_output = false;
        } else if (arg == "--output-folder") {
            if (i + 1 >= argc) {
                std::cerr << "Error: --output-folder requires a path argument.\n";
                return 1;
            }
            output_folder = argv[++i];
        } else {
            quantities.push_back(arg);
        }
    }

    if (quantities.empty()) {
        std::cerr << "Error: No quantities provided.\n";
        return 1;
    }

    if (save_output && !std::filesystem::exists(output_folder)) {
        std::filesystem::create_directories(output_folder);
    }

    std::unordered_map<MergeKey, std::shared_ptr<Analysis>, MergeKeyHash> analysis_map;

    for (const auto& [path, key] : input_files) {
        auto analysis = AnalysisRegistry::instance().create(analysis_name);
        if (!analysis) {
            std::cerr << "Unknown analysis: " << analysis_name << "\n";
            return 1;
        }
        analysis->set_merge_keys(key);

        auto dispatcher = std::make_shared<DispatchingAccessor>();
        dispatcher->register_analysis(analysis);

        BinaryReader reader(path, quantities, dispatcher);
        reader.read();

        auto& existing = analysis_map[key];
        if (existing) {
            *existing += *analysis;
        } else {
            analysis_map[key] = std::move(analysis);
        }
    }

    for (const auto& [key, analysis] : analysis_map) {
        analysis->finalize();
        std::string label = label_from_key(key);

   
        if (save_output) {
            std::filesystem::path filename = output_folder / (analysis_name + ".yaml");
            
            save_all_to_yaml(filename.string(), analysis_map);
        }
        if (print_output) {
            std::cout << "=== Result for " << (label.empty() ? "(no key)" : label) << " ===\n";
            analysis->print_result_to(std::cout);
        }
    }

    return 0;
}
