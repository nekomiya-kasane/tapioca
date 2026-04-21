#pragma once

/**
 * @file ansi_emitter.h
 * @brief ANSI escape sequence builder with state-tracking minimal-delta emitter.
 *
 * Moved from tapiru/core/ansi.h with namespace tapioca.
 * The emitter tracks the "hardware state" (what the terminal currently displays)
 * and computes the shortest SGR transition between styles.
 */

#include "tapioca/style.h"
#include "tapioca/terminal.h"

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>

namespace tapioca {

    // ── Low-level ANSI sequence builders ────────────────────────────────────

    namespace ansi {

        namespace detail {

            /** @brief Append an unsigned integer to a string without heap allocation. */
            inline void append_uint(std::string &out, uint32_t val) {
                char buf[12];
                auto [ptr, ec] = std::to_chars(buf, buf + sizeof(buf), val);
                out.append(buf, static_cast<size_t>(ptr - buf));
            }

        } // namespace detail

        /** @brief Move cursor to absolute position (1-based row, col). */
        inline std::string cursor_to(uint32_t row, uint32_t col) {
            std::string s;
            s.reserve(16);
            s += "\033[";
            detail::append_uint(s, row);
            s += ';';
            detail::append_uint(s, col);
            s += 'H';
            return s;
        }

        /** @brief Move cursor up by n rows. */
        inline std::string cursor_up(uint32_t n = 1) {
            if (n == 0) {
                return {};
            }
            std::string s;
            s.reserve(12);
            s += "\033[";
            detail::append_uint(s, n);
            s += 'A';
            return s;
        }

        /** @brief Move cursor down by n rows. */
        inline std::string cursor_down(uint32_t n = 1) {
            if (n == 0) {
                return {};
            }
            std::string s;
            s.reserve(12);
            s += "\033[";
            detail::append_uint(s, n);
            s += 'B';
            return s;
        }

        /** @brief Move cursor right by n columns. */
        inline std::string cursor_right(uint32_t n = 1) {
            if (n == 0) {
                return {};
            }
            std::string s;
            s.reserve(12);
            s += "\033[";
            detail::append_uint(s, n);
            s += 'C';
            return s;
        }

        /** @brief Move cursor left by n columns. */
        inline std::string cursor_left(uint32_t n = 1) {
            if (n == 0) {
                return {};
            }
            std::string s;
            s.reserve(12);
            s += "\033[";
            detail::append_uint(s, n);
            s += 'D';
            return s;
        }

        inline constexpr const char *cursor_hide = "\033[?25l";
        inline constexpr const char *cursor_show = "\033[?25h";
        inline constexpr const char *clear_screen = "\033[2J";
        inline constexpr const char *clear_to_end = "\033[0J";
        inline constexpr const char *clear_line = "\033[2K";
        inline constexpr const char *reset = "\033[0m";

        inline void append_fg_params(std::string &out, const color &c) {
            switch (c.kind) {
            case color_kind::default_color:
                out += "39";
                break;
            case color_kind::indexed_16:
                if (c.r < 8) {
                    detail::append_uint(out, 30 + c.r);
                } else {
                    detail::append_uint(out, 90 + (c.r - 8));
                }
                break;
            case color_kind::indexed_256:
                out += "38;5;";
                detail::append_uint(out, c.r);
                break;
            case color_kind::rgb:
                out += "38;2;";
                detail::append_uint(out, c.r);
                out += ';';
                detail::append_uint(out, c.g);
                out += ';';
                detail::append_uint(out, c.b);
                break;
            }
        }

        inline void append_bg_params(std::string &out, const color &c) {
            switch (c.kind) {
            case color_kind::default_color:
                out += "49";
                break;
            case color_kind::indexed_16:
                if (c.r < 8) {
                    detail::append_uint(out, 40 + c.r);
                } else {
                    detail::append_uint(out, 100 + (c.r - 8));
                }
                break;
            case color_kind::indexed_256:
                out += "48;5;";
                detail::append_uint(out, c.r);
                break;
            case color_kind::rgb:
                out += "48;2;";
                detail::append_uint(out, c.r);
                out += ';';
                detail::append_uint(out, c.g);
                out += ';';
                detail::append_uint(out, c.b);
                break;
            }
        }

