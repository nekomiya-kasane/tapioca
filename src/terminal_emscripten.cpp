/**
 * @file terminal_emscripten.cpp
 * @brief Emscripten backend for terminal functions.
 *
 * When compiled with Emscripten, this file replaces terminal.cpp.
 * It uses EM_ASM / EM_JS to query the browser DOM for terminal dimensions,
 * color depth, and TTY status.
 */

#ifdef __EMSCRIPTEN__

#include "tapioca/terminal.h"

#include <emscripten.h>
#include <emscripten/html5.h>

namespace tapioca::terminal {

    // ── get_size ────────────────────────────────────────────────────────────

    terminal_size get_size() noexcept {
        int w = EM_ASM_INT({
            var el = document.getElementById('tapiru-terminal');
            if (el) {
                // Estimate columns from element width and a monospace char width (~8px)
                var style = window.getComputedStyle(el);
                var fontSize = parseFloat(style.fontSize) || 16;
                var charW = fontSize * 0.6;
                return Math.floor(el.clientWidth / charW) || 80;
            }
            return 80;
        });
        int h = EM_ASM_INT({
            var el = document.getElementById('tapiru-terminal');
            if (el) {
                var style = window.getComputedStyle(el);
                var fontSize = parseFloat(style.fontSize) || 16;
                var lineH = fontSize * 1.2;
                return Math.floor(el.clientHeight / lineH) || 24;
            }
            return 24;
        });
        return {static_cast<uint32_t>(w), static_cast<uint32_t>(h)};
    }

    // ── detect_color_depth ──────────────────────────────────────────────────

    color_depth detect_color_depth() noexcept {
        // Browsers support true color via CSS / ANSI-to-HTML conversion
        return color_depth::true_color;
    }

    // ── is_tty ──────────────────────────────────────────────────────────────

    bool is_tty(FILE *) noexcept {
        // In the browser, we always render to a DOM element — treat as TTY
        return true;
    }

    // ── enable_vt_processing ────────────────────────────────────────────────

    bool enable_vt_processing() noexcept {
        // No-op in browser; ANSI sequences are parsed by our JS layer
        return true;
    }

    // ── enable_utf8 ─────────────────────────────────────────────────────────

    void enable_utf8() noexcept {
        // Browser is always UTF-8
    }

    // ── detect_osc_support ───────────────────────────────────────────────

    osc_support detect_osc_support() noexcept {
        // Browser environment: no real terminal OSC support
        return {};
    }

} // namespace tapioca::terminal

// ── terminal_caps::detect (Emscripten) ──────────────────────────────────

namespace tapioca {

    terminal_caps terminal_caps::detect() noexcept {
        terminal_caps caps{};
        caps.max_colors = color_depth::true_color;
        caps.supported_attrs = static_cast<attr>(0x0FFF);
        caps.osc = {}; // no real OSC in browser
        caps.vt_sequences = true;
        caps.unicode = true;
        return caps;
    }

} // namespace tapioca

#endif // __EMSCRIPTEN__
