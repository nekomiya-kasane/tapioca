#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <string_view>

#include "tapioca/unicode_width.h"

using namespace tapioca;

// ── char_width: ASCII ────────────────────────────────────────────────────

TEST(CharWidth, AsciiPrintable) {
    EXPECT_EQ(char_width(U'A'), 1);
    EXPECT_EQ(char_width(U'z'), 1);
    EXPECT_EQ(char_width(U'0'), 1);
    EXPECT_EQ(char_width(U' '), 1);
    EXPECT_EQ(char_width(U'~'), 1);
}

TEST(CharWidth, Tab) {
    // Tab is excluded from control-char check (handled by caller)
    EXPECT_NE(char_width(U'\t'), -1);
}

TEST(CharWidth, Newline) {
    // Newline is excluded from control-char check
    EXPECT_NE(char_width(U'\n'), -1);
}

// ── char_width: control characters ──────────────────────────────────────

TEST(CharWidth, C0ControlChars) {
    // C0 control characters (except HT=0x09, LF=0x0A) return -1
    EXPECT_EQ(char_width(U'\x00'), -1);  // NUL
    EXPECT_EQ(char_width(U'\x01'), -1);  // SOH
    EXPECT_EQ(char_width(U'\x07'), -1);  // BEL
    EXPECT_EQ(char_width(U'\x08'), -1);  // BS
    EXPECT_EQ(char_width(U'\x0B'), -1);  // VT
    EXPECT_EQ(char_width(U'\x0C'), -1);  // FF
    EXPECT_EQ(char_width(U'\x0D'), -1);  // CR
    EXPECT_EQ(char_width(U'\x1B'), -1);  // ESC
    EXPECT_EQ(char_width(U'\x1F'), -1);  // US
}

TEST(CharWidth, DEL) {
    EXPECT_EQ(char_width(U'\x7F'), -1);
}

TEST(CharWidth, C1ControlChars) {
    // C1 control characters (0x80..0x9F) return -1
    EXPECT_EQ(char_width(U'\x80'), -1);
    EXPECT_EQ(char_width(U'\x85'), -1);  // NEL
    EXPECT_EQ(char_width(U'\x9F'), -1);
}

// ── char_width: soft hyphen ─────────────────────────────────────────────

TEST(CharWidth, SoftHyphen) {
    EXPECT_EQ(char_width(U'\u00AD'), 1);
}

// ── char_width: zero-width characters ───────────────────────────────────

TEST(CharWidth, CombiningDiacriticalMarks) {
    EXPECT_EQ(char_width(U'\u0300'), 0);  // Combining grave accent
    EXPECT_EQ(char_width(U'\u0301'), 0);  // Combining acute accent
    EXPECT_EQ(char_width(U'\u036F'), 0);  // Last in combining diacriticals
}

TEST(CharWidth, ZeroWidthSpace) {
    EXPECT_EQ(char_width(U'\u200B'), 0);  // ZWSP
}

TEST(CharWidth, ZeroWidthJoiners) {
    EXPECT_EQ(char_width(U'\u200C'), 0);  // ZWNJ
    EXPECT_EQ(char_width(U'\u200D'), 0);  // ZWJ
}

TEST(CharWidth, BidiMarks) {
    EXPECT_EQ(char_width(U'\u200E'), 0);  // LRM
    EXPECT_EQ(char_width(U'\u200F'), 0);  // RLM
    EXPECT_EQ(char_width(U'\u202A'), 0);  // LRE
    EXPECT_EQ(char_width(U'\u202E'), 0);  // RLO
    EXPECT_EQ(char_width(U'\u2066'), 0);  // LRI
    EXPECT_EQ(char_width(U'\u2069'), 0);  // PDI
}

TEST(CharWidth, VariationSelectors) {
    EXPECT_EQ(char_width(U'\uFE00'), 0);
    EXPECT_EQ(char_width(U'\uFE0F'), 0);
}

TEST(CharWidth, BOM) {
    EXPECT_EQ(char_width(U'\uFEFF'), 0);
}

TEST(CharWidth, VariationSelectorsSupplement) {
    EXPECT_EQ(char_width(U'\U000E0100'), 0);
    EXPECT_EQ(char_width(U'\U000E01EF'), 0);
}

