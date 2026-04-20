/**
 * @file pal.cpp
 * @brief Platform Abstraction Layer implementation (clipboard via OSC 52).
 */

#include "tapioca/pal.h"

#include <cstdint>
#include <string>
#include <string_view>

#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif

namespace tapioca::pal {

    // ── stdout_sink ──────────────────────────────────────────────────────────

#ifdef __EMSCRIPTEN__

    // Emscripten ANSI→HTML stdout sink.
    // Uses EM_JS to embed a JS ANSI parser. Key constraints inside EM_JS body:
    //   - EM_JS stringifies via C preprocessor #code, collapsing \\\\ → \\.
    //     So "\\\\[" in source becomes "\\[" in JS — still one escape layer short.
    //     Solution: build all special chars via String.fromCharCode() to avoid
    //     any escape-level ambiguity.
    //   - Must handle both browser (DOM output) and Node.js (raw ANSI passthrough).
    // clang-format off
EM_JS(void, tapioca_ansi_write_impl, (const char* ptr, int len), {
    var text = UTF8ToString(ptr, len);
    // Node.js: pass ANSI codes through directly — terminal handles them natively
    if (typeof document == "undefined") {
        if (typeof process != "undefined" && process.stdout) {
            process.stdout.write(text);
        }
        return;
    }
    // Browser: parse ANSI SGR sequences and render as styled HTML
    if (!Module._tapiocaEl) {
        var el = document.getElementById("tapiru-terminal");
        if (!el) el = document.getElementById("output");
        if (!el) {
            el = document.createElement("pre");
            el.id = "tapiru-terminal";
            el.style.cssText = "font-family:monospace;white-space:pre-wrap;background:#1e1e1e;color:#d4d4d4;padding:15px;";
            document.body.appendChild(el);
        }
        Module._tapiocaEl = el;
        Module._ansiFg = "";
        Module._ansiBg = "";
        Module._ansiBold = false;
        Module._ansiDim = false;
        Module._ansiItalic = false;
        Module._ansiUnderline = false;
        Module._ansiStrike = false;
        Module._ansiOverline = false;
        Module._ansi16 = [
            "#000000","#aa0000","#00aa00","#aa5500","#0000aa","#aa00aa","#00aaaa","#aaaaaa",
            "#555555","#ff5555","#55ff55","#ffff55","#5555ff","#ff55ff","#55ffff","#ffffff"
        ];
        Module._ansi256 = Module._ansi16.slice();
        for (var i = 0; i < 216; i++) {
            var r = Math.floor(i / 36), g = Math.floor((i % 36) / 6), b = i % 6;
            var tr = r ? r * 40 + 55 : 0, tg = g ? g * 40 + 55 : 0, tb = b ? b * 40 + 55 : 0;
            Module._ansi256.push("rgb(" + tr + "," + tg + "," + tb + ")");
        }
        for (var i = 0; i < 24; i++) {
            var v = i * 10 + 8;
            Module._ansi256.push("rgb(" + v + "," + v + "," + v + ")");
        }
    }
    var el = Module._tapiocaEl;
    // Build regex: ESC \[ (params) m
    // Escape chain for "[": C source "\\\\[" -> #code stringify "\\[" -> JS string "\[" -> regex \[ (literal bracket)
    // ESC char via String.fromCharCode(27) to avoid \x1b being mangled.
    var pat = String.fromCharCode(27) + "\\\\[([0-9;]*)m";
    var re = new RegExp(pat, "g");
    var lastIdx = 0;
    var html = "";
    var fg = Module._ansiFg, bg = Module._ansiBg;
    var bold = Module._ansiBold, dim = Module._ansiDim, italic = Module._ansiItalic;
    var ul = Module._ansiUnderline, strike = Module._ansiStrike, ol = Module._ansiOverline;
    function mkSpan() {
        var parts = [];
        if (fg) parts.push("color:" + fg);
        if (bg) parts.push("background-color:" + bg);
        if (bold) parts.push("font-weight:bold");
        if (dim) parts.push("opacity:0.6");
        if (italic) parts.push("font-style:italic");
        var td = [];
        if (ul) td.push("underline");
        if (strike) td.push("line-through");
        if (ol) td.push("overline");
        if (td.length) parts.push("text-decoration:" + td.join(" "));
        if (parts.length) return "<span style=" + String.fromCharCode(34) + parts.join(";") + String.fromCharCode(34) + ">";
        return "";
    }
    function escHtml(s) {
        return s.replace(new RegExp("&","g"),"&amp;").replace(new RegExp("<","g"),"&lt;").replace(new RegExp(">","g"),"&gt;");
    }
    var match;
    while ((match = re.exec(text)) != null) {
        if (match.index > lastIdx) {
            var chunk = escHtml(text.substring(lastIdx, match.index));
            var sp = mkSpan();
            html += sp ? sp + chunk + "</span>" : chunk;
        }
        lastIdx = re.lastIndex;
        var ps = match[1] ? match[1].split(";").map(Number) : [0];
        for (var pi = 0; pi < ps.length; pi++) {
            var p = ps[pi];
            if (p == 0) { fg=""; bg=""; bold=false; dim=false; italic=false; ul=false; strike=false; ol=false; }
            else if (p == 1) bold = true;
            else if (p == 2) dim = true;
            else if (p == 3) italic = true;
            else if (p == 4) ul = true;
            else if (p == 9) strike = true;
            else if (p == 22) { bold = false; dim = false; }
            else if (p == 23) italic = false;
            else if (p == 24) ul = false;
            else if (p == 29) strike = false;
            else if (p == 53) ol = true;
            else if (p == 55) ol = false;
            else if (p >= 30 && p <= 37) fg = Module._ansi16[p - 30];
            else if (p >= 40 && p <= 47) bg = Module._ansi16[p - 40];
            else if (p >= 90 && p <= 97) fg = Module._ansi16[p - 90 + 8];
            else if (p >= 100 && p <= 107) bg = Module._ansi16[p - 100 + 8];
            else if (p == 39) fg = "";
            else if (p == 49) bg = "";
            else if (p == 38 && pi + 1 < ps.length) {
                if (ps[pi+1] == 2 && pi + 4 < ps.length) {
                    fg = "rgb(" + ps[pi+2] + "," + ps[pi+3] + "," + ps[pi+4] + ")";
                    pi += 4;
                } else if (ps[pi+1] == 5 && pi + 2 < ps.length) {
                    fg = Module._ansi256[ps[pi+2]] || "";
                    pi += 2;
                }
            } else if (p == 48 && pi + 1 < ps.length) {
                if (ps[pi+1] == 2 && pi + 4 < ps.length) {
                    bg = "rgb(" + ps[pi+2] + "," + ps[pi+3] + "," + ps[pi+4] + ")";
                    pi += 4;
                } else if (ps[pi+1] == 5 && pi + 2 < ps.length) {
                    bg = Module._ansi256[ps[pi+2]] || "";
                    pi += 2;
                }
            }
        }
    }
    if (lastIdx < text.length) {
        var chunk = escHtml(text.substring(lastIdx));
        var sp = mkSpan();
        html += sp ? sp + chunk + "</span>" : chunk;
    }
    html = html.replace(new RegExp(String.fromCharCode(10), "g"), "<br>");
    el.innerHTML += html;
    Module._ansiFg = fg; Module._ansiBg = bg;
    Module._ansiBold = bold; Module._ansiDim = dim; Module._ansiItalic = italic;
    Module._ansiUnderline = ul; Module._ansiStrike = strike; Module._ansiOverline = ol;
});
    // clang-format on