        inline void append_attr_params(std::string &out, attr a, bool sep) {
            auto emit = [&](const char *code) {
                if (sep) {
                    out += ';';
                }
                out += code;
                sep = true;
            };
            if (has_attr(a, attr::bold)) {
                emit("1");
            }
            if (has_attr(a, attr::dim)) {
                emit("2");
            }
            if (has_attr(a, attr::italic)) {
                emit("3");
            }
            if (has_attr(a, attr::underline)) {
                emit("4");
            }
            if (has_attr(a, attr::blink)) {
                emit("5");
            }
            if (has_attr(a, attr::reverse)) {
                emit("7");
            }
            if (has_attr(a, attr::hidden)) {
                emit("8");
            }
            if (has_attr(a, attr::strike)) {
                emit("9");
            }
            if (has_attr(a, attr::double_underline)) {
                emit("21");
            }
            if (has_attr(a, attr::overline)) {
                emit("53");
            }
            if (has_attr(a, attr::superscript)) {
                emit("73");
            }
            if (has_attr(a, attr::subscript)) {
                emit("74");
            }
        }

        inline std::string sgr(const style &s) {
            if (s.is_default()) {
                return {};
            }
            std::string out;
            out.reserve(32);
            out += "\033[";
            bool need_sep = false;
            if (s.attrs != attr::none) {
                append_attr_params(out, s.attrs, false);
                need_sep = true;
            }
            if (!s.fg.is_default()) {
                if (need_sep) {
                    out += ';';
                }
                append_fg_params(out, s.fg);
                need_sep = true;
            }
            if (!s.bg.is_default()) {
                if (need_sep) {
                    out += ';';
                }
                append_bg_params(out, s.bg);
            }
            out += 'm';
            return out;
        }

        // ── OSC sequences ───────────────────────────────────────────────────────

        inline std::string set_title(std::string_view title) {
            std::string s;
            s.reserve(title.size() + 8);
            s += "\033]0;";
            s += title;
            s += "\033\\";
            return s;
        }

        inline std::string set_icon_name(std::string_view name) {
            std::string s;
            s.reserve(name.size() + 8);
            s += "\033]1;";
            s += name;
            s += "\033\\";
            return s;
        }

        inline std::string set_window_title(std::string_view title) {
            std::string s;
            s.reserve(title.size() + 8);
            s += "\033]2;";
            s += title;
            s += "\033\\";
            return s;
        }

        inline std::string set_palette_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
            char buf[48];
            int n = std::snprintf(buf, sizeof(buf), "\033]4;%u;rgb:%02x/%02x/%02x\033\\", index, r, g, b);
            return std::string(buf, static_cast<size_t>(n));
        }

        inline std::string query_palette_color(uint8_t index) {
            char buf[16];
            int n = std::snprintf(buf, sizeof(buf), "\033]4;%u;?\033\\", index);
            return std::string(buf, static_cast<size_t>(n));
        }

        inline std::string reset_palette_color(int index = -1) {
            if (index < 0) {
                return "\033]104\033\\";
            }
            char buf[16];
            int n = std::snprintf(buf, sizeof(buf), "\033]104;%d\033\\", index);
            return std::string(buf, static_cast<size_t>(n));
        }

        inline std::string report_cwd(std::string_view hostname, std::string_view path) {
            std::string s;
            s.reserve(16 + hostname.size() + path.size());
            s += "\033]7;file://";
            s += hostname;
            s += path;
            s += "\033\\";
            return s;
        }

