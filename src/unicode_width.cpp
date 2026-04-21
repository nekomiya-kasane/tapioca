/**
 * @file unicode_width.cpp
 * @brief Unicode character width implementation with range-based EAW lookup.
 *
 * The wide_ranges table covers East Asian Wide (W) and Fullwidth (F) characters.
 * The zero_width_ranges table covers combining marks and other zero-width characters.
 * Derived from Unicode 15.1 EastAsianWidth.txt and DerivedGeneralCategory.txt.
 */

#include "tapioca/unicode_width.h"

#include <algorithm>

namespace tapioca {

    namespace {

        struct range {
            char32_t lo;
            char32_t hi;
        };

        // Zero-width character ranges (combining marks, format chars, etc.)
        constexpr range zero_width_ranges[] = {
            {0x0300, 0x036F}, // Combining Diacritical Marks
            {0x0483, 0x0489}, // Combining Cyrillic
            {0x0591, 0x05BD}, // Hebrew combining
            {0x05BF, 0x05BF},   {0x05C1, 0x05C2},   {0x05C4, 0x05C5},   {0x05C7, 0x05C7},
            {0x0610, 0x061A}, // Arabic combining
            {0x064B, 0x065F},   {0x0670, 0x0670},   {0x06D6, 0x06DC},   {0x06DF, 0x06E4},
            {0x06E7, 0x06E8},   {0x06EA, 0x06ED},   {0x0711, 0x0711},   {0x0730, 0x074A}, // Syriac combining
            {0x07A6, 0x07B0},                                                             // Thaana combining
            {0x07EB, 0x07F3},   {0x07FD, 0x07FD},   {0x0816, 0x0819},   {0x081B, 0x0823},
            {0x0825, 0x0827},   {0x0829, 0x082D},   {0x0859, 0x085B},   {0x0898, 0x089F},
            {0x08CA, 0x08E1},   {0x08E3, 0x0902},   {0x093A, 0x093A},   {0x093C, 0x093C},
            {0x0941, 0x0948},   {0x094D, 0x094D},   {0x0951, 0x0957},   {0x0962, 0x0963},
            {0x0981, 0x0981},   {0x09BC, 0x09BC},   {0x09C1, 0x09C4},   {0x09CD, 0x09CD},
            {0x09E2, 0x09E3},   {0x09FE, 0x09FE},   {0x0A01, 0x0A02},   {0x0A3C, 0x0A3C},
            {0x0A41, 0x0A42},   {0x0A47, 0x0A48},   {0x0A4B, 0x0A4D},   {0x0A51, 0x0A51},
            {0x0A70, 0x0A71},   {0x0A75, 0x0A75},   {0x0A81, 0x0A82},   {0x0ABC, 0x0ABC},
            {0x0AC1, 0x0AC5},   {0x0AC7, 0x0AC8},   {0x0ACD, 0x0ACD},   {0x0AE2, 0x0AE3},
            {0x0AFA, 0x0AFF},   {0x0B01, 0x0B01},   {0x0B3C, 0x0B3C},   {0x0B3F, 0x0B3F},
            {0x0B41, 0x0B44},   {0x0B4D, 0x0B4D},   {0x0B55, 0x0B56},   {0x0B62, 0x0B63},
            {0x0B82, 0x0B82},   {0x0BC0, 0x0BC0},   {0x0BCD, 0x0BCD},   {0x0C00, 0x0C00},
            {0x0C04, 0x0C04},   {0x0C3C, 0x0C3C},   {0x0C3E, 0x0C40},   {0x0C46, 0x0C48},
            {0x0C4A, 0x0C4D},   {0x0C55, 0x0C56},   {0x0C62, 0x0C63},   {0x0C81, 0x0C81},
            {0x0CBC, 0x0CBC},   {0x0CBF, 0x0CBF},   {0x0CC6, 0x0CC6},   {0x0CCC, 0x0CCD},
            {0x0CE2, 0x0CE3},   {0x0D00, 0x0D01},   {0x0D3B, 0x0D3C},   {0x0D41, 0x0D44},
            {0x0D4D, 0x0D4D},   {0x0D62, 0x0D63},   {0x0D81, 0x0D81},   {0x0DCA, 0x0DCA},
            {0x0DD2, 0x0DD4},   {0x0DD6, 0x0DD6},   {0x0E31, 0x0E31},   {0x0E34, 0x0E3A},
            {0x0E47, 0x0E4E},   {0x0EB1, 0x0EB1},   {0x0EB4, 0x0EBC},   {0x0EC8, 0x0ECE},
            {0x0F18, 0x0F19},   {0x0F35, 0x0F35},   {0x0F37, 0x0F37},   {0x0F39, 0x0F39},
            {0x0F71, 0x0F7E},   {0x0F80, 0x0F84},   {0x0F86, 0x0F87},   {0x0F8D, 0x0F97},
            {0x0F99, 0x0FBC},   {0x0FC6, 0x0FC6},   {0x102D, 0x1030},   {0x1032, 0x1037},
            {0x1039, 0x103A},   {0x103D, 0x103E},   {0x1058, 0x1059},   {0x105E, 0x1060},
            {0x1071, 0x1074},   {0x1082, 0x1082},   {0x1085, 0x1086},   {0x108D, 0x108D},
            {0x109D, 0x109D},   {0x1160, 0x11FF}, // Hangul Jungseong + Jongseong
            {0x135D, 0x135F},   {0x1712, 0x1714},   {0x1732, 0x1733},   {0x1752, 0x1753},
            {0x1772, 0x1773},   {0x17B4, 0x17B5},   {0x17B7, 0x17BD},   {0x17C6, 0x17C6},
            {0x17C9, 0x17D3},   {0x17DD, 0x17DD},   {0x180B, 0x180F}, // Mongolian free variation selectors
            {0x1885, 0x1886},   {0x18A9, 0x18A9},   {0x1920, 0x1922},   {0x1927, 0x1928},
            {0x1932, 0x1932},   {0x1939, 0x193B},   {0x1A17, 0x1A18},   {0x1A1B, 0x1A1B},
            {0x1A56, 0x1A56},   {0x1A58, 0x1A5E},   {0x1A60, 0x1A60},   {0x1A62, 0x1A62},
            {0x1A65, 0x1A6C},   {0x1A73, 0x1A7C},   {0x1A7F, 0x1A7F},   {0x1AB0, 0x1ACE},
            {0x1B00, 0x1B03},   {0x1B34, 0x1B34},   {0x1B36, 0x1B3A},   {0x1B3C, 0x1B3C},
            {0x1B42, 0x1B42},   {0x1B6B, 0x1B73},   {0x1B80, 0x1B81},   {0x1BA2, 0x1BA5},
            {0x1BA8, 0x1BA9},   {0x1BAB, 0x1BAD},   {0x1BE6, 0x1BE6},   {0x1BE8, 0x1BE9},
            {0x1BED, 0x1BED},   {0x1BEF, 0x1BF1},   {0x1C2C, 0x1C33},   {0x1C36, 0x1C37},
            {0x1CD0, 0x1CD2},   {0x1CD4, 0x1CE0},   {0x1CE2, 0x1CE8},   {0x1CED, 0x1CED},
            {0x1CF4, 0x1CF4},   {0x1CF8, 0x1CF9},   {0x1DC0, 0x1DFF}, // Combining Diacritical Marks Supplement
            {0x200B, 0x200F},                                         // Zero-width space, joiners, direction marks
            {0x202A, 0x202E},                                         // Bidi formatting
            {0x2060, 0x2064},                                         // Word joiner, invisible operators
            {0x2066, 0x206F},                                         // Bidi isolates
            {0x20D0, 0x20F0},                                         // Combining Diacritical Marks for Symbols
            {0xFE00, 0xFE0F},                                         // Variation Selectors
            {0xFE20, 0xFE2F},                                         // Combining Half Marks
            {0xFEFF, 0xFEFF},                                         // BOM / ZWNBSP
            {0xFFF9, 0xFFFB},                                         // Interlinear annotation
            {0x1D167, 0x1D169}, {0x1D173, 0x1D182}, {0x1D185, 0x1D18B}, {0x1D1AA, 0x1D1AD},
            {0x1D242, 0x1D244}, {0xE0001, 0xE0001}, // Language tag
            {0xE0020, 0xE007F},                     // Tag components
            {0xE0100, 0xE01EF},                     // Variation Selectors Supplement
        };

