#include "ezcodec/Codec.h"
#include "ezcodec/Picture.h"
#include "ezcodec/DCT.h"
#include "ezcodec/Quantization.h"
#include "ezcodec/ThreadPool.h"
#include "ezcodec/EzcFormat.h"
#include "ezcodec/FermatColor.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>

#include "stb/stb_image_write.h"

// ---------------------------------------------------------------------------
// Helper: run DCT + quantize on a block vector (multi-threaded)
// ---------------------------------------------------------------------------
static std::vector<Block8x8i16> dctAndQuantize(
    const std::vector<Block8x8ui16>& dataBlocks, int quality)
{
    const size_t n = dataBlocks.size();

    std::vector<Block8x8i16> dctBlocks;
    dctBlocks.reserve(n);
    for (const auto& b : dataBlocks)
        dctBlocks.emplace_back(b.getBlockX(), b.getBlockY());

    {
        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < n; i++)
            futures.emplace_back(pool.enqueue([&, i] {
                DCT::forwardDCT(dataBlocks[i], dctBlocks[i]);
            }));
        for (auto& f : futures) f.get();
    }

    std::vector<Block8x8i16> quantized;
    quantized.reserve(n);
    for (const auto& b : dctBlocks)
        quantized.emplace_back(b.getBlockX(), b.getBlockY());

    {
        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < n; i++)
            futures.emplace_back(pool.enqueue([&, i] {
                Quantization::quantize(dctBlocks[i], quantized[i], quality);
            }));
        for (auto& f : futures) f.get();
    }

    return quantized;
}

// ---------------------------------------------------------------------------
// Helper: dequantize + IDCT (multi-threaded)
// ---------------------------------------------------------------------------
static std::vector<Block8x8ui16> dequantizeAndIDCT(
    const std::vector<Block8x8i16>& quantized, int quality)
{
    const size_t n = quantized.size();

    std::vector<Block8x8i16> dequant;
    dequant.reserve(n);
    for (const auto& b : quantized)
        dequant.emplace_back(b.getBlockX(), b.getBlockY());

    {
        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < n; i++)
            futures.emplace_back(pool.enqueue([&, i] {
                Quantization::dequantize(quantized[i], dequant[i], quality);
            }));
        for (auto& f : futures) f.get();
    }

    std::vector<Block8x8ui16> reconstructed;
    reconstructed.reserve(n);
    for (const auto& b : dequant)
        reconstructed.emplace_back(b.getBlockX(), b.getBlockY());

    {
        ThreadPool pool(std::thread::hardware_concurrency());
        std::vector<std::future<void>> futures;
        for (size_t i = 0; i < n; i++)
            futures.emplace_back(pool.enqueue([&, i] {
                DCT::inverseDCT(dequant[i], reconstructed[i]);
            }));
        for (auto& f : futures) f.get();
    }

    return reconstructed;
}

