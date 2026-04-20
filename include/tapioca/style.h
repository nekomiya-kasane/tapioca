#pragma once

/**
 * @file style.h
 * @brief Color and Style types for terminal rendering.
 */

#include "tapioca/exports.h"

#include <cstdint>
#include <functional>

namespace tapioca {

// ── Color ───────────────────────────────────────────────────────────────

/** @brief Discriminator for color representation. */
enum class color_kind : uint8_t {
    default_color = 0,
    indexed_16 = 1,
    indexed_256 = 2,
    rgb = 3,
};

/**
 * @brief Compact color representation (4 bytes).
 *
 * Supports: terminal default, 16-color ANSI, 256-color palette, and 24-bit RGB.
 */
struct color {
    color_kind kind = color_kind::default_color;
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;

    /** @brief Default terminal color. */
    static constexpr color default_color() noexcept { return {}; }

    /** @brief Standard 16-color ANSI (0-15). */
    static constexpr color from_index_16(uint8_t idx) noexcept { return {color_kind::indexed_16, idx, 0, 0}; }

    /** @brief 256-color palette (0-255). */
    static constexpr color from_index_256(uint8_t idx) noexcept { return {color_kind::indexed_256, idx, 0, 0}; }

    /** @brief 24-bit true color. */
    static constexpr color from_rgb(uint8_t red, uint8_t green, uint8_t blue) noexcept {
        return {color_kind::rgb, red, green, blue};
    }

    [[nodiscard]] constexpr bool is_default() const noexcept { return kind == color_kind::default_color; }

    /** @brief Downgrade this color to fit a target color depth. */
    [[nodiscard]] TAPIOCA_API color downgrade(uint8_t target_depth) const noexcept;

    [[nodiscard]] constexpr bool operator==(const color &) const noexcept = default;
};

static_assert(sizeof(color) == 4, "color must be 4 bytes");

// ── Named ANSI colors (foreground indices 0-15) ────────────────────────

namespace colors {

inline constexpr color black = color::from_index_16(0);
inline constexpr color red = color::from_index_16(1);
inline constexpr color green = color::from_index_16(2);
inline constexpr color yellow = color::from_index_16(3);
inline constexpr color blue = color::from_index_16(4);
inline constexpr color magenta = color::from_index_16(5);
inline constexpr color cyan = color::from_index_16(6);
inline constexpr color white = color::from_index_16(7);
inline constexpr color bright_black = color::from_index_16(8);
inline constexpr color bright_red = color::from_index_16(9);
inline constexpr color bright_green = color::from_index_16(10);
inline constexpr color bright_yellow = color::from_index_16(11);
inline constexpr color bright_blue = color::from_index_16(12);
inline constexpr color bright_magenta = color::from_index_16(13);
inline constexpr color bright_cyan = color::from_index_16(14);
inline constexpr color bright_white = color::from_index_16(15);

} // namespace colors

// ── Text attributes ─────────────────────────────────────────────────────

/** @brief Bitmask for text decoration attributes. */
enum class attr : uint16_t {
    none = 0,
    bold = 1 << 0,
    dim = 1 << 1,
    italic = 1 << 2,
    underline = 1 << 3,
    blink = 1 << 4,
    reverse = 1 << 5,
    hidden = 1 << 6,
    strike = 1 << 7,
    double_underline = 1 << 8,
    overline = 1 << 9,
    superscript = 1 << 10,
    subscript = 1 << 11,
};

[[nodiscard]] constexpr attr operator|(attr a, attr b) noexcept {
    return static_cast<attr>(static_cast<uint16_t>(a) | static_cast<uint16_t>(b));
}

[[nodiscard]] constexpr attr operator&(attr a, attr b) noexcept {
    return static_cast<attr>(static_cast<uint16_t>(a) & static_cast<uint16_t>(b));
}

[[nodiscard]] constexpr attr operator~(attr a) noexcept {
    return static_cast<attr>(~static_cast<uint16_t>(a));
}

constexpr attr &operator|=(attr &a, attr b) noexcept {
    a = a | b;
    return a;
}

constexpr attr &operator&=(attr &a, attr b) noexcept {
    a = a & b;
    return a;
}

[[nodiscard]] constexpr bool has_attr(attr set, attr flag) noexcept {
    return (set & flag) != attr::none;
}

// ── Style ───────────────────────────────────────────────────────────────

/**
 * @brief Complete visual style: foreground + background + text attributes.
 *
 * 10 bytes total (4 + 4 + 2).
 */
struct style {
    color fg = color::default_color();
    color bg = color::default_color();
    attr attrs = attr::none;

    [[nodiscard]] constexpr bool operator==(const style &) const noexcept = default;

    /** @brief Returns true if this is a completely default/unstyled style. */
    [[nodiscard]] constexpr bool is_default() const noexcept {
        return fg.is_default() && bg.is_default() && attrs == attr::none;
    }
};

static_assert(sizeof(style) == 10, "style must be 10 bytes");

} // namespace tapioca

// ── std::hash specializations ───────────────────────────────────────────

template <> struct std::hash<tapioca::color> {
    [[nodiscard]] size_t operator()(const tapioca::color &c) const noexcept {
        uint32_t v;
        std::memcpy(&v, &c, sizeof(v));
        return std::hash<uint32_t>{}(v);
    }
};

template <> struct std::hash<tapioca::style> {
    [[nodiscard]] size_t operator()(const tapioca::style &s) const noexcept {
        auto h1 = std::hash<tapioca::color>{}(s.fg);
        auto h2 = std::hash<tapioca::color>{}(s.bg);
        auto h3 = std::hash<uint16_t>{}(static_cast<uint16_t>(s.attrs));
        h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
        h1 ^= h3 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
        return h1;
    }
};
