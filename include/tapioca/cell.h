#pragma once

/**
 * @file cell.h
 * @brief Single character cell in the terminal grid.
 */

#include <cstdint>

#include "tapioca/style_table.h"

namespace tapioca {

/**
 * @brief One character cell on the terminal canvas.
 *
 * Layout: 8 bytes total (4 + 2 + 1 + 1 padding).
 *   - codepoint: the Unicode character (U+0000 = empty/space)
 *   - sid:       index into the global style_table
 *   - width:     display width in columns (0 = combining, 1 = narrow, 2 = wide)
 */
struct cell {
    char32_t codepoint = U' ';
    style_id sid       = default_style_id;
    uint8_t  width     = 1;
    uint8_t  alpha     = 255;  // 0 = fully transparent, 255 = fully opaque

    [[nodiscard]] constexpr bool operator==(const cell&) const noexcept = default;
};

static_assert(sizeof(cell) == 8, "cell must be 8 bytes");

}  // namespace tapioca