// ---------------------------------------------------------------------------
// encode
// ---------------------------------------------------------------------------
int encode(const std::string& inputPng,
           const std::string& outputEzc,
           int quality,
           const FermatParams& fermat)
{
    Picture picture(inputPng.c_str(), 3);
    if (!picture.isValid()) {
        std::cerr << "Failed to load image: " << inputPng << std::endl;
        return 1;
    }

    const int imageWidth  = picture.getWidth();
    const int imageHeight = picture.getHeight();
    const int blockDim    = 8;

    EzcHeader header;
    header.width       = static_cast<uint16_t>(imageWidth);
    header.height      = static_cast<uint16_t>(imageHeight);
    header.quality     = static_cast<uint8_t>(quality);
    header.blockDim    = static_cast<uint8_t>(blockDim);
    header.blockCountX = static_cast<uint16_t>((imageWidth  + blockDim - 1) / blockDim);
    header.blockCountY = static_cast<uint16_t>((imageHeight + blockDim - 1) / blockDim);

    std::cout << "Image: " << imageWidth << "x" << imageHeight << std::endl;

    std::vector<std::vector<Block8x8i16>> allQuantized;

    if (fermat.enabled) {
        // --- Fermat spiral mode ---
        // Convert every RGB pixel to t ∈ [0,1], store as [0,255] grayscale

        const unsigned char* raw = picture.getData();
        const int N = fermat.N;

        // Build t-value blocks manually from raw interleaved RGB data
        const int blockCountX = header.blockCountX;
        const int blockCountY = header.blockCountY;

        std::vector<Block8x8ui16> tBlocks, lBlocks;
        tBlocks.reserve(static_cast<size_t>(blockCountX) * blockCountY);
        lBlocks.reserve(static_cast<size_t>(blockCountX) * blockCountY);

        for (int by = 0; by < imageHeight; by += blockDim) {
            for (int bx = 0; bx < imageWidth; bx += blockDim) {
                tBlocks.emplace_back(bx / blockDim, by / blockDim);
                lBlocks.emplace_back(bx / blockDim, by / blockDim);
                auto& tBlock = tBlocks.back();
                auto& lBlock = lBlocks.back();
                for (int row = 0; row < blockDim; row++) {
                    for (int col = 0; col < blockDim; col++) {
                        int imgX = bx + col;
                        int imgY = by + row;
                        if (imgX < imageWidth && imgY < imageHeight) {
                            int px = (imgY * imageWidth + imgX) * 3;
                            float t = Fermat::fromRGB(raw[px], raw[px+1], raw[px+2], N);
                            float l = Fermat::getLightness(raw[px], raw[px+1], raw[px+2]);
                            tBlock.at(row, col) = static_cast<uint16_t>(t * 4095.0f + 0.5f);
                            lBlock.at(row, col) = static_cast<uint16_t>(l * 255.0f + 0.5f);
                        } else {
                            tBlock.at(row, col) = 0;
                            lBlock.at(row, col) = 0;
                        }
                    }
                }
            }
        }

        std::cout << "Fermat: " << tBlocks.size()
                  << " blocks (N=" << N << ", with lightness plane)" << std::endl;

        allQuantized.push_back(dctAndQuantize(tBlocks, quality));
        allQuantized.push_back(dctAndQuantize(lBlocks, quality));

        header.version       = 3;
        header.channels      = 2;
        header.fermat_N      = static_cast<uint8_t>(std::clamp(N, 1, 255));
        header.fermat_L_x100 = 0; // unused: L encoded per-pixel in plane 1

        std::cout << "Fermat DCT + Quantization completed (quality=" << quality << ")." << std::endl;

    } else {
        // --- Standard RGB mode ---
        const int numChannels = picture.getChannels();
        header.version  = 2;
        header.channels = static_cast<uint8_t>(numChannels);

        allQuantized.resize(numChannels);
        for (int c = 0; c < numChannels; c++) {
            std::cout << "Channel " << c << ": "
                      << picture.getChannelBlocks(c).size() << " blocks" << std::endl;
            allQuantized[c] = dctAndQuantize(picture.getChannelBlocks(c), quality);
        }
        std::cout << "Forward DCT + Quantization completed (quality=" << quality << ")." << std::endl;
    }

    if (!writeEzc(outputEzc, header, allQuantized)) {
        std::cerr << "Failed to write output file: " << outputEzc << std::endl;
        return 1;
    }

    std::cout << "Encoded to: " << outputEzc << std::endl;
    return 0;
}

