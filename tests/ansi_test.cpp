#include "tapioca/ansi_emitter.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <string>

using namespace tapioca;

// ── Cursor movement sequences ───────────────────────────────────────────

TEST(AnsiCursor, CursorTo) {
    auto s = ansi::cursor_to(1, 1);
    EXPECT_EQ(s, "\033[1;1H");

    s = ansi::cursor_to(10, 42);
    EXPECT_EQ(s, "\033[10;42H");
}

TEST(AnsiCursor, CursorUp) {
    EXPECT_EQ(ansi::cursor_up(3), "\033[3A");
    EXPECT_EQ(ansi::cursor_up(0), "");
}

TEST(AnsiCursor, CursorDown) {
    EXPECT_EQ(ansi::cursor_down(5), "\033[5B");
    EXPECT_EQ(ansi::cursor_down(0), "");
}

TEST(AnsiCursor, CursorRight) {
    EXPECT_EQ(ansi::cursor_right(2), "\033[2C");
    EXPECT_EQ(ansi::cursor_right(0), "");
}

TEST(AnsiCursor, CursorLeft) {
    EXPECT_EQ(ansi::cursor_left(1), "\033[1D");
    EXPECT_EQ(ansi::cursor_left(0), "");
}

TEST(AnsiCursor, CursorHideShow) {
    EXPECT_STREQ(ansi::cursor_hide, "\033[?25l");
    EXPECT_STREQ(ansi::cursor_show, "\033[?25h");
}

// ── Clear sequences ─────────────────────────────────────────────────────

TEST(AnsiClear, Constants) {
    EXPECT_STREQ(ansi::clear_screen, "\033[2J");
    EXPECT_STREQ(ansi::clear_to_end, "\033[0J");
    EXPECT_STREQ(ansi::clear_line, "\033[2K");
    EXPECT_STREQ(ansi::reset, "\033[0m");
}

// ── SGR (Select Graphic Rendition) ──────────────────────────────────────

TEST(AnsiSgr, DefaultStyleIsEmpty) {
    auto s = ansi::sgr(style{});
    EXPECT_TRUE(s.empty());
}

TEST(AnsiSgr, BoldOnly) {
    style st;
    st.attrs = attr::bold;
    auto s = ansi::sgr(st);
    EXPECT_EQ(s, "\033[1m");
}

TEST(AnsiSgr, FgRgb) {
    style st;
    st.fg = color::from_rgb(255, 128, 0);
    auto s = ansi::sgr(st);
    EXPECT_EQ(s, "\033[38;2;255;128;0m");
}

TEST(AnsiSgr, BgIndex256) {
    style st;
    st.bg = color::from_index_256(42);
    auto s = ansi::sgr(st);
    EXPECT_EQ(s, "\033[48;5;42m");
}

TEST(AnsiSgr, FgIndex16Low) {
    style st;
    st.fg = color::from_index_16(3); // yellow, SGR code 33
    auto s = ansi::sgr(st);
    EXPECT_EQ(s, "\033[33m");
}

TEST(AnsiSgr, FgIndex16Bright) {
    style st;
    st.fg = color::from_index_16(10); // bright green, SGR code 92
    auto s = ansi::sgr(st);
    EXPECT_EQ(s, "\033[92m");
}

TEST(AnsiSgr, Combined) {
    style st;
    st.attrs = attr::bold | attr::italic;
    st.fg = color::from_rgb(255, 0, 0);
    st.bg = color::from_index_16(0);
    auto s = ansi::sgr(st);
    // Should contain bold(1), italic(3), fg rgb, bg
    EXPECT_NE(s.find("1"), std::string::npos);
    EXPECT_NE(s.find("3"), std::string::npos);
    EXPECT_NE(s.find("38;2;255;0;0"), std::string::npos);
    EXPECT_TRUE(s.front() == '\033');
    EXPECT_TRUE(s.back() == 'm');
}

// ── OSC sequences ───────────────────────────────────────────────────────

TEST(AnsiOsc, SetTitle) {
    auto s = ansi::set_title("My App");
    EXPECT_EQ(s, "\033]0;My App\033\\");
}

TEST(AnsiOsc, SetIconName) {
    auto s = ansi::set_icon_name("icon");
    EXPECT_EQ(s, "\033]1;icon\033\\");
}

