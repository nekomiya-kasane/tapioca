/**
 * @file terminal.cpp
 * @brief Platform-specific terminal capability detection and control.
 */

#include "tapioca/terminal.h"

#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#    include <Windows.h>
#    include <io.h>
#else
#    include <sys/ioctl.h>
#    include <unistd.h>
#endif

namespace tapioca::terminal {

    // ── get_size ────────────────────────────────────────────────────────────

    terminal_size get_size() noexcept {
#ifdef _WIN32
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(h, &csbi)) {
            return {static_cast<uint32_t>(csbi.srWindow.Right - csbi.srWindow.Left + 1),
                    static_cast<uint32_t>(csbi.srWindow.Bottom - csbi.srWindow.Top + 1)};
        }
#else
        struct winsize ws{};
        if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
            return {static_cast<uint32_t>(ws.ws_col), static_cast<uint32_t>(ws.ws_row)};
        }
#endif
        // Fallback: check COLUMNS/LINES env vars
        const char *cols = std::getenv("COLUMNS");
        const char *rows = std::getenv("LINES");
        if (cols && rows) {
            return {static_cast<uint32_t>(std::atoi(cols)), static_cast<uint32_t>(std::atoi(rows))};
        }
        return {80, 24}; // sensible default
    }

    // ── detect_color_depth ──────────────────────────────────────────────────

    color_depth detect_color_depth() noexcept {
        if (!is_tty()) {
            return color_depth::none;
        }

        // Check COLORTERM for true color
        const char *colorterm = std::getenv("COLORTERM");
        if (colorterm) {
            if (std::strcmp(colorterm, "truecolor") == 0 || std::strcmp(colorterm, "24bit") == 0) {
                return color_depth::true_color;
            }
        }

        // Windows Terminal always supports true color
        if (std::getenv("WT_SESSION")) {
            return color_depth::true_color;
        }

        // Check TERM for 256-color
        const char *term = std::getenv("TERM");
        if (term) {
            if (std::strstr(term, "256color")) {
                return color_depth::palette_256;
            }
            if (std::strstr(term, "color") || std::strstr(term, "xterm") || std::strstr(term, "screen") ||
                std::strstr(term, "tmux") || std::strstr(term, "rxvt") || std::strstr(term, "linux")) {
                return color_depth::basic_16;
            }
        }

#ifdef _WIN32
        // Modern Windows consoles support at least 16 colors
        return color_depth::basic_16;
#else
        return color_depth::basic_16;
#endif
    }

    // ── is_tty ──────────────────────────────────────────────────────────────

    bool is_tty(FILE *f) noexcept {
#ifdef _WIN32
        return _isatty(_fileno(f)) != 0;
#else
        return isatty(fileno(f)) != 0;
#endif
    }

    // ── enable_vt_processing ────────────────────────────────────────────────

    bool enable_vt_processing() noexcept {
#ifdef _WIN32
        bool ok = true;
        for (DWORD id : {STD_OUTPUT_HANDLE, STD_ERROR_HANDLE}) {
            HANDLE h = GetStdHandle(id);
            if (h == INVALID_HANDLE_VALUE) {
                ok = false;
                continue;
            }
            DWORD mode = 0;
            if (!GetConsoleMode(h, &mode)) {
                ok = false;
                continue;
            }
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (!SetConsoleMode(h, mode)) {
                ok = false;
            }
        }
        return ok;
#else
        return true; // POSIX terminals natively support VT sequences
#endif
    }

    // ── enable_utf8 ──────────────────────────────────────────────────────

    void enable_utf8() noexcept {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
#endif
    }

    // ── detect_osc_support ───────────────────────────────────────────────

    osc_support detect_osc_support() noexcept {
        osc_support caps{};

        if (!is_tty()) {
            return caps;
        }

        // OSC 0/1/2 (title) is supported by virtually every terminal
        caps.title = true;

        const char *term_program = std::getenv("TERM_PROGRAM");
        const char *term = std::getenv("TERM");
        const char *wt_session = std::getenv("WT_SESSION");
        const char *vte_version = std::getenv("VTE_VERSION");
        const char *kitty_pid = std::getenv("KITTY_PID");
        const char *wezterm = std::getenv("WEZTERM_EXECUTABLE");
        const char *tmux = std::getenv("TMUX");

        // ── iTerm2 ──────────────────────────────────────────────────────
        bool is_iterm2 = term_program && std::strcmp(term_program, "iTerm.app") == 0;
        if (is_iterm2) {
            caps.palette = true;
            caps.cwd = true;
            caps.hyperlink = true;
            caps.notify = true; // OSC 9 is iTerm2-specific
            caps.color_query = true;
            caps.clipboard = true;
            caps.shell_integration = true;
            return caps;
        }

        // ── kitty ───────────────────────────────────────────────────────
        if (kitty_pid) {
            caps.palette = true;
            caps.cwd = true;
            caps.hyperlink = true;
            caps.color_query = true;
            caps.clipboard = true;
            caps.shell_integration = true;
            return caps;
        }

        // ── WezTerm ─────────────────────────────────────────────────────
        if (wezterm) {
            caps.palette = true;
            caps.cwd = true;
            caps.hyperlink = true;
            caps.color_query = true;
            caps.clipboard = true;
            caps.shell_integration = true;
            return caps;
        }

        // ── Windows Terminal ─────────────────────────────────────────────
        if (wt_session) {
            caps.palette = true;
            caps.hyperlink = true;
            caps.color_query = true;
            caps.clipboard = true; // WT 1.18+ supports OSC 52
            return caps;
        }

        // ── VTE-based (GNOME Terminal, Tilix, Terminator, etc.) ─────────
        if (vte_version) {
            int ver = std::atoi(vte_version);
            caps.palette = true;
            caps.color_query = true;
            if (ver >= 5000) { // VTE 0.50+
                caps.hyperlink = true;
                caps.cwd = true;
            }
            return caps;
        }

        // ── tmux ────────────────────────────────────────────────────────
        if (tmux) {
            caps.palette = true;
            caps.color_query = true;
            // tmux passes through OSC 52 if set-clipboard is on (default: external)
            caps.clipboard = true;
            // tmux passes through OSC 8 since 3.1
            caps.hyperlink = true;
            return caps;
        }

        // ── xterm ───────────────────────────────────────────────────────
        if (term && std::strstr(term, "xterm")) {
            caps.palette = true;
            caps.color_query = true;
            caps.clipboard = true;
            caps.hyperlink = true;
            return caps;
        }

        // ── foot ────────────────────────────────────────────────────────
        if (term && std::strstr(term, "foot")) {
            caps.palette = true;
            caps.cwd = true;
            caps.hyperlink = true;
            caps.color_query = true;
            caps.clipboard = true;
            caps.shell_integration = true;
            return caps;
        }

        // ── Generic fallback: assume palette at minimum for color terms ──
        if (term) {
            if (std::strstr(term, "256color") || std::strstr(term, "color")) {
                caps.palette = true;
            }
        }

        return caps;
    }

} // namespace tapioca::terminal

