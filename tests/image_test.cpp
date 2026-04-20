#include "tapioca/image.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace tapioca;

// ── encode_sixel ────────────────────────────────────────────────────────

TEST(EncodeSixel, NullPixels) {
    image_encode_options opts{.width = 10, .height = 10};
    auto result = encode_sixel(nullptr, opts);
    EXPECT_TRUE(result.empty());
}

TEST(EncodeSixel, ZeroWidth) {
    std::vector<uint8_t> pixels(40, 0xFF);
    image_encode_options opts{.width = 0, .height = 10};
    auto result = encode_sixel(pixels.data(), opts);
    EXPECT_TRUE(result.empty());
}

TEST(EncodeSixel, ZeroHeight) {
    std::vector<uint8_t> pixels(40, 0xFF);
    image_encode_options opts{.width = 10, .height = 0};
    auto result = encode_sixel(pixels.data(), opts);
    EXPECT_TRUE(result.empty());
}

TEST(EncodeSixel, ValidPixels) {
    // 2x2 RGBA pixels
    std::vector<uint8_t> pixels(2 * 2 * 4, 0xFF);
    image_encode_options opts{.width = 2, .height = 2};
    auto result = encode_sixel(pixels.data(), opts);
    EXPECT_FALSE(result.empty());
    // Should start with DCS q
    EXPECT_NE(result.find("\033Pq"), std::string::npos);
    // Should end with ST
    EXPECT_NE(result.find("\033\\"), std::string::npos);
    // Should contain dimensions
    EXPECT_NE(result.find("2"), std::string::npos);
}

// ── encode_kitty ────────────────────────────────────────────────────────

TEST(EncodeKitty, NullPixels) {
    image_encode_options opts{.width = 10, .height = 10};
    auto result = encode_kitty(nullptr, opts);
    EXPECT_TRUE(result.empty());
}

TEST(EncodeKitty, ZeroWidth) {
    std::vector<uint8_t> pixels(40, 0xFF);
    image_encode_options opts{.width = 0, .height = 10};
    auto result = encode_kitty(pixels.data(), opts);
    EXPECT_TRUE(result.empty());
}

TEST(EncodeKitty, ZeroHeight) {
    std::vector<uint8_t> pixels(40, 0xFF);
    image_encode_options opts{.width = 10, .height = 0};
    auto result = encode_kitty(pixels.data(), opts);
    EXPECT_TRUE(result.empty());
}

TEST(EncodeKitty, ValidPixels) {
    std::vector<uint8_t> pixels(2 * 2 * 4, 0xFF);
    image_encode_options opts{.width = 2, .height = 2};
    auto result = encode_kitty(pixels.data(), opts);
    EXPECT_FALSE(result.empty());
    // Should start with APC Kitty sequence
    EXPECT_NE(result.find("\033_G"), std::string::npos);
    // Should contain dimensions
    EXPECT_NE(result.find("s=2"), std::string::npos);
    EXPECT_NE(result.find("v=2"), std::string::npos);
}

// ── encode_iterm2 ───────────────────────────────────────────────────────

TEST(EncodeIterm2, NullPixels) {
    image_encode_options opts{.width = 10, .height = 10};
    auto result = encode_iterm2(nullptr, opts);
    EXPECT_TRUE(result.empty());
}

TEST(EncodeIterm2, ZeroWidth) {
    std::vector<uint8_t> pixels(40, 0xFF);
    image_encode_options opts{.width = 0, .height = 10};
    auto result = encode_iterm2(pixels.data(), opts);
    EXPECT_TRUE(result.empty());
}

TEST(EncodeIterm2, ZeroHeight) {
    std::vector<uint8_t> pixels(40, 0xFF);
    image_encode_options opts{.width = 10, .height = 0};
    auto result = encode_iterm2(pixels.data(), opts);
    EXPECT_TRUE(result.empty());
}

TEST(EncodeIterm2, ValidPixels) {
    std::vector<uint8_t> pixels(2 * 2 * 4, 0xFF);
    image_encode_options opts{.width = 2, .height = 2};
    auto result = encode_iterm2(pixels.data(), opts);
    EXPECT_FALSE(result.empty());
    // Should contain OSC 1337
    EXPECT_NE(result.find("\033]1337;File=inline=1"), std::string::npos);
    // Should end with BEL
    EXPECT_NE(result.find("\007"), std::string::npos);
}

