#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstring>

#include "ezcodec/Picture.h"
#include "ezcodec/Block.h"
#include "ezcodec/DCT.h"
#include "ezcodec/Quantization.h"
#include "ezcodec/ThreadPool.h"
#include "ezcodec/EzcFormat.h"
#include "ezcodec/Codec.h"

static int testsPassed = 0;
static int testsFailed = 0;

#define ASSERT_TRUE(cond, msg) \
    do { \
        if (!(cond)) { \
            std::cerr << "FAIL: " << (msg) << " (" << __FILE__ << ":" << __LINE__ << ")" << std::endl; \
            testsFailed++; \
            return; \
        } \
    } while(0)

static void testBlockCreation() {
    std::cout << "  Block creation... ";
    Block8x8ui16 block(0, 0);
    ASSERT_TRUE(block.size() == 64, "Block8x8 should have 64 elements");
    ASSERT_TRUE(block.dimension() == 8, "Block8x8 dimension should be 8");
    ASSERT_TRUE(block[0] == 0, "Block should be zero-initialized");

    block.at(3, 4) = 42;
    ASSERT_TRUE(block.at(3, 4) == 42, "at(row, col) read/write should work");
    ASSERT_TRUE(block[3 * 8 + 4] == 42, "operator[] should match at()");

    std::cout << "PASS" << std::endl;
    testsPassed++;
}

static void testDCTRoundTrip() {
    std::cout << "  DCT round-trip... ";
    Block8x8ui16 original(0, 0);
    for (size_t i = 0; i < 64; i++) {
        original[i] = static_cast<uint16_t>(100 + (i % 10) * 15);
    }

    Block8x8i16 dctResult(0, 0);
    DCT::forwardDCT(original, dctResult);

    Block8x8i16 dequant(0, 0);
    for (size_t i = 0; i < 64; i++) {
        dequant[i] = dctResult[i];
    }

    Block8x8ui16 reconstructed(0, 0);
    DCT::inverseDCT(dequant, reconstructed);

    int maxError = 0;
    for (size_t i = 0; i < 64; i++) {
        int diff = static_cast<int>(original[i]) - static_cast<int>(reconstructed[i]);
        if (diff < 0) diff = -diff;
        if (diff > maxError) maxError = diff;
    }

    ASSERT_TRUE(maxError <= 2, "DCT round-trip error should be <= 2 (integer rounding)");
    std::cout << "PASS (max error: " << maxError << ")" << std::endl;
    testsPassed++;
}

static void testQuantizationRoundTrip() {
    std::cout << "  Quantization round-trip... ";
    Block8x8i16 original(0, 0);
    for (size_t i = 0; i < 64; i++) {
        original[i] = static_cast<int16_t>(50 - static_cast<int>(i));
    }

    Block8x8i16 quantized(0, 0);
    Quantization::quantize(original, quantized, 90);

    Block8x8i16 dequantized(0, 0);
    Quantization::dequantize(quantized, dequantized, 90);

    // With lossy quantization, values should be approximately correct
    bool reasonable = true;
    for (size_t i = 0; i < 64; i++) {
        int diff = static_cast<int>(original[i]) - static_cast<int>(dequantized[i]);
        if (diff < 0) diff = -diff;
        if (diff > 50) {
            reasonable = false;
            break;
        }
    }

    ASSERT_TRUE(reasonable, "Quantization round-trip should produce reasonable values");
    std::cout << "PASS" << std::endl;
    testsPassed++;
}

static void testEzcFormatRoundTrip() {
    std::cout << "  EZC format round-trip... ";

    EzcHeader headerOut;
    headerOut.version     = 1;
    headerOut.width       = 16;
    headerOut.height      = 16;
    headerOut.quality     = 75;
    headerOut.blockDim    = 8;
    headerOut.blockCountX = 2;
    headerOut.blockCountY = 2;

    std::vector<Block8x8i16> blocksOut;
    for (int b = 0; b < 4; b++) {
        blocksOut.emplace_back(b % 2, b / 2);
        for (size_t i = 0; i < 64; i++) {
            blocksOut.back()[i] = static_cast<int16_t>(b * 64 + i - 128);
        }
    }

    const std::string testFile = "test_format.ezc";
    bool writeOk = writeEzc(testFile, headerOut, blocksOut);
    ASSERT_TRUE(writeOk, "writeEzc should succeed");

    EzcHeader headerIn;
    std::vector<Block8x8i16> blocksIn;
    bool readOk = readEzc(testFile, headerIn, blocksIn);
    ASSERT_TRUE(readOk, "readEzc should succeed");

    ASSERT_TRUE(headerIn.width == headerOut.width, "Width should match");
    ASSERT_TRUE(headerIn.height == headerOut.height, "Height should match");
    ASSERT_TRUE(headerIn.quality == headerOut.quality, "Quality should match");
    ASSERT_TRUE(headerIn.blockCountX == headerOut.blockCountX, "BlockCountX should match");
    ASSERT_TRUE(headerIn.blockCountY == headerOut.blockCountY, "BlockCountY should match");
    ASSERT_TRUE(blocksIn.size() == blocksOut.size(), "Block count should match");

    for (size_t b = 0; b < blocksIn.size(); b++) {
        for (size_t i = 0; i < 64; i++) {
            ASSERT_TRUE(blocksIn[b][i] == blocksOut[b][i], "Block data should match");
        }
    }

    std::remove(testFile.c_str());
    std::cout << "PASS" << std::endl;
    testsPassed++;
}

static void testThreadPool() {
    std::cout << "  ThreadPool... ";
    ThreadPool pool(4);
    std::vector<std::future<int>> results;

    for (int i = 0; i < 100; i++) {
        results.emplace_back(pool.enqueue([](int val) { return val * val; }, i));
    }

    for (int i = 0; i < 100; i++) {
        ASSERT_TRUE(results[i].get() == i * i, "ThreadPool result should be correct");
    }

    std::cout << "PASS" << std::endl;
    testsPassed++;
}

int main() {
    std::cout << "=== EzCodec Unit Tests ===" << std::endl;

    std::cout << "\n[Block]" << std::endl;
    testBlockCreation();

    std::cout << "\n[DCT]" << std::endl;
    testDCTRoundTrip();

    std::cout << "\n[Quantization]" << std::endl;
    testQuantizationRoundTrip();

    std::cout << "\n[EZC Format]" << std::endl;
    testEzcFormatRoundTrip();

    std::cout << "\n[ThreadPool]" << std::endl;
    testThreadPool();

    std::cout << "\n=== Results: " << testsPassed << " passed, "
              << testsFailed << " failed ===" << std::endl;

    return testsFailed > 0 ? 1 : 0;
}