// ── char_width: wide characters ─────────────────────────────────────────

TEST(CharWidth, HangulJamo) {
    EXPECT_EQ(char_width(U'\u1100'), 2);
    EXPECT_EQ(char_width(U'\u115F'), 2);
}

TEST(CharWidth, CJKUnifiedIdeographs) {
    EXPECT_EQ(char_width(U'\u4E00'), 2);  // "one" in CJK
    EXPECT_EQ(char_width(U'\u9FFF'), 2);
}

TEST(CharWidth, Hiragana) {
    EXPECT_EQ(char_width(U'\u3042'), 2);  // Hiragana "a"
    EXPECT_EQ(char_width(U'\u3093'), 2);  // Hiragana "n"
}

TEST(CharWidth, Katakana) {
    EXPECT_EQ(char_width(U'\u30A2'), 2);  // Katakana "a"
}

TEST(CharWidth, FullwidthForms) {
    EXPECT_EQ(char_width(U'\uFF01'), 2);  // Fullwidth exclamation
    EXPECT_EQ(char_width(U'\uFF5E'), 2);  // Fullwidth tilde
}

TEST(CharWidth, Emoji) {
    EXPECT_EQ(char_width(U'\U0001F600'), 2);  // Grinning face
    EXPECT_EQ(char_width(U'\U0001F4A9'), 2);  // Pile of poo
    EXPECT_EQ(char_width(U'\U0001F680'), 2);  // Rocket
}

TEST(CharWidth, CJKExtensionB) {
    EXPECT_EQ(char_width(U'\U00020000'), 2);
    EXPECT_EQ(char_width(U'\U0002A6DF'), 2);
}

// ── char_width: narrow Latin/Greek ──────────────────────────────────────

TEST(CharWidth, LatinExtended) {
    EXPECT_EQ(char_width(U'\u00C0'), 1);  // A with grave
    EXPECT_EQ(char_width(U'\u00FF'), 1);  // y with diaeresis
}

TEST(CharWidth, GreekLetters) {
    EXPECT_EQ(char_width(U'\u03B1'), 1);  // alpha
    EXPECT_EQ(char_width(U'\u03C9'), 1);  // omega
}

// ── utf8_decode ─────────────────────────────────────────────────────────

TEST(Utf8Decode, EmptyInput) {
    char32_t cp;
    int bytes = utf8_decode("", 0, cp);
    EXPECT_EQ(bytes, 0);
    EXPECT_EQ(cp, U'\xFFFD');
}

TEST(Utf8Decode, Ascii) {
    char32_t cp;
    int bytes = utf8_decode("A", 1, cp);
    EXPECT_EQ(bytes, 1);
    EXPECT_EQ(cp, U'A');
}

TEST(Utf8Decode, TwoByteChar) {
    // U+00E9 (e with acute) = 0xC3 0xA9
    const char data[] = "\xC3\xA9";
    char32_t cp;
    int bytes = utf8_decode(data, 2, cp);
    EXPECT_EQ(bytes, 2);
    EXPECT_EQ(cp, U'\u00E9');
}

TEST(Utf8Decode, ThreeByteChar) {
    // U+4E00 (CJK "one") = 0xE4 0xB8 0x80
    const char data[] = "\xE4\xB8\x80";
    char32_t cp;
    int bytes = utf8_decode(data, 3, cp);
    EXPECT_EQ(bytes, 3);
    EXPECT_EQ(cp, U'\u4E00');
}

TEST(Utf8Decode, FourByteChar) {
    // U+1F600 (grinning face) = 0xF0 0x9F 0x98 0x80
    const char data[] = "\xF0\x9F\x98\x80";
    char32_t cp;
    int bytes = utf8_decode(data, 4, cp);
    EXPECT_EQ(bytes, 4);
    EXPECT_EQ(cp, U'\U0001F600');
}

TEST(Utf8Decode, TruncatedTwoByte) {
    // Two-byte sequence but only 1 byte available
    const char data[] = "\xC3";
    char32_t cp;
    int bytes = utf8_decode(data, 1, cp);
    EXPECT_EQ(bytes, 1);
    EXPECT_EQ(cp, U'\xFFFD');
}