        inline std::string hyperlink_open(std::string_view url, std::string_view id = {}) {
            std::string s;
            s.reserve(url.size() + id.size() + 16);
            s += "\033]8;";
            if (!id.empty()) {
                s += "id=";
                s += id;
            }
            s += ';';
            s += url;
            s += "\033\\";
            return s;
        }

        inline constexpr const char *hyperlink_close = "\033]8;;\033\\";

        inline std::string notify(std::string_view message) {
            std::string s;
            s.reserve(message.size() + 8);
            s += "\033]9;";
            s += message;
            s += "\033\\";
            return s;
        }

        inline std::string set_foreground_color(uint8_t r, uint8_t g, uint8_t b) {
            char buf[40];
            int n = std::snprintf(buf, sizeof(buf), "\033]10;rgb:%02x/%02x/%02x\033\\", r, g, b);
            return std::string(buf, static_cast<size_t>(n));
        }
        inline constexpr const char *query_foreground_color = "\033]10;?\033\\";

        inline std::string set_background_color(uint8_t r, uint8_t g, uint8_t b) {
            char buf[40];
            int n = std::snprintf(buf, sizeof(buf), "\033]11;rgb:%02x/%02x/%02x\033\\", r, g, b);
            return std::string(buf, static_cast<size_t>(n));
        }
        inline constexpr const char *query_background_color = "\033]11;?\033\\";

        inline std::string set_cursor_color(uint8_t r, uint8_t g, uint8_t b) {
            char buf[40];
            int n = std::snprintf(buf, sizeof(buf), "\033]12;rgb:%02x/%02x/%02x\033\\", r, g, b);
            return std::string(buf, static_cast<size_t>(n));
        }
        inline constexpr const char *query_cursor_color = "\033]12;?\033\\";

        inline constexpr const char *reset_foreground_color = "\033]110\033\\";
        inline constexpr const char *reset_background_color = "\033]111\033\\";
        inline constexpr const char *reset_cursor_color = "\033]112\033\\";

        inline std::string clipboard_write(std::string_view clipboard, std::string_view base64_data) {
            std::string s;
            s.reserve(clipboard.size() + base64_data.size() + 12);
            s += "\033]52;";
            s += clipboard;
            s += ';';
            s += base64_data;
            s += "\033\\";
            return s;
        }

        inline std::string clipboard_query(std::string_view clipboard) {
            std::string s;
            s.reserve(clipboard.size() + 12);
            s += "\033]52;";
            s += clipboard;
            s += ";?\033\\";
            return s;
        }

        inline std::string clipboard_clear(std::string_view clipboard) {
            std::string s;
            s.reserve(clipboard.size() + 12);
            s += "\033]52;";
            s += clipboard;
            s += ";\033\\";
            return s;
        }

        // ── Shell integration (OSC 133) ─────────────────────────────────────────

        inline constexpr const char *prompt_start = "\033]133;A\033\\";
        inline constexpr const char *command_start = "\033]133;B\033\\";
        inline constexpr const char *output_start = "\033]133;C\033\\";

