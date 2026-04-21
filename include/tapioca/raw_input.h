#pragma once

/**
 * @file raw_input.h
 * @brief Cross-platform raw terminal input: RAII raw mode and event polling.
 */

#include "tapioca/exports.h"
#include "tapioca/input.h"

#include <cstdint>
#include <memory>
#include <optional>

namespace tapioca {

    /**
     * @brief RAII guard that enables raw terminal mode and mouse tracking.
     *
     * On construction, switches the terminal to raw/unbuffered mode and enables
     * mouse button-event tracking (SGR 1002+1006 on POSIX, ENABLE_MOUSE_INPUT on
     * Win32).  The destructor restores the original terminal settings.
     *
     * Only one instance should be active at a time.
     */
    class TAPIOCA_API raw_mode {
      public:
        raw_mode();
        ~raw_mode();

        raw_mode(const raw_mode &) = delete;
        raw_mode &operator=(const raw_mode &) = delete;
        raw_mode(raw_mode &&) noexcept;
        raw_mode &operator=(raw_mode &&) noexcept;

      private:
        struct impl;
        std::unique_ptr<impl> impl_;
    };

    /**
     * @brief Poll for the next input event from the terminal.
     *
     * Blocks up to @p timeout_ms milliseconds waiting for input.
     * Returns std::nullopt on timeout (no event available).
     *
     * @note Requires an active raw_mode instance for correct operation.
     */
    [[nodiscard]] TAPIOCA_API std::optional<input_event> poll_event(int timeout_ms = 50);

} // namespace tapioca
