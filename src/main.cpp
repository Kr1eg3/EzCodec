#include <iostream>
#include <string>
#include <algorithm>
#include "ezcodec/Codec.h"

static void printUsage(const char* progName) {
    std::cout << "Usage:\n"
              << "  " << progName << " encode -i <input.png> -o <output.ezc> [-q <quality>]\n"
              << "  " << progName << " decode -i <input.ezc> -o <output.png>\n"
              << "  " << progName << " --help\n"
              << "  " << progName << " --version\n"
              << "\n"
              << "Options:\n"
              << "  -i, --input    Input file path (required)\n"
              << "  -o, --output   Output file path (required)\n"
              << "  -q, --quality  Compression quality 1-100 (encode only, default: 50)\n";
}

static void printVersion() {
    std::cout << "EzCodec 1.0.0" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "--help" || cmd == "-h") {
        printUsage(argv[0]);
        return 0;
    }
    if (cmd == "--version" || cmd == "-v") {
        printVersion();
        return 0;
    }

    bool isEncode = (cmd == "encode");
    bool isDecode = (cmd == "decode");

    if (!isEncode && !isDecode) {
        std::cerr << "Unknown command: " << cmd << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    std::string inputPath;
    std::string outputPath;
    int quality = 50;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            inputPath = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outputPath = argv[++i];
        } else if ((arg == "-q" || arg == "--quality") && i + 1 < argc) {
            quality = std::stoi(argv[++i]);
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    if (inputPath.empty()) {
        std::cerr << "Missing required argument: -i <input>" << std::endl;
        return 1;
    }
    if (outputPath.empty()) {
        std::cerr << "Missing required argument: -o <output>" << std::endl;
        return 1;
    }

    if (isEncode) {
        quality = std::clamp(quality, 1, 100);
        return encode(inputPath, outputPath, quality);
    } else {
        return decode(inputPath, outputPath);
    }
}
