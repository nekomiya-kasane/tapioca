/**
 * @file raw_input.cpp
 * @brief Cross-platform raw terminal input implementation.
 */

#include "tapioca/raw_input.h"
#include "tapioca/terminal.h"

#include <cstring>

#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#else
#  include <sys/select.h>
#  include <termios.h>
#  include <unistd.h>
#endif

namespace tapioca {

// ═══════════════════════════════════════════════════════════════════════
//  Platform state
// ═══════════════════════════════════════════════════════════════════════

#ifdef _WIN32

struct raw_mode::impl {
    HANDLE  stdin_handle = INVALID_HANDLE_VALUE;
    DWORD   old_mode     = 0;
};

raw_mode::raw_mode() : impl_(std::make_unique<impl>()) {
    impl_->stdin_handle = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(impl_->stdin_handle, &impl_->old_mode);
    SetConsoleMode(impl_->stdin_handle, ENABLE_EXTENDED_FLAGS | ENABLE_WINDOW_INPUT
                   | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT);
}

raw_mode::~raw_mode() {
    if (impl_ && impl_->stdin_handle != INVALID_HANDLE_VALUE)
        SetConsoleMode(impl_->stdin_handle, impl_->old_mode);
}

raw_mode::raw_mode(raw_mode&&) noexcept = default;
raw_mode& raw_mode::operator=(raw_mode&&) noexcept = default;

#else  // POSIX

struct raw_mode::impl {
    struct termios old_termios {};
};

raw_mode::raw_mode() : impl_(std::make_unique<impl>()) {
    tcgetattr(STDIN_FILENO, &impl_->old_termios);
    struct termios raw = impl_->old_termios;
    raw.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    // Enable SGR mouse tracking with button-event mode (1002) for drag
    ::write(STDOUT_FILENO, "\x1b[?1002h\x1b[?1006h", 16);
    // Enable Kitty keyboard protocol: flags=0b11 (disambiguate + report event types)
    ::write(STDOUT_FILENO, "\x1b[>3u", 5);
}

raw_mode::~raw_mode() {
    if (impl_) {
        // Disable Kitty keyboard protocol (pop)
        ::write(STDOUT_FILENO, "\x1b[<u", 4);
        ::write(STDOUT_FILENO, "\x1b[?1006l\x1b[?1002l", 16);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &impl_->old_termios);
    }
}

raw_mode::raw_mode(raw_mode&&) noexcept = default;
raw_mode& raw_mode::operator=(raw_mode&&) noexcept = default;

#endif

// ═══════════════════════════════════════════════════════════════════════
//  Shared polling state
// ═══════════════════════════════════════════════════════════════════════

static bool s_mouse_btn_held = false;

#ifdef _WIN32
// Win32 UTF-16 surrogate buffering
static wchar_t s_high_surrogate = 0;
#endif

// ═══════════════════════════════════════════════════════════════════════
//  Win32 implementation
// ═══════════════════════════════════════════════════════════════════════

#ifdef _WIN32

static key_mod win32_mods(DWORD state) {
    key_mod m = key_mod::none;
    if (state & SHIFT_PRESSED)                              m = m | key_mod::shift;
    if (state & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))   m = m | key_mod::ctrl;
    if (state & (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED))     m = m | key_mod::alt;
    return m;
}

