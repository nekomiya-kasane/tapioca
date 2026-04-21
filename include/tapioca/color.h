#pragma once

/**
 * @file color.h
 * @brief Extended color module: gradients, color space conversions, contrast.
 *
 * Builds on tapioca::color (defined in style.h) with:
 * - Linear gradient interpolation (moved from tapiru/core/gradient.h)
 * - HSL <-> RGB conversion
 * - OKLCH <-> RGB conversion (perceptual uniform color space)
 * - WCAG 2.1 relative luminance and contrast ratio
 * - Perceptual color blending (OKLCH-based)
 */

#include "tapioca/layout_types.h"
#include "tapioca/style.h"

#include <cmath>
#include <cstdint>

namespace tapioca {

    // ── Gradient ────────────────────────────────────────────────────────────

    /** @brief Direction of a linear gradient. */
    enum class gradient_direction : uint8_t {
        horizontal, // left -> right
        vertical,   // top -> bottom
    };

    /**
     * @brief A linear gradient between two colors.
     */
    struct linear_gradient {
        color start;
        color end;
        gradient_direction dir = gradient_direction::horizontal;
    };

    /**
     * @brief Resolve a gradient color at position (x, y) within the given area.
     *
     * Returns the interpolated RGB color. Non-RGB start/end colors are returned as-is (start).
     */
    inline color resolve_gradient(const linear_gradient &g, uint32_t x, uint32_t y, rect area) {
        if (area.w == 0 && area.h == 0) {
            return g.start;
        }
        if (g.start.kind != color_kind::rgb || g.end.kind != color_kind::rgb) {
            return g.start;
        }

        float t = 0.0f;
        if (g.dir == gradient_direction::horizontal) {
            uint32_t span = area.w > 1 ? area.w - 1 : 1;
            uint32_t local_x = (x >= area.x) ? x - area.x : 0;
            t = static_cast<float>(local_x) / static_cast<float>(span);
        } else {
            uint32_t span = area.h > 1 ? area.h - 1 : 1;
            uint32_t local_y = (y >= area.y) ? y - area.y : 0;
            t = static_cast<float>(local_y) / static_cast<float>(span);
        }

        if (t < 0.0f) {
            t = 0.0f;
        }
        if (t > 1.0f) {
            t = 1.0f;
        }

        return color::from_rgb(static_cast<uint8_t>(g.start.r + (g.end.r - g.start.r) * t),
                               static_cast<uint8_t>(g.start.g + (g.end.g - g.start.g) * t),
                               static_cast<uint8_t>(g.start.b + (g.end.b - g.start.b) * t));
    }

    // ── HSL ─────────────────────────────────────────────────────────────────

    /** @brief HSL color representation (all components in [0, 1]). */
    struct hsl {
        float h = 0.0f; // hue        [0, 1) maps to [0, 360)
        float s = 0.0f; // saturation [0, 1]
        float l = 0.0f; // lightness  [0, 1]
    };

    /** @brief Convert sRGB [0,255] to HSL. */
    [[nodiscard]] inline hsl rgb_to_hsl(uint8_t r8, uint8_t g8, uint8_t b8) noexcept {
        float r = r8 / 255.0f, g = g8 / 255.0f, b = b8 / 255.0f;
        float mx = std::fmax(r, std::fmax(g, b));
        float mn = std::fmin(r, std::fmin(g, b));
        float d = mx - mn;
        float l = (mx + mn) * 0.5f;
        if (d < 1e-6f) {
            return {0.0f, 0.0f, l};
        }

        float s = (l > 0.5f) ? d / (2.0f - mx - mn) : d / (mx + mn);
        float h = 0.0f;
        if (mx == r) {
            h = (g - b) / d + (g < b ? 6.0f : 0.0f);
        } else if (mx == g) {
            h = (b - r) / d + 2.0f;
        } else {
            h = (r - g) / d + 4.0f;
        }
        h /= 6.0f;
        return {h, s, l};
    }

    /** @brief Convert HSL to sRGB color. */
    [[nodiscard]] inline color hsl_to_rgb(hsl c) noexcept {
        auto hue2rgb = [](float p, float q, float t) -> float {
            if (t < 0.0f) {
                t += 1.0f;
            }
            if (t > 1.0f) {
                t -= 1.0f;
            }
            if (t < 1.0f / 6.0f) {
                return p + (q - p) * 6.0f * t;
            }
            if (t < 1.0f / 2.0f) {
                return q;
            }
            if (t < 2.0f / 3.0f) {
                return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
            }
            return p;
        };
        if (c.s < 1e-6f) {
            auto v = static_cast<uint8_t>(c.l * 255.0f + 0.5f);
            return color::from_rgb(v, v, v);
        }
        float q = (c.l < 0.5f) ? c.l * (1.0f + c.s) : c.l + c.s - c.l * c.s;
        float p = 2.0f * c.l - q;
        return color::from_rgb(static_cast<uint8_t>(hue2rgb(p, q, c.h + 1.0f / 3.0f) * 255.0f + 0.5f),
                               static_cast<uint8_t>(hue2rgb(p, q, c.h) * 255.0f + 0.5f),
                               static_cast<uint8_t>(hue2rgb(p, q, c.h - 1.0f / 3.0f) * 255.0f + 0.5f));
    }

    // ── OKLCH ───────────────────────────────────────────────────────────────

    /** @brief OKLCH color representation (perceptually uniform). */
    struct oklch {
        float L = 0.0f; // lightness [0, 1]
        float C = 0.0f; // chroma    [0, ~0.4]
        float h = 0.0f; // hue       [0, 360) degrees
    };

