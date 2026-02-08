#pragma once

#include "ezcodec/Block.h"
#include <cmath>

class DCT {
public:
    template<typename SrcT, typename DstT, TxSize Size>
    static void forwardDCT(const Block<SrcT, Size>& srcBlock, Block<DstT, Size>& dstBlock) {
        constexpr int dim = getTxDimension(Size);
        constexpr size_t count = getTxElementCount(Size);

        double temp[count];
        for (int u = 0; u < dim; u++) {
            for (int v = 0; v < dim; v++) {
                double sum = 0.0;
                for (int x = 0; x < dim; x++) {
                    for (int y = 0; y < dim; y++) {
                        sum += static_cast<double>(srcBlock[y * dim + x]) *
                               std::cos((2*x + 1) * u * PI / (2.0 * dim)) *
                               std::cos((2*y + 1) * v * PI / (2.0 * dim));
                    }
                }
                temp[v * dim + u] = (1.0 / (dim / 2.0)) * c(u) * c(v) * sum;
            }
        }

        for (size_t i = 0; i < count; i++) {
            dstBlock[i] = static_cast<DstT>(temp[i]);
        }
    }

    template<typename SrcT, typename DstT, TxSize Size>
    static void inverseDCT(const Block<SrcT, Size>& srcBlock, Block<DstT, Size>& dstBlock) {
        constexpr int dim = getTxDimension(Size);
        constexpr size_t count = getTxElementCount(Size);

        double temp[count];
        for (int x = 0; x < dim; x++) {
            for (int y = 0; y < dim; y++) {
                double sum = 0.0;
                for (int u = 0; u < dim; u++) {
                    for (int v = 0; v < dim; v++) {
                        sum += c(u) * c(v) * static_cast<double>(srcBlock[v * dim + u]) *
                               std::cos((2*x + 1) * u * PI / (2.0 * dim)) *
                               std::cos((2*y + 1) * v * PI / (2.0 * dim));
                    }
                }
                temp[y * dim + x] = (1.0 / (dim / 2.0)) * sum;
            }
        }

        for (size_t i = 0; i < count; i++) {
            dstBlock[i] = static_cast<DstT>(temp[i]);
        }
    }

private:
    static constexpr double PI = 3.14159265358979323846;

    static double c(int i) {
        return (i == 0) ? 1.0 / std::sqrt(2.0) : 1.0;
    }
};