static std::optional<input_event> win32_key(const KEY_EVENT_RECORD& ke) {
    if (!ke.bKeyDown) return std::nullopt;

    auto vk   = ke.wVirtualKeyCode;
    auto ch   = ke.uChar.UnicodeChar;
    auto mods = win32_mods(ke.dwControlKeyState);

    // Special keys
    special_key sk = special_key::none;
    switch (vk) {
    case VK_UP:     sk = special_key::up;        break;
    case VK_DOWN:   sk = special_key::down;       break;
    case VK_LEFT:   sk = special_key::left;       break;
    case VK_RIGHT:  sk = special_key::right;      break;
    case VK_RETURN: sk = special_key::enter;      break;
    case VK_ESCAPE: sk = special_key::escape;     break;
    case VK_TAB:    sk = special_key::tab;        break;
    case VK_BACK:   sk = special_key::backspace;  break;
    case VK_DELETE: sk = special_key::delete_;     break;
    case VK_INSERT: sk = special_key::insert;     break;
    case VK_PRIOR:  sk = special_key::page_up;    break;
    case VK_NEXT:   sk = special_key::page_down;  break;
    case VK_HOME:   sk = special_key::home;       break;
    case VK_END:    sk = special_key::end;        break;
    case VK_F1:     sk = special_key::f1;         break;
    case VK_F2:     sk = special_key::f2;         break;
    case VK_F3:     sk = special_key::f3;         break;
    case VK_F4:     sk = special_key::f4;         break;
    case VK_F5:     sk = special_key::f5;         break;
    case VK_F6:     sk = special_key::f6;         break;
    case VK_F7:     sk = special_key::f7;         break;
    case VK_F8:     sk = special_key::f8;         break;
    case VK_F9:     sk = special_key::f9;         break;
    case VK_F10:    sk = special_key::f10;        break;
    case VK_F11:    sk = special_key::f11;        break;
    case VK_F12:    sk = special_key::f12;        break;
    default: break;
    }

    if (sk != special_key::none) {
        return input_event{key_event{0, sk, mods}};
    }

    // Printable character (including CJK via UTF-16 surrogates)
    if (ch != 0) {
        char32_t cp = 0;
        if (ch >= 0xD800 && ch <= 0xDBFF) {
            // High surrogate -- buffer and wait for low surrogate
            s_high_surrogate = ch;
            return std::nullopt;
        }
        if (ch >= 0xDC00 && ch <= 0xDFFF) {
            // Low surrogate -- combine with buffered high surrogate
            if (s_high_surrogate != 0) {
                cp = 0x10000 + ((static_cast<char32_t>(s_high_surrogate) - 0xD800) << 10)
                             + (static_cast<char32_t>(ch) - 0xDC00);
                s_high_surrogate = 0;
            } else {
                return std::nullopt;  // orphan low surrogate
            }
        } else {
            s_high_surrogate = 0;
            cp = static_cast<char32_t>(ch);
        }
        // Skip bare control characters that aren't mapped to special keys
        if (cp < 0x20 && sk == special_key::none) return std::nullopt;
        return input_event{key_event{cp, special_key::none, mods}};
    }

    return std::nullopt;
}

