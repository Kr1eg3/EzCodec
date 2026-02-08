#pragma once

#include <vector>
#include <memory>
#include "ezcodec/Block.h"

class Picture {
public:
    Picture(const char* filename);
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

    [[nodiscard]] inline bool isValid() const {
        return (data != nullptr && width > 0 && height > 0 && bitdepth > 0);
    }

    [[nodiscard]] std::vector<Block8x8ui16>& getBlocks() {
        return dataBlocks;
    }

    [[nodiscard]] const std::vector<Block8x8ui16>& getBlocks() const {
        return dataBlocks;
    }

    template<typename T, TxSize Size>
    [[nodiscard]] std::vector<Block<T, Size>> splitIntoBlocks() const {
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
                            block.at(row, col) = static_cast<T>(data[imgY * width + imgX]);
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
    int width = 0;
    int height = 0;
    int bitdepth = 0;

    // Raw image data in [0, 255] range from stbi_load (grayscale)
    unsigned char* data = nullptr;

    // Image split into 8x8 pixel blocks on load
    std::vector<Block8x8ui16> dataBlocks;
};
