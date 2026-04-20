#pragma once

/**
 * @file image.h
 * @brief Terminal inline image protocol encoders.
 *
 * Supports three major terminal image protocols:
 * - **Sixel**: DEC standard, supported by xterm, mlterm, foot, WezTerm
 * - **Kitty graphics**: APC-based, supported by kitty, WezTerm, Ghostty
 * - **iTerm2 inline images**: OSC 1337, supported by iTerm2, WezTerm, Hyper
 *
 * All encoders produce escape sequences that can be written to the terminal
 * via a pal::output_sink. The caller provides raw pixel data (RGBA).
 */

#include <cstdint>
#include <string>
#include <string_view>

#include "tapioca/exports.h"

namespace tapioca {

/** @brief Supported terminal image protocols. */
enum class image_protocol : uint8_t {
    none   = 0,
    sixel  = 1,
    kitty  = 2,
    iterm2 = 3,
};

/** @brief Pixel format for image data. */
enum class pixel_format : uint8_t {
    rgba8 = 0,  // 4 bytes per pixel: R, G, B, A
    rgb8  = 1,  // 3 bytes per pixel: R, G, B
};

/**
 * @brief Options for image encoding.
 */
struct image_encode_options {
    uint32_t     width       = 0;      // image width in pixels
    uint32_t     height      = 0;      // image height in pixels
    pixel_format format      = pixel_format::rgba8;
    uint32_t     cell_width  = 0;      // desired width in terminal cells (0 = auto)
    uint32_t     cell_height = 0;      // desired height in terminal cells (0 = auto)
    bool         preserve_aspect = true;
};

// ── Sixel ───────────────────────────────────────────────────────────────

/**
 * @brief Encode pixel data as a Sixel escape sequence.
 *
 * Sixel uses a 256-color palette. The encoder quantizes the input image
 * to fit the palette and produces DCS sequences.
 *
 * @param pixels  Raw pixel data (width * height * bpp bytes).
 * @param opts    Encoding options (width, height, format).
 * @return Sixel escape sequence string ready to write to the terminal.
 */
[[nodiscard]] TAPIOCA_API std::string encode_sixel(
    const uint8_t* pixels, const image_encode_options& opts);

// ── Kitty graphics protocol ─────────────────────────────────────────────

/**
 * @brief Encode pixel data using the Kitty graphics protocol (APC sequences).
 *
 * Kitty supports true-color images transmitted as base64-encoded PNG or
 * raw RGBA data. Large images are split into chunks.
 *
 * @param pixels  Raw pixel data (width * height * bpp bytes).
 * @param opts    Encoding options (width, height, format).
 * @return Kitty APC escape sequence(s) ready to write to the terminal.
 */
[[nodiscard]] TAPIOCA_API std::string encode_kitty(
    const uint8_t* pixels, const image_encode_options& opts);

// ── iTerm2 inline images ────────────────────────────────────────────────

/**
 * @brief Encode pixel data using the iTerm2 inline image protocol (OSC 1337).
 *
 * The image data is base64-encoded and wrapped in an OSC 1337 sequence.
 * This protocol is also supported by WezTerm and Hyper.
 *
 * @param pixels  Raw pixel data (width * height * bpp bytes).
 * @param opts    Encoding options (width, height, format).
 * @return OSC 1337 escape sequence string ready to write to the terminal.
 */
[[nodiscard]] TAPIOCA_API std::string encode_iterm2(
    const uint8_t* pixels, const image_encode_options& opts);

// ── Protocol detection ──────────────────────────────────────────────────

/**
 * @brief Detect the best image protocol supported by the current terminal.
 *
 * Checks environment variables (TERM_PROGRAM, KITTY_WINDOW_ID, etc.)
 * to determine which protocol to use. Returns image_protocol::none if
 * no image protocol is detected.
 */
[[nodiscard]] TAPIOCA_API image_protocol detect_image_protocol() noexcept;

/**
 * @brief Encode pixel data using the specified protocol.
 *
 * Convenience function that dispatches to the appropriate encoder.
 * Returns an empty string for image_protocol::none.
 */
[[nodiscard]] TAPIOCA_API std::string encode_image(
    image_protocol proto, const uint8_t* pixels, const image_encode_options& opts);

}  // namespace tapioca
