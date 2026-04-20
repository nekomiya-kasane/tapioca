#pragma once

/**
 * @file canvas.h
 * @brief Double-buffered character grid for terminal rendering.
 */

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <vector>

#include "tapioca/cell.h"
#include "tapioca/style.h"
#include "tapioca/style_table.h"

namespace tapioca {

/**
 * @brief A flat 2D grid of cells with double-buffering for diff-based rendering.
 *
 * Maintains two buffers: `current` (what the terminal currently shows) and
 * `next` (what we want to show). After rendering, call `swap()` to promote
 * `next` to `current`.
 */
class canvas {
public:
    canvas() = default;

    /**
     * @brief Construct a canvas of the given dimensions.
     * Both buffers are initialized to default (space) cells.
     */
    canvas(uint32_t width, uint32_t height)
        : width_(width)
        , height_(height)
        , current_(static_cast<size_t>(width) * height)
        , next_(static_cast<size_t>(width) * height) {}

    /** @brief Resize the canvas, clearing both buffers. */
    void resize(uint32_t width, uint32_t height) {
        width_ = width;
        height_ = height;
        auto sz = static_cast<size_t>(width) * height;
        current_.assign(sz, cell{});
        next_.assign(sz, cell{});
    }

    [[nodiscard]] uint32_t width() const noexcept { return width_; }
    [[nodiscard]] uint32_t height() const noexcept { return height_; }
    [[nodiscard]] size_t   cell_count() const noexcept { return next_.size(); }

    // ── Next-buffer write access ────────────────────────────────────────

    /** @brief Write a cell into the next buffer at (x, y). Skips fully transparent cells. */
    void set(uint32_t x, uint32_t y, const cell& c) {
        assert(x < width_ && y < height_);
        if (c.alpha == 0) return;  // fully transparent -- skip
        next_[index(x, y)] = c;
    }

    /**
     * @brief Write a cell with alpha blending against the existing cell.
     *
     * For alpha < 255, blends fg/bg RGB colors with the existing cell's colors.
     * Only works for RGB colors; non-RGB colors fall back to simple overwrite.
     */
    void set_blended(uint32_t x, uint32_t y, const cell& c, style_table& styles) {
        assert(x < width_ && y < height_);
        if (c.alpha == 0) return;
        if (c.alpha == 255) { next_[index(x, y)] = c; return; }

        auto idx = index(x, y);
        const auto& dst = next_[idx];
        const auto& src_sty = styles.lookup(c.sid);
        const auto& dst_sty = styles.lookup(dst.sid);

        auto blend_color = [](color src, color dst, uint8_t a) -> color {
            if (src.kind != color_kind::rgb || dst.kind != color_kind::rgb) return src;
            uint16_t inv = 255 - a;
            return color::from_rgb(
                static_cast<uint8_t>((src.r * a + dst.r * inv) / 255),
                static_cast<uint8_t>((src.g * a + dst.g * inv) / 255),
                static_cast<uint8_t>((src.b * a + dst.b * inv) / 255)
            );
        };

        // When the source is a space, treat it as a tint overlay:
        // preserve the destination codepoint, width, fg -- only blend bg.
        const bool tint_only = (c.codepoint == U' ');

        style blended;
        blended.fg    = tint_only ? dst_sty.fg : blend_color(src_sty.fg, dst_sty.fg, c.alpha);
        blended.bg    = blend_color(src_sty.bg, dst_sty.bg, c.alpha);
        blended.attrs = tint_only ? dst_sty.attrs : src_sty.attrs;

        cell result;
        result.codepoint = tint_only ? dst.codepoint : c.codepoint;
        result.width     = tint_only ? dst.width     : c.width;
        result.sid       = styles.intern(blended);
        result.alpha     = 255;  // blended result is fully opaque
        next_[idx] = result;
    }

    /** @brief Read a cell from the next buffer at (x, y). */
    [[nodiscard]] const cell& get(uint32_t x, uint32_t y) const {
        assert(x < width_ && y < height_);
        return next_[index(x, y)];
    }

    /** @brief Clear the next buffer to default cells. */
    void clear_next() {
        std::fill(next_.begin(), next_.end(), cell{});
    }

    /** @brief Copy current buffer to next buffer (for dirty-rect optimization). */
    void copy_current_to_next() {
        next_ = current_;
    }

    // ── Current-buffer read access ──────────────────────────────────────

    /** @brief Read a cell from the current buffer at (x, y). */
    [[nodiscard]] const cell& get_current(uint32_t x, uint32_t y) const {
        assert(x < width_ && y < height_);
        return current_[index(x, y)];
    }

    // ── Diff & swap ─────────────────────────────────────────────────────

    /**
     * @brief Callback-based diff between current and next buffers.
     *
     * For each cell that differs, calls `fn(x, y, next_cell)`.
     * @tparam Fn callable with signature `void(uint32_t x, uint32_t y, const cell& c)`
     */
    template <typename Fn>
    void diff(Fn&& fn) const {
        for (uint32_t y = 0; y < height_; ++y) {
            for (uint32_t x = 0; x < width_; ++x) {
                auto idx = index(x, y);
                if (current_[idx] != next_[idx]) {
                    fn(x, y, next_[idx]);
                }
            }
        }
    }

    /**
     * @brief Count the number of cells that differ between current and next.
     */
    [[nodiscard]] size_t diff_count() const {
        size_t count = 0;
        for (size_t i = 0; i < next_.size(); ++i) {
            if (current_[i] != next_[i]) {
                ++count;
            }
        }
        return count;
    }

    /** @brief Promote next buffer to current via O(1) pointer swap. Next buffer is NOT cleared. */
    void swap() {
        std::swap(current_, next_);
    }

    /** @brief Invalidate current buffer so next diff treats every cell as changed (force full redraw). */
    void invalidate_current() {
        std::fill(current_.begin(), current_.end(), cell{U'\0', default_style_id, 0, 0});  // alpha=0 sentinel
    }

    /** @brief Direct read-only access to the next buffer. */
    [[nodiscard]] const std::vector<cell>& next_buffer() const noexcept { return next_; }

    /** @brief Direct read-only access to the current buffer. */
    [[nodiscard]] const std::vector<cell>& current_buffer() const noexcept { return current_; }

private:
    [[nodiscard]] size_t index(uint32_t x, uint32_t y) const noexcept {
        return static_cast<size_t>(y) * width_ + x;
    }

    uint32_t width_  = 0;
    uint32_t height_ = 0;
    std::vector<cell> current_;
    std::vector<cell> next_;
};

}  // namespace tapioca
