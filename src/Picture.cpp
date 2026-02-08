#include "ezcodec/Picture.h"
#include <iostream>
#include "stb/stb_image.h"

Picture::Picture(const char* filename) {
    data = stbi_load(filename, &width, &height, &bitdepth, 1);
    if (data == nullptr) {
        std::cerr << "Failed to load image: " << filename << std::endl;
        return;
    }

    dataBlocks = splitIntoBlocks<uint16_t, TxSize::TX_8x8>();
}

Picture::~Picture() {
    if (data) {
        stbi_image_free(data);
    }
}
