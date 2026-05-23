#pragma once

#include "grbl_protocol/modal_state.hpp"

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace grbl_protocol {

enum class MachineState {
    Idle,
    Run,
    Hold,
    Jog,
    Alarm,
    Door,
    Check,
    Home,
    Sleep,
};

// GRBL/FluidNC supports up to 6 axes (XYZABC). `count` says how many of
// the leading entries in `axes` are real; the rest are zero-initialized
// but should not be read. Wire format is positional, so axis 0 == X,
// 1 == Y, 2 == Z, then ABC if present.
//
// The x()..c() accessors are convenience over axes[0..5]; they do NOT
// check `count`, so on a 3-axis machine pos.a() returns 0.0f rather
// than nullopt. Callers that care about axis presence should check
// count first.
struct Position {
    std::array<float, 6> axes{};
    std::uint8_t count{};

    constexpr float x() const { return axes[0]; }
    constexpr float y() const { return axes[1]; }
    constexpr float z() const { return axes[2]; }
    constexpr float a() const { return axes[3]; }
    constexpr float b() const { return axes[4]; }
    constexpr float c() const { return axes[5]; }
};

struct FeedSpindle {
    float feed{};
    float spindle{};
};

// Bit-per-pin view of GRBL's "Pn:" field. Unrecognized characters are
// silently ignored so future GRBL forks don't fail the parse.
struct PinFlags {
    bool x{};
    bool y{};
    bool z{};
    bool probe{};
    bool door{};
    bool hold{};
    bool soft_reset{};
    bool cycle_start{};
};

// "Ov:" feed/rapid/spindle override percentages (typical range 0..200).
struct Overrides {
    std::uint8_t feed{};
    std::uint8_t rapid{};
    std::uint8_t spindle{};
};

// "A:" accessory state -- a flag bag identical in shape to PinFlags.
// Unrecognized characters are silently ignored.
struct AccessoryState {
    bool spindle_cw{};   // S
    bool spindle_ccw{};  // C
    bool flood{};        // F
    bool mist{};         // M
};

// "Bf:" planner / serial-rx buffer state.
struct BufferState {
    std::uint16_t planner_blocks{};
    std::uint16_t rx_bytes{};
};

struct StatusReport {
    MachineState state{};
    std::optional<int> sub_state;
    std::optional<Position> mpos;
    std::optional<Position> wpos;
    std::optional<Position> wco;
    std::optional<FeedSpindle> fs;
    std::optional<PinFlags> pins;
    std::optional<Overrides> ov;
    std::optional<AccessoryState> a;
    std::optional<BufferState> bf;
    std::optional<int> line_number;
    std::optional<ModalState> modals;  // populated from the `G:` field if present
};

std::optional<StatusReport> parse_status_report(std::string_view line);

// Canonical short label for the enum value. For enums whose wire token
// IS the natural name (MachineState, ResponseKind, MotionMode, WCS),
// name() returns the exact wire string -- so it round-trips with what
// FluidNC/GRBL sends. Returned pointer is to a string literal; safe to
// store, never null.
const char* name(MachineState s);

} // namespace grbl_protocol
