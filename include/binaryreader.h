// BinaryReader.h
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
#include <memory>

// Enum classes and helper structures
enum class Quantity {
    MASS, P0, PX, PY, PZ,
    PDG, NCOLL, CHARGE
};

enum class QuantityType {
    Double,
    Int32
};

struct QuantityInfo {
    Quantity quantity;
    QuantityType type;
};

size_t type_size(QuantityType t);

extern const std::unordered_map<std::string, QuantityInfo> quantity_string_map;

std::unordered_map<Quantity, size_t>
compute_quantity_layout(const std::vector<std::string>& names);

std::vector<char> read_chunk(std::ifstream& bfile, size_t size);

// Template helpers

template <typename T>
T extract_and_advance(const std::vector<char>& buffer, size_t& offset) {
    T value;
    std::memcpy(&value, buffer.data() + offset, sizeof(T));
    offset += sizeof(T);
    return value;
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

// Data structures
struct Header {
    char magic_number[5] = {};
    uint16_t format_version = 0;
    uint16_t format_variant = 0;
    std::string smash_version;

    void read(std::ifstream& bfile);
    void print() const;
};

struct EndBlock {
    uint32_t event_number;
    uint32_t ensamble_number;
    double impact_parameter;
    char empty;

    static constexpr size_t SIZE = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(double) + sizeof(char);
    void read(std::ifstream& bfile);
};

struct ParticleBlock {
    int32_t event_number;
    int32_t ensamble_number;
    uint32_t npart;
    std::vector<std::vector<char>> particles;

    void read(std::ifstream& bfile, size_t particle_size);
};

// Accessor base class
class Accessor {
public:
    virtual void on_particle_block(const ParticleBlock& block) {}
    virtual void on_end_block(const EndBlock& block) {}
    virtual ~Accessor() = default;

    void set_layout(const std::unordered_map<Quantity, size_t>* layout_in);

    template<typename T>
    T quantity(const std::string& name, const ParticleBlock& block, size_t particle_index) const;

    int32_t get_int(const std::string& name, const ParticleBlock& block, size_t i) const;
    double get_double(const std::string& name, const ParticleBlock& block, size_t i) const;
    virtual void on_header(Header& header_in){};
protected:
    const std::unordered_map<Quantity, size_t>* layout = nullptr;
    Header header;
};


template<typename T>
T Accessor::quantity(const std::string& name, const ParticleBlock& block, size_t particle_index) const {
    if (!layout) {
        throw std::runtime_error("Layout not set in Accessor");
    }
    if (particle_index >= block.particles.size()) {
        throw std::out_of_range("Invalid particle index");
    }
    return get_quantity<T>(block.particles[particle_index], name, *layout);
}

// BinaryReader class
class BinaryReader {
public:
    BinaryReader(const std::string& filename,
                 const std::vector<std::string>& selected,
                 std::shared_ptr<Accessor> accessor_in);
    void read();

private:
    std::ifstream file;
    size_t particle_size = 0;
    Header header;
    std::shared_ptr<Accessor> accessor;
    std::unordered_map<Quantity, size_t> layout;

    bool check_next(std::ifstream& bfile);
};

#endif // BINARY_READER_H