TEST(EncodeIterm2, WithCellDimensions) {
    std::vector<uint8_t> pixels(4 * 4 * 4, 0xFF);
    image_encode_options opts{.width = 4, .height = 4, .cell_width = 10, .cell_height = 5, .preserve_aspect = false};
    auto result = encode_iterm2(pixels.data(), opts);
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("width=10"), std::string::npos);
    EXPECT_NE(result.find("height=5"), std::string::npos);
    EXPECT_NE(result.find("preserveAspectRatio=0"), std::string::npos);
}

TEST(EncodeIterm2, PreserveAspectDefault) {
    std::vector<uint8_t> pixels(2 * 2 * 4, 0xFF);
    image_encode_options opts{.width = 2, .height = 2};
    auto result = encode_iterm2(pixels.data(), opts);
    EXPECT_NE(result.find("preserveAspectRatio=1"), std::string::npos);
}

// ── encode_image dispatch ───────────────────────────────────────────────

TEST(EncodeImage, DispatchNone) {
    std::vector<uint8_t> pixels(16, 0xFF);
    image_encode_options opts{.width = 2, .height = 2};
    auto result = encode_image(image_protocol::none, pixels.data(), opts);
    EXPECT_TRUE(result.empty());
}

TEST(EncodeImage, DispatchSixel) {
    std::vector<uint8_t> pixels(16, 0xFF);
    image_encode_options opts{.width = 2, .height = 2};
    auto result = encode_image(image_protocol::sixel, pixels.data(), opts);
    EXPECT_NE(result.find("\033Pq"), std::string::npos);
}

TEST(EncodeImage, DispatchKitty) {
    std::vector<uint8_t> pixels(16, 0xFF);
    image_encode_options opts{.width = 2, .height = 2};
    auto result = encode_image(image_protocol::kitty, pixels.data(), opts);
    EXPECT_NE(result.find("\033_G"), std::string::npos);
}

TEST(EncodeImage, DispatchIterm2) {
    std::vector<uint8_t> pixels(16, 0xFF);
    image_encode_options opts{.width = 2, .height = 2};
    auto result = encode_image(image_protocol::iterm2, pixels.data(), opts);
    EXPECT_NE(result.find("\033]1337"), std::string::npos);
}

// ── detect_image_protocol ───────────────────────────────────────────────

TEST(DetectImageProtocol, ReturnsValidProtocol) {
    // We can't control env vars easily, but we can verify it doesn't crash
    // and returns a valid enum value
    auto proto = detect_image_protocol();
    EXPECT_GE(static_cast<int>(proto), 0);
    EXPECT_LE(static_cast<int>(proto), 3);
}

// ── image_encode_options defaults ───────────────────────────────────────

TEST(ImageEncodeOptions, Defaults) {
    image_encode_options opts{};
    EXPECT_EQ(opts.width, 0u);
    EXPECT_EQ(opts.height, 0u);
    EXPECT_EQ(opts.format, pixel_format::rgba8);
    EXPECT_EQ(opts.cell_width, 0u);
    EXPECT_EQ(opts.cell_height, 0u);
    EXPECT_TRUE(opts.preserve_aspect);
}

// ── pixel_format enum ───────────────────────────────────────────────────

TEST(PixelFormat, Values) {
    EXPECT_EQ(static_cast<uint8_t>(pixel_format::rgba8), 0);
    EXPECT_EQ(static_cast<uint8_t>(pixel_format::rgb8), 1);
}

// ── image_protocol enum ─────────────────────────────────────────────────

TEST(ImageProtocol, Values) {
    EXPECT_EQ(static_cast<uint8_t>(image_protocol::none), 0);
    EXPECT_EQ(static_cast<uint8_t>(image_protocol::sixel), 1);
    EXPECT_EQ(static_cast<uint8_t>(image_protocol::kitty), 2);
    EXPECT_EQ(static_cast<uint8_t>(image_protocol::iterm2), 3);
}
