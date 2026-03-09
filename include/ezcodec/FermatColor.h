#pragma once

#include <cmath>
#include <cstdint>
#include <algorithm>

namespace Fermat {

// t ∈ [0,1] → RGB
// N — число витков спирали (разрешение палитры)
// L — яркость HSL [0,1] (рекомендуется 0.5)
inline void toRGB(float t, int N, float L,
                  uint8_t& outR, uint8_t& outG, uint8_t& outB)
{
    float S = std::sqrt(t);
    float H = std::fmod(static_cast<float>(N) * t, 1.0f);
    float a = S * std::fmin(L, 1.0f - L);

    const int k[3] = {0, 8, 4};
    float ch[3];
    for (int i = 0; i < 3; i++) {
        float lam = std::fmod(static_cast<float>(k[i]) + 12.0f * H, 12.0f);
        float val = lam - 3.0f;
        float v2  = 9.0f - lam;
        if (v2 < val) val = v2;
        if (val >  1.0f) val =  1.0f;
        if (val < -1.0f) val = -1.0f;
        ch[i] = L - a * val;
    }

    outR = static_cast<uint8_t>(std::clamp(ch[0] * 255.0f + 0.5f, 0.0f, 255.0f));
    outG = static_cast<uint8_t>(std::clamp(ch[1] * 255.0f + 0.5f, 0.0f, 255.0f));
    outB = static_cast<uint8_t>(std::clamp(ch[2] * 255.0f + 0.5f, 0.0f, 255.0f));
}

// RGB → HSL lightness L = (cmax + cmin) / 2
inline float getLightness(uint8_t r, uint8_t g, uint8_t b)
{
    float R = r / 255.0f, G = g / 255.0f, B = b / 255.0f;
    float cmax = R, cmin = R;
    if (G > cmax) cmax = G;  if (B > cmax) cmax = B;
    if (G < cmin) cmin = G;  if (B < cmin) cmin = B;
    return (cmax + cmin) * 0.5f;
}

// RGB → t ∈ [0,1]
// N должен совпадать с тем, что использовался при кодировании
inline float fromRGB(uint8_t r, uint8_t g, uint8_t b, int N)
{
    float R = r / 255.0f, G = g / 255.0f, B = b / 255.0f;
    float cmax = R, cmin = R;
    if (G > cmax) cmax = G;  if (B > cmax) cmax = B;
    if (G < cmin) cmin = G;  if (B < cmin) cmin = B;
    float delta = cmax - cmin;

    float H = 0.0f;
    if (delta >= 1e-6f) {
        float h;
        if      (cmax == R) h =        (G - B) / delta;
        else if (cmax == G) h = 2.0f + (B - R) / delta;
        else                h = 4.0f + (R - G) / delta;
        h /= 6.0f;
        H = h - std::floor(h);
    }

    // Используем chroma (delta) вместо HSL-S:
    // HSL-S зависит от L, а delta = cmax-cmin — нет.
    // toRGB при L=0.5 даёт chroma = sqrt(t)*0.5, то есть sqrt(t) = delta → t = delta².
    float t_S = delta * delta;
    float best_t = t_S, best_d = 1e9f;
    for (int k = 0; k < N; k++) {
        float t_H = (H + static_cast<float>(k)) / static_cast<float>(N);
        float d   = std::fabs(t_H - t_S);
        if (d < best_d) { best_d = d; best_t = t_H; }
    }

    // Взвешенное среднее: у краёв доверяем Hue, у центра — chroma
    float t = (1.0f - delta) * t_S + delta * best_t;
    return std::clamp(t, 0.0f, 1.0f);
}

} // namespace Fermat