        // Wide character ranges (East Asian Wide + Fullwidth)
        constexpr range wide_ranges[] = {
            {0x1100, 0x115F},                                         // Hangul Jamo
            {0x231A, 0x231B},                                         // Watch, Hourglass
            {0x2329, 0x232A},                                         // Angle brackets
            {0x23E9, 0x23F3},                                         // Various symbols
            {0x23F8, 0x23FA},   {0x25FD, 0x25FE},   {0x2614, 0x2615}, // Umbrella, Hot beverage
            {0x2648, 0x2653},                                         // Zodiac
            {0x267F, 0x267F},   {0x2693, 0x2693},   {0x26A1, 0x26A1},
            {0x26AA, 0x26AB},   {0x26BD, 0x26BE},   {0x26C4, 0x26C5},
            {0x26CE, 0x26CE},   {0x26D4, 0x26D4},   {0x26EA, 0x26EA},
            {0x26F2, 0x26F3},   {0x26F5, 0x26F5},   {0x26FA, 0x26FA},
            {0x26FD, 0x26FD},   {0x2702, 0x2702},   {0x2705, 0x2705},
            {0x2708, 0x270D},   {0x270F, 0x270F},   {0x2712, 0x2712},
            {0x2714, 0x2714},   {0x2716, 0x2716},   {0x271D, 0x271D},
            {0x2721, 0x2721},   {0x2728, 0x2728},   {0x2733, 0x2734},
            {0x2744, 0x2744},   {0x2747, 0x2747},   {0x274C, 0x274C},
            {0x274E, 0x274E},   {0x2753, 0x2755},   {0x2757, 0x2757},
            {0x2763, 0x2764},   {0x2795, 0x2797},   {0x27A1, 0x27A1},
            {0x27B0, 0x27B0},   {0x27BF, 0x27BF},   {0x2934, 0x2935},
            {0x2B05, 0x2B07},   {0x2B1B, 0x2B1C},   {0x2B50, 0x2B50},
            {0x2B55, 0x2B55},   {0x2E80, 0x2E99},   // CJK Radicals Supplement
            {0x2E9B, 0x2EF3},   {0x2F00, 0x2FD5},   // Kangxi Radicals
            {0x2FF0, 0x2FFB},                       // Ideographic Description Characters
            {0x3000, 0x303E},                       // CJK Symbols and Punctuation
            {0x3041, 0x3096},                       // Hiragana
            {0x3099, 0x30FF},                       // Katakana
            {0x3105, 0x312F},                       // Bopomofo
            {0x3131, 0x318E},                       // Hangul Compatibility Jamo
            {0x3190, 0x31E3},                       // Kanbun + CJK Strokes
            {0x31F0, 0x321E},                       // Katakana Phonetic Extensions
            {0x3220, 0x3247},                       // Enclosed CJK
            {0x3250, 0x4DBF},                       // CJK Unified Ideographs Extension A
            {0x4E00, 0xA48C},                       // CJK Unified Ideographs
            {0xA490, 0xA4C6},                       // Yi Radicals
            {0xA960, 0xA97C},                       // Hangul Jamo Extended-A
            {0xAC00, 0xD7A3},                       // Hangul Syllables
            {0xF900, 0xFAFF},                       // CJK Compatibility Ideographs
            {0xFE10, 0xFE19},                       // Vertical Forms
            {0xFE30, 0xFE6B},                       // CJK Compatibility Forms
            {0xFF01, 0xFF60},                       // Fullwidth Forms
            {0xFFE0, 0xFFE6},                       // Fullwidth Signs
            {0x16FE0, 0x16FE4},                     // Ideographic Symbols
            {0x16FF0, 0x16FF1}, {0x17000, 0x187F7}, // Tangut
            {0x18800, 0x18CD5}, {0x18D00, 0x18D08}, {0x1AFF0, 0x1AFF3},
            {0x1AFF5, 0x1AFFB}, {0x1AFFD, 0x1AFFE}, {0x1B000, 0x1B122}, // Kana Supplement + Extended
            {0x1B132, 0x1B132}, {0x1B150, 0x1B152}, {0x1B155, 0x1B155},
            {0x1B164, 0x1B167}, {0x1B170, 0x1B2FB}, // Nushu
            {0x1F004, 0x1F004},                     // Mahjong tile
            {0x1F0CF, 0x1F0CF},                     // Playing card
            {0x1F18E, 0x1F18E}, {0x1F191, 0x1F19A}, {0x1F200, 0x1F202},
            {0x1F210, 0x1F23B}, {0x1F240, 0x1F248}, {0x1F250, 0x1F251},
            {0x1F260, 0x1F265}, {0x1F300, 0x1F320}, // Miscellaneous Symbols and Pictographs
            {0x1F32D, 0x1F335}, {0x1F337, 0x1F37C}, {0x1F37E, 0x1F393},
            {0x1F3A0, 0x1F3CA}, {0x1F3CF, 0x1F3D3}, {0x1F3E0, 0x1F3F0},
            {0x1F3F4, 0x1F3F4}, {0x1F3F8, 0x1F43E}, {0x1F440, 0x1F440},
            {0x1F442, 0x1F4FC}, {0x1F4FF, 0x1F53D}, {0x1F54B, 0x1F54E},
            {0x1F550, 0x1F567}, {0x1F57A, 0x1F57A}, {0x1F595, 0x1F596},
            {0x1F5A4, 0x1F5A4}, {0x1F5FB, 0x1F64F}, // Emoticons
            {0x1F680, 0x1F6C5},                     // Transport and Map
            {0x1F6CC, 0x1F6CC}, {0x1F6D0, 0x1F6D2}, {0x1F6D5, 0x1F6D7},
            {0x1F6DC, 0x1F6DF}, {0x1F6EB, 0x1F6EC}, {0x1F6F4, 0x1F6FC},
            {0x1F7E0, 0x1F7EB}, {0x1F7F0, 0x1F7F0}, {0x1F90C, 0x1F93A},
            {0x1F93C, 0x1F945}, {0x1F947, 0x1F9FF}, // Supplemental Symbols and Pictographs
            {0x1FA00, 0x1FA53}, {0x1FA60, 0x1FA6D}, {0x1FA70, 0x1FA7C},
            {0x1FA80, 0x1FA88}, {0x1FA90, 0x1FABD}, {0x1FABF, 0x1FAC5},
            {0x1FACE, 0x1FADB}, {0x1FAE0, 0x1FAE8}, {0x1FAF0, 0x1FAF8},
            {0x20000, 0x2FFFD}, // CJK Unified Ideographs Extension B-F
            {0x30000, 0x3FFFD}, // CJK Unified Ideographs Extension G-I
        };

