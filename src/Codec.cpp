#include "ezcodec/Codec.h"
#include "ezcodec/Picture.h"
#include "ezcodec/DCT.h"
#include "ezcodec/Quantization.h"
#include "ezcodec/ThreadPool.h"
#include "ezcodec/EzcFormat.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>

#include "stb/stb_image_write.h"

int encode(const std::string& inputPng,
           const std::string& outputEzc,
           int quality) {

    // Load image (grayscale)
    Picture picture(inputPng.c_str());
    if (!picture.isValid()) {
        std::cerr << "Failed to load image: " << inputPng << std::endl;
        return 1;
    }

    auto& dataBlocks = picture.getBlocks();
    const int imageWidth = picture.getWidth();
    const int imageHeight = picture.getHeight();

    std::cout << "Image: " << imageWidth << "x" << imageHeight << std::endl;
    std::cout << "Blocks: " << dataBlocks.size() << std::endl;

    // Prepare DCT output blocks
    std::vector<Block8x8i16> dctBlocks;
    dctBlocks.reserve(dataBlocks.size());
    for (const auto& block : dataBlocks) {
        dctBlocks.emplace_back(block.getBlockX(), block.getBlockY());
    }

    // Forward DCT (multi-threaded)
    {
        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < dataBlocks.size(); i++) {
            futures.emplace_back(pool.enqueue([&, i] {
                DCT::forwardDCT(dataBlocks[i], dctBlocks[i]);
            }));
        }
        for (auto& f : futures) f.get();
    }
    std::cout << "Forward DCT completed." << std::endl;

    // Quantize (multi-threaded)
    std::vector<Block8x8i16> quantizedBlocks;
    quantizedBlocks.reserve(dctBlocks.size());
    for (const auto& block : dctBlocks) {
        quantizedBlocks.emplace_back(block.getBlockX(), block.getBlockY());
    }

    {
        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < dctBlocks.size(); i++) {
            futures.emplace_back(pool.enqueue([&, i] {
                Quantization::quantize(dctBlocks[i], quantizedBlocks[i], quality);
            }));
        }
        for (auto& f : futures) f.get();
    }
    std::cout << "Quantization completed (quality=" << quality << ")." << std::endl;

    // Write .ezc file
    const int blockDim = 8;
    EzcHeader header;
    header.version     = 1;
    header.width       = static_cast<uint16_t>(imageWidth);
    header.height      = static_cast<uint16_t>(imageHeight);
    header.quality     = static_cast<uint8_t>(quality);
    header.blockDim    = static_cast<uint8_t>(blockDim);
    header.blockCountX = static_cast<uint16_t>((imageWidth + blockDim - 1) / blockDim);
    header.blockCountY = static_cast<uint16_t>((imageHeight + blockDim - 1) / blockDim);

    if (!writeEzc(outputEzc, header, quantizedBlocks)) {
        std::cerr << "Failed to write output file: " << outputEzc << std::endl;
        return 1;
    }

    std::cout << "Encoded to: " << outputEzc << std::endl;
    return 0;
}

int decode(const std::string& inputEzc,
           const std::string& outputPng) {

    // Read .ezc file
    EzcHeader header;
    std::vector<Block8x8i16> quantizedBlocks;
    if (!readEzc(inputEzc, header, quantizedBlocks)) {
        std::cerr << "Failed to read input file: " << inputEzc << std::endl;
        return 1;
    }

    const int imageWidth  = header.width;
    const int imageHeight = header.height;
    const int quality     = header.quality;

    std::cout << "Image: " << imageWidth << "x" << imageHeight
              << ", quality=" << quality << std::endl;
    std::cout << "Blocks: " << quantizedBlocks.size() << std::endl;

    // Dequantize (multi-threaded)
    std::vector<Block8x8i16> dequantizedBlocks;
    dequantizedBlocks.reserve(quantizedBlocks.size());
    for (const auto& block : quantizedBlocks) {
        dequantizedBlocks.emplace_back(block.getBlockX(), block.getBlockY());
    }

    {
        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < quantizedBlocks.size(); i++) {
            futures.emplace_back(pool.enqueue([&, i] {
                Quantization::dequantize(quantizedBlocks[i], dequantizedBlocks[i], quality);
            }));
        }
        for (auto& f : futures) f.get();
    }
    std::cout << "Dequantization completed." << std::endl;

    // Inverse DCT (multi-threaded)
    std::vector<Block8x8ui16> reconstructedBlocks;
    reconstructedBlocks.reserve(dequantizedBlocks.size());
    for (const auto& block : dequantizedBlocks) {
        reconstructedBlocks.emplace_back(block.getBlockX(), block.getBlockY());
    }

    {
        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < dequantizedBlocks.size(); i++) {
            futures.emplace_back(pool.enqueue([&, i] {
                DCT::inverseDCT(dequantizedBlocks[i], reconstructedBlocks[i]);
            }));
        }
        for (auto& f : futures) f.get();
    }
    std::cout << "Inverse DCT completed." << std::endl;

    // Reconstruct pixel buffer
    std::vector<unsigned char> pixels(imageWidth * imageHeight, 0);

    for (const auto& block : reconstructedBlocks) {
        int bx = block.getBlockX();
        int by = block.getBlockY();

        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                int imgX = bx * 8 + x;
                int imgY = by * 8 + y;

                if (imgX < imageWidth && imgY < imageHeight) {
                    int value = static_cast<int>(block.at(y, x));
                    value = std::clamp(value, 0, 255);
                    pixels[imgY * imageWidth + imgX] = static_cast<unsigned char>(value);
                }
            }
        }
    }

    // Save as PNG
    if (!stbi_write_png(outputPng.c_str(), imageWidth, imageHeight, 1,
                        pixels.data(), imageWidth)) {
        std::cerr << "Failed to write PNG: " << outputPng << std::endl;
        return 1;
    }

    std::cout << "Decoded to: " << outputPng << std::endl;
    return 0;
}
