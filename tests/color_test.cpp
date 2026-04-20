#include "tapioca/color.h"

#include <cmath>
#include <cstdint>
#include <gtest/gtest.h>

using namespace tapioca;

// ── Gradient ────────────────────────────────────────────────────────────

TEST(Gradient, HorizontalLeftEdge) {
    linear_gradient g{color::from_rgb(0, 0, 0), color::from_rgb(255, 255, 255), gradient_direction::horizontal};
    rect area{0, 0, 100, 50};
    auto c = resolve_gradient(g, 0, 25, area);
    EXPECT_EQ(c.r, 0);
    EXPECT_EQ(c.g, 0);
    EXPECT_EQ(c.b, 0);
}

TEST(Gradient, HorizontalRightEdge) {
    linear_gradient g{color::from_rgb(0, 0, 0), color::from_rgb(255, 255, 255), gradient_direction::horizontal};
    rect area{0, 0, 100, 50};
    auto c = resolve_gradient(g, 99, 25, area);
    EXPECT_EQ(c.r, 255);
    EXPECT_EQ(c.g, 255);
    EXPECT_EQ(c.b, 255);
}

TEST(Gradient, HorizontalMidpoint) {
    linear_gradient g{color::from_rgb(0, 0, 0), color::from_rgb(254, 254, 254), gradient_direction::horizontal};
    rect area{0, 0, 101, 50}; // span = 100, midpoint at x=50
    auto c = resolve_gradient(g, 50, 25, area);
    EXPECT_NEAR(c.r, 127, 1);
    EXPECT_NEAR(c.g, 127, 1);
    EXPECT_NEAR(c.b, 127, 1);
}

TEST(Gradient, VerticalTopEdge) {
    linear_gradient g{color::from_rgb(100, 0, 0), color::from_rgb(200, 0, 0), gradient_direction::vertical};
    rect area{0, 0, 50, 100};
    auto c = resolve_gradient(g, 25, 0, area);
    EXPECT_EQ(c.r, 100);
}

TEST(Gradient, VerticalBottomEdge) {
    linear_gradient g{color::from_rgb(100, 0, 0), color::from_rgb(200, 0, 0), gradient_direction::vertical};
    rect area{0, 0, 50, 100};
    auto c = resolve_gradient(g, 25, 99, area);
    EXPECT_EQ(c.r, 200);
}

TEST(Gradient, ZeroArea) {
    linear_gradient g{color::from_rgb(42, 42, 42), color::from_rgb(200, 200, 200)};
    auto c = resolve_gradient(g, 0, 0, {0, 0, 0, 0});
    EXPECT_EQ(c.r, 42);
}

TEST(Gradient, NonRgbFallback) {
    linear_gradient g{color::from_index_16(1), color::from_rgb(200, 200, 200)};
    auto c = resolve_gradient(g, 50, 0, {0, 0, 100, 100});
    // Non-RGB start returns start as-is
    EXPECT_EQ(c, g.start);
}

// ── HSL <-> RGB ─────────────────────────────────────────────────────────

TEST(Hsl, PureRed) {
    auto h = rgb_to_hsl(255, 0, 0);
    EXPECT_NEAR(h.h, 0.0f, 0.01f);
    EXPECT_NEAR(h.s, 1.0f, 0.01f);
    EXPECT_NEAR(h.l, 0.5f, 0.01f);
}

TEST(Hsl, PureGreen) {
    auto h = rgb_to_hsl(0, 255, 0);
    EXPECT_NEAR(h.h, 1.0f / 3.0f, 0.01f);
    EXPECT_NEAR(h.s, 1.0f, 0.01f);
    EXPECT_NEAR(h.l, 0.5f, 0.01f);
}

TEST(Hsl, PureBlue) {
    auto h = rgb_to_hsl(0, 0, 255);
    EXPECT_NEAR(h.h, 2.0f / 3.0f, 0.01f);
    EXPECT_NEAR(h.s, 1.0f, 0.01f);
    EXPECT_NEAR(h.l, 0.5f, 0.01f);
}

TEST(Hsl, White) {
    auto h = rgb_to_hsl(255, 255, 255);
    EXPECT_NEAR(h.s, 0.0f, 0.01f);
    EXPECT_NEAR(h.l, 1.0f, 0.01f);
}

TEST(Hsl, Black) {
    auto h = rgb_to_hsl(0, 0, 0);
    EXPECT_NEAR(h.s, 0.0f, 0.01f);
    EXPECT_NEAR(h.l, 0.0f, 0.01f);
}

TEST(Hsl, Gray) {
    auto h = rgb_to_hsl(128, 128, 128);
    EXPECT_NEAR(h.s, 0.0f, 0.01f);
    EXPECT_NEAR(h.l, 128.0f / 255.0f, 0.01f);
}

TEST(Hsl, RoundTrip) {
    auto original = color::from_rgb(173, 42, 200);
    auto h = rgb_to_hsl(original.r, original.g, original.b);
    auto recovered = hsl_to_rgb(h);
    EXPECT_NEAR(recovered.r, original.r, 1);
    EXPECT_NEAR(recovered.g, original.g, 1);
    EXPECT_NEAR(recovered.b, original.b, 1);
}

