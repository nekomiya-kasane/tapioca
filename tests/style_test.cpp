#include "tapioca/style.h"
#include "tapioca/style_table.h"

#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>

using namespace tapioca;

// ── color ───────────────────────────────────────────────────────────────

TEST(Color, DefaultColor) {
    auto c = color::default_color();
    EXPECT_EQ(c.kind, color_kind::default_color);
    EXPECT_TRUE(c.is_default());
}

TEST(Color, FromRgb) {
    auto c = color::from_rgb(255, 128, 0);
    EXPECT_EQ(c.kind, color_kind::rgb);
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 128);
    EXPECT_EQ(c.b, 0);
    EXPECT_FALSE(c.is_default());
}

TEST(Color, FromIndex16) {
    auto c = color::from_index_16(5);
    EXPECT_EQ(c.kind, color_kind::indexed_16);
    EXPECT_EQ(c.r, 5);
}

TEST(Color, FromIndex256) {
    auto c = color::from_index_256(200);
    EXPECT_EQ(c.kind, color_kind::indexed_256);
    EXPECT_EQ(c.r, 200);
}

TEST(Color, SizeOf) {
    static_assert(sizeof(color) == 4);
}

TEST(Color, Equality) {
    EXPECT_EQ(color::from_rgb(1, 2, 3), color::from_rgb(1, 2, 3));
    EXPECT_NE(color::from_rgb(1, 2, 3), color::from_rgb(1, 2, 4));
    EXPECT_NE(color::from_rgb(1, 2, 3), color::from_index_16(1));
    EXPECT_EQ(color::default_color(), color::default_color());
}

TEST(Color, NamedColors) {
    EXPECT_EQ(colors::red.kind, color_kind::indexed_16);
    EXPECT_EQ(colors::red.r, 1);
    EXPECT_EQ(colors::bright_red.r, 9);
    EXPECT_EQ(colors::white.r, 7);
    EXPECT_EQ(colors::black.r, 0);
}

// ── attr ────────────────────────────────────────────────────────────────

TEST(Attr, BitwiseOr) {
    auto a = attr::bold | attr::italic;
    EXPECT_TRUE(has_attr(a, attr::bold));
    EXPECT_TRUE(has_attr(a, attr::italic));
    EXPECT_FALSE(has_attr(a, attr::underline));
}

TEST(Attr, BitwiseAnd) {
    auto a = attr::bold | attr::italic | attr::underline;
    auto b = a & attr::bold;
    EXPECT_TRUE(has_attr(b, attr::bold));
    EXPECT_FALSE(has_attr(b, attr::italic));
}

TEST(Attr, BitwiseNot) {
    auto a = attr::bold | attr::italic;
    auto b = ~a;
    EXPECT_FALSE(has_attr(b, attr::bold));
    EXPECT_FALSE(has_attr(b, attr::italic));
    EXPECT_TRUE(has_attr(b, attr::underline));
}

TEST(Attr, OrEquals) {
    auto a = attr::none;
    a |= attr::bold;
    a |= attr::strike;
    EXPECT_TRUE(has_attr(a, attr::bold));
    EXPECT_TRUE(has_attr(a, attr::strike));
}

TEST(Attr, AndEquals) {
    auto a = attr::bold | attr::italic | attr::underline;
    a &= attr::bold | attr::underline;
    EXPECT_TRUE(has_attr(a, attr::bold));
    EXPECT_FALSE(has_attr(a, attr::italic));
    EXPECT_TRUE(has_attr(a, attr::underline));
}

TEST(Attr, NoneHasNothing) {
    EXPECT_FALSE(has_attr(attr::none, attr::bold));
    EXPECT_FALSE(has_attr(attr::none, attr::italic));
}

// ── style ───────────────────────────────────────────────────────────────

TEST(Style, SizeOf) {
    static_assert(sizeof(style) == 10);
}

TEST(Style, DefaultIsDefault) {
    style s;
    EXPECT_TRUE(s.is_default());
    EXPECT_TRUE(s.fg.is_default());
    EXPECT_TRUE(s.bg.is_default());
    EXPECT_EQ(s.attrs, attr::none);
}

TEST(Style, NonDefault) {
    style s;
    s.fg = color::from_rgb(255, 0, 0);
    EXPECT_FALSE(s.is_default());
}

TEST(Style, Equality) {
    style a, b;
    EXPECT_EQ(a, b);
    a.fg = color::from_rgb(1, 2, 3);
    EXPECT_NE(a, b);
    b.fg = color::from_rgb(1, 2, 3);
    EXPECT_EQ(a, b);
}

// ── style_table ─────────────────────────────────────────────────────────

TEST(StyleTable, DefaultStyleAtZero) {
    style_table tbl;
    EXPECT_EQ(tbl.size(), 1u);
    auto s = tbl.lookup(default_style_id);
    EXPECT_TRUE(s.is_default());
}

TEST(StyleTable, InternDedup) {
    style_table tbl;
    style s;
    s.fg = color::from_rgb(255, 0, 0);
    s.attrs = attr::bold;

    auto id1 = tbl.intern(s);
    auto id2 = tbl.intern(s);
    EXPECT_EQ(id1, id2);
    EXPECT_EQ(tbl.size(), 2u); // default + one interned
}

TEST(StyleTable, InternUnique) {
    style_table tbl;

    style a;
    a.fg = color::from_rgb(255, 0, 0);
    style b;
    b.fg = color::from_rgb(0, 255, 0);
    style c;
    c.fg = color::from_rgb(0, 0, 255);

    auto ida = tbl.intern(a);
    auto idb = tbl.intern(b);
    auto idc = tbl.intern(c);

    EXPECT_NE(ida, idb);
    EXPECT_NE(idb, idc);
    EXPECT_EQ(tbl.size(), 4u); // default + 3
}

TEST(StyleTable, Lookup) {
    style_table tbl;
    style s;
    s.fg = color::from_rgb(10, 20, 30);
    s.bg = color::from_rgb(40, 50, 60);
    s.attrs = attr::italic;

    auto id = tbl.intern(s);
    auto &recovered = tbl.lookup(id);
    EXPECT_EQ(recovered, s);
}

TEST(StyleTable, Clear) {
    style_table tbl;
    style s;
    s.fg = color::from_rgb(1, 2, 3);
    tbl.intern(s);
    EXPECT_EQ(tbl.size(), 2u);

    tbl.clear();
    EXPECT_EQ(tbl.size(), 1u); // only default remains
    EXPECT_TRUE(tbl.lookup(0).is_default());
}

TEST(StyleTable, InternDefault) {
    style_table tbl;
    auto id = tbl.intern(style{});
    EXPECT_EQ(id, default_style_id);
    EXPECT_EQ(tbl.size(), 1u);
}

// ── std::hash specializations ───────────────────────────────────────────

TEST(ColorHash, DifferentColorsHash) {
    std::hash<color> h;
    auto a = h(color::from_rgb(255, 0, 0));
    auto b = h(color::from_rgb(0, 255, 0));
    // Not guaranteed but extremely likely to differ
    EXPECT_NE(a, b);
}

TEST(StyleHash, Deterministic) {
    std::hash<style> h;
    style s;
    s.fg = color::from_rgb(10, 20, 30);
    s.attrs = attr::bold;
    EXPECT_EQ(h(s), h(s));
}
