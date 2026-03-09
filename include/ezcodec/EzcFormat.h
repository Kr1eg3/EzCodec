#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "ezcodec/Block.h"

struct EzcHeader {
    uint8_t  version       = 2;
    uint16_t width         = 0;
    uint16_t height        = 0;
    uint8_t  quality       = 50;
    uint8_t  blockDim      = 8;
    uint16_t blockCountX   = 0;
    uint16_t blockCountY   = 0;
    uint8_t  channels      = 1;   // was "reserved = 0" in v1; 0 on wire means 1 (legacy)
    // v3 only (Fermat spiral color encoding):
    uint8_t  fermat_N      = 12;  // number of spiral turns
    uint8_t  fermat_L_x100 = 50;  // HSL lightness × 100 (50 → L=0.50)
};

// Multi-channel write/read (primary API).
bool writeEzc(const std::string& path,
              const EzcHeader& header,
              const std::vector<std::vector<Block8x8i16>>& channelBlocks);

bool readEzc(const std::string& path,
             EzcHeader& header,
             std::vector<std::vector<Block8x8i16>>& channelBlocks);

// Legacy single-channel wrappers (backward compat).
bool writeEzc(const std::string& path,
              const EzcHeader& header,
              const std::vector<Block8x8i16>& quantizedBlocks);

bool readEzc(const std::string& path,
             EzcHeader& header,
             std::vector<Block8x8i16>& quantizedBlocks);
