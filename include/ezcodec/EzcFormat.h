#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "ezcodec/Block.h"

struct EzcHeader {
    uint8_t  version     = 1;
    uint16_t width       = 0;
    uint16_t height      = 0;
    uint8_t  quality     = 50;
    uint8_t  blockDim    = 8;
    uint16_t blockCountX = 0;
    uint16_t blockCountY = 0;
};

// Write quantized blocks to an .ezc file.
// Returns true on success.
bool writeEzc(const std::string& path,
              const EzcHeader& header,
              const std::vector<Block8x8i16>& quantizedBlocks);

// Read an .ezc file into header + quantized blocks.
// Returns true on success.
bool readEzc(const std::string& path,
             EzcHeader& header,
             std::vector<Block8x8i16>& quantizedBlocks);
