#pragma once

#include <string>

// Encode a PNG image to .ezc format.
// Returns 0 on success, non-zero on failure.
int encode(const std::string& inputPng,
           const std::string& outputEzc,
           int quality);

// Decode an .ezc file back to a PNG image.
// Returns 0 on success, non-zero on failure.
int decode(const std::string& inputEzc,
           const std::string& outputPng);
