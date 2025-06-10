#include <iostream>
#include <memory>
#include "binaryreader.h"

// Accessor that prints basic info and first PDGID from each particle block
class PrintingAccessor : public Accessor {
public:

    void on_particle_block(const ParticleBlock& block) override {
        std::cout << "Particle Block:\n";
        std::cout << "  Event #:    " << block.event_number << "\n";
        std::cout << "  Ensemble #: " << block.ensamble_number << "\n";
        std::cout << "  Particles:  " << block.npart << "\n";

        if (!block.particles.empty()) {
            const auto& first = block.particles[0];
            
        if (!layout) throw std::runtime_error("Layout not set in Accessor");
            size_t offset = layout->at(Quantity::PDGID);
            int32_t pdgid;
            std::memcpy(&pdgid, first.data() + offset, sizeof(int32_t));
            std::cout << "  First PDGID: " << pdgid << "\n";
        }
    }

    void on_end_block(const EndBlock& block) override {
        std::cout << "End Block:\n";
        std::cout << "  Event #:    " << block.event_number << "\n";
        std::cout << "  Impact par: " << block.impact_parameter << "\n";
    }
};

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <binary file path>\n";
        return 1;
    }

    const std::string filename = argv[1];
    std::vector<std::string> selected = {
        "mass", "p0", "pz", "pdgid", "ncoll"
    };

    try {
        auto accessor = std::make_shared<PrintingAccessor>();
        BinaryReader reader(filename, selected, accessor);
        reader.read();
    } catch (const std::exception& e) {
        std::cerr << "Error while reading file: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
