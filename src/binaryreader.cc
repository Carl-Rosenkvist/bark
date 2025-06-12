#include "binaryreader.h"

const std::unordered_map<std::string, QuantityInfo> quantity_string_map = {
    {"mass",   {Quantity::MASS,   QuantityType::Double}},
    {"p0",     {Quantity::P0,     QuantityType::Double}},
    {"px",     {Quantity::PX,     QuantityType::Double}},
    {"py",     {Quantity::PY,     QuantityType::Double}},
    {"pz",     {Quantity::PZ,     QuantityType::Double}},
    {"pdg",    {Quantity::PDG,    QuantityType::Int32}},
    {"ncoll",  {Quantity::NCOLL,  QuantityType::Int32}},
    {"charge", {Quantity::CHARGE, QuantityType::Int32}},
};

size_t type_size(QuantityType t) {
    switch (t) {
        case QuantityType::Double: return sizeof(double);
        case QuantityType::Int32:  return sizeof(int32_t);
        default: throw std::logic_error("Unknown QuantityType");
    }
}

std::unordered_map<Quantity, size_t>
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

std::vector<char> read_chunk(std::ifstream& bfile, size_t size) {
    std::vector<char> buffer(size);
    bfile.read(buffer.data(), size);
    if (!bfile) throw std::runtime_error("Read failed");
    return buffer;
}

void Header::read(std::ifstream& bfile) {
    bfile.read(magic_number, 4);
    magic_number[4] = '\0';

    bfile.read(reinterpret_cast<char*>(&format_version), sizeof(format_version));
    bfile.read(reinterpret_cast<char*>(&format_variant), sizeof(format_variant));

    uint32_t len;
    bfile.read(reinterpret_cast<char*>(&len), sizeof(len));
    std::vector<char> version_buffer(len);
    bfile.read(version_buffer.data(), len);
    smash_version.assign(version_buffer.begin(), version_buffer.end());

    if (!bfile) {
        throw std::runtime_error("Failed to read header from binary file");
    }
}

void Header::print() const {
    std::cout << "Magic Number:   " << magic_number << "\n";
    std::cout << "Format Version: " << format_version << "\n";
    std::cout << "Format Variant: " << format_variant << "\n";
    std::cout << "Smash Version:  " << smash_version << "\n";
}

void EndBlock::read(std::ifstream& bfile) {
    std::vector<char> buffer = read_chunk(bfile, SIZE);
    size_t offset = 0;
    event_number     = extract_and_advance<uint32_t>(buffer, offset);
    ensamble_number  = extract_and_advance<int32_t>(buffer, offset);
    impact_parameter = extract_and_advance<double>(buffer, offset);
    empty            = extract_and_advance<char>(buffer, offset);
}

void ParticleBlock::read(std::ifstream& bfile, size_t particle_size) {
    constexpr size_t HEADER_SIZE = sizeof(int32_t) + sizeof(int32_t) + sizeof(uint32_t);
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

void Accessor::set_layout(const std::unordered_map<Quantity, size_t>* layout_in) {
    layout = layout_in;
}

int32_t Accessor::get_int(const std::string& name, const ParticleBlock& block, size_t i) const {
    return quantity<int32_t>(name, block, i);
}

double Accessor::get_double(const std::string& name, const ParticleBlock& block, size_t i) const {
    return quantity<double>(name, block, i);
}

BinaryReader::BinaryReader(const std::string& filename,
                           const std::vector<std::string>& selected,
                           std::shared_ptr<Accessor> accessor_in)
    : file(filename, std::ios::binary), accessor(std::move(accessor_in))
{
    if (!file) {
        throw std::runtime_error("Could not open file: " + filename);
    }

    layout = compute_quantity_layout(selected);

    for (const std::string& name : selected) {
        const auto& info = quantity_string_map.at(name);
        particle_size += type_size(info.type);
    }

    if (!accessor) throw std::runtime_error("An accessor is needed!");
    accessor->set_layout(&layout);
}

void BinaryReader::read() {
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

bool BinaryReader::check_next(std::ifstream& bfile) {
    char blockType;
    bfile.read(reinterpret_cast<char*>(&blockType), sizeof(char));
    if (blockType == 'p' || blockType == 'f' || blockType == 'i') {
        bfile.seekg(-1, std::ios_base::cur);
        return true;
    } else {
        return false;
    }
}
