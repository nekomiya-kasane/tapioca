#pragma once

/**
 * @file unicode_width.h
 * @brief Unicode character and string width calculation.
 *
 * Uses a compact range table derived from Unicode East Asian Width (EAW) data.
 * Handles combining characters (width 0), narrow (width 1), and wide/fullwidth (width 2).
 */

#include <cstdint>
#include <string_view>

#include "tapioca/exports.h"

namespace tapioca {

/**
 * @brief Returns the display width of a single Unicode codepoint.
 *
 * @return 0 for combining/zero-width, 1 for narrow, 2 for wide/fullwidth.
 *         Returns -1 for non-printable control characters (C0/C1 except tab/newline).
 */
[[nodiscard]] TAPIOCA_API int char_width(char32_t cp) noexcept;

/**
 * @brief Returns the total display width of a UTF-8 string.
 *
 * Iterates codepoints, summing char_width() for each. Skips invalid sequences.
 * Control characters (char_width == -1) are treated as width 0.
 */
[[nodiscard]] TAPIOCA_API int string_width(std::string_view utf8) noexcept;

/**
 * @brief Decode one UTF-8 codepoint from the given position.
 *
 * @param data  pointer to UTF-8 bytes
 * @param len   remaining bytes available
 * @param[out] cp  decoded codepoint (U+FFFD on error)
 * @return number of bytes consumed (1-4), or 1 on error (advances past bad byte)
 */
[[nodiscard]] TAPIOCA_API int utf8_decode(const char* data, size_t len, char32_t& cp) noexcept;

}  // namespace tapioca
