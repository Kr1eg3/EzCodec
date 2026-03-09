#include "ezcodec/EzcFormat.h"
#include <fstream>
#include <iostream>
#include <cstring>

static constexpr uint8_t EZC_MAGIC[4] = { 'E', 'Z', 'C', '\0' };
static constexpr uint8_t EZC_VERSION_FERMAT = 3;

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

// Write 16-byte header to an open stream
static void writeHeader(std::ofstream& out, const EzcHeader& header) {
    out.write(reinterpret_cast<const char*>(EZC_MAGIC), 4);
    out.put(static_cast<char>(header.version));
    writeU16(out, header.width);
    writeU16(out, header.height);
    out.put(static_cast<char>(header.quality));
    out.put(static_cast<char>(header.blockDim));
    writeU16(out, header.blockCountX);
    writeU16(out, header.blockCountY);
    out.put(static_cast<char>(header.channels)); // was "reserved = 0" in v1

    // v3: write 2 extra bytes for Fermat parameters
    if (header.version == EZC_VERSION_FERMAT) {
        out.put(static_cast<char>(header.fermat_N));
        out.put(static_cast<char>(header.fermat_L_x100));
    }
}

// Read 16-byte header from an open stream; returns false on error
static bool readHeader(std::ifstream& in, EzcHeader& header) {
    uint8_t magic[4];
    in.read(reinterpret_cast<char*>(magic), 4);
    if (std::memcmp(magic, EZC_MAGIC, 4) != 0) {
        std::cerr << "Invalid .ezc file: bad magic number" << std::endl;
        return false;
    }

    header.version = static_cast<uint8_t>(in.get());
    if (header.version != 1 && header.version != 2 && header.version != EZC_VERSION_FERMAT) {
        std::cerr << "Unsupported .ezc version: " << static_cast<int>(header.version) << std::endl;
        return false;
    }

    header.width       = readU16(in);
    header.height      = readU16(in);
    header.quality     = static_cast<uint8_t>(in.get());
    header.blockDim    = static_cast<uint8_t>(in.get());
    header.blockCountX = readU16(in);
    header.blockCountY = readU16(in);
    header.channels    = static_cast<uint8_t>(in.get()); // 0 in v1 files → treat as 1

    // v3: read 2 extra bytes for Fermat parameters
    if (header.version == EZC_VERSION_FERMAT) {
        header.fermat_N      = static_cast<uint8_t>(in.get());
        header.fermat_L_x100 = static_cast<uint8_t>(in.get());
    }

    return static_cast<bool>(in);
}

// Multi-channel write
bool writeEzc(const std::string& path,
              const EzcHeader& header,
              const std::vector<std::vector<Block8x8i16>>& channelBlocks) {
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open file for writing: " << path << std::endl;
        return false;
    }

    writeHeader(out, header);

    // Write all channels sequentially (channel-major / planar)
    for (const auto& blocks : channelBlocks) {
        for (const auto& block : blocks) {
            for (size_t i = 0; i < 64; i++) {
                writeI16(out, static_cast<int16_t>(block[i]));
            }
        }
    }

    if (!out) {
        std::cerr << "Error writing to file: " << path << std::endl;
        return false;
    }

    return true;
}

// Multi-channel read
bool readEzc(const std::string& path,
             EzcHeader& header,
             std::vector<std::vector<Block8x8i16>>& channelBlocks) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to open file for reading: " << path << std::endl;
        return false;
    }

    if (!readHeader(in, header)) return false;

    const int numChannels = (header.channels == 0) ? 1 : static_cast<int>(header.channels);
    const size_t blocksPerChannel =
        static_cast<size_t>(header.blockCountX) * header.blockCountY;

    channelBlocks.clear();
    channelBlocks.resize(numChannels);

    for (int c = 0; c < numChannels; c++) {
        channelBlocks[c].reserve(blocksPerChannel);
        for (size_t b = 0; b < blocksPerChannel; b++) {
            int blockX = static_cast<int>(b % header.blockCountX);
            int blockY = static_cast<int>(b / header.blockCountX);
            channelBlocks[c].emplace_back(blockX, blockY);
            auto& block = channelBlocks[c].back();
            for (size_t i = 0; i < 64; i++) {
                block[i] = readI16(in);
            }
        }
    }

    if (!in) {
        std::cerr << "Error reading .ezc block data" << std::endl;
        return false;
    }

    return true;
}

// Legacy single-channel wrapper
bool writeEzc(const std::string& path,
              const EzcHeader& header,
              const std::vector<Block8x8i16>& quantizedBlocks) {
    return writeEzc(path, header, std::vector<std::vector<Block8x8i16>>{ quantizedBlocks });
}

// Legacy single-channel wrapper
bool readEzc(const std::string& path,
             EzcHeader& header,
             std::vector<Block8x8i16>& quantizedBlocks) {
    std::vector<std::vector<Block8x8i16>> channelBlocks;
    if (!readEzc(path, header, channelBlocks)) return false;
    quantizedBlocks = std::move(channelBlocks[0]);
    return true;
}