        inline std::string command_finished(int exit_code) {
            char buf[24];
            int n = std::snprintf(buf, sizeof(buf), "\033]133;D;%d\033\\", exit_code);
            return std::string(buf, static_cast<size_t>(n));
        }

    } // namespace ansi

    // ── State-tracking emitter ──────────────────────────────────────────────

    /**
     * @brief Tracks terminal hardware style state and emits minimal ANSI transitions.
     *
     * Optionally accepts a terminal_caps to automatically downgrade colors and
     * strip unsupported text attributes. Default-constructed emitter assumes a
     * full-featured modern terminal (zero behavior change for existing code).
     *
     * Usage:
     *   ansi_emitter em;                              // modern terminal (default)
     *   ansi_emitter em(terminal_caps::detect());     // auto-detect
     *   ansi_emitter em(terminal_caps::win_cmd_vt()); // known environment
     *   em.transition(target_style, out);
     *   em.reset(out);
     */
    class ansi_emitter {
      public:
        /** @brief Construct with full modern terminal capabilities (default). */
        ansi_emitter() noexcept = default;

        /** @brief Construct with explicit terminal capabilities for degradation. */
        explicit ansi_emitter(const terminal_caps &caps) noexcept : caps_(caps) {}

        /** @brief Current hardware state (what the terminal is displaying). */
        [[nodiscard]] const style &hw_state() const noexcept { return hw_; }

        /** @brief Current terminal capabilities. */
        [[nodiscard]] const terminal_caps &caps() const noexcept { return caps_; }

        /** @brief Replace terminal capabilities (also invalidates hw state). */
        void set_caps(const terminal_caps &caps) noexcept {
            caps_ = caps;
            hw_ = style{};
        }

        /**
         * @brief Emit the minimal ANSI sequence to transition from current hw
         *        state to `target`, after downgrading to terminal capabilities.
         *
         * If VT sequences are disabled (legacy conhost), produces no output.
         */
        void transition(const style &target, std::string &out) {
            if (!caps_.vt_sequences) {
                return;
            }

            style effective = downgrade_style(target);
            if (hw_ == effective) {
                return;
            }

            std::string delta;
            build_delta(effective, delta);

            std::string reset_path;
            reset_path += "\033[0m";
            reset_path += ansi::sgr(effective);

            if (delta.size() <= reset_path.size()) {
                out += delta;
            } else {
                out += reset_path;
            }

            hw_ = effective;
        }

        /**
         * @brief Emit a full reset and clear hardware state.
         */
        void reset(std::string &out) {
            if (!caps_.vt_sequences) {
                return;
            }
            if (!hw_.is_default()) {
                out += "\033[0m";
                hw_ = style{};
            }
        }

        /** @brief Reset internal state without emitting anything. */
        void invalidate() noexcept { hw_ = style{}; }

      private:
        /**
         * @brief Downgrade a style to match terminal capabilities.
         *
         * - Colors are downgraded via color::downgrade() to caps_.max_colors.
         * - Attributes not in caps_.supported_attrs are stripped.
         */
        [[nodiscard]] style downgrade_style(const style &s) const noexcept {
            style out = s;
            auto depth = static_cast<uint8_t>(caps_.max_colors);
            out.fg = s.fg.downgrade(depth);
            out.bg = s.bg.downgrade(depth);
            out.attrs = s.attrs & caps_.supported_attrs;
            return out;
        }

        void build_delta(const style &target, std::string &out) {
            out.reserve(32);
            out += "\033[";
            bool need_sep = false;
            auto removed = hw_.attrs & ~target.attrs;
            auto added = target.attrs & ~hw_.attrs;
            auto emit_off = [&](attr flag, const char *code) {
                if (has_attr(removed, flag)) {
                    if (need_sep) {
                        out += ';';
                    }
                    out += code;
                    need_sep = true;
                }
            };
            emit_off(attr::bold, "22");
            emit_off(attr::dim, "22");
            emit_off(attr::italic, "23");
            emit_off(attr::underline, "24");
            emit_off(attr::blink, "25");
            emit_off(attr::reverse, "27");
            emit_off(attr::hidden, "28");
            emit_off(attr::strike, "29");
            emit_off(attr::double_underline, "24");
            emit_off(attr::overline, "55");
            emit_off(attr::superscript, "75");
            emit_off(attr::subscript, "75");
            ansi::append_attr_params(out, added, need_sep);
            if (added != attr::none) {
                need_sep = true;
            }
            if (hw_.fg != target.fg) {
                if (need_sep) {
                    out += ';';
                }
                ansi::append_fg_params(out, target.fg);
                need_sep = true;
            }
            if (hw_.bg != target.bg) {
                if (need_sep) {
                    out += ';';
                }
                ansi::append_bg_params(out, target.bg);
            }
            out += 'm';
        }

        style hw_;
        terminal_caps caps_;
    };

} // namespace tapioca
