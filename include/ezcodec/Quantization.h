#pragma once

#include "ezcodec/Block.h"
#include <array>

class Quantization {
public:
    // Standard JPEG luminance quantization table
    static constexpr std::array<int, 64> JPEG_LUMINANCE_QUANTIZATION_TABLE = {
        16, 11, 10, 16,  24,  40,  51,  61,
        12, 12, 14, 19,  26,  58,  60,  55,
        14, 13, 16, 24,  40,  57,  69,  56,
        14, 17, 22, 29,  51,  87,  80,  62,
        18, 22, 37, 56,  68, 109, 103,  77,
        24, 35, 55, 64,  81, 104, 113,  92,
        49, 64, 78, 87, 103, 121, 120, 101,
        72, 92, 95, 98, 112, 100, 103,  99
    };

    // Quantize a block of DCT coefficients
    // srcBlock - block with DCT coefficients (int16_t or float)
    // dstBlock - block for quantized coefficients (int16_t)
    // quality  - compression quality (1-100, where 100 = minimum compression)
    template<typename SrcT, typename DstT, TxSize Size>
    static void quantize(const Block<SrcT, Size>& srcBlock,
                        Block<DstT, Size>& dstBlock,
                        int quality = 50);

    // Dequantize a block
    // srcBlock - block with quantized coefficients
    // dstBlock - block for recovered DCT coefficients
    template<typename SrcT, typename DstT, TxSize Size>
    static void dequantize(const Block<SrcT, Size>& srcBlock,
                          Block<DstT, Size>& dstBlock,
                          int quality = 50);

private:
    // Calculate scaling factor based on quality
    static int getScaleFactor(int quality);

    // Get quantization table value scaled by quality
    static int getQuantizationValue(int index, int quality);
};

// Template method implementations
template<typename SrcT, typename DstT, TxSize Size>
void Quantization::quantize(const Block<SrcT, Size>& srcBlock,
                           Block<DstT, Size>& dstBlock,
                           int quality) {
    constexpr size_t elementCount = getTxElementCount(Size);

    // For sizes other than 8x8, use simple scalar quantization
    // (real JPEG only uses 8x8 blocks)
    if constexpr (Size != TxSize::TX_8x8) {
        int scaleFactor = getScaleFactor(quality);
        for (size_t i = 0; i < elementCount; i++) {
            dstBlock[i] = static_cast<DstT>(srcBlock[i] / scaleFactor);
        }
        return;
    }

    // Quantization for 8x8 blocks using the JPEG table
    for (size_t i = 0; i < elementCount; i++) {
        int quantValue = getQuantizationValue(static_cast<int>(i), quality);
        dstBlock[i] = static_cast<DstT>(
            (srcBlock[i] >= 0)
                ? (srcBlock[i] + quantValue / 2) / quantValue
                : (srcBlock[i] - quantValue / 2) / quantValue
        );
    }
}

template<typename SrcT, typename DstT, TxSize Size>
void Quantization::dequantize(const Block<SrcT, Size>& srcBlock,
                             Block<DstT, Size>& dstBlock,
                             int quality) {
    constexpr size_t elementCount = getTxElementCount(Size);

    if constexpr (Size != TxSize::TX_8x8) {
        // Simple dequantization for non-8x8 sizes
        int scaleFactor = getScaleFactor(quality);
        for (size_t i = 0; i < elementCount; i++) {
            dstBlock[i] = static_cast<DstT>(srcBlock[i] * scaleFactor);
        }
        return;
    }

    // Dequantization for 8x8 blocks
    for (size_t i = 0; i < elementCount; i++) {
        int quantValue = getQuantizationValue(static_cast<int>(i), quality);
        dstBlock[i] = static_cast<DstT>(srcBlock[i] * quantValue);
    }
}
