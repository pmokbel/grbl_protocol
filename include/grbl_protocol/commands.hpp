#pragma once

#include <cstddef>
#include <cstdio>
#include <optional>
#include <string_view>

// GRBL v1.1 / FluidNC system commands sent over the line-buffered
// channel (ASCII, caller appends '\n'). Contrast with realtime.hpp,
// which holds the single-byte out-of-band commands.
//
// Use:
//   send_line(commands::ModalQuery);            // "$G"
//   char buf[64];
//   auto n = commands::format_jog(buf, sizeof(buf),
//       { .x = 10.0f, .feed = 500.0f, .relative = true });
//   if (n) send_line({buf, n});
//
// All constants are universal across classic GRBL v1.1, grblHAL, and
// FluidNC unless noted.

namespace grbl_protocol::commands {

// --- Bare query / info -------------------------------------------------

inline constexpr std::string_view Help              = "$";   // help text
inline constexpr std::string_view ListSettings      = "$$";  // settings
inline constexpr std::string_view ListParameters    = "$#";  // WCS/G92/TLO/probe
inline constexpr std::string_view ModalQuery        = "$G";  // [GC:...]
inline constexpr std::string_view BuildInfo         = "$I";  // [VER:][OPT:]
inline constexpr std::string_view ListStartupBlocks = "$N";  // $N0=, $N1=

// --- Bare control ------------------------------------------------------

inline constexpr std::string_view Home              = "$H";  // home all
inline constexpr std::string_view Unlock            = "$X";  // clear alarm
inline constexpr std::string_view CheckModeToggle   = "$C";  // dry-run toggle

// FluidNC + grblHAL extension; not in classic GRBL v1.1.
inline constexpr std::string_view Sleep             = "$SLP";

// --- Destructive: factory restores -------------------------------------
// These wipe state on the controller. Confirm with the operator first.

inline constexpr std::string_view ResetSettings     = "$RST=$";  // $-settings
inline constexpr std::string_view ResetParameters   = "$RST=#";  // WCS/G92/TLO
inline constexpr std::string_view ResetAll          = "$RST=*";  // everything

// --- Parameterized builders -------------------------------------------
//
// All builders write a NUL-free ASCII command into a caller-provided
// buffer (NOT newline-terminated; caller appends '\n'). Return value
// is the number of bytes written, or 0 on overflow or invalid input.
// No heap allocation — safe for embedded.

// Jog: $J=[G90|G91][G21|G20][G53] [X..][Y..][Z..][A..][B..][C..] F<rate>
//
// At least one axis must be set and feed must be > 0; otherwise 0 is
// returned. Defaults match the common interactive-jog case: incremental
// (G91), millimetres (G21), work coords (no G53).
struct JogParams {
    std::optional<float> x, y, z, a, b, c;
    float feed     = 0.0f;
    bool  relative = true;   // G91 (true) vs G90 (false)
    bool  mm       = true;   // G21 (true) vs G20 (false)
    bool  machine  = false;  // adds G53
};

inline std::size_t format_jog(char* buf, std::size_t cap,
                              const JogParams& p) {
    if (!buf || cap == 0) return 0;
    if (p.feed <= 0.0f) return 0;
    if (!p.x && !p.y && !p.z && !p.a && !p.b && !p.c) return 0;

    std::size_t n = 0;
    auto write = [&](const char* fmt, auto... args) -> bool {
        int m = std::snprintf(buf + n, cap - n, fmt, args...);
        if (m < 0 || static_cast<std::size_t>(m) >= cap - n) return false;
        n += static_cast<std::size_t>(m);
        return true;
    };

    if (!write("$J=%s%s%s",
               p.relative ? "G91" : "G90",
               p.mm       ? "G21" : "G20",
               p.machine  ? "G53" : "")) return 0;

    auto axis = [&](char letter, std::optional<float> v) -> bool {
        if (!v) return true;
        return write("%c%.3f", letter, *v);
    };
    if (!axis('X', p.x)) return 0;
    if (!axis('Y', p.y)) return 0;
    if (!axis('Z', p.z)) return 0;
    if (!axis('A', p.a)) return 0;
    if (!axis('B', p.b)) return 0;
    if (!axis('C', p.c)) return 0;

    if (!write("F%.3f", p.feed)) return 0;
    return n;
}

// Set or clear a startup block slot. GRBL exposes two slots (0, 1).
// Pass an empty `line` to clear. Returns 0 if slot is out of range or
// the buffer is too small.
inline std::size_t format_startup_block(char* buf, std::size_t cap,
                                        int slot, std::string_view line) {
    if (!buf || cap == 0) return 0;
    if (slot != 0 && slot != 1) return 0;
    int m = std::snprintf(buf, cap, "$N%d=%.*s",
                          slot,
                          static_cast<int>(line.size()),
                          line.data());
    if (m < 0 || static_cast<std::size_t>(m) >= cap) return 0;
    return static_cast<std::size_t>(m);
}

} // namespace grbl_protocol::commands