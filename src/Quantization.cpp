#include "ezcodec/Quantization.h"
#include <algorithm>

int Quantization::getScaleFactor(int quality) {
    // Clamp quality to [1, 100]
    quality = std::clamp(quality, 1, 100);

    // Scaling formula from the JPEG standard
    int scale;
    if (quality < 50) {
        scale = 5000 / quality;
    } else {
        scale = 200 - quality * 2;
    }

    return std::max(scale, 1);
}

int Quantization::getQuantizationValue(int index, int quality) {
    // Get base value from the quantization table
    int baseValue = JPEG_LUMINANCE_QUANTIZATION_TABLE[index];

    // Scale based on quality
    int scale = getScaleFactor(quality);

    // Apply scaling: (baseValue * scale + 50) / 100
    int quantValue = (baseValue * scale + 50) / 100;

    // Minimum quantization value = 1
    return std::max(quantValue, 1);
}