        bool in_ranges(char32_t cp, const range *table, size_t count) noexcept {
            // Binary search on sorted ranges
            size_t lo = 0, hi = count;
            while (lo < hi) {
                size_t mid = lo + (hi - lo) / 2;
                if (cp > table[mid].hi) {
                    lo = mid + 1;
                } else if (cp < table[mid].lo) {
                    hi = mid;
                } else {
                    return true;
                }
            }
            return false;
        }

    } // anonymous namespace

    // ── char_width ──────────────────────────────────────────────────────────

    int char_width(char32_t cp) noexcept {
        // C0 control characters (except HT, LF which are handled by caller)
        if (cp < 0x20 && cp != 0x09 && cp != 0x0A) {
            return -1;
        }
        // DEL
        if (cp == 0x7F) {
            return -1;
        }
        // C1 control characters
        if (cp >= 0x80 && cp < 0xA0) {
            return -1;
        }

        // Soft hyphen
        if (cp == 0x00AD) {
            return 1;
        }

        // Zero-width
        if (in_ranges(cp, zero_width_ranges, std::size(zero_width_ranges))) {
            return 0;
        }

        // Wide
        if (in_ranges(cp, wide_ranges, std::size(wide_ranges))) {
            return 2;
        }

        // Default: narrow
        return 1;
    }

