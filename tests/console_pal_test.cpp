#include "tapioca/console.h"
#include "tapioca/pal.h"
#include "tapioca/style.h"

#include <gtest/gtest.h>
#include <string>
#include <string_view>

using namespace tapioca;

// ── pal::string_sink ────────────────────────────────────────────────────

TEST(PalStringSink, AppendsData) {
    std::string buf;
    auto sink = pal::string_sink(buf);
    sink("hello");
    sink(" world");
    EXPECT_EQ(buf, "hello world");
}

TEST(PalStringSink, EmptyWrite) {
    std::string buf;
    auto sink = pal::string_sink(buf);
    sink("");
    EXPECT_TRUE(buf.empty());
}

// ── pal::clipboard_set ──────────────────────────────────────────────────

TEST(PalClipboard, SetEmptyData) {
    std::string buf;
    auto sink = pal::string_sink(buf);
    pal::clipboard_set("", sink);
    // No-op for empty data
    EXPECT_TRUE(buf.empty());
}

TEST(PalClipboard, SetNullSink) {
    // Null sink should not crash
    output_sink null_sink;
    pal::clipboard_set("test", null_sink);
}

TEST(PalClipboard, SetProducesOSC52) {
    std::string buf;
    auto sink = pal::string_sink(buf);
    pal::clipboard_set("hello", sink);
    // Should contain OSC 52 sequence
    EXPECT_NE(buf.find("\033]52;c;"), std::string::npos);
    // Should end with ST
    EXPECT_NE(buf.find("\033\\"), std::string::npos);
    // Should contain base64 of "hello" = "aGVsbG8="
    EXPECT_NE(buf.find("aGVsbG8="), std::string::npos);
}

TEST(PalClipboard, SetBase64SingleByte) {
    std::string buf;
    auto sink = pal::string_sink(buf);
    pal::clipboard_set("A", sink);
    // base64("A") = "QQ=="
    EXPECT_NE(buf.find("QQ=="), std::string::npos);
}

TEST(PalClipboard, SetBase64TwoBytes) {
    std::string buf;
    auto sink = pal::string_sink(buf);
    pal::clipboard_set("AB", sink);
    // base64("AB") = "QUI="
    EXPECT_NE(buf.find("QUI="), std::string::npos);
}

TEST(PalClipboard, SetBase64ThreeBytes) {
    std::string buf;
    auto sink = pal::string_sink(buf);
    pal::clipboard_set("ABC", sink);
    // base64("ABC") = "QUJD" (no padding)
    EXPECT_NE(buf.find("QUJD"), std::string::npos);
}

// ── pal::clipboard_request ──────────────────────────────────────────────

TEST(PalClipboard, RequestNullSink) {
    output_sink null_sink;
    pal::clipboard_request(null_sink);
    // Should not crash
}

TEST(PalClipboard, RequestProducesOSC52Query) {
    std::string buf;
    auto sink = pal::string_sink(buf);
    pal::clipboard_request(sink);
    EXPECT_NE(buf.find("\033]52;c;?"), std::string::npos);
    EXPECT_NE(buf.find("\033\\"), std::string::npos);
}

// ── basic_console with custom config ────────────────────────────────────

TEST(BasicConsole, CustomSinkWrite) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::true_color;

    basic_console con(cfg);
    con.write("hello");
    EXPECT_EQ(buf, "hello");
}

TEST(BasicConsole, Newline) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::true_color;

    basic_console con(cfg);
    con.newline();
    EXPECT_EQ(buf, "\n");
}

TEST(BasicConsole, WriteAndNewline) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::true_color;

    basic_console con(cfg);
    con.write("line1");
    con.newline();
    con.write("line2");
    EXPECT_EQ(buf, "line1\nline2");
}

// ── basic_console: color_enabled ────────────────────────────────────────

TEST(BasicConsole, ColorEnabledDefault) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::true_color;

    basic_console con(cfg);
    EXPECT_TRUE(con.color_enabled());
}