static std::optional<input_event> win32_mouse(const MOUSE_EVENT_RECORD& me) {
    uint32_t mx = static_cast<uint32_t>(me.dwMousePosition.X);
    uint32_t my = static_cast<uint32_t>(me.dwMousePosition.Y);
    auto mods = win32_mods(me.dwControlKeyState);

    bool btn_now = (me.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;

    // Scroll wheel
    if (me.dwEventFlags == MOUSE_WHEELED) {
        auto delta = static_cast<short>(HIWORD(me.dwButtonState));
        mouse_button btn = (delta > 0) ? mouse_button::scroll_up : mouse_button::scroll_down;
        return input_event{mouse_event{mx, my, btn, mouse_action::press, mods}};
    }

    // Button state change (press or release)
    if (me.dwEventFlags == 0) {
        if (btn_now && !s_mouse_btn_held) {
            s_mouse_btn_held = true;
            return input_event{mouse_event{mx, my, mouse_button::left, mouse_action::press, mods}};
        }
        if (!btn_now && s_mouse_btn_held) {
            s_mouse_btn_held = false;
            return input_event{mouse_event{mx, my, mouse_button::left, mouse_action::release, mods}};
        }
        // Right / middle button
        if (me.dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
            return input_event{mouse_event{mx, my, mouse_button::right, mouse_action::press, mods}};
        }
    }

    // Mouse movement
    if (me.dwEventFlags == MOUSE_MOVED) {
        auto action = s_mouse_btn_held ? mouse_action::drag : mouse_action::move;
        auto btn    = s_mouse_btn_held ? mouse_button::left : mouse_button::none;
        return input_event{mouse_event{mx, my, btn, action, mods}};
    }

    return std::nullopt;
}

std::optional<input_event> poll_event(int timeout_ms) {
    HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
    if (WaitForSingleObject(h, static_cast<DWORD>(timeout_ms)) != WAIT_OBJECT_0)
        return std::nullopt;

    INPUT_RECORD rec;
    DWORD count = 0;
    if (!ReadConsoleInputW(h, &rec, 1, &count) || count == 0)
        return std::nullopt;

    switch (rec.EventType) {
    case KEY_EVENT:
        return win32_key(rec.Event.KeyEvent);

    case MOUSE_EVENT:
        return win32_mouse(rec.Event.MouseEvent);

    case WINDOW_BUFFER_SIZE_EVENT: {
        auto& sz = rec.Event.WindowBufferSizeEvent.dwSize;
        return input_event{resize_event{
            static_cast<uint32_t>(sz.X),
            static_cast<uint32_t>(sz.Y)
        }};
    }

    default:
        break;
    }
    return std::nullopt;
}

#else  // ═══════════════════════════════════════════════════════════════
//  POSIX implementation
// ═══════════════════════════════════════════════════════════════════════

// Decode a UTF-8 leading byte to determine the expected sequence length.
static int utf8_seq_len(uint8_t b) {
    if (b < 0x80) return 1;
    if ((b & 0xE0) == 0xC0) return 2;
    if ((b & 0xF0) == 0xE0) return 3;
    if ((b & 0xF8) == 0xF0) return 4;
    return 1;  // invalid, treat as single byte
}

// Decode a UTF-8 sequence to a codepoint.
static char32_t utf8_decode(const uint8_t* buf, int len) {
    if (len == 1) return buf[0];
    if (len == 2) return ((buf[0] & 0x1F) << 6) | (buf[1] & 0x3F);
    if (len == 3) return ((buf[0] & 0x0F) << 12) | ((buf[1] & 0x3F) << 6) | (buf[2] & 0x3F);
    if (len == 4) return ((buf[0] & 0x07) << 18) | ((buf[1] & 0x3F) << 12)
                       | ((buf[2] & 0x3F) << 6) | (buf[3] & 0x3F);
    return 0xFFFD;  // replacement character
}

// Parse SGR mouse sequence: ESC [ < Cb ; Cx ; Cy M/m
static std::optional<input_event> parse_sgr_mouse(const char* buf, int n) {
    if (n < 6) return std::nullopt;
    if (buf[0] != 27 || buf[1] != '[' || buf[2] != '<') return std::nullopt;

    int cb = 0, cx = 0, cy = 0;
    int pos = 3;
    while (pos < n && buf[pos] >= '0' && buf[pos] <= '9') cb = cb * 10 + (buf[pos++] - '0');
    if (pos < n && buf[pos] == ';') pos++;
    while (pos < n && buf[pos] >= '0' && buf[pos] <= '9') cx = cx * 10 + (buf[pos++] - '0');
    if (pos < n && buf[pos] == ';') pos++;
    while (pos < n && buf[pos] >= '0' && buf[pos] <= '9') cy = cy * 10 + (buf[pos++] - '0');
    char final_ch = (pos < n) ? buf[pos] : 0;

    uint32_t mx = (cx > 0) ? static_cast<uint32_t>(cx - 1) : 0;  // SGR is 1-based
    uint32_t my = (cy > 0) ? static_cast<uint32_t>(cy - 1) : 0;

    // Modifier bits in cb: bit 2 = shift, bit 3 = meta/alt, bit 4 = ctrl
    key_mod mods = key_mod::none;
    if (cb & 4)  mods = mods | key_mod::shift;
    if (cb & 8)  mods = mods | key_mod::alt;
    if (cb & 16) mods = mods | key_mod::ctrl;

    int base_cb = cb & 3;  // lower 2 bits = button

    // Scroll wheel: cb 64 = scroll up, cb 65 = scroll down
    if ((cb & 64) != 0) {
        auto btn = (base_cb == 0) ? mouse_button::scroll_up : mouse_button::scroll_down;
        return input_event{mouse_event{mx, my, btn, mouse_action::press, mods}};
    }

    // Button release
    if (final_ch == 'm') {
        s_mouse_btn_held = false;
        mouse_button btn = mouse_button::left;
        if (base_cb == 1) btn = mouse_button::middle;
        if (base_cb == 2) btn = mouse_button::right;
        return input_event{mouse_event{mx, my, btn, mouse_action::release, mods}};
    }

    // Button press or motion
    if (final_ch == 'M') {
        // cb bit 5 (32) = motion flag
        bool motion = (cb & 32) != 0;
        if (!motion) {
            // Button press
            mouse_button btn = mouse_button::left;
            if (base_cb == 1) btn = mouse_button::middle;
            if (base_cb == 2) btn = mouse_button::right;
            if (base_cb == 0) s_mouse_btn_held = true;
            return input_event{mouse_event{mx, my, btn, mouse_action::press, mods}};
        }
        // Motion: drag if button held, else move
        if (base_cb == 0 || s_mouse_btn_held) {
            return input_event{mouse_event{mx, my, mouse_button::left, mouse_action::drag, mods}};
        }
        return input_event{mouse_event{mx, my, mouse_button::none, mouse_action::move, mods}};
    }

    return std::nullopt;
}

// Parse Kitty keyboard protocol CSI u sequence: ESC [ codepoint [; modifier [:event_type]] u
static std::optional<input_event> parse_kitty_csi_u(const char* buf, int n) {
    // Minimum: ESC [ <digit> u  (4 bytes)
    if (n < 4 || buf[0] != 27 || buf[1] != '[' || buf[n - 1] != 'u') return std::nullopt;

    int pos = 2;
    // Parse codepoint
    uint32_t codepoint = 0;
    while (pos < n - 1 && buf[pos] >= '0' && buf[pos] <= '9') {
        codepoint = codepoint * 10 + static_cast<uint32_t>(buf[pos] - '0');
        ++pos;
    }

    uint32_t mod_bits = 0;
    key_action action = key_action::press;

    if (pos < n - 1 && buf[pos] == ';') {
        ++pos;
        // Parse modifier
        while (pos < n - 1 && buf[pos] >= '0' && buf[pos] <= '9') {
            mod_bits = mod_bits * 10 + static_cast<uint32_t>(buf[pos] - '0');
            ++pos;
        }
        // Parse optional event type after ':'
        if (pos < n - 1 && buf[pos] == ':') {
            ++pos;
            int ev_type = 0;
            while (pos < n - 1 && buf[pos] >= '0' && buf[pos] <= '9') {
                ev_type = ev_type * 10 + (buf[pos] - '0');
                ++pos;
            }
            if (ev_type >= 1 && ev_type <= 3) {
                action = static_cast<key_action>(ev_type);
            }
        }
    }

    // Decode Kitty modifier bitmask (subtract 1 from reported value)
    key_mod mods = key_mod::none;
    if (mod_bits > 0) {
        uint32_t m = mod_bits - 1;
        if (m & 1)  mods = mods | key_mod::shift;
        if (m & 2)  mods = mods | key_mod::alt;
        if (m & 4)  mods = mods | key_mod::ctrl;
        if (m & 8)  mods = mods | key_mod::super_;
        if (m & 16) mods = mods | key_mod::hyper;
        if (m & 32) mods = mods | key_mod::caps_lock;
        if (m & 64) mods = mods | key_mod::num_lock;
    }

    // Map well-known Kitty codepoints to special_key
    special_key sk = special_key::none;
    switch (codepoint) {
    case 13:    sk = special_key::enter;     break;
    case 27:    sk = special_key::escape;    break;
    case 9:     sk = special_key::tab;       break;
    case 127:   sk = special_key::backspace; break;
    case 57358: sk = special_key::insert;    break;  // Kitty functional codepoints
    case 57359: sk = special_key::delete_;   break;
    case 57352: sk = special_key::home;      break;
    case 57353: sk = special_key::end;       break;
    case 57354: sk = special_key::page_up;   break;
    case 57355: sk = special_key::page_down; break;
    case 57356: sk = special_key::up;        break;
    case 57357: sk = special_key::down;      break;
    // F1-F12 mapped by Unicode Private Use Area in Kitty spec
    case 57364: sk = special_key::f1;  break;
    case 57365: sk = special_key::f2;  break;
    case 57366: sk = special_key::f3;  break;
    case 57367: sk = special_key::f4;  break;
    case 57368: sk = special_key::f5;  break;
    case 57369: sk = special_key::f6;  break;
    case 57370: sk = special_key::f7;  break;
    case 57371: sk = special_key::f8;  break;
    case 57372: sk = special_key::f9;  break;
    case 57373: sk = special_key::f10; break;
    case 57374: sk = special_key::f11; break;
    case 57375: sk = special_key::f12; break;
    default: break;
    }

    if (sk != special_key::none) {
        return input_event{key_event{0, sk, mods, action}};
    }

    return input_event{key_event{static_cast<char32_t>(codepoint), special_key::none, mods, action}};
}

// Parse special key sequences: ESC [ ...
static std::optional<input_event> parse_escape_seq(const char* buf, int n) {
    if (n < 3 || buf[0] != 27 || buf[1] != '[') return std::nullopt;

    // Kitty keyboard protocol: CSI ... u
    if (buf[n - 1] == 'u') {
        auto ev = parse_kitty_csi_u(buf, n);
        if (ev) return ev;
    }

    // SGR mouse
    if (buf[2] == '<') return parse_sgr_mouse(buf, n);

    // Modifier detection for arrow/special keys: ESC [ 1 ; <mod> <letter>
    key_mod mods = key_mod::none;
    char final_ch = buf[2];
    if (n >= 6 && buf[2] == '1' && buf[3] == ';') {
        int mod_num = buf[4] - '0';
        // xterm modifier encoding: 2=shift, 3=alt, 4=shift+alt, 5=ctrl, etc.
        if (mod_num >= 2) {
            mod_num -= 1;
            if (mod_num & 1) mods = mods | key_mod::shift;
            if (mod_num & 2) mods = mods | key_mod::alt;
            if (mod_num & 4) mods = mods | key_mod::ctrl;
        }
        final_ch = buf[5];
    }

    special_key sk = special_key::none;
    switch (final_ch) {
    case 'A': sk = special_key::up;    break;
    case 'B': sk = special_key::down;  break;
    case 'C': sk = special_key::right; break;
    case 'D': sk = special_key::left;  break;
    case 'H': sk = special_key::home;  break;
    case 'F': sk = special_key::end;   break;
    case 'Z': return input_event{key_event{0, special_key::tab, key_mod::shift}};
    default: break;
    }
    if (sk != special_key::none)
        return input_event{key_event{0, sk, mods}};

    // CSI number ~ sequences: page up/down, delete, insert
    if (n >= 4 && buf[n - 1] == '~') {
        int num = 0;
        for (int i = 2; i < n - 1 && buf[i] >= '0' && buf[i] <= '9'; ++i)
            num = num * 10 + (buf[i] - '0');
        switch (num) {
        case 2:  return input_event{key_event{0, special_key::insert,    mods}};
        case 3:  return input_event{key_event{0, special_key::delete_,   mods}};
        case 5:  return input_event{key_event{0, special_key::page_up,   mods}};
        case 6:  return input_event{key_event{0, special_key::page_down, mods}};
        case 15: return input_event{key_event{0, special_key::f5,        mods}};
        case 17: return input_event{key_event{0, special_key::f6,        mods}};
        case 18: return input_event{key_event{0, special_key::f7,        mods}};
        case 19: return input_event{key_event{0, special_key::f8,        mods}};
        case 20: return input_event{key_event{0, special_key::f9,        mods}};
        case 21: return input_event{key_event{0, special_key::f10,       mods}};
        case 23: return input_event{key_event{0, special_key::f11,       mods}};
        case 24: return input_event{key_event{0, special_key::f12,       mods}};
        default: break;
        }
    }

    // ESC O <letter> -- SS3 function keys (F1-F4)
    if (n >= 3 && buf[1] == 'O') {
        switch (buf[2]) {
        case 'P': return input_event{key_event{0, special_key::f1, mods}};
        case 'Q': return input_event{key_event{0, special_key::f2, mods}};
        case 'R': return input_event{key_event{0, special_key::f3, mods}};
        case 'S': return input_event{key_event{0, special_key::f4, mods}};
        default: break;
        }
    }

    return std::nullopt;
}

std::optional<input_event> poll_event(int timeout_ms) {
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    struct timeval tv;
    tv.tv_sec  = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) <= 0)
        return std::nullopt;

    char buf[64];
    int n = static_cast<int>(::read(STDIN_FILENO, buf, sizeof(buf)));
    if (n <= 0) return std::nullopt;

    // Escape sequences
    if (n >= 2 && buf[0] == 27) {
        // ESC [ or ESC O
        if (buf[1] == '[' || buf[1] == 'O') {
            auto ev = parse_escape_seq(buf, n);
            if (ev) return ev;
        }
        // Bare ESC
        if (n == 1) return input_event{key_event{0, special_key::escape, key_mod::none}};
        // Alt + key: ESC followed by a printable character
        if (n == 2 && buf[1] >= 0x20) {
            return input_event{key_event{static_cast<char32_t>(buf[1]),
                                         special_key::none, key_mod::alt}};
        }
    }

    // Single byte
    if (n == 1) {
        uint8_t b = static_cast<uint8_t>(buf[0]);
        if (b == 27) return input_event{key_event{0, special_key::escape, key_mod::none}};
        if (b == '\n' || b == '\r') return input_event{key_event{0, special_key::enter, key_mod::none}};
        if (b == '\t') return input_event{key_event{0, special_key::tab, key_mod::none}};
        if (b == 127) return input_event{key_event{0, special_key::backspace, key_mod::none}};
        // Ctrl+letter (1-26)
        if (b >= 1 && b <= 26) {
            return input_event{key_event{static_cast<char32_t>('a' + b - 1),
                                         special_key::none, key_mod::ctrl}};
        }
        if (b >= 0x20 && b < 0x7F) {
            return input_event{key_event{static_cast<char32_t>(b), special_key::none, key_mod::none}};
        }
    }

    // Multi-byte UTF-8 (CJK, emoji, etc.)
    if (n >= 2) {
        uint8_t b0 = static_cast<uint8_t>(buf[0]);
        int seq_len = utf8_seq_len(b0);
        if (seq_len > 1 && seq_len <= n) {
            char32_t cp = utf8_decode(reinterpret_cast<const uint8_t*>(buf), seq_len);
            if (cp != 0xFFFD) {
                return input_event{key_event{cp, special_key::none, key_mod::none}};
            }
        }
    }

    return std::nullopt;
}

#endif  // _WIN32

}  // namespace tapioca