    // ── utf8_decode ─────────────────────────────────────────────────────────

    int utf8_decode(const char *data, size_t len, char32_t &cp) noexcept {
        if (len == 0) {
            cp = U'\xFFFD';
            return 0;
        }

        auto b0 = static_cast<uint8_t>(data[0]);

        // ASCII
        if (b0 < 0x80) {
            cp = b0;
            return 1;
        }

        // 2-byte
        if ((b0 & 0xE0) == 0xC0) {
            if (len < 2 || (static_cast<uint8_t>(data[1]) & 0xC0) != 0x80) {
                cp = U'\xFFFD';
                return 1;
            }
            cp = (static_cast<char32_t>(b0 & 0x1F) << 6) | (static_cast<char32_t>(data[1]) & 0x3F);
            if (cp < 0x80) {
                cp = U'\xFFFD';
                return 2;
            } // overlong
            return 2;
        }

        // 3-byte
        if ((b0 & 0xF0) == 0xE0) {
            if (len < 3 || (static_cast<uint8_t>(data[1]) & 0xC0) != 0x80 ||
                (static_cast<uint8_t>(data[2]) & 0xC0) != 0x80) {
                cp = U'\xFFFD';
                return 1;
            }
            cp = (static_cast<char32_t>(b0 & 0x0F) << 12) | (static_cast<char32_t>(data[1] & 0x3F) << 6) |
                 (static_cast<char32_t>(data[2]) & 0x3F);
            if (cp < 0x800) {
                cp = U'\xFFFD';
                return 3;
            } // overlong
            return 3;
        }

        // 4-byte
        if ((b0 & 0xF8) == 0xF0) {
            if (len < 4 || (static_cast<uint8_t>(data[1]) & 0xC0) != 0x80 ||
                (static_cast<uint8_t>(data[2]) & 0xC0) != 0x80 || (static_cast<uint8_t>(data[3]) & 0xC0) != 0x80) {
                cp = U'\xFFFD';
                return 1;
            }
            cp = (static_cast<char32_t>(b0 & 0x07) << 18) | (static_cast<char32_t>(data[1] & 0x3F) << 12) |
                 (static_cast<char32_t>(data[2] & 0x3F) << 6) | (static_cast<char32_t>(data[3]) & 0x3F);
            if (cp < 0x10000 || cp > 0x10FFFF) {
                cp = U'\xFFFD';
                return 4;
            }
            return 4;
        }

        cp = U'\xFFFD';
        return 1;
    }

    // ── string_width ────────────────────────────────────────────────────────

    int string_width(std::string_view utf8) noexcept {
        int total = 0;
        const char *p = utf8.data();
        size_t remaining = utf8.size();

        while (remaining > 0) {
            char32_t cp;
            int bytes = utf8_decode(p, remaining, cp);
            if (bytes == 0) {
                break;
            }

            int w = char_width(cp);
            if (w > 0) {
                total += w; // skip control chars (w == -1) and combining (w == 0)
            }

            p += bytes;
            remaining -= static_cast<size_t>(bytes);
        }

        return total;
    }

} // namespace tapioca
