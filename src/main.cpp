#include <iostream>
#include <string>
#include <algorithm>
#include "ezcodec/Codec.h"

static void printUsage(const char* progName) {
    std::cout << "Usage:\n"
              << "  " << progName << " encode -i <input.png> -o <output.ezc> [-q <quality>] [--fermat] [--fermat-n N] [--fermat-l L]\n"
              << "  " << progName << " decode -i <input.ezc> -o <output.png>\n"
              << "  " << progName << " fermat -i <input.png> -o <output.png> [--fermat-n N] [--fermat-l L]\n"
              << "  " << progName << " --help\n"
              << "  " << progName << " --version\n"
              << "\n"
              << "Options:\n"
              << "  -i, --input      Input file path (required)\n"
              << "  -o, --output     Output file path (required)\n"
              << "  -q, --quality    Compression quality 1-100 (encode only, default: 50)\n"
              << "\n"
              << "Fermat spiral color encoding (encode only):\n"
              << "  --fermat         Enable Fermat spiral color mode (1 channel instead of 3)\n"
              << "  --fermat-n N     Number of spiral turns (default: 3)\n"
              << "  --fermat-l L     HSL lightness 0.0-1.0 (default: 0.5)\n"
              << "\n"
              << "Direct Fermat round-trip (fermat subcommand):\n"
              << "  Converts PNG → (RGB→t→RGB) → PNG without DCT compression.\n"
              << "  Shows pure color encoding quality, no codec artifacts.\n";
}

static void printVersion() {
    std::cout << "EzCodec 1.1.0" << std::endl;
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

    bool isEncode  = (cmd == "encode");
    bool isDecode  = (cmd == "decode");
    bool isFermat  = (cmd == "fermat");

    if (!isEncode && !isDecode && !isFermat) {
        std::cerr << "Unknown command: " << cmd << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    std::string inputPath;
    std::string outputPath;
    int quality = 50;
    FermatParams fermat;

    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "-i" || arg == "--input") && i + 1 < argc) {
            inputPath = argv[++i];
        } else if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
            outputPath = argv[++i];
        } else if ((arg == "-q" || arg == "--quality") && i + 1 < argc) {
            quality = std::stoi(argv[++i]);
        } else if (arg == "--fermat") {
            fermat.enabled = true;
        } else if (arg == "--fermat-n" && i + 1 < argc) {
            fermat.N = std::stoi(argv[++i]);
        } else if (arg == "--fermat-l" && i + 1 < argc) {
            fermat.L = std::stof(argv[++i]);
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
        fermat.N = std::clamp(fermat.N, 1, 255);
        fermat.L = std::clamp(fermat.L, 0.0f, 1.0f);
        return encode(inputPath, outputPath, quality, fermat);
    } else if (isDecode) {
        return decode(inputPath, outputPath);
    } else {
        fermat.N = std::clamp(fermat.N, 1, 255);
        fermat.L = std::clamp(fermat.L, 0.0f, 1.0f);
        return fermatDirect(inputPath, outputPath, fermat.N, fermat.L);
    }
}
