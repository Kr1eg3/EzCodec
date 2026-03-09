#pragma once

#include <vector>
#include <memory>
#include "ezcodec/Block.h"

class Picture {
public:
    // Load as grayscale (1 channel)
    Picture(const char* filename);

    // Load with desired number of channels (1 = grayscale, 3 = RGB)
    Picture(const char* filename, int desired_channels);

    ~Picture();

    [[nodiscard]] inline unsigned char* getData() const {
        return data;
    }

    [[nodiscard]] inline int getWidth() const {
        return width;
    }

    [[nodiscard]] inline int getHeight() const {
        return height;
    }

    [[nodiscard]] inline int getBitdepth() const {
        return bitdepth;
    }

    [[nodiscard]] inline int getChannels() const {
        return channels;
    }

    [[nodiscard]] inline bool isValid() const {
        return (data != nullptr && width > 0 && height > 0 && bitdepth > 0);
    }

    // Access blocks for a specific channel (0=R/Gray, 1=G, 2=B)
    [[nodiscard]] std::vector<Block8x8ui16>& getChannelBlocks(int channel) {
        return channelBlocks[channel];
    }
    [[nodiscard]] const std::vector<Block8x8ui16>& getChannelBlocks(int channel) const {
        return channelBlocks[channel];
    }

    // Legacy accessor: returns channel 0 (backward compat)
    [[nodiscard]] std::vector<Block8x8ui16>& getBlocks() {
        return channelBlocks[0];
    }
    [[nodiscard]] const std::vector<Block8x8ui16>& getBlocks() const {
        return channelBlocks[0];
    }

    template<typename T, TxSize Size>
    [[nodiscard]] std::vector<Block<T, Size>> splitIntoBlocks(int channelIndex = 0,
                                                               int channelStride = 1) const {
        constexpr int blockDim = getTxDimension(Size);
        const int blockCountX = (width + blockDim - 1) / blockDim;
        const int blockCountY = (height + blockDim - 1) / blockDim;

        std::vector<Block<T, Size>> blocks;
        blocks.reserve(blockCountX * blockCountY);

        int blockY = 0;
        for (int by = 0; by < height; by += blockDim, blockY++) {
            int blockX = 0;
            for (int bx = 0; bx < width; bx += blockDim, blockX++) {
                blocks.emplace_back(blockX, blockY);
                auto& block = blocks.back();

                for (int row = 0; row < blockDim; row++) {
                    for (int col = 0; col < blockDim; col++) {
                        int imgX = bx + col;
                        int imgY = by + row;

                        if (imgX < width && imgY < height) {
                            block.at(row, col) = static_cast<T>(
                                data[(imgY * width + imgX) * channelStride + channelIndex]);
                        } else {
                            block.at(row, col) = T{};
                        }
                    }
                }
            }
        }

        return blocks;
    }

private:
    int width    = 0;
    int height   = 0;
    int bitdepth = 0;
    int channels = 1;

    // Raw image data from stbi_load (interleaved if channels > 1)
    unsigned char* data = nullptr;

    // Per-channel 8x8 pixel blocks: channelBlocks[c] for channel c
    std::vector<std::vector<Block8x8ui16>> channelBlocks;
};