TEST(AnsiOsc, SetWindowTitle) {
    auto s = ansi::set_window_title("Window");
    EXPECT_EQ(s, "\033]2;Window\033\\");
}

TEST(AnsiOsc, HyperlinkOpen) {
    auto s = ansi::hyperlink_open("https://example.com");
    EXPECT_EQ(s, "\033]8;;https://example.com\033\\");
}

TEST(AnsiOsc, HyperlinkOpenWithId) {
    auto s = ansi::hyperlink_open("https://example.com", "link1");
    EXPECT_EQ(s, "\033]8;id=link1;https://example.com\033\\");
}

TEST(AnsiOsc, HyperlinkClose) {
    EXPECT_STREQ(ansi::hyperlink_close, "\033]8;;\033\\");
}

TEST(AnsiOsc, Notify) {
    auto s = ansi::notify("Build done");
    EXPECT_EQ(s, "\033]9;Build done\033\\");
}

TEST(AnsiOsc, ShellIntegration) {
    EXPECT_STREQ(ansi::prompt_start, "\033]133;A\033\\");
    EXPECT_STREQ(ansi::command_start, "\033]133;B\033\\");
    EXPECT_STREQ(ansi::output_start, "\033]133;C\033\\");
}

TEST(AnsiOsc, CommandFinished) {
    EXPECT_EQ(ansi::command_finished(0), "\033]133;D;0\033\\");
    EXPECT_EQ(ansi::command_finished(1), "\033]133;D;1\033\\");
}

TEST(AnsiOsc, ReportCwd) {
    auto s = ansi::report_cwd("localhost", "/home/user");
    EXPECT_EQ(s, "\033]7;file://localhost/home/user\033\\");
}

TEST(AnsiOsc, ClipboardWrite) {
    auto s = ansi::clipboard_write("c", "SGVsbG8=");
    EXPECT_EQ(s, "\033]52;c;SGVsbG8=\033\\");
}

TEST(AnsiOsc, ClipboardQuery) {
    auto s = ansi::clipboard_query("c");
    EXPECT_EQ(s, "\033]52;c;?\033\\");
}

TEST(AnsiOsc, ClipboardClear) {
    auto s = ansi::clipboard_clear("c");
    EXPECT_EQ(s, "\033]52;c;\033\\");
}

TEST(AnsiOsc, SetPaletteColor) {
    auto s = ansi::set_palette_color(1, 0xaa, 0xbb, 0xcc);
    EXPECT_EQ(s, "\033]4;1;rgb:aa/bb/cc\033\\");
}

TEST(AnsiOsc, ResetPaletteAll) {
    EXPECT_EQ(ansi::reset_palette_color(), "\033]104\033\\");
}

TEST(AnsiOsc, ResetPaletteIndex) {
    auto s = ansi::reset_palette_color(5);
    EXPECT_EQ(s, "\033]104;5\033\\");
}

TEST(AnsiOsc, ColorQueryConstants) {
    EXPECT_STREQ(ansi::query_foreground_color, "\033]10;?\033\\");
    EXPECT_STREQ(ansi::query_background_color, "\033]11;?\033\\");
    EXPECT_STREQ(ansi::query_cursor_color, "\033]12;?\033\\");
}

TEST(AnsiOsc, ColorResetConstants) {
    EXPECT_STREQ(ansi::reset_foreground_color, "\033]110\033\\");
    EXPECT_STREQ(ansi::reset_background_color, "\033]111\033\\");
    EXPECT_STREQ(ansi::reset_cursor_color, "\033]112\033\\");
}

// ── ansi_emitter ────────────────────────────────────────────────────────

TEST(AnsiEmitter, DefaultHwStateIsDefault) {
    ansi_emitter em;
    EXPECT_TRUE(em.hw_state().is_default());
}

TEST(AnsiEmitter, TransitionFromDefault) {
    ansi_emitter em;
    std::string out;
    style target;
    target.fg = color::from_rgb(255, 0, 0);
    em.transition(target, out);
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find("38;2;255;0;0"), std::string::npos);
}

TEST(AnsiEmitter, TransitionSameStyleNoOutput) {
    ansi_emitter em;
    std::string out;

    style target;
    target.fg = color::from_rgb(255, 0, 0);
    em.transition(target, out);
    out.clear();

    // Same style again - no output
    em.transition(target, out);
    EXPECT_TRUE(out.empty());
}

