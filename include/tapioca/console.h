#pragma once

/**
 * @file console.h
 * @brief Basic styled terminal console (output infrastructure layer).
 *
 * Provides the output sink, configuration, and ANSI emission primitives
 * that higher-level consoles (e.g. tapiru::console with markup/widgets)
 * build upon.
 *
 * Usage:
 *   tapioca::basic_console con;
 *   con.write("plain text");
 *   con.styled_write(my_style, "styled text");
 *   con.newline();
 */

#include "tapioca/ansi_emitter.h"
#include "tapioca/exports.h"
#include "tapioca/pal.h"
#include "tapioca/terminal.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace tapioca {

    /**
     * @brief Console configuration.
     */
    struct console_config {
        output_sink sink;
        color_depth depth = color_depth::true_color;
        bool force_color = false;
        bool no_color = false;
    };

    /**
     * @brief Basic styled terminal console (tapioca layer).
     *
     * Manages an output sink, terminal capabilities, and an ANSI emitter.
     * Provides plain-text write, styled write, and color state queries.
     *
     * tapiru::console inherits from this to add markup parsing, widget
     * rendering, and highlighter support.
     */
    class TAPIOCA_API basic_console {
      public:
        /** @brief Construct with default stdout sink and auto-detected capabilities. */
        basic_console();

        /** @brief Construct with custom configuration. */
        explicit basic_console(console_config cfg);

        virtual ~basic_console() = default;

        // ── Plain output ────────────────────────────────────────────────────

        /** @brief Write plain text with no style or markup processing. */
        void write(std::string_view text);

        /** @brief Write a newline. */
        void newline();

        // ── Styled output ───────────────────────────────────────────────────

        /**
         * @brief Write text with a specific style applied.
         *
         * Transitions the emitter to the target style, writes text,
         * then resets. Does not append a newline.
         */
        void styled_write(const style &sty, std::string_view text);

        /**
         * @brief Emit a transition to the target style, write text (no reset).
         *
         * Useful for batching multiple styled writes before a final reset().
         */
        void emit_styled(const style &sty, std::string_view text, std::string &buf);

        /** @brief Reset the ANSI emitter and write the reset sequence. */
        void emit_reset(std::string &buf);

        /** @brief Flush the accumulated buffer to the sink. */
        void flush_to_sink(std::string_view buf);

        // ── State queries ───────────────────────────────────────────────────

        /** @brief Check if color output is currently enabled. */
        [[nodiscard]] bool color_enabled() const noexcept;

        /** @brief Get the current color depth setting. */
        [[nodiscard]] color_depth depth() const noexcept { return config_.depth; }

        /** @brief Get terminal width (or 80 as fallback). */
        [[nodiscard]] uint32_t term_width() const noexcept;

        /** @brief Access the ANSI emitter (for subclasses). */
        [[nodiscard]] ansi_emitter &emitter() noexcept { return emitter_; }
        [[nodiscard]] const ansi_emitter &emitter() const noexcept { return emitter_; }

        /** @brief Access the configuration. */
        [[nodiscard]] const console_config &config() const noexcept { return config_; }

      protected:
        console_config config_;
        ansi_emitter emitter_;
    };

} // namespace tapioca
