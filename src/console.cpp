/**
 * @file console.cpp
 * @brief basic_console implementation.
 */

#include "tapioca/console.h"

#include <cstdio>

namespace tapioca {

// ── Constructors ────────────────────────────────────────────────────────

basic_console::basic_console() {
    config_.sink = pal::stdout_sink();
    config_.depth = terminal::detect_color_depth();
    if (terminal::is_tty()) {
        terminal::enable_utf8();
        terminal::enable_vt_processing();
    }
}

basic_console::basic_console(console_config cfg) : config_(std::move(cfg)) {
    if (!config_.sink) {
        config_.sink = pal::stdout_sink();
    }
}

// ── Plain output ────────────────────────────────────────────────────────

void basic_console::write(std::string_view text) {
    config_.sink(text);
}

void basic_console::newline() {
    config_.sink("\n");
}

// ── Styled output ───────────────────────────────────────────────────────

void basic_console::styled_write(const style &sty, std::string_view text) {
    if (!color_enabled()) {
        config_.sink(text);
        return;
    }
    std::string buf;
    buf.reserve(text.size() + 32);
    emitter_.transition(sty, buf);
    buf.append(text);
    emitter_.reset(buf);
    config_.sink(buf);
}

void basic_console::emit_styled(const style &sty, std::string_view text, std::string &buf) {
    if (color_enabled()) {
        emitter_.transition(sty, buf);
    }
    buf.append(text);
}

void basic_console::emit_reset(std::string &buf) {
    if (color_enabled()) {
        emitter_.reset(buf);
    }
}

void basic_console::flush_to_sink(std::string_view buf) {
    config_.sink(buf);
}

// ── State queries ───────────────────────────────────────────────────────

bool basic_console::color_enabled() const noexcept {
    if (config_.no_color) {
        return false;
    }
    if (config_.force_color) {
        return true;
    }
    return config_.depth != color_depth::none;
}

uint32_t basic_console::term_width() const noexcept {
    auto sz = terminal::get_size();
    return sz.width > 0 ? sz.width : 80;
}

} // namespace tapioca
