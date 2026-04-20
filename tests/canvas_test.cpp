#include "tapioca/canvas.h"

#include <cstdint>
#include <gtest/gtest.h>
#include <vector>

using namespace tapioca;

// ── Construction ────────────────────────────────────────────────────────

TEST(Canvas, DefaultEmpty) {
    canvas c;
    EXPECT_EQ(c.width(), 0u);
    EXPECT_EQ(c.height(), 0u);
    EXPECT_EQ(c.cell_count(), 0u);
}

TEST(Canvas, ConstructWithSize) {
    canvas c(80, 24);
    EXPECT_EQ(c.width(), 80u);
    EXPECT_EQ(c.height(), 24u);
    EXPECT_EQ(c.cell_count(), 80u * 24u);
}

TEST(Canvas, DefaultCellIsSpace) {
    canvas c(10, 10);
    auto &cl = c.get(0, 0);
    EXPECT_EQ(cl.codepoint, U' ');
    EXPECT_EQ(cl.sid, default_style_id);
    EXPECT_EQ(cl.width, 1);
    EXPECT_EQ(cl.alpha, 255);
}

// ── Set / Get ───────────────────────────────────────────────────────────

TEST(Canvas, SetAndGet) {
    canvas c(10, 10);
    cell ch{U'A', 0, 1, 255};
    c.set(5, 3, ch);
    auto &result = c.get(5, 3);
    EXPECT_EQ(result.codepoint, U'A');
}

TEST(Canvas, SetTransparentSkipped) {
    canvas c(10, 10);
    cell original = c.get(5, 3);
    cell transparent{U'X', 0, 1, 0}; // alpha=0 -> transparent
    c.set(5, 3, transparent);
    // Should be unchanged since alpha=0 is skipped
    EXPECT_EQ(c.get(5, 3), original);
}

TEST(Canvas, GetCurrent) {
    canvas c(10, 10);
    // Initially current buffer is all default cells
    auto &cl = c.get_current(0, 0);
    EXPECT_EQ(cl.codepoint, U' ');
}

// ── Diff ────────────────────────────────────────────────────────────────

TEST(Canvas, DiffCountInitiallyAllChanged) {
    canvas c(10, 10);
    // Both buffers are identical at construction
    EXPECT_EQ(c.diff_count(), 0u);
}

TEST(Canvas, DiffCountAfterSet) {
    canvas c(10, 10);
    c.set(0, 0, {U'A', 0, 1, 255});
    c.set(1, 0, {U'B', 0, 1, 255});
    EXPECT_EQ(c.diff_count(), 2u);
}

TEST(Canvas, DiffCallback) {
    canvas c(5, 5);
    c.set(2, 3, {U'X', 0, 1, 255});

    std::vector<std::pair<uint32_t, uint32_t>> changed;
    c.diff([&](uint32_t x, uint32_t y, const cell &) { changed.emplace_back(x, y); });

    ASSERT_EQ(changed.size(), 1u);
    EXPECT_EQ(changed[0].first, 2u);
    EXPECT_EQ(changed[0].second, 3u);
}

// ── Swap ────────────────────────────────────────────────────────────────

TEST(Canvas, SwapPromotesNextToCurrent) {
    canvas c(10, 10);
    c.set(0, 0, {U'Z', 0, 1, 255});
    EXPECT_NE(c.get_current(0, 0).codepoint, U'Z');

    c.swap();
    EXPECT_EQ(c.get_current(0, 0).codepoint, U'Z');
}

TEST(Canvas, SwapThenDiffIsZero) {
    canvas c(10, 10);
    c.set(0, 0, {U'Z', 0, 1, 255});
    c.swap();
    // After swap, next still has the old next buffer (now current's old), but next wasn't cleared
    // Both buffers should have Z at (0,0) since swap just swaps pointers
    // The next buffer is now the old current, which was default
    EXPECT_EQ(c.diff_count(), 1u); // Z is in current but not in next (which is old current)
}

// ── Clear ───────────────────────────────────────────────────────────────

TEST(Canvas, ClearNext) {
    canvas c(10, 10);
    c.set(0, 0, {U'Z', 0, 1, 255});
    c.clear_next();
    EXPECT_EQ(c.get(0, 0).codepoint, U' ');
    EXPECT_EQ(c.diff_count(), 0u);
}

// ── Resize ──────────────────────────────────────────────────────────────

TEST(Canvas, Resize) {
    canvas c(10, 10);
    c.set(0, 0, {U'A', 0, 1, 255});
    c.resize(20, 5);
    EXPECT_EQ(c.width(), 20u);
    EXPECT_EQ(c.height(), 5u);
    EXPECT_EQ(c.cell_count(), 100u);
    // After resize, both buffers are cleared
    EXPECT_EQ(c.get(0, 0).codepoint, U' ');
}

// ── Copy current to next ────────────────────────────────────────────────

TEST(Canvas, CopyCurrentToNext) {
    canvas c(10, 10);
    c.set(5, 5, {U'Q', 0, 1, 255});
    c.swap(); // now current has Q at (5,5), next has old current (default)

    c.copy_current_to_next();
    // Now next should match current
    EXPECT_EQ(c.diff_count(), 0u);
    EXPECT_EQ(c.get(5, 5).codepoint, U'Q');
}

// ── Invalidate ──────────────────────────────────────────────────────────

TEST(Canvas, InvalidateForcesFullRedraw) {
    canvas c(10, 10);
    c.swap(); // both buffers are default
    EXPECT_EQ(c.diff_count(), 0u);

    c.invalidate_current();
    // Current is now filled with sentinel cells, next is default -> all differ
    EXPECT_EQ(c.diff_count(), 100u);
}

// ── Buffer access ───────────────────────────────────────────────────────

TEST(Canvas, NextBuffer) {
    canvas c(5, 5);
    EXPECT_EQ(c.next_buffer().size(), 25u);
}

TEST(Canvas, CurrentBuffer) {
    canvas c(5, 5);
    EXPECT_EQ(c.current_buffer().size(), 25u);
}

// ── cell struct ─────────────────────────────────────────────────────────

TEST(Cell, SizeOf) {
    static_assert(sizeof(cell) == 8);
}

TEST(Cell, DefaultValues) {
    cell c;
    EXPECT_EQ(c.codepoint, U' ');
    EXPECT_EQ(c.sid, default_style_id);
    EXPECT_EQ(c.width, 1);
    EXPECT_EQ(c.alpha, 255);
}

TEST(Cell, Equality) {
    cell a{U'X', 1, 2, 128};
    cell b{U'X', 1, 2, 128};
    cell c{U'Y', 1, 2, 128};
    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}
