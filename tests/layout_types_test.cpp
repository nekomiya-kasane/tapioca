#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

#include "tapioca/layout_types.h"

using namespace tapioca;

// ── rect ────────────────────────────────────────────────────────────────

TEST(Rect, DefaultIsEmpty) {
    rect r;
    EXPECT_TRUE(r.empty());
    EXPECT_EQ(r.x, 0u);
    EXPECT_EQ(r.y, 0u);
    EXPECT_EQ(r.w, 0u);
    EXPECT_EQ(r.h, 0u);
}

TEST(Rect, NonEmpty) {
    rect r{10, 20, 100, 50};
    EXPECT_FALSE(r.empty());
}

TEST(Rect, EmptyWidth) {
    rect r{0, 0, 0, 50};
    EXPECT_TRUE(r.empty());
}

TEST(Rect, EmptyHeight) {
    rect r{0, 0, 100, 0};
    EXPECT_TRUE(r.empty());
}

TEST(Rect, Equality) {
    rect a{1, 2, 3, 4};
    rect b{1, 2, 3, 4};
    rect c{1, 2, 3, 5};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(Rect, InsetBasic) {
    rect r{10, 20, 100, 80};
    auto inset = r.inset(5, 10, 5, 10);
    EXPECT_EQ(inset.x, 20u);   // x + left
    EXPECT_EQ(inset.y, 25u);   // y + top
    EXPECT_EQ(inset.w, 80u);   // w - left - right
    EXPECT_EQ(inset.h, 70u);   // h - top - bottom
}

TEST(Rect, InsetClampToZero) {
    rect r{0, 0, 10, 10};
    auto inset = r.inset(20, 20, 20, 20);
    // Inset larger than size: width and height should be clamped to 0
    EXPECT_EQ(inset.w, 0u);
    EXPECT_EQ(inset.h, 0u);
}

TEST(Rect, InsetExact) {
    rect r{0, 0, 20, 10};
    auto inset = r.inset(5, 10, 5, 10);
    EXPECT_EQ(inset.w, 0u);
    EXPECT_EQ(inset.h, 0u);
}

// ── measurement ─────────────────────────────────────────────────────────

TEST(Measurement, Default) {
    measurement m;
    EXPECT_EQ(m.width, 0u);
    EXPECT_EQ(m.height, 0u);
}

// ── box_constraints ─────────────────────────────────────────────────────

TEST(BoxConstraints, Default) {
    box_constraints bc;
    EXPECT_EQ(bc.min_w, 0u);
    EXPECT_EQ(bc.max_w, unbounded);
    EXPECT_EQ(bc.min_h, 0u);
    EXPECT_EQ(bc.max_h, unbounded);
}

TEST(BoxConstraints, Tight) {
    auto bc = box_constraints::tight(40, 20);
    EXPECT_EQ(bc.min_w, 40u);
    EXPECT_EQ(bc.max_w, 40u);
    EXPECT_EQ(bc.min_h, 20u);
    EXPECT_EQ(bc.max_h, 20u);
}

TEST(BoxConstraints, Loose) {
    auto bc = box_constraints::loose(100, 50);
    EXPECT_EQ(bc.min_w, 0u);
    EXPECT_EQ(bc.max_w, 100u);
    EXPECT_EQ(bc.min_h, 0u);
    EXPECT_EQ(bc.max_h, 50u);
}

TEST(BoxConstraints, ConstrainWithin) {
    auto bc = box_constraints::tight(50, 30);
    measurement m{50, 30};
    auto result = bc.constrain(m);
    EXPECT_EQ(result.width, 50u);
    EXPECT_EQ(result.height, 30u);
}

TEST(BoxConstraints, ConstrainClampUp) {
    box_constraints bc{20, 100, 10, 50};
    measurement m{5, 5};  // below minimum
    auto result = bc.constrain(m);
    EXPECT_EQ(result.width, 20u);
    EXPECT_EQ(result.height, 10u);
}

TEST(BoxConstraints, ConstrainClampDown) {
    box_constraints bc{0, 50, 0, 30};
    measurement m{100, 100};  // above maximum
    auto result = bc.constrain(m);
    EXPECT_EQ(result.width, 50u);
    EXPECT_EQ(result.height, 30u);
}

TEST(BoxConstraints, Equality) {
    auto a = box_constraints::tight(10, 20);
    auto b = box_constraints::tight(10, 20);
    auto c = box_constraints::tight(10, 30);
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

// ── border_style / border_chars ─────────────────────────────────────────

TEST(BorderChars, NoneIsSpaces) {
    auto bc = get_border_chars(border_style::none);
    EXPECT_EQ(bc.top_left, U' ');
    EXPECT_EQ(bc.horizontal, U' ');
    EXPECT_EQ(bc.vertical, U' ');
}

TEST(BorderChars, AsciiIsAscii) {
    auto bc = get_border_chars(border_style::ascii);
    EXPECT_EQ(bc.top_left, U'+');
    EXPECT_EQ(bc.horizontal, U'-');
    EXPECT_EQ(bc.vertical, U'|');
    EXPECT_EQ(bc.cross, U'+');
}

TEST(BorderChars, RoundedIsUnicode) {
    auto bc = get_border_chars(border_style::rounded);
    EXPECT_EQ(bc.top_left, U'\x256D');
    EXPECT_EQ(bc.horizontal, U'\x2500');
    EXPECT_EQ(bc.vertical, U'\x2502');
}

TEST(BorderChars, HeavyIsUnicode) {
    auto bc = get_border_chars(border_style::heavy);
    EXPECT_EQ(bc.top_left, U'\x250F');
    EXPECT_EQ(bc.horizontal, U'\x2501');
}

TEST(BorderChars, DoubleIsUnicode) {
    auto bc = get_border_chars(border_style::double_);
    EXPECT_EQ(bc.top_left, U'\x2554');
    EXPECT_EQ(bc.horizontal, U'\x2550');
}

// ── unbounded constant ──────────────────────────────────────────────────

TEST(Unbounded, IsMaxUint32) {
    EXPECT_EQ(unbounded, std::numeric_limits<uint32_t>::max());
}
