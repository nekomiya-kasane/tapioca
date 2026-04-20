#pragma once

/**
 * @file layout_types.h
 * @brief Core geometry and border types shared between tapioca and tapiru.
 *
 * Moved from tapiru/layout/types.h: rect, measurement, box_constraints,
 * border_style, border_chars, get_border_chars().
 * Layout-specific enums (overflow_mode, justify, align_items, gauge_direction)
 * remain in tapiru.
 */

#include "tapioca/exports.h"

#include <algorithm>
#include <cstdint>
#include <limits>

namespace tapioca {

// ── Measurement ─────────────────────────────────────────────────────────

/** @brief Result of a widget's measure pass: desired size. */
struct measurement {
    uint32_t width = 0;
    uint32_t height = 0;
};

// ── Rect ────────────────────────────────────────────────────────────────

/** @brief Axis-aligned rectangle in terminal cell coordinates. */
struct rect {
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t w = 0;
    uint32_t h = 0;

    [[nodiscard]] constexpr bool operator==(const rect &) const noexcept = default;

    [[nodiscard]] constexpr bool empty() const noexcept { return w == 0 || h == 0; }

    /** @brief Return a sub-rect inset by the given amounts. */
    [[nodiscard]] constexpr rect inset(uint32_t top, uint32_t right, uint32_t bottom, uint32_t left) const noexcept {
        uint32_t dx = std::min(left + right, w);
        uint32_t dy = std::min(top + bottom, h);
        return {x + left, y + top, w - dx, h - dy};
    }
};

// ── BoxConstraints ──────────────────────────────────────────────────────

inline constexpr uint32_t unbounded = std::numeric_limits<uint32_t>::max();

/**
 * @brief Flutter-style box constraints for layout negotiation.
 *
 * Parent passes constraints down; child returns a measurement within bounds.
 */
struct box_constraints {
    uint32_t min_w = 0;
    uint32_t max_w = unbounded;
    uint32_t min_h = 0;
    uint32_t max_h = unbounded;

    /** @brief Constrain a measurement to fit within these bounds. */
    [[nodiscard]] constexpr measurement constrain(measurement m) const noexcept {
        return {
            std::clamp(m.width, min_w, max_w),
            std::clamp(m.height, min_h, max_h),
        };
    }

    /** @brief Create tight constraints (exact size). */
    [[nodiscard]] static constexpr box_constraints tight(uint32_t w, uint32_t h) noexcept { return {w, w, h, h}; }

    /** @brief Create loose constraints (0 to max). */
    [[nodiscard]] static constexpr box_constraints loose(uint32_t max_w_, uint32_t max_h_) noexcept {
        return {0, max_w_, 0, max_h_};
    }

    [[nodiscard]] constexpr bool operator==(const box_constraints &) const noexcept = default;
};

// ── Border characters ───────────────────────────────────────────────────

/** @brief Border drawing style for panels and tables. */
enum class border_style : uint8_t {
    none = 0,
    ascii = 1,
    rounded = 2,
    heavy = 3,
    double_ = 4,
};

/**
 * @brief Set of characters used to draw a box border.
 */
struct border_chars {
    char32_t top_left;
    char32_t top_right;
    char32_t bottom_left;
    char32_t bottom_right;
    char32_t horizontal;
    char32_t vertical;
    char32_t cross;
    char32_t t_down;  // ┬
    char32_t t_up;    // ┴
    char32_t t_right; // ├
    char32_t t_left;  // ┤
};

/** @brief Get the border character set for a given style. */
[[nodiscard]] inline constexpr border_chars get_border_chars(border_style bs) noexcept {
    switch (bs) {
    case border_style::ascii:
        return {'+', '+', '+', '+', '-', '|', '+', '+', '+', '+', '+'};
    case border_style::rounded:
        return {U'\x256D', U'\x256E', U'\x2570', U'\x256F', U'\x2500', U'\x2502',
                U'\x253C', U'\x252C', U'\x2534', U'\x251C', U'\x2524'};
    case border_style::heavy:
        return {U'\x250F', U'\x2513', U'\x2517', U'\x251B', U'\x2501', U'\x2503',
                U'\x254B', U'\x2533', U'\x253B', U'\x2523', U'\x252B'};
    case border_style::double_:
        return {U'\x2554', U'\x2557', U'\x255A', U'\x255D', U'\x2550', U'\x2551',
                U'\x256C', U'\x2566', U'\x2569', U'\x2560', U'\x2563'};
    case border_style::none:
    default:
        return {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
    }
}

} // namespace tapioca
