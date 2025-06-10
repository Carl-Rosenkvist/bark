#ifndef BINARY_READER_H
#define BINARY_READER_H

#include <array>
#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <cstring>
#include <algorithm>
#include <unordered_map>

// Generic field extraction with offset advancement
template <typename T>
T extract_and_advance(const std::vector<char>& buffer, size_t& offset) {
    T value;
    std::memcpy(&value, buffer.data() + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

// Buffered read from stream
inline std::vector<char> read_chunk(std::ifstream& bfile, size_t size) {
    std::vector<char> buffer(size);
    bfile.read(buffer.data(), size);
    if (!bfile) throw std::runtime_error("Read failed");
    return buffer;
}

// Reads the final block of an event
struct EndBlock {
    uint32_t event_number;

    uint32_t ensamble_number;
    double impact_parameter;
    char empty;

    static constexpr size_t SIZE = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(double) + sizeof(char);

    void read(std::ifstream& bfile) {
        std::vector<char> buffer = read_chunk(bfile, SIZE);

        size_t offset = 0;
        event_number     = extract_and_advance<uint32_t>(buffer, offset);
        ensamble_number  = extract_and_advance<int32_t>(buffer, offset);
        impact_parameter = extract_and_advance<double>(buffer, offset);
        empty            = extract_and_advance<char>(buffer, offset);
    }
};

// Reads a particle block with raw per-particle buffers
struct ParticleBlock {
    int32_t event_number;
    int32_t ensamble_number;
    uint32_t npart;
    std::vector<std::vector<char>> particles;

    void read(std::ifstream& bfile, size_t particle_size) {
        constexpr size_t HEADER_SIZE =
            sizeof(int32_t) + sizeof(int32_t) + sizeof(uint32_t);

        std::vector<char> buffer = read_chunk(bfile, HEADER_SIZE);

        size_t offset = 0;
        event_number     = extract_and_advance<int32_t>(buffer, offset);
        ensamble_number  = extract_and_advance<int32_t>(buffer, offset);
        npart            = extract_and_advance<uint32_t>(buffer, offset);

        std::vector<char> flat = read_chunk(bfile, npart * particle_size);
        particles.resize(npart);
        for (size_t i = 0; i < npart; ++i) {
            particles[i].assign(
                flat.begin() + i * particle_size,
                flat.begin() + (i + 1) * particle_size
            );
        }
    }
};


// Reads the header block of a file
struct Header {
    char magic_number[5] = {};
    uint16_t format_version = 0;
    uint16_t format_variant = 0;
    std::string smash_version;

    void read(std::ifstream& bfile) {
        // Read magic number
        bfile.read(magic_number, 4);
        magic_number[4] = '\0';

        // Read fixed header fields
        bfile.read(reinterpret_cast<char*>(&format_version), sizeof(format_version));
        bfile.read(reinterpret_cast<char*>(&format_variant), sizeof(format_variant));

        // Read variable-length SMASH version string
        uint32_t len;
        bfile.read(reinterpret_cast<char*>(&len), sizeof(len));

        std::vector<char> version_buffer(len);
        bfile.read(version_buffer.data(), len);
        smash_version.assign(version_buffer.begin(), version_buffer.end());

        if (!bfile) {
            throw std::runtime_error("Failed to read header from binary file");
        }
    }

    void print() const {
        std::cout << "Magic Number:   " << magic_number << "\n";
        std::cout << "Format Version: " << format_version << "\n";
        std::cout << "Format Variant: " << format_variant << "\n";
        std::cout << "Smash Version:  " << smash_version << "\n";
    }
};

// Declare your enum class
enum class Quantity {
    MASS, P0, PX, PY, PZ,
    PDGID, NCOLL,
    // ... more
};

enum class QuantityType {
    Double,
    Int32
};

struct QuantityInfo {
    Quantity quantity;
    QuantityType type;
};

inline size_t type_size(QuantityType t) {
    switch (t) {
        case QuantityType::Double: return sizeof(double);
        case QuantityType::Int32:  return sizeof(int32_t);
        default: throw std::logic_error("Unknown QuantityType");
    }
}

// Use a global string-to-info map (single source of truth)
const std::unordered_map<std::string, QuantityInfo> quantity_string_map = {
    {"mass",   {Quantity::MASS,   QuantityType::Double}},
    {"p0",     {Quantity::P0,     QuantityType::Double}},
    {"px",     {Quantity::PX,     QuantityType::Double}},
    {"py",     {Quantity::PY,     QuantityType::Double}},
    {"pz",     {Quantity::PZ,     QuantityType::Double}},
    {"pdgid",  {Quantity::PDGID,  QuantityType::Int32}},
    {"ncoll",  {Quantity::NCOLL,  QuantityType::Int32}},
    // ... more
};





inline std::unordered_map<Quantity, size_t>
compute_quantity_layout(const std::vector<std::string>& names) {
    std::unordered_map<Quantity, size_t> layout;
    size_t offset = 0;

    for (const auto& name : names) {
        auto it = quantity_string_map.find(name);
        if (it == quantity_string_map.end())
            throw std::runtime_error("Unknown quantity: " + name);

        layout[it->second.quantity] = offset;
        offset += type_size(it->second.type); 
    }

    return layout;
}


template<typename T>
T get_quantity(const std::vector<char>& particle,
               const std::string& name,
               const std::unordered_map<Quantity, size_t>& layout)
{
    auto it_info = quantity_string_map.find(name);
    if (it_info == quantity_string_map.end())
        throw std::runtime_error("Unknown quantity: " + name);

    if (std::is_same_v<T, double> && it_info->second.type != QuantityType::Double)
        throw std::runtime_error("Requested double, but quantity is not double");
    if (std::is_same_v<T, int32_t> && it_info->second.type != QuantityType::Int32)
        throw std::runtime_error("Requested int32, but quantity is not int32");

    Quantity q = it_info->second.quantity;
    auto it = layout.find(q);
    if (it == layout.end())
        throw std::runtime_error("Quantity not in layout: " + name);

    T value;
    std::memcpy(&value, particle.data() + it->second, sizeof(T));
    return value;
}


class Accessor {
public:
    virtual void on_particle_block(const ParticleBlock& block) {}
    virtual void on_end_block(const EndBlock& block) {}
    virtual ~Accessor() = default;

    void set_layout(const std::unordered_map<Quantity, size_t>* layout_in) {
        layout = layout_in;
    }

    template<typename T>
    T quantity(const std::string& name, const ParticleBlock& block, size_t particle_index) const {
        if (!layout) {
            throw std::runtime_error("Layout not set in Accessor");
        }
        if (particle_index >= block.particles.size()) {
            throw std::out_of_range("Invalid particle index");
        }
        return get_quantity<T>(block.particles[particle_index], name, *layout);
    }


    int32_t get_int(const std::string& name, const ParticleBlock& block, size_t i) const {
        return quantity<int32_t>(name, block, i);
    }

    double get_double(const std::string& name, const ParticleBlock& block, size_t i) const {
        return quantity<double>(name, block, i);
    }

protected:
    const std::unordered_map<Quantity, size_t>* layout = nullptr;
};

class BinaryReader {
public:
    BinaryReader(const std::string& filename,
                 const std::vector<std::string>& selected,
                 std::shared_ptr<Accessor> accessor_in)
        : file(filename, std::ios::binary),
          accessor(std::move(accessor_in))
    {
        if (!file) {
            throw std::runtime_error("Could not open file: " + filename);
        }
            // Compute layout
    layout = compute_quantity_layout(selected);

    for (const std::string& name : selected) {
        const auto& info = quantity_string_map.at(name);
        particle_size += type_size(info.type);
    }
    if(!accessor) throw std::runtime_error("A accessor is needed!");
    if(accessor) accessor->set_layout(&layout);
    


    }
    void read() {
        header.read(file);
        char blockType;

        while (file.read(&blockType, sizeof(blockType))) {
            switch (blockType) {
                case 'p': {

                   
                    ParticleBlock p_block;
                    p_block.read(file, particle_size);
                    if (accessor && check_next(file)) accessor->on_particle_block(p_block);

                    break;
                }
                case 'f': {
                    
                    EndBlock e_block;

                    e_block.read(file);
                    if (accessor && check_next(file)) accessor->on_end_block(e_block);
                    break;
                }
                case 'i':
                    break;
                default:
                    break;
            }
        }
    }

private:
    std::ifstream file;
    size_t particle_size = 0;
    Header header;
    std::shared_ptr<Accessor> accessor;

    std::unordered_map<Quantity, size_t> layout;


bool check_next(std::ifstream& bfile ){
  char blockType;
  bfile.read(reinterpret_cast<char*>(&blockType),sizeof(char));
  if(blockType == 'p' || blockType == 'f' || blockType == 'i'){ 
    bfile.seekg(-1, std::ios_base::cur);
    return true;
  }
  else return false;
}
};




#endif // BINARY_READER_H