TEST(BasicConsole, ColorDisabledNoColor) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::true_color;
    cfg.no_color = true;

    basic_console con(cfg);
    EXPECT_FALSE(con.color_enabled());
}

TEST(BasicConsole, ColorDisabledDepthNone) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::none;

    basic_console con(cfg);
    EXPECT_FALSE(con.color_enabled());
}

TEST(BasicConsole, ForceColorOverridesDepthNone) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::none;
    cfg.force_color = true;

    basic_console con(cfg);
    EXPECT_TRUE(con.color_enabled());
}

TEST(BasicConsole, NoColorOverridesForceColor) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::true_color;
    cfg.no_color = true;
    cfg.force_color = true;

    basic_console con(cfg);
    // no_color is checked first
    EXPECT_FALSE(con.color_enabled());
}

// ── basic_console: styled_write ─────────────────────────────────────────

TEST(BasicConsole, StyledWriteWithColorDisabled) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::none;

    basic_console con(cfg);
    style sty;
    sty.fg = color::from_rgb(255, 0, 0);
    con.styled_write(sty, "plain text");
    // Color disabled -- output is just the plain text (no ANSI codes)
    EXPECT_EQ(buf, "plain text");
}

TEST(BasicConsole, StyledWriteWithColorEnabled) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::true_color;

    basic_console con(cfg);
    style sty;
    sty.fg = color::from_rgb(255, 0, 0);
    con.styled_write(sty, "red text");
    // Should contain ANSI escape codes + the text + reset
    EXPECT_NE(buf.find("red text"), std::string::npos);
    EXPECT_NE(buf.find("\033["), std::string::npos); // ANSI CSI
}

// ── basic_console: emit_styled / emit_reset / flush_to_sink ─────────────

TEST(BasicConsole, EmitStyledBatching) {
    std::string output;
    console_config cfg;
    cfg.sink = pal::string_sink(output);
    cfg.depth = color_depth::true_color;

    basic_console con(cfg);
    std::string buf;

    style sty;
    sty.fg = color::from_rgb(0, 255, 0);
    con.emit_styled(sty, "green", buf);
    con.emit_reset(buf);
    con.flush_to_sink(buf);

    EXPECT_NE(output.find("green"), std::string::npos);
    EXPECT_NE(output.find("\033["), std::string::npos);
}

TEST(BasicConsole, EmitStyledColorDisabled) {
    std::string output;
    console_config cfg;
    cfg.sink = pal::string_sink(output);
    cfg.depth = color_depth::none;

    basic_console con(cfg);
    std::string buf;

    style sty;
    sty.fg = color::from_rgb(0, 255, 0);
    con.emit_styled(sty, "text", buf);
    con.emit_reset(buf);
    con.flush_to_sink(buf);

    // No ANSI codes when color disabled
    EXPECT_EQ(output, "text");
}

// ── basic_console: config/depth accessors ───────────────────────────────

TEST(BasicConsole, DepthAccessor) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::palette_256;

    basic_console con(cfg);
    EXPECT_EQ(con.depth(), color_depth::palette_256);
}

TEST(BasicConsole, ConfigAccessor) {
    std::string buf;
    console_config cfg;
    cfg.sink = pal::string_sink(buf);
    cfg.depth = color_depth::basic_16;
    cfg.no_color = true;

    basic_console con(cfg);
    EXPECT_EQ(con.config().depth, color_depth::basic_16);
    EXPECT_TRUE(con.config().no_color);
}

// ── basic_console: null sink fallback ───────────────────────────────────

TEST(BasicConsole, NullSinkFallsBackToStdout) {
    console_config cfg;
    // sink is default (nullptr) -- should get replaced by stdout_sink
    cfg.depth = color_depth::none;

    basic_console con(cfg);
    // Just verify it doesn't crash
    EXPECT_NE(con.config().sink, nullptr);
}