// ---------------------------------------------------------------------------
// decode
// ---------------------------------------------------------------------------
int decode(const std::string& inputEzc,
           const std::string& outputPng)
{
    EzcHeader header;
    std::vector<std::vector<Block8x8i16>> allQuantized;
    if (!readEzc(inputEzc, header, allQuantized)) {
        std::cerr << "Failed to read input file: " << inputEzc << std::endl;
        return 1;
    }

    const int imageWidth  = header.width;
    const int imageHeight = header.height;
    const int quality     = header.quality;

    std::cout << "Image: " << imageWidth << "x" << imageHeight
              << ", quality=" << quality << std::endl;

    if (header.version == 3) {
        // --- Fermat spiral decode ---
        const int N = header.fermat_N;
        const bool hasLPlane = (header.channels == 2);

        if (hasLPlane)
            std::cout << "Fermat mode: N=" << N << ", lightness plane enabled" << std::endl;
        else
            std::cout << "Fermat mode: N=" << N << ", L=" << header.fermat_L_x100 / 100.0f << std::endl;

        auto tReconstructed = dequantizeAndIDCT(allQuantized[0], quality);
        std::vector<Block8x8ui16> lReconstructed;
        if (hasLPlane)
            lReconstructed = dequantizeAndIDCT(allQuantized[1], quality);

        // Build a flat L-map for random access
        std::vector<float> lMap;
        if (hasLPlane) {
            lMap.assign(imageWidth * imageHeight, 0.5f);
            for (const auto& block : lReconstructed) {
                int bx = block.getBlockX();
                int by = block.getBlockY();
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int imgX = bx * 8 + x;
                        int imgY = by * 8 + y;
                        if (imgX < imageWidth && imgY < imageHeight) {
                            lMap[imgY * imageWidth + imgX] =
                                std::clamp(static_cast<int>(block.at(y, x)), 0, 255) / 255.0f;
                        }
                    }
                }
            }
        }

        // Convert t-values back to RGB
        std::vector<unsigned char> pixels(imageWidth * imageHeight * 3, 0);
        const float fixedL = hasLPlane ? 0.5f : header.fermat_L_x100 / 100.0f;

        for (const auto& block : tReconstructed) {
            int bx = block.getBlockX();
            int by = block.getBlockY();
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    int imgX = bx * 8 + x;
                    int imgY = by * 8 + y;
                    if (imgX < imageWidth && imgY < imageHeight) {
                        float t = std::clamp(static_cast<int>(block.at(y, x)), 0, 4095)
                                  / 4095.0f;
                        float L = hasLPlane ? lMap[imgY * imageWidth + imgX] : fixedL;
                        uint8_t r, g, b;
                        Fermat::toRGB(t, N, L, r, g, b);
                        int px = (imgY * imageWidth + imgX) * 3;
                        pixels[px]     = r;
                        pixels[px + 1] = g;
                        pixels[px + 2] = b;
                    }
                }
            }
        }

        std::cout << "Fermat inverse DCT completed." << std::endl;

        if (!stbi_write_png(outputPng.c_str(), imageWidth, imageHeight,
                            3, pixels.data(), imageWidth * 3)) {
            std::cerr << "Failed to write PNG: " << outputPng << std::endl;
            return 1;
        }

    } else {
        // --- Standard RGB decode ---
        const int numChannels = (header.channels == 0) ? 1 : static_cast<int>(header.channels);
        std::cout << "Channels: " << numChannels << std::endl;

        std::vector<unsigned char> pixels(imageWidth * imageHeight * numChannels, 0);

        for (int c = 0; c < numChannels; c++) {
            auto reconstructed = dequantizeAndIDCT(allQuantized[c], quality);

            for (const auto& block : reconstructed) {
                int bx = block.getBlockX();
                int by = block.getBlockY();
                for (int y = 0; y < 8; y++) {
                    for (int x = 0; x < 8; x++) {
                        int imgX = bx * 8 + x;
                        int imgY = by * 8 + y;
                        if (imgX < imageWidth && imgY < imageHeight) {
                            int value = std::clamp(static_cast<int>(block.at(y, x)), 0, 255);
                            pixels[(imgY * imageWidth + imgX) * numChannels + c] =
                                static_cast<unsigned char>(value);
                        }
                    }
                }
            }
        }

        std::cout << "Inverse DCT completed." << std::endl;

        if (!stbi_write_png(outputPng.c_str(), imageWidth, imageHeight,
                            numChannels, pixels.data(), imageWidth * numChannels)) {
            std::cerr << "Failed to write PNG: " << outputPng << std::endl;
            return 1;
        }
    }

    std::cout << "Decoded to: " << outputPng << std::endl;
    return 0;
}

// ---------------------------------------------------------------------------
// fermatDirect: RGB → t → RGB, no DCT/quantization
// ---------------------------------------------------------------------------
int fermatDirect(const std::string& inputPng,
                 const std::string& outputPng,
                 int N, float L)
{
    Picture picture(inputPng.c_str(), 3);
    if (!picture.isValid()) {
        std::cerr << "Failed to load image: " << inputPng << std::endl;
        return 1;
    }

    const int W = picture.getWidth();
    const int H = picture.getHeight();
    const unsigned char* raw = picture.getData();

    std::cout << "Image: " << W << "x" << H
              << " | Fermat direct N=" << N << " L=" << L << std::endl;

    std::vector<unsigned char> out(W * H * 3);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            const int src = (y * W + x) * 3;
            const float t      = Fermat::fromRGB(raw[src], raw[src+1], raw[src+2], N);
            const float lPixel = Fermat::getLightness(raw[src], raw[src+1], raw[src+2]);
            uint8_t r, g, b;
            Fermat::toRGB(t, N, lPixel, r, g, b);
            out[src]   = r;
            out[src+1] = g;
            out[src+2] = b;
        }
    }

    if (!stbi_write_png(outputPng.c_str(), W, H, 3, out.data(), W * 3)) {
        std::cerr << "Failed to write PNG: " << outputPng << std::endl;
        return 1;
    }

    std::cout << "Fermat direct → " << outputPng << std::endl;
    return 0;
}
