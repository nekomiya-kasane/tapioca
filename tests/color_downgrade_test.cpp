#include "tapioca/style.h"

#include <cstdint>
#include <gtest/gtest.h>

using namespace tapioca;

// ── downgrade: default color passthrough ────────────────────────────────

TEST(ColorDowngrade, DefaultColorUnchanged) {
    auto c = color::default_color();
    // Any target depth should leave default unchanged
    EXPECT_EQ(c.downgrade(0), color::default_color());
    EXPECT_EQ(c.downgrade(1), color::default_color());
    EXPECT_EQ(c.downgrade(2), color::default_color());
    EXPECT_EQ(c.downgrade(3), color::default_color());
}

// ── downgrade: no downgrade needed ──────────────────────────────────────

TEST(ColorDowngrade, NoDowngradeNeeded16To16) {
    auto c = color::from_index_16(5);
    auto d = c.downgrade(1); // target=basic_16, current=indexed_16
    EXPECT_EQ(d, c);
}

TEST(ColorDowngrade, NoDowngradeNeeded16ToHigher) {
    auto c = color::from_index_16(5);
    auto d = c.downgrade(2); // target=256, current=16 -- no downgrade
    EXPECT_EQ(d, c);
}

TEST(ColorDowngrade, NoDowngradeNeeded256To256) {
    auto c = color::from_index_256(200);
    auto d = c.downgrade(2);
    EXPECT_EQ(d, c);
}

TEST(ColorDowngrade, NoDowngradeNeededRgbToTrueColor) {
    auto c = color::from_rgb(255, 128, 0);
    auto d = c.downgrade(3); // target=true_color
    EXPECT_EQ(d, c);
}

// ── downgrade: strip to none ────────────────────────────────────────────

TEST(ColorDowngrade, RgbToNone) {
    auto c = color::from_rgb(255, 128, 0);
    auto d = c.downgrade(0);
    EXPECT_TRUE(d.is_default());
}

TEST(ColorDowngrade, Index256ToNone) {
    auto c = color::from_index_256(200);
    auto d = c.downgrade(0);
    EXPECT_TRUE(d.is_default());
}

TEST(ColorDowngrade, Index16ToNone) {
    auto c = color::from_index_16(5);
    auto d = c.downgrade(0);
    EXPECT_TRUE(d.is_default());
}

// ── downgrade: RGB -> 256 ───────────────────────────────────────────────

TEST(ColorDowngrade, RgbTo256PureRed) {
    auto c = color::from_rgb(255, 0, 0);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
    // Should map to a reasonable red in the 256 palette
    // The exact index depends on OKLCH distance, but should be valid
    EXPECT_GE(d.r, 0);
    EXPECT_LE(d.r, 255);
}