TEST(Hsl, AchromaticToRgb) {
    // saturation ~0 should give gray
    hsl gray{0.0f, 0.0f, 0.5f};
    auto c = hsl_to_rgb(gray);
    EXPECT_EQ(c.kind, color_kind::rgb);
    EXPECT_NEAR(c.r, 128, 1);
    EXPECT_NEAR(c.g, 128, 1);
    EXPECT_NEAR(c.b, 128, 1);
}

// ── OKLCH <-> RGB ───────────────────────────────────────────────────────

TEST(Oklch, BlackRoundTrip) {
    auto o = rgb_to_oklch(0, 0, 0);
    EXPECT_NEAR(o.L, 0.0f, 0.01f);
    EXPECT_NEAR(o.C, 0.0f, 0.01f);
    auto c = oklch_to_rgb(o);
    EXPECT_NEAR(c.r, 0, 1);
    EXPECT_NEAR(c.g, 0, 1);
    EXPECT_NEAR(c.b, 0, 1);
}

TEST(Oklch, WhiteRoundTrip) {
    auto o = rgb_to_oklch(255, 255, 255);
    EXPECT_NEAR(o.L, 1.0f, 0.02f);
    EXPECT_NEAR(o.C, 0.0f, 0.01f);
    auto c = oklch_to_rgb(o);
    EXPECT_NEAR(c.r, 255, 2);
    EXPECT_NEAR(c.g, 255, 2);
    EXPECT_NEAR(c.b, 255, 2);
}

TEST(Oklch, PureRedRoundTrip) {
    auto o = rgb_to_oklch(255, 0, 0);
    EXPECT_GT(o.L, 0.4f);
    EXPECT_GT(o.C, 0.1f);
    auto c = oklch_to_rgb(o);
    EXPECT_NEAR(c.r, 255, 3);
    EXPECT_NEAR(c.g, 0, 3);
    EXPECT_NEAR(c.b, 0, 3);
}

TEST(Oklch, ArbitraryRoundTrip) {
    auto o = rgb_to_oklch(100, 150, 200);
    auto c = oklch_to_rgb(o);
    EXPECT_NEAR(c.r, 100, 3);
    EXPECT_NEAR(c.g, 150, 3);
    EXPECT_NEAR(c.b, 200, 3);
}

// ── WCAG Contrast ───────────────────────────────────────────────────────

TEST(Contrast, BlackVsWhite) {
    auto ratio = contrast_ratio(color::from_rgb(0, 0, 0), color::from_rgb(255, 255, 255));
    EXPECT_NEAR(ratio, 21.0f, 0.1f);
}

TEST(Contrast, SameColor) {
    auto ratio = contrast_ratio(color::from_rgb(128, 128, 128), color::from_rgb(128, 128, 128));
    EXPECT_NEAR(ratio, 1.0f, 0.01f);
}

TEST(Contrast, OrderIndependent) {
    auto r1 = contrast_ratio(color::from_rgb(0, 0, 0), color::from_rgb(255, 255, 255));
    auto r2 = contrast_ratio(color::from_rgb(255, 255, 255), color::from_rgb(0, 0, 0));
    EXPECT_FLOAT_EQ(r1, r2);
}

TEST(Luminance, Black) {
    auto l = relative_luminance(color::from_rgb(0, 0, 0));
    EXPECT_NEAR(l, 0.0f, 0.001f);
}

TEST(Luminance, White) {
    auto l = relative_luminance(color::from_rgb(255, 255, 255));
    EXPECT_NEAR(l, 1.0f, 0.01f);
}

TEST(Luminance, NonRgbIsZero) {
    auto l = relative_luminance(color::from_index_16(5));
    EXPECT_FLOAT_EQ(l, 0.0f);
}

// ── Perceptual blending ─────────────────────────────────────────────────

TEST(BlendOklch, ZeroFactor) {
    auto a = color::from_rgb(255, 0, 0);
    auto b = color::from_rgb(0, 0, 255);
    auto c = blend_oklch(a, b, 0.0f);
    EXPECT_EQ(c, a);
}

TEST(BlendOklch, OneFactor) {
    auto a = color::from_rgb(255, 0, 0);
    auto b = color::from_rgb(0, 0, 255);
    auto c = blend_oklch(a, b, 1.0f);
    EXPECT_EQ(c, b);
}

TEST(BlendOklch, HalfBlend) {
    auto a = color::from_rgb(255, 0, 0);
    auto b = color::from_rgb(0, 0, 255);
    auto c = blend_oklch(a, b, 0.5f);
    // Result should be somewhere between a and b in OKLCH space
    EXPECT_EQ(c.kind, color_kind::rgb);
    // Not pure red or pure blue
    EXPECT_NE(c, a);
    EXPECT_NE(c, b);
}

TEST(BlendOklch, NonRgbFallback) {
    auto a = color::from_index_16(1);
    auto b = color::from_rgb(0, 255, 0);
    auto c = blend_oklch(a, b, 0.5f);
    EXPECT_EQ(c, a); // non-RGB returns a
}

TEST(BlendOklch, SameColor) {
    auto a = color::from_rgb(100, 150, 200);
    auto c = blend_oklch(a, a, 0.5f);
    EXPECT_NEAR(c.r, a.r, 2);
    EXPECT_NEAR(c.g, a.g, 2);
    EXPECT_NEAR(c.b, a.b, 2);
}
