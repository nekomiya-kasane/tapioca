/**
 * @file raw_input_emscripten.cpp
 * @brief Emscripten backend for raw input: raw_mode (no-op) and poll_event (JS interop).
 *
 * When compiled with Emscripten, this file replaces raw_input.cpp.
 * Keyboard events are captured via emscripten_set_keydown_callback and queued
 * in a ring buffer that poll_event drains.
 */

#ifdef __EMSCRIPTEN__

#include "tapioca/raw_input.h"
#include "tapioca/terminal.h"

#include <array>
#include <cstring>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <mutex>

namespace tapioca {

// ═══════════════════════════════════════════════════════════════════════
//  raw_mode — no-op on Emscripten
// ═══════════════════════════════════════════════════════════════════════

struct raw_mode::impl {};

raw_mode::raw_mode() : impl_(std::make_unique<impl>()) {
    // Install keyboard listener on the tapiru-terminal element
    EM_ASM({
        if (!window._tapiru_input_installed) {
            window._tapiru_input_installed = true;
            window._tapiru_key_queue = [];
            var el = document.getElementById('tapiru-terminal') || document;
            el.addEventListener(
                'keydown', function(e) {
                    e.preventDefault();
                    window._tapiru_key_queue.push({
                        key : e.key,
                        code : e.code,
                        ctrl : e.ctrlKey,
                        alt : e.altKey,
                        shift : e.shiftKey,
                        codepoint : (e.key.length == = 1) ? e.key.charCodeAt(0) : 0
                    });
                });
        }
    });
}

raw_mode::~raw_mode() = default;
raw_mode::raw_mode(raw_mode &&) noexcept = default;
raw_mode &raw_mode::operator=(raw_mode &&) noexcept = default;

// ═══════════════════════════════════════════════════════════════════════
//  poll_event — drain JS key queue
// ═══════════════════════════════════════════════════════════════════════

static special_key js_key_to_special(const char *key) {
    if (std::strcmp(key, "ArrowUp") == 0) return special_key::up;
    if (std::strcmp(key, "ArrowDown") == 0) return special_key::down;
    if (std::strcmp(key, "ArrowLeft") == 0) return special_key::left;
    if (std::strcmp(key, "ArrowRight") == 0) return special_key::right;
    if (std::strcmp(key, "Enter") == 0) return special_key::enter;
    if (std::strcmp(key, "Escape") == 0) return special_key::escape;
    if (std::strcmp(key, "Tab") == 0) return special_key::tab;
    if (std::strcmp(key, "Backspace") == 0) return special_key::backspace;
    if (std::strcmp(key, "Delete") == 0) return special_key::delete_;
    if (std::strcmp(key, "Insert") == 0) return special_key::insert;
    if (std::strcmp(key, "PageUp") == 0) return special_key::page_up;
    if (std::strcmp(key, "PageDown") == 0) return special_key::page_down;
    if (std::strcmp(key, "Home") == 0) return special_key::home;
    if (std::strcmp(key, "End") == 0) return special_key::end;
    if (std::strcmp(key, "F1") == 0) return special_key::f1;
    if (std::strcmp(key, "F2") == 0) return special_key::f2;
    if (std::strcmp(key, "F3") == 0) return special_key::f3;
    if (std::strcmp(key, "F4") == 0) return special_key::f4;
    if (std::strcmp(key, "F5") == 0) return special_key::f5;
    if (std::strcmp(key, "F6") == 0) return special_key::f6;
    if (std::strcmp(key, "F7") == 0) return special_key::f7;
    if (std::strcmp(key, "F8") == 0) return special_key::f8;
    if (std::strcmp(key, "F9") == 0) return special_key::f9;
    if (std::strcmp(key, "F10") == 0) return special_key::f10;
    if (std::strcmp(key, "F11") == 0) return special_key::f11;
    if (std::strcmp(key, "F12") == 0) return special_key::f12;
    return special_key::none;
}

std::optional<input_event> poll_event(int timeout_ms) {
    (void)timeout_ms;

    // Check if there's a key in the JS queue
    int has_key = EM_ASM_INT({ return (window._tapiru_key_queue && window._tapiru_key_queue.length > 0) ? 1 : 0; });

    if (!has_key) return std::nullopt;

    // Extract key info from JS
    char key_buf[64] = {};
    int codepoint = 0;
    int ctrl = 0, alt = 0, shift = 0;

    EM_ASM(
        {
            var ev = window._tapiru_key_queue.shift();
            if (!ev) return;
            var keyStr = ev.key || '';
            // Copy key string to WASM memory
            var len = Math.min(keyStr.length, 62);
            for (var i = 0; i < len; i++) {
                HEAP8[$0 + i] = keyStr.charCodeAt(i);
            }
            HEAP8[$0 + len] = 0;
            HEAP32[$1 >> 2] = ev.codepoint || 0;
            HEAP32[$2 >> 2] = ev.ctrl ? 1 : 0;
            HEAP32[$3 >> 2] = ev.alt ? 1 : 0;
            HEAP32[$4 >> 2] = ev.shift ? 1 : 0;
        },
        key_buf, &codepoint, &ctrl, &alt, &shift);

    key_mod mods = key_mod::none;
    if (ctrl) mods = mods | key_mod::ctrl;
    if (alt) mods = mods | key_mod::alt;
    if (shift) mods = mods | key_mod::shift;

    special_key sk = js_key_to_special(key_buf);
    if (sk != special_key::none) {
        return input_event{key_event{0, sk, mods}};
    }

    if (codepoint > 0) {
        return input_event{key_event{static_cast<char32_t>(codepoint), special_key::none, mods}};
    }

    return std::nullopt;
}

} // namespace tapioca

#endif // __EMSCRIPTEN__