// ── terminal_caps::detect ────────────────────────────────────────────────

namespace tapioca {

    terminal_caps terminal_caps::detect() noexcept {
        terminal_caps caps{};

        // Color depth
        caps.max_colors = terminal::detect_color_depth();

        // OSC support
        caps.osc = terminal::detect_osc_support();

        // No TTY = no VT sequences at all
        if (!terminal::is_tty()) {
            caps.vt_sequences = false;
            caps.unicode = false;
            caps.supported_attrs = attr::none;
            return caps;
        }

#ifdef _WIN32
        // Check if VT processing is available
        HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
        if (h != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            if (GetConsoleMode(h, &mode)) {
                caps.vt_sequences = (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
                if (!caps.vt_sequences) {
                    // Try to enable VT — if it succeeds, the console supports it
                    caps.vt_sequences = terminal::enable_vt_processing();
                }
            } else {
                caps.vt_sequences = false;
            }
        }

        if (!caps.vt_sequences) {
            // Legacy conhost: no ANSI support at all
            caps.max_colors = color_depth::none;
            caps.supported_attrs = attr::none;
            caps.unicode = false;
            return caps;
        }

        // Windows Terminal = full modern
        if (std::getenv("WT_SESSION")) {
            caps.supported_attrs = static_cast<attr>(0x0FFF);
            caps.unicode = true;
            return caps;
        }

        // Conhost with VT enabled (Win10 1511+): limited attr support
        caps.supported_attrs = attr::bold | attr::underline | attr::reverse | attr::dim | attr::hidden | attr::strike;
        // Conhost doesn't render italic, overline, superscript, subscript well
        // Check console codepage for unicode
        caps.unicode = (GetConsoleOutputCP() == CP_UTF8);

#else
        // POSIX: VT always available
        caps.vt_sequences = true;

        const char *term = std::getenv("TERM");

        // linux console (VT) has very limited attrs
        if (term && std::strcmp(term, "linux") == 0) {
            caps.supported_attrs =
                attr::bold | attr::dim | attr::underline | attr::blink | attr::reverse | attr::hidden;
            caps.unicode = false;
            return caps;
        }

        // Most modern terminals support all attrs
        caps.supported_attrs = static_cast<attr>(0x0FFF);

        // Unicode: check locale
        const char *lang = std::getenv("LANG");
        const char *lc_all = std::getenv("LC_ALL");
        const char *lc_ctype = std::getenv("LC_CTYPE");
        auto has_utf8 = [](const char *s) -> bool {
            if (!s) {
                return false;
            }
            return std::strstr(s, "UTF-8") || std::strstr(s, "utf-8") || std::strstr(s, "UTF8") ||
                   std::strstr(s, "utf8");
        };
        caps.unicode = has_utf8(lc_all) || has_utf8(lc_ctype) || has_utf8(lang);

        // xterm-256color variants: trim exotic attrs that older xterms skip
        if (term && std::strstr(term, "xterm") && !std::strstr(term, "kitty")) {
            caps.supported_attrs = attr::bold | attr::dim | attr::italic | attr::underline | attr::blink |
                                   attr::reverse | attr::hidden | attr::strike;
        }
#endif

        return caps;
    }

} // namespace tapioca
