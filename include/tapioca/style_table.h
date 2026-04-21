#pragma once

/**
 * @file style_table.h
 * @brief Pooled style deduplication table mapping unique styles to compact 16-bit IDs.
 */

#include "tapioca/style.h"

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace tapioca {

/** @brief Compact style identifier (16-bit index into style_table). */
using style_id = uint16_t;

/** @brief The default style is always at index 0. */
inline constexpr style_id default_style_id = 0;

/**
 * @brief Interning table that maps unique style values to compact 16-bit IDs.
 *
 * Guarantees: default style always at index 0. Max 65535 unique styles.
 */
class style_table {
  public:
    style_table() {
        // Ensure default style is at index 0.
        styles_.push_back(style{});
        map_[style{}] = 0;
    }

    /** @brief Intern a style, returning its ID. Returns existing ID if already present. */
    [[nodiscard]] style_id intern(const style &s) {
        auto it = map_.find(s);
        if (it != map_.end()) {
            return it->second;
        }
        auto id = static_cast<style_id>(styles_.size());
        assert(styles_.size() < 65535 && "style_table overflow");
        styles_.push_back(s);
        map_[s] = id;
        return id;
    }

    /** @brief Look up a style by ID. */
    [[nodiscard]] const style &lookup(style_id id) const {
        assert(id < styles_.size());
        return styles_[id];
    }

    /** @brief Number of unique styles. */
    [[nodiscard]] size_t size() const noexcept { return styles_.size(); }

    /** @brief Clear all styles except the default. */
    void clear() {
        styles_.resize(1);
        map_.clear();
        map_[style{}] = 0;
    }

  private:
    std::vector<style> styles_;
    std::unordered_map<style, style_id> map_;
};

} // namespace tapioca
