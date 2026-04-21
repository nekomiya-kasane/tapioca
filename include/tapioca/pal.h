#pragma once

/**
 * @file pal.h
 * @brief Platform Abstraction Layer for terminal I/O.
 *
 * Provides:
 * - output_sink type and factory functions (stdout, stderr, file, string buffer)
 * - Blocking write/flush primitives
 * - Clipboard access via OSC 52 (for terminals that support it)
 *
 * This is the lowest-level I/O layer in tapioca. Higher-level constructs
 * (console, screen, live renderer) build on top of these primitives.
 */

#include "tapioca/exports.h"

#include <cstdio>
#include <functional>
#include <string>
#include <string_view>

namespace tapioca {

    // ── Output sink ─────────────────────────────────────────────────────────

    /**
     * @brief Sink function type: receives a string_view to write to output.
     *
     * Default sink writes to stdout. Custom sinks can be used for testing,
     * capturing output to a string buffer, or redirecting to a file.
     */
    using output_sink = std::function<void(std::string_view)>;

    namespace pal {

        /** @brief Create a sink that writes to stdout.
         *  On Emscripten: calls JS to render ANSI-styled HTML into the DOM.
         *  On native: writes via fwrite to stdout.
         */
        [[nodiscard]] TAPIOCA_API output_sink stdout_sink() noexcept;

        /** @brief Create a sink that writes to stderr via fwrite. */
        [[nodiscard]] inline output_sink stderr_sink() noexcept {
            return [](std::string_view data) { std::fwrite(data.data(), 1, data.size(), stderr); };
        }

        /** @brief Create a sink that appends to a string buffer (for testing). */
        [[nodiscard]] inline output_sink string_sink(std::string &buffer) noexcept {
            return [&buffer](std::string_view data) { buffer.append(data); };
        }

        /** @brief Create a sink that writes to an arbitrary FILE*. */
        [[nodiscard]] inline output_sink file_sink(FILE *f) noexcept {
            return [f](std::string_view data) { std::fwrite(data.data(), 1, data.size(), f); };
        }

        // ── Low-level write / flush ─────────────────────────────────────────────

        /** @brief Write raw bytes to stdout (blocking). */
        inline void write_stdout(std::string_view data) noexcept {
            std::fwrite(data.data(), 1, data.size(), stdout);
        }

        /** @brief Flush stdout. */
        inline void flush_stdout() noexcept {
            std::fflush(stdout);
        }

        /** @brief Write raw bytes to a FILE* (blocking). */
        inline void write_file(FILE *f, std::string_view data) noexcept {
            std::fwrite(data.data(), 1, data.size(), f);
        }

        /** @brief Flush a FILE*. */
        inline void flush_file(FILE *f) noexcept {
            std::fflush(f);
        }

        // ── Clipboard (OSC 52) ─────────────────────────────────────────────────

        /**
         * @brief Set clipboard contents via OSC 52 escape sequence.
         *
         * Emits: ESC ] 52 ; c ; <base64-data> ESC \\
         * Requires terminal OSC clipboard support (e.g. xterm, kitty, WezTerm).
         * No-op if data is empty.
         *
         * @param data  Raw UTF-8 text to place on the clipboard.
         * @param sink  Output sink to write the escape sequence to.
         */
        TAPIOCA_API void clipboard_set(std::string_view data, const output_sink &sink);

        /**
         * @brief Request clipboard contents via OSC 52 escape sequence.
         *
         * Emits: ESC ] 52 ; c ; ? ESC \\
         * The terminal will respond with the clipboard contents (requires raw mode
         * to read the response). This only sends the request; reading the response
         * is the caller's responsibility.
         *
         * @param sink  Output sink to write the escape sequence to.
         */
        TAPIOCA_API void clipboard_request(const output_sink &sink);

    } // namespace pal

} // namespace tapioca