TEST(ColorDowngrade, RgbTo256PureGreen) {
    auto c = color::from_rgb(0, 255, 0);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngrade, RgbTo256PureBlue) {
    auto c = color::from_rgb(0, 0, 255);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngrade, RgbTo256White) {
    auto c = color::from_rgb(255, 255, 255);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngrade, RgbTo256Black) {
    auto c = color::from_rgb(0, 0, 0);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngrade, RgbTo256Gray) {
    // Gray values should prefer grayscale ramp
    auto c = color::from_rgb(128, 128, 128);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

// ── downgrade: RGB -> 16 ────────────────────────────────────────────────

TEST(ColorDowngrade, RgbTo16PureRed) {
    auto c = color::from_rgb(255, 0, 0);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    // Should map to one of the 16 ANSI colors
    EXPECT_LE(d.r, 15);
}

TEST(ColorDowngrade, RgbTo16PureGreen) {
    auto c = color::from_rgb(0, 255, 0);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    EXPECT_LE(d.r, 15);
}

TEST(ColorDowngrade, RgbTo16PureBlue) {
    auto c = color::from_rgb(0, 0, 255);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    EXPECT_LE(d.r, 15);
}

TEST(ColorDowngrade, RgbTo16White) {
    auto c = color::from_rgb(255, 255, 255);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    EXPECT_LE(d.r, 15);
}

TEST(ColorDowngrade, RgbTo16Black) {
    auto c = color::from_rgb(0, 0, 0);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    EXPECT_LE(d.r, 15);
}

// ── downgrade: 256 -> 16 ────────────────────────────────────────────────

TEST(ColorDowngrade, Index256BasicColorTo16) {
    // Index < 16 should stay as-is when downgrading to 16
    auto c = color::from_index_256(5);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    EXPECT_EQ(d.r, 5);
}

TEST(ColorDowngrade, Index256CubeColorTo16) {
    // Cube color (index >= 16, < 232) should be converted to nearest 16-color
    auto c = color::from_index_256(196); // bright red in cube
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    EXPECT_LE(d.r, 15);
}

TEST(ColorDowngrade, Index256GrayscaleTo16) {
    // Grayscale ramp (index >= 232) should be converted to nearest 16-color
    auto c = color::from_index_256(240);
    auto d = c.downgrade(1);
    EXPECT_EQ(d.kind, color_kind::indexed_16);
    EXPECT_LE(d.r, 15);
}

// ── downgrade: 256 -> 256 passthrough ───────────────────────────────────

TEST(ColorDowngrade, Index256SameDepthPassthrough) {
    auto c = color::from_index_256(100);
    auto d = c.downgrade(2);
    EXPECT_EQ(d, c);
}

// ── downgrade: 16 -> 16 passthrough ────────────────────────────────────

TEST(ColorDowngrade, Index16AlreadyMinimal) {
    auto c = color::from_index_16(7);
    auto d = c.downgrade(1);
    EXPECT_EQ(d, c);
}

// ── downgrade: OKLCH produces consistent results ────────────────────────

TEST(ColorDowngrade, RgbTo256Deterministic) {
    auto c = color::from_rgb(123, 45, 67);
    auto d1 = c.downgrade(2);
    auto d2 = c.downgrade(2);
    EXPECT_EQ(d1, d2);
}

TEST(ColorDowngrade, RgbTo16Deterministic) {
    auto c = color::from_rgb(123, 45, 67);
    auto d1 = c.downgrade(1);
    auto d2 = c.downgrade(1);
    EXPECT_EQ(d1, d2);
}

// ── downgrade: various cube zones (covers nearest_cube branches) ────────

TEST(ColorDowngrade, RgbTo256LowValues) {
    // r,g,b < 48 -- nearest_cube returns 0
    auto c = color::from_rgb(10, 20, 30);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngrade, RgbTo256MidLowValues) {
    // r,g,b around 80 -- nearest_cube returns 1
    auto c = color::from_rgb(80, 80, 80);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngrade, RgbTo256MidValues) {
    // r,g,b around 140 -- nearest_cube returns 2
    auto c = color::from_rgb(140, 140, 140);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngrade, RgbTo256MidHighValues) {
    // r,g,b around 180 -- nearest_cube returns 3
    auto c = color::from_rgb(180, 180, 180);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngrade, RgbTo256HighValues) {
    // r,g,b around 220 -- nearest_cube returns 4
    auto c = color::from_rgb(220, 220, 220);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

TEST(ColorDowngrade, RgbTo256MaxValues) {
    // r,g,b >= 235 -- nearest_cube returns 5
    auto c = color::from_rgb(240, 240, 240);
    auto d = c.downgrade(2);
    EXPECT_EQ(d.kind, color_kind::indexed_256);
}

// ── downgrade: mixed RGB values exercise neighbor search ────────────────

TEST(ColorDowngrade, RgbTo256MixedColors) {
    // Different r,g,b to exercise the 3D neighbor search
    auto c1 = color::from_rgb(200, 50, 100);
    auto c2 = color::from_rgb(50, 200, 100);
    auto c3 = color::from_rgb(100, 50, 200);
    auto d1 = c1.downgrade(2);
    auto d2 = c2.downgrade(2);
    auto d3 = c3.downgrade(2);
    EXPECT_EQ(d1.kind, color_kind::indexed_256);
    EXPECT_EQ(d2.kind, color_kind::indexed_256);
    EXPECT_EQ(d3.kind, color_kind::indexed_256);
    // Different inputs should generally give different outputs
    // (not guaranteed but very likely for these distinct colors)
}