TEST(Utf8Decode, TruncatedThreeByte) {
    // Three-byte sequence but only 2 bytes available
    const char data[] = "\xE4\xB8";
    char32_t cp;
    int bytes = utf8_decode(data, 2, cp);
    EXPECT_EQ(bytes, 1);
    EXPECT_EQ(cp, U'\xFFFD');
}

TEST(Utf8Decode, TruncatedFourByte) {
    // Four-byte sequence but only 3 bytes available
    const char data[] = "\xF0\x9F\x98";
    char32_t cp;
    int bytes = utf8_decode(data, 3, cp);
    EXPECT_EQ(bytes, 1);
    EXPECT_EQ(cp, U'\xFFFD');
}

TEST(Utf8Decode, InvalidContinuationByte) {
    // Two-byte lead but bad continuation
    const char data[] = "\xC3\x00";
    char32_t cp;
    int bytes = utf8_decode(data, 2, cp);
    EXPECT_EQ(bytes, 1);
    EXPECT_EQ(cp, U'\xFFFD');
}

TEST(Utf8Decode, OverlongTwoByte) {
    // Overlong encoding of U+0001: 0xC0 0x81 (should be 0x01)
    const char data[] = "\xC0\x81";
    char32_t cp;
    int bytes = utf8_decode(data, 2, cp);
    EXPECT_EQ(bytes, 2);
    EXPECT_EQ(cp, U'\xFFFD');
}

TEST(Utf8Decode, OverlongThreeByte) {
    // Overlong encoding of U+007F: 0xE0 0x81 0xBF
    const char data[] = "\xE0\x81\xBF";
    char32_t cp;
    int bytes = utf8_decode(data, 3, cp);
    EXPECT_EQ(bytes, 3);
    EXPECT_EQ(cp, U'\xFFFD');
}

TEST(Utf8Decode, OverlongFourByte) {
    // Overlong encoding of U+0800: 0xF0 0x80 0xA0 0x80
    const char data[] = "\xF0\x80\xA0\x80";
    char32_t cp;
    int bytes = utf8_decode(data, 4, cp);
    EXPECT_EQ(bytes, 4);
    EXPECT_EQ(cp, U'\xFFFD');
}

TEST(Utf8Decode, InvalidLeadByte) {
    // 0xFE is not a valid UTF-8 lead byte
    const char data[] = "\xFE";
    char32_t cp;
    int bytes = utf8_decode(data, 1, cp);
    EXPECT_EQ(bytes, 1);
    EXPECT_EQ(cp, U'\xFFFD');
}

// ── string_width ────────────────────────────────────────────────────────

TEST(StringWidth, Empty) {
    EXPECT_EQ(string_width(""), 0);
}

TEST(StringWidth, AsciiOnly) {
    EXPECT_EQ(string_width("Hello"), 5);
    EXPECT_EQ(string_width("abc123"), 6);
}

TEST(StringWidth, CJKString) {
    // Three CJK characters: each is 2 wide
    EXPECT_EQ(string_width("\xE4\xB8\x80\xE4\xBA\x8C\xE4\xB8\x89"), 6);
}

TEST(StringWidth, MixedAsciiAndCJK) {
    // "A" (1) + U+4E00 (2) + "B" (1) = 4
    EXPECT_EQ(string_width("A\xE4\xB8\x80" "B"), 4);
}

TEST(StringWidth, CombiningCharacters) {
    // "e" (1) + combining acute accent (0) = 1
    EXPECT_EQ(string_width("e\xCC\x81"), 1);
}

TEST(StringWidth, Emoji) {
    // U+1F600 grinning face = width 2
    EXPECT_EQ(string_width("\xF0\x9F\x98\x80"), 2);
}

TEST(StringWidth, ZeroWidthSpaceInString) {
    // "a" + ZWSP + "b" = 2
    EXPECT_EQ(string_width("a\xE2\x80\x8B" "b"), 2);
}

TEST(StringWidth, ControlCharsSkipped) {
    // ESC (0x1B) is control => width -1 => skipped
    // '[', '3', '1', 'm' are ASCII printable => width 1 each => total 4
    EXPECT_EQ(string_width("\x1B[31m"), 4);
}

TEST(StringWidth, HangulSyllables) {
    // U+AC00 = 2
    EXPECT_EQ(string_width("\xEA\xB0\x80"), 2);
}