    output_sink stdout_sink() noexcept {
        return [](std::string_view data) { tapioca_ansi_write_impl(data.data(), static_cast<int>(data.size())); };
    }

#else  // native

    output_sink stdout_sink() noexcept {
        return [](std::string_view data) { std::fwrite(data.data(), 1, data.size(), stdout); };
    }

#endif  // __EMSCRIPTEN__

    namespace {

        // Base64 encoding table
        constexpr char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        std::string base64_encode(std::string_view input) {
            std::string out;
            out.reserve(((input.size() + 2) / 3) * 4);

            size_t i = 0;
            while (i + 2 < input.size()) {
                uint32_t triple = (static_cast<uint8_t>(input[i]) << 16) | (static_cast<uint8_t>(input[i + 1]) << 8)
                                  | static_cast<uint8_t>(input[i + 2]);
                out += b64_table[(triple >> 18) & 0x3F];
                out += b64_table[(triple >> 12) & 0x3F];
                out += b64_table[(triple >> 6) & 0x3F];
                out += b64_table[triple & 0x3F];
                i += 3;
            }

            if (i + 1 == input.size()) {
                uint32_t val = static_cast<uint8_t>(input[i]) << 16;
                out += b64_table[(val >> 18) & 0x3F];
                out += b64_table[(val >> 12) & 0x3F];
                out += '=';
                out += '=';
            } else if (i + 2 == input.size()) {
                uint32_t val = (static_cast<uint8_t>(input[i]) << 16) | (static_cast<uint8_t>(input[i + 1]) << 8);
                out += b64_table[(val >> 18) & 0x3F];
                out += b64_table[(val >> 12) & 0x3F];
                out += b64_table[(val >> 6) & 0x3F];
                out += '=';
            }

            return out;
        }

    }  // anonymous namespace

    void clipboard_set(std::string_view data, const output_sink& sink) {
        if (data.empty() || !sink) {
            return;
        }

        std::string seq;
        seq.reserve(data.size() * 2 + 16);
        seq += "\033]52;c;";
        seq += base64_encode(data);
        seq += "\033\\";
        sink(seq);
    }

    void clipboard_request(const output_sink& sink) {
        if (!sink) {
            return;
        }
        sink(std::string_view{"\033]52;c;?\033\\"});
    }

}  // namespace tapioca::pal
