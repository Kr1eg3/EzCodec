#include "ezcodec/Picture.h"
#include <iostream>
#include "stb/stb_image.h"

Picture::Picture(const char* filename)
    : Picture(filename, 1) {}

Picture::Picture(const char* filename, int desired_channels) {
    data = stbi_load(filename, &width, &height, &bitdepth, desired_channels);
    if (data == nullptr) {
        std::cerr << "Failed to load image: " << filename << std::endl;
        return;
    }

    channels = desired_channels;
    channelBlocks.resize(channels);
    for (int c = 0; c < channels; c++) {
        channelBlocks[c] = splitIntoBlocks<uint16_t, TxSize::TX_8x8>(c, channels);
    }
}

Picture::~Picture() {
    if (data) {
        stbi_image_free(data);
    }
}
