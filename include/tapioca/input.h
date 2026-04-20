#pragma once

/**
 * @file input.h
 * @brief Input event types for interactive terminal UIs.
 *
 * This file contains only the event type definitions (key_event, mouse_event,
 * resize_event, input_event) and related enums. Event routing (event_table,
 * focus_manager, hit_test) stays in tapiru.
 */

#include <cstdint>
#include <variant>

#include "tapioca/exports.h"

namespace tapioca {

// ── Key modifiers ──────────────────────────────────────────────────────

enum class key_mod : uint8_t {
    none      = 0,
    shift     = 1,
    ctrl      = 2,
    alt       = 4,
    super_    = 8,   // Kitty protocol: super/meta key
    hyper     = 16,  // Kitty protocol: hyper key
    caps_lock = 32,  // Kitty protocol: caps lock active
    num_lock  = 64,  // Kitty protocol: num lock active
};

inline key_mod operator|(key_mod a, key_mod b) { return static_cast<key_mod>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b)); }
inline key_mod operator&(key_mod a, key_mod b) { return static_cast<key_mod>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b)); }
inline bool has_mod(key_mod mods, key_mod m) { return (static_cast<uint8_t>(mods) & static_cast<uint8_t>(m)) != 0; }

// ── Special keys ───────────────────────────────────────────────────────

enum class special_key : uint16_t {
    none = 0,
    enter, escape, tab, backspace, delete_,
    up, down, left, right,
    home, end, page_up, page_down,
    insert,
    f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12,
};

// ── Key action (Kitty keyboard protocol) ──────────────────────────────

enum class key_action : uint8_t {
    press   = 1,
    repeat  = 2,
    release = 3,
};

// ── Event types ────────────────────────────────────────────────────────

struct key_event {
    char32_t    codepoint = 0;
    special_key key       = special_key::none;
    key_mod     mods      = key_mod::none;
    key_action  action    = key_action::press;  // Kitty: press/repeat/release
};

enum class mouse_button : uint8_t {
    none = 0, left = 1, middle = 2, right = 3,
    scroll_up = 4, scroll_down = 5,
};

enum class mouse_action : uint8_t {
    press = 0, release = 1, move = 2, drag = 3,
};

struct mouse_event {
    uint32_t     x      = 0;
    uint32_t     y      = 0;
    mouse_button button = mouse_button::none;
    mouse_action action = mouse_action::press;
    key_mod      mods   = key_mod::none;
};

struct resize_event {
    uint32_t width  = 0;
    uint32_t height = 0;
};

/** @brief Discriminated union of all input event types. */
using input_event = std::variant<key_event, mouse_event, resize_event>;

// ── Node ID ────────────────────────────────────────────────────────────

using node_id = uint32_t;

}  // namespace tapioca
