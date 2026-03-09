#pragma once

#include <string>

struct FermatParams {
    bool  enabled  = false;
    int   N        = 3;
    float L        = 0.5f;
};

// Encode a PNG image to .ezc format.
// Returns 0 on success, non-zero on failure.
int encode(const std::string& inputPng,
           const std::string& outputEzc,
           int quality,
           const FermatParams& fermat = FermatParams{});

// Decode an .ezc file back to a PNG image.
// Returns 0 on success, non-zero on failure.
int decode(const std::string& inputEzc,
           const std::string& outputPng);

// Direct Fermat round-trip: PNG → (RGB→t→RGB) → PNG, no DCT.
// Returns 0 on success, non-zero on failure.
int fermatDirect(const std::string& inputPng,
                 const std::string& outputPng,
                 int N, float L);