    namespace detail {

        /** @brief Linearize a single sRGB channel. */
        [[nodiscard]] inline float srgb_to_linear(float c) noexcept {
            return (c <= 0.04045f) ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f);
        }

        /** @brief Gamma-encode a single linear-light channel to sRGB. */
        [[nodiscard]] inline float linear_to_srgb(float c) noexcept {
            if (c <= 0.0f) {
                return 0.0f;
            }
            if (c >= 1.0f) {
                return 1.0f;
            }
            return (c <= 0.0031308f) ? c * 12.92f : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
        }

    } // namespace detail

    /** @brief Convert sRGB [0,255] to OKLCH. */
    [[nodiscard]] inline oklch rgb_to_oklch(uint8_t r8, uint8_t g8, uint8_t b8) noexcept {
        float r = detail::srgb_to_linear(r8 / 255.0f);
        float g = detail::srgb_to_linear(g8 / 255.0f);
        float b = detail::srgb_to_linear(b8 / 255.0f);

        // Linear RGB -> LMS (using OKLab M1 matrix)
        float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
        float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
        float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

        // Cube root
        l = std::cbrt(l);
        m = std::cbrt(m);
        s = std::cbrt(s);

        // LMS -> OKLab
        float L = 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s;
        float a_ = 1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s;
        float b_ = 0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s;

        // OKLab -> OKLCH
        float C = std::sqrt(a_ * a_ + b_ * b_);
        float h = std::atan2(b_, a_) * (180.0f / 3.14159265358979f);
        if (h < 0.0f) {
            h += 360.0f;
        }

        return {L, C, h};
    }

    /** @brief Convert OKLCH to sRGB color (clamped to gamut). */
    [[nodiscard]] inline color oklch_to_rgb(oklch c) noexcept {
        float hr = c.h * (3.14159265358979f / 180.0f);
        float a_ = c.C * std::cos(hr);
        float b_ = c.C * std::sin(hr);

        // OKLab -> LMS
        float l = c.L + 0.3963377774f * a_ + 0.2158037573f * b_;
        float m = c.L - 0.1055613458f * a_ - 0.0638541728f * b_;
        float s = c.L - 0.0894841775f * a_ - 1.2914855480f * b_;

        // Cube
        l = l * l * l;
        m = m * m * m;
        s = s * s * s;

        // LMS -> linear RGB
        float r = 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
        float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
        float b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;

        // Clamp & gamma encode
        auto to_u8 = [](float v) -> uint8_t { return static_cast<uint8_t>(detail::linear_to_srgb(v) * 255.0f + 0.5f); };
        return color::from_rgb(to_u8(r), to_u8(g), to_u8(b));
    }

    // ── WCAG Contrast ───────────────────────────────────────────────────────

    /**
     * @brief Compute WCAG 2.1 relative luminance of an sRGB color.
     *
     * Returns a value in [0, 1] where 0 = darkest black, 1 = brightest white.
     * Only meaningful for RGB colors; returns 0 for non-RGB.
     */
    [[nodiscard]] inline float relative_luminance(color c) noexcept {
        if (c.kind != color_kind::rgb) {
            return 0.0f;
        }
        float r = detail::srgb_to_linear(c.r / 255.0f);
        float g = detail::srgb_to_linear(c.g / 255.0f);
        float b = detail::srgb_to_linear(c.b / 255.0f);
        return 0.2126f * r + 0.7152f * g + 0.0722f * b;
    }

    /**
     * @brief WCAG 2.1 contrast ratio between two colors.
     *
     * Returns a value in [1, 21]. WCAG AA requires >= 4.5 for normal text,
     * >= 3.0 for large text. AAA requires >= 7.0 / >= 4.5.
     */
    [[nodiscard]] inline float contrast_ratio(color a, color b) noexcept {
        float la = relative_luminance(a);
        float lb = relative_luminance(b);
        if (la < lb) {
            float tmp = la;
            la = lb;
            lb = tmp;
        }
        return (la + 0.05f) / (lb + 0.05f);
    }

    // ── Perceptual blending ─────────────────────────────────────────────────

    /**
     * @brief Blend two RGB colors in OKLCH space (perceptually uniform).
     *
     * @param a First color (must be RGB).
     * @param b Second color (must be RGB).
     * @param t Blend factor [0, 1]: 0 = a, 1 = b.
     * @return Blended color in sRGB. Returns `a` if either input is non-RGB.
     */
    [[nodiscard]] inline color blend_oklch(color a, color b, float t) noexcept {
        if (a.kind != color_kind::rgb || b.kind != color_kind::rgb) {
            return a;
        }
        if (t <= 0.0f) {
            return a;
        }
        if (t >= 1.0f) {
            return b;
        }

        oklch ca = rgb_to_oklch(a.r, a.g, a.b);
        oklch cb = rgb_to_oklch(b.r, b.g, b.b);

        // Interpolate lightness and chroma linearly
        float L = ca.L + (cb.L - ca.L) * t;
        float C = ca.C + (cb.C - ca.C) * t;

        // Interpolate hue via shortest arc
        float dh = cb.h - ca.h;
        if (dh > 180.0f) {
            dh -= 360.0f;
        }
        if (dh < -180.0f) {
            dh += 360.0f;
        }
        float h = ca.h + dh * t;
        if (h < 0.0f) {
            h += 360.0f;
        }
        if (h >= 360.0f) {
            h -= 360.0f;
        }

        return oklch_to_rgb({L, C, h});
    }

} // namespace tapioca
