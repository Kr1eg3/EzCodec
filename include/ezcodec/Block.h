#pragma once

#include <memory>
#include <stdexcept>
#include <algorithm>
#include <type_traits>

enum class TxSize {
    TX_4x4   = 16,
    TX_8x8   = 64,
    TX_16x16 = 256,
    TX_32x32 = 1024
};

constexpr int getTxDimension(TxSize size) {
    switch(size) {
        case TxSize::TX_4x4:   return 4;
        case TxSize::TX_8x8:   return 8;
        case TxSize::TX_16x16: return 16;
        case TxSize::TX_32x32: return 32;
        default: return 8;
    }
}

constexpr size_t getTxElementCount(TxSize size) {
    return static_cast<size_t>(size);
}

template<typename T>
struct is_numeric {
    static constexpr bool value = std::is_arithmetic_v<T>;
};

template<typename T>
inline constexpr bool is_numeric_v = is_numeric<T>::value;

template<
    typename T = uint16_t,
    TxSize Size = TxSize::TX_8x8,
    typename = std::enable_if_t<is_numeric_v<T>>
>
class Block {
public:
    static constexpr TxSize block_size_type = Size;
    static constexpr size_t block_element_count = getTxElementCount(Size);
    static constexpr int block_dimension = getTxDimension(Size);

    Block(int x, int y)
        : blockX(x)
        , blockY(y)
        , data(std::make_unique<T[]>(block_element_count)) {
        std::fill_n(data.get(), block_element_count, T{});
    }

    Block(const Block& other)
        : blockX(other.blockX)
        , blockY(other.blockY)
        , data(std::make_unique<T[]>(block_element_count)) {
        std::copy_n(other.data.get(), block_element_count, data.get());
    }

    Block(Block&& other) noexcept = default;

    ~Block() = default;

    Block& operator=(const Block& other) {
        if (this != &other) {
            blockX = other.blockX;
            blockY = other.blockY;
            std::copy_n(other.data.get(), block_element_count, data.get());
        }
        return *this;
    }

    Block& operator=(Block&& other) noexcept = default;

    [[nodiscard]] T& operator[](size_t index) {
        return data[index];
    }

    [[nodiscard]] const T& operator[](size_t index) const {
        return data[index];
    }

    [[nodiscard]] T& at(size_t index) {
        if (index >= block_element_count) {
            throw std::out_of_range("Block index out of range");
        }
        return data[index];
    }

    [[nodiscard]] const T& at(size_t index) const {
        if (index >= block_element_count) {
            throw std::out_of_range("Block index out of range");
        }
        return data[index];
    }

    [[nodiscard]] T& at(size_t row, size_t col) {
        size_t index = row * block_dimension + col;
        if (index >= block_element_count || row >= block_dimension || col >= block_dimension) {
            throw std::out_of_range("Block coordinates out of range");
        }
        return data[index];
    }

    [[nodiscard]] const T& at(size_t row, size_t col) const {
        size_t index = row * block_dimension + col;
        if (index >= block_element_count || row >= block_dimension || col >= block_dimension) {
            throw std::out_of_range("Block coordinates out of range");
        }
        return data[index];
    }

    [[nodiscard]] int getBlockX() const { return blockX; }
    [[nodiscard]] int getBlockY() const { return blockY; }

    [[nodiscard]] constexpr size_t size() const { return block_element_count; }
    [[nodiscard]] constexpr int dimension() const { return block_dimension; }
    [[nodiscard]] constexpr TxSize sizeType() const { return block_size_type; }

    [[nodiscard]] T* getData() { return data.get(); }
    [[nodiscard]] const T* getData() const { return data.get(); }

    void fill(T value) {
        std::fill_n(data.get(), block_element_count, value);
    }

private:
    std::unique_ptr<T[]> data;
    int blockX;
    int blockY;
};

// Common block type aliases
using Block4x4f   = Block<float, TxSize::TX_4x4>;
using Block4x4d   = Block<double, TxSize::TX_4x4>;
using Block4x4i   = Block<int, TxSize::TX_4x4>;
using Block4x4ui8 = Block<uint8_t, TxSize::TX_4x4>;
using Block4x4ui16 = Block<uint16_t, TxSize::TX_4x4>;

using Block8x8f   = Block<float, TxSize::TX_8x8>;
using Block8x8d   = Block<double, TxSize::TX_8x8>;
using Block8x8i   = Block<int, TxSize::TX_8x8>;
using Block8x8i16 = Block<int16_t, TxSize::TX_8x8>;
using Block8x8ui8 = Block<uint8_t, TxSize::TX_8x8>;
using Block8x8ui16 = Block<uint16_t, TxSize::TX_8x8>;

using Block16x16f = Block<float, TxSize::TX_16x16>;
using Block32x32f = Block<float, TxSize::TX_32x32>;
