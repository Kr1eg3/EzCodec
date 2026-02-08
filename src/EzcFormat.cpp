#include "ezcodec/EzcFormat.h"
#include <fstream>
#include <iostream>
#include <cstring>

static constexpr uint8_t EZC_MAGIC[4] = { 'E', 'Z', 'C', '\0' };
static constexpr uint8_t EZC_VERSION = 1;

// Helper: write a little-endian uint16_t
static void writeU16(std::ofstream& out, uint16_t val) {
    uint8_t buf[2];
    buf[0] = static_cast<uint8_t>(val & 0xFF);
    buf[1] = static_cast<uint8_t>((val >> 8) & 0xFF);
    out.write(reinterpret_cast<const char*>(buf), 2);
}

// Helper: read a little-endian uint16_t
static uint16_t readU16(std::ifstream& in) {
    uint8_t buf[2];
    in.read(reinterpret_cast<char*>(buf), 2);
    return static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8);
}

// Helper: write a little-endian int16_t
static void writeI16(std::ofstream& out, int16_t val) {
    writeU16(out, static_cast<uint16_t>(val));
}

// Helper: read a little-endian int16_t
static int16_t readI16(std::ifstream& in) {
    return static_cast<int16_t>(readU16(in));
}

bool writeEzc(const std::string& path,
              const EzcHeader& header,
              const std::vector<Block8x8i16>& quantizedBlocks) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open file for writing: " << path << std::endl;
        return false;
    }

    // Write 16-byte header
    out.write(reinterpret_cast<const char*>(EZC_MAGIC), 4);
    out.put(static_cast<char>(header.version));
    writeU16(out, header.width);
    writeU16(out, header.height);
    out.put(static_cast<char>(header.quality));
    out.put(static_cast<char>(header.blockDim));
    writeU16(out, header.blockCountX);
    writeU16(out, header.blockCountY);
    out.put(0); // reserved

    // Write block data: 64 x int16_t per block
    for (const auto& block : quantizedBlocks) {
        for (size_t i = 0; i < 64; i++) {
            writeI16(out, static_cast<int16_t>(block[i]));
        }
    }

    if (!out) {
        std::cerr << "Error writing to file: " << path << std::endl;
        return false;
    }

    return true;
}

bool readEzc(const std::string& path,
             EzcHeader& header,
             std::vector<Block8x8i16>& quantizedBlocks) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open file for reading: " << path << std::endl;
        return false;
    }

    // Read and validate magic number
    uint8_t magic[4];
    in.read(reinterpret_cast<char*>(magic), 4);
    if (std::memcmp(magic, EZC_MAGIC, 4) != 0) {
        std::cerr << "Invalid .ezc file: bad magic number" << std::endl;
        return false;
    }

    // Read header fields
    header.version = static_cast<uint8_t>(in.get());
    if (header.version != EZC_VERSION) {
        std::cerr << "Unsupported .ezc version: " << static_cast<int>(header.version) << std::endl;
        return false;
    }

    header.width       = readU16(in);
    header.height      = readU16(in);
    header.quality     = static_cast<uint8_t>(in.get());
    header.blockDim    = static_cast<uint8_t>(in.get());
    header.blockCountX = readU16(in);
    header.blockCountY = readU16(in);
    in.get(); // reserved byte

    if (!in) {
        std::cerr << "Error reading .ezc header" << std::endl;
        return false;
    }

    // Read block data
    size_t totalBlocks = static_cast<size_t>(header.blockCountX) * header.blockCountY;
    quantizedBlocks.clear();
    quantizedBlocks.reserve(totalBlocks);

    for (size_t b = 0; b < totalBlocks; b++) {
        int blockX = static_cast<int>(b % header.blockCountX);
        int blockY = static_cast<int>(b / header.blockCountX);
        quantizedBlocks.emplace_back(blockX, blockY);
        auto& block = quantizedBlocks.back();

        for (size_t i = 0; i < 64; i++) {
            block[i] = readI16(in);
        }
    }

    if (!in) {
        std::cerr << "Error reading .ezc block data" << std::endl;
        return false;
    }

    return true;
}
