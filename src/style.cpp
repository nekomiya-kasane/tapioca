/**
 * @file style.cpp
 * @brief Color downgrade algorithm implementation.
 */

#include "tapioca/style.h"

#include "tapioca/terminal.h"

#include <cfloat>
#include <cmath>
#include <cstdint>

namespace tapioca {

// ── 256-color palette RGB values ────────────────────────────────────────

namespace {

struct rgb_triple {
    uint8_t r, g, b;
};

// Standard 16 ANSI colors (approximate)
constexpr rgb_triple ansi_16[] = {
    {0, 0, 0},       // 0  black
    {128, 0, 0},     // 1  red
    {0, 128, 0},     // 2  green
    {128, 128, 0},   // 3  yellow
    {0, 0, 128},     // 4  blue
    {128, 0, 128},   // 5  magenta
    {0, 128, 128},   // 6  cyan
    {192, 192, 192}, // 7  white
    {128, 128, 128}, // 8  bright black
    {255, 0, 0},     // 9  bright red
    {0, 255, 0},     // 10 bright green
    {255, 255, 0},   // 11 bright yellow
    {0, 0, 255},     // 12 bright blue
    {255, 0, 255},   // 13 bright magenta
    {0, 255, 255},   // 14 bright cyan
    {255, 255, 255}, // 15 bright white
};

// 6x6x6 color cube component values (indices 16-231)
constexpr uint8_t cube_values[] = {0, 95, 135, 175, 215, 255};

// Grayscale ramp (indices 232-255): 8, 18, 28, ..., 238
constexpr uint8_t gray_value(uint8_t idx) {
    return static_cast<uint8_t>(8 + (idx - 232) * 10);
}

// ── OKLCH perceptual distance ────────────────────────────────────────

float srgb_to_linear(float c) {
    return (c <= 0.04045f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

struct oklch_triple {
    float L, C, h;
};

oklch_triple rgb_to_oklch(uint8_t r8, uint8_t g8, uint8_t b8) {
    float r = srgb_to_linear(r8 / 255.0f);
    float g = srgb_to_linear(g8 / 255.0f);
    float b = srgb_to_linear(b8 / 255.0f);
    float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
    float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
    float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;
    l = std::cbrt(l);
    m = std::cbrt(m);
    s = std::cbrt(s);
    float L = 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s;
    float a_ = 1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s;
    float b_ = 0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s;
    float C = std::sqrt(a_ * a_ + b_ * b_);
    float h = std::atan2(b_, a_) * (180.0f / 3.14159265358979f);
    if (h < 0.0f) h += 360.0f;
    return {L, C, h};
}

/** @brief Perceptual distance in OKLCH space (weighted deltaE). */
float oklch_distance(oklch_triple a, oklch_triple b) {
    float dL = a.L - b.L;
    float dC = a.C - b.C;
    float dh = a.h - b.h;
    if (dh > 180.0f) dh -= 360.0f;
    if (dh < -180.0f) dh += 360.0f;
    // Convert hue difference to chord distance in chroma plane
    float avg_C = (a.C + b.C) * 0.5f;
    float dH = 2.0f * avg_C * std::sin((dh * 3.14159265358979f / 180.0f) * 0.5f);
    // Weighted: lightness is most important, then chroma, then hue
    return dL * dL + dC * dC + dH * dH;
}

// Find nearest 256-color index for an RGB value (OKLCH perceptual distance)
uint8_t rgb_to_256(uint8_t r, uint8_t g, uint8_t b) {
    oklch_triple src = rgb_to_oklch(r, g, b);

    // Check cube colors (16-231) — start with nearest-cube heuristic
    auto nearest_cube = [](uint8_t v) -> uint8_t {
        if (v < 48) return 0;
        if (v < 115) return 1;
        if (v < 155) return 2;
        if (v < 195) return 3;
        if (v < 235) return 4;
        return 5;
    };
    uint8_t ci = nearest_cube(r);
    uint8_t cj = nearest_cube(g);
    uint8_t ck = nearest_cube(b);
    uint8_t best_idx = static_cast<uint8_t>(16 + 36 * ci + 6 * cj + ck);
    float best_dist = oklch_distance(src, rgb_to_oklch(cube_values[ci], cube_values[cj], cube_values[ck]));

    // Also check neighboring cube cells (+-1 on each axis) for better match
    for (int di = -1; di <= 1; ++di) {
        int ni = ci + di;
        if (ni < 0 || ni > 5) continue;
        for (int dj = -1; dj <= 1; ++dj) {
            int nj = cj + dj;
            if (nj < 0 || nj > 5) continue;
            for (int dk = -1; dk <= 1; ++dk) {
                int nk = ck + dk;
                if (nk < 0 || nk > 5) continue;
                float d = oklch_distance(src, rgb_to_oklch(cube_values[ni], cube_values[nj], cube_values[nk]));
                if (d < best_dist) {
                    best_dist = d;
                    best_idx = static_cast<uint8_t>(16 + 36 * ni + 6 * nj + nk);
                }
            }
        }
    }

    // Check grayscale ramp (232-255)
    for (int i = 232; i <= 255; ++i) {
        uint8_t gv = gray_value(static_cast<uint8_t>(i));
        float d = oklch_distance(src, rgb_to_oklch(gv, gv, gv));
        if (d < best_dist) {
            best_dist = d;
            best_idx = static_cast<uint8_t>(i);
        }
    }

    return best_idx;
}

// Find nearest 16-color index for an RGB value (OKLCH perceptual distance)
uint8_t rgb_to_16(uint8_t r, uint8_t g, uint8_t b) {
    oklch_triple src = rgb_to_oklch(r, g, b);
    uint8_t best = 0;
    float best_dist = FLT_MAX;
    for (uint8_t i = 0; i < 16; ++i) {
        float d = oklch_distance(src, rgb_to_oklch(ansi_16[i].r, ansi_16[i].g, ansi_16[i].b));
        if (d < best_dist) {
            best_dist = d;
            best = i;
        }
    }
    return best;
}

// Convert 256-color index to RGB
rgb_triple idx256_to_rgb(uint8_t idx) {
    if (idx < 16) return ansi_16[idx];
    if (idx < 232) {
        uint8_t i = idx - 16;
        uint8_t ri = i / 36;
        uint8_t gi = (i % 36) / 6;
        uint8_t bi = i % 6;
        return {cube_values[ri], cube_values[gi], cube_values[bi]};
    }
    uint8_t gv = gray_value(idx);
    return {gv, gv, gv};
}

} // anonymous namespace

color color::downgrade(uint8_t target_depth) const noexcept {
    // target_depth maps to color_depth enum values:
    // 0 = none, 1 = basic_16, 2 = palette_256, 3 = true_color

    if (is_default()) return *this;

    // No downgrade needed if target >= current
    if (target_depth >= static_cast<uint8_t>(kind)) return *this;

    // Target = none: strip all color
    if (target_depth == 0) return default_color();

    // Current is RGB
    if (kind == color_kind::rgb) {
        if (target_depth >= 2) {
            // RGB -> 256
            return from_index_256(rgb_to_256(r, g, b));
        }
        // RGB -> 16
        return from_index_16(rgb_to_16(r, g, b));
    }

    // Current is 256-color
    if (kind == color_kind::indexed_256) {
        if (target_depth >= 2) return *this;
        // 256 -> 16
        if (r < 16) return from_index_16(r); // already a basic color
        auto [cr, cg, cb] = idx256_to_rgb(r);
        return from_index_16(rgb_to_16(cr, cg, cb));
    }

    // Current is 16-color -- already minimal
    return *this;
}

} // namespace tapioca