TEST(AnsiEmitter, ResetFromDefault) {
    ansi_emitter em;
    std::string out;
    em.reset(out);
    // hw is already default, no output
    EXPECT_TRUE(out.empty());
}

TEST(AnsiEmitter, ResetFromStyled) {
    ansi_emitter em;
    std::string out;
    style target;
    target.attrs = attr::bold;
    em.transition(target, out);
    out.clear();

    em.reset(out);
    EXPECT_EQ(out, "\033[0m");
    EXPECT_TRUE(em.hw_state().is_default());
}

TEST(AnsiEmitter, Invalidate) {
    ansi_emitter em;
    std::string out;
    style target;
    target.fg = color::from_rgb(10, 20, 30);
    em.transition(target, out);
    out.clear();

    em.invalidate();
    EXPECT_TRUE(em.hw_state().is_default());

    // Should produce output again since hw was reset
    em.transition(target, out);
    EXPECT_FALSE(out.empty());
}

// ── ansi_emitter with terminal_caps ─────────────────────────────────────

TEST(AnsiEmitter, LegacyWinCmdNoOutput) {
    ansi_emitter em(terminal_caps::legacy_win_cmd());
    std::string out;
    style target;
    target.fg = color::from_rgb(255, 0, 0);
    target.attrs = attr::bold;
    em.transition(target, out);
    // Legacy CMD has vt_sequences=false -> no output
    EXPECT_TRUE(out.empty());
}

TEST(AnsiEmitter, LegacyWinCmdResetNoOutput) {
    ansi_emitter em(terminal_caps::legacy_win_cmd());
    std::string out;
    em.reset(out);
    EXPECT_TRUE(out.empty());
}

TEST(AnsiEmitter, SetCapsInvalidatesHw) {
    ansi_emitter em;
    std::string out;
    style target;
    target.attrs = attr::bold;
    em.transition(target, out);
    EXPECT_FALSE(em.hw_state().is_default());

    em.set_caps(terminal_caps::modern());
    EXPECT_TRUE(em.hw_state().is_default());
}

// ── terminal_caps presets ───────────────────────────────────────────────

TEST(TerminalCaps, Modern) {
    auto c = terminal_caps::modern();
    EXPECT_EQ(c.max_colors, color_depth::true_color);
    EXPECT_TRUE(c.vt_sequences);
    EXPECT_TRUE(c.unicode);
}

TEST(TerminalCaps, LegacyWinCmd) {
    auto c = terminal_caps::legacy_win_cmd();
    EXPECT_EQ(c.max_colors, color_depth::none);
    EXPECT_FALSE(c.vt_sequences);
    EXPECT_FALSE(c.unicode);
    EXPECT_EQ(c.supported_attrs, attr::none);
}

TEST(TerminalCaps, WinCmdVt) {
    auto c = terminal_caps::win_cmd_vt();
    EXPECT_EQ(c.max_colors, color_depth::basic_16);
    EXPECT_TRUE(c.vt_sequences);
    EXPECT_FALSE(c.unicode);
    EXPECT_TRUE(has_attr(c.supported_attrs, attr::bold));
    EXPECT_TRUE(has_attr(c.supported_attrs, attr::underline));
    EXPECT_FALSE(has_attr(c.supported_attrs, attr::italic));
}

TEST(TerminalCaps, BasicAnsi) {
    auto c = terminal_caps::basic_ansi();
    EXPECT_EQ(c.max_colors, color_depth::basic_16);
    EXPECT_TRUE(c.vt_sequences);
    EXPECT_TRUE(has_attr(c.supported_attrs, attr::bold));
    EXPECT_TRUE(has_attr(c.supported_attrs, attr::dim));
    EXPECT_TRUE(c.osc.title);
    EXPECT_FALSE(c.osc.hyperlink);
}

TEST(TerminalCaps, Color256) {
    auto c = terminal_caps::color_256();
    EXPECT_EQ(c.max_colors, color_depth::palette_256);
    EXPECT_TRUE(c.unicode);
    EXPECT_TRUE(has_attr(c.supported_attrs, attr::strike));
}
