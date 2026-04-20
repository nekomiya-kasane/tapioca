/**
 * @file image.cpp
 * @brief Terminal inline image protocol encoder stubs.
 *
 * Stub implementations for Sixel, Kitty graphics, and iTerm2 inline images.
 * Full implementations will be added when image rendering is needed.
 */

#include "tapioca/image.h"

#include <cstdlib>
#include <cstring>
#include <string>

namespace tapioca {

// ── Sixel (stub) ────────────────────────────────────────────────────────

std::string encode_sixel(const uint8_t* pixels, const image_encode_options& opts) {
    if (!pixels || opts.width == 0 || opts.height == 0) return {};

    // TODO: Full sixel quantization + encoding
    // For now, return a minimal sixel sequence that displays a placeholder
    std::string out;
    out += "\033Pq";                    // DCS q (sixel start)
    out += "\"1;1;";                    // aspect ratio 1:1
    out += std::to_string(opts.width);
    out += ";";
    out += std::to_string(opts.height);
    // Empty sixel data — terminal shows blank
    out += "\033\\";                    // ST (string terminator)
    return out;
}

// ── Kitty graphics (stub) ───────────────────────────────────────────────

std::string encode_kitty(const uint8_t* pixels, const image_encode_options& opts) {
    if (!pixels || opts.width == 0 || opts.height == 0) return {};

    // TODO: Full kitty protocol with chunked base64 transmission
    // Stub: emit a minimal "direct" transmission with no data
    std::string out;
    out += "\033_Ga=T,f=32,s=";
    out += std::to_string(opts.width);
    out += ",v=";
    out += std::to_string(opts.height);
    out += ";";
    // Empty payload — kitty will ignore or show placeholder
    out += "\033\\";
    return out;
}

// ── iTerm2 inline images (stub) ─────────────────────────────────────────

std::string encode_iterm2(const uint8_t* pixels, const image_encode_options& opts) {
    if (!pixels || opts.width == 0 || opts.height == 0) return {};

    // TODO: Full iTerm2 protocol with base64 PNG/raw encoding
    // Stub: emit a minimal OSC 1337 with empty inline data
    std::string out;
    out += "\033]1337;File=inline=1;width=";
    out += std::to_string(opts.cell_width > 0 ? opts.cell_width : opts.width);
    out += ";height=";
    out += std::to_string(opts.cell_height > 0 ? opts.cell_height : opts.height);
    out += ";preserveAspectRatio=";
    out += (opts.preserve_aspect ? "1" : "0");
    out += ":";
    // Empty base64 data
    out += "\007";  // BEL terminator
    return out;
}

// ── Protocol detection ──────────────────────────────────────────────────

image_protocol detect_image_protocol() noexcept {
    // Check for kitty
    if (std::getenv("KITTY_WINDOW_ID")) return image_protocol::kitty;

    // Check for iTerm2
    const char* term_prog = std::getenv("TERM_PROGRAM");
    if (term_prog) {
        if (std::strcmp(term_prog, "iTerm.app") == 0) return image_protocol::iterm2;
        if (std::strcmp(term_prog, "WezTerm") == 0)   return image_protocol::kitty;
    }

    // Check for sixel support via TERM
    const char* term = std::getenv("TERM");
    if (term) {
        if (std::strstr(term, "sixel")) return image_protocol::sixel;
        // xterm with sixel is common
        if (std::strcmp(term, "xterm-256color") == 0) {
            // Could be sixel-capable xterm, but can't confirm without DA2 query
            // Conservative: return none
        }
    }

    // Check GHOSTTY_RESOURCES_DIR for Ghostty (supports kitty protocol)
    if (std::getenv("GHOSTTY_RESOURCES_DIR")) return image_protocol::kitty;

    return image_protocol::none;
}

// ── Dispatch ────────────────────────────────────────────────────────────

std::string encode_image(image_protocol proto, const uint8_t* pixels,
                         const image_encode_options& opts) {
    switch (proto) {
        case image_protocol::sixel:  return encode_sixel(pixels, opts);
        case image_protocol::kitty:  return encode_kitty(pixels, opts);
        case image_protocol::iterm2: return encode_iterm2(pixels, opts);
        case image_protocol::none:   return {};
    }
    return {};
}

}  // namespace tapioca
