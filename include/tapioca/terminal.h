#pragma once

/**
 * @file terminal.h
 * @brief Platform-abstracted terminal capability detection and control.
 */

#include "tapioca/exports.h"
#include "tapioca/style.h"

#include <cstdint>
#include <cstdio>

namespace tapioca {

/** @brief Terminal dimensions in character cells. */
struct terminal_size {
    uint32_t width = 0;
    uint32_t height = 0;
};

/** @brief Supported color depth levels. */
enum class color_depth : uint8_t {
    none = 0,
    basic_16 = 1,
    palette_256 = 2,
    true_color = 3,
};

/**
 * @brief Static utility for querying and controlling the terminal.
 *
 * All methods are thread-safe for reads. Write operations (enable_raw_mode, etc.)
 * should be called from a single thread.
 */
namespace terminal {

/** @brief Query the current terminal size. Returns {0,0} if not a terminal. */
[[nodiscard]] TAPIOCA_API terminal_size get_size() noexcept;

/** @brief Detect the maximum color depth supported by the terminal. */
[[nodiscard]] TAPIOCA_API color_depth detect_color_depth() noexcept;

/** @brief Returns true if the given FILE* is connected to a terminal (TTY). */
[[nodiscard]] TAPIOCA_API bool is_tty(FILE *f = stdout) noexcept;

/** @brief Enable virtual terminal processing (Windows-specific, no-op on POSIX). */
TAPIOCA_API bool enable_vt_processing() noexcept;

/** @brief Set console output codepage to UTF-8 (Windows-specific, no-op on POSIX). */
TAPIOCA_API void enable_utf8() noexcept;

/**
 * @brief OSC sequence support flags detected from the terminal environment.
 */
struct osc_support {
    bool title = false;
    bool palette = false;
    bool cwd = false;
    bool hyperlink = false;
    bool notify = false;
    bool color_query = false;
    bool clipboard = false;
    bool shell_integration = false;
};

/**
 * @brief Detect which OSC sequences the current terminal supports.
 */
[[nodiscard]] TAPIOCA_API osc_support detect_osc_support() noexcept;

} // namespace terminal

// ── Terminal capability aggregate ────────────────────────────────────────

/**
 * @brief Aggregated terminal capabilities used by ansi_emitter for
 *        automatic feature degradation.
 *
 * Default-constructed caps assume full modern terminal support (true_color,
 * all attrs, all OSC, VT sequences, unicode). This means existing code that
 * never touches caps gets the same behavior as before.
 *
 * Use terminal_caps::detect() for runtime detection, or the named presets
 * (legacy_win_cmd, basic_ansi, modern) for known environments.
 */
struct terminal_caps {
    /** @brief Maximum color depth the terminal supports. */
    color_depth max_colors = color_depth::true_color;

    /**
     * @brief Bitmask of text attributes the terminal can render.
     *
     * Attributes not in this mask will be silently stripped by
     * ansi_emitter::transition(). Default = all attributes supported.
     */
    attr supported_attrs = static_cast<attr>(0x0FFF);

    /** @brief Per-feature OSC support flags. */
    terminal::osc_support osc{
        /* title */ true,
        /* palette */ true,
        /* cwd */ true,
        /* hyperlink */ true,
        /* notify */ true,
        /* color_query */ true,
        /* clipboard */ true,
        /* shell_integration */ true,
    };

    /** @brief Terminal understands basic CSI sequences (cursor move, clear). */
    bool vt_sequences = true;

    /** @brief Terminal can render multi-byte UTF-8 and wide characters. */
    bool unicode = true;

    /** @brief Terminal supports the Kitty keyboard protocol (CSI u encoding). */
    bool kitty_keyboard = false;

    // ── Presets ──────────────────────────────────────────────────────

    /**
     * @brief Full-featured modern terminal (default).
     *
     * Matches kitty, WezTerm, Windows Terminal, iTerm2, foot, etc.
     */
    static constexpr terminal_caps modern() noexcept { return {}; }

    /**
     * @brief Legacy Windows CMD (conhost) without VT processing.
     *
     * No ANSI escape support at all. The emitter will produce no output.
     */
    static constexpr terminal_caps legacy_win_cmd() noexcept {
        terminal_caps c{};
        c.max_colors = color_depth::none;
        c.supported_attrs = attr::none;
        c.osc = {};
        c.vt_sequences = false;
        c.unicode = false;
        return c;
    }

    /**
     * @brief Windows CMD with VT processing enabled (Win10 1511+).
     *
     * Basic 16 colors, bold/underline only, no OSC, limited unicode.
     */
    static constexpr terminal_caps win_cmd_vt() noexcept {
        terminal_caps c{};
        c.max_colors = color_depth::basic_16;
        c.supported_attrs = attr::bold | attr::underline | attr::reverse;
        c.osc = {};
        c.vt_sequences = true;
        c.unicode = false;
        return c;
    }

    /**
     * @brief Basic ANSI terminal (e.g. linux console, older xterm).
     *
     * 16 colors, standard SGR attributes, no OSC beyond title.
     */
    static constexpr terminal_caps basic_ansi() noexcept {
        terminal_caps c{};
        c.max_colors = color_depth::basic_16;
        c.supported_attrs = attr::bold | attr::dim | attr::underline | attr::blink | attr::reverse | attr::hidden;
        c.osc = {/* title */ true};
        c.vt_sequences = true;
        c.unicode = false;
        return c;
    }

    /**
     * @brief 256-color terminal (e.g. xterm-256color, screen-256color).
     */
    static constexpr terminal_caps color_256() noexcept {
        terminal_caps c{};
        c.max_colors = color_depth::palette_256;
        c.supported_attrs = attr::bold | attr::dim | attr::italic | attr::underline | attr::blink | attr::reverse |
                            attr::hidden | attr::strike;
        c.osc = {/* title */ true, /* palette */ true};
        c.vt_sequences = true;
        c.unicode = true;
        return c;
    }

    // ── Runtime detection ────────────────────────────────────────────

    /**
     * @brief Detect capabilities from the current terminal environment.
     *
     * Aggregates detect_color_depth(), detect_osc_support(), and
     * platform-specific attribute/VT/unicode checks.
     */
    [[nodiscard]] static TAPIOCA_API terminal_caps detect() noexcept;
};

} // namespace tapioca
