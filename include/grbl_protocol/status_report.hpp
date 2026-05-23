#pragma once

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
struct Position {
    std::array<float, 6> axes{};
    std::uint8_t count{};
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

struct StatusReport {
    MachineState state{};
    std::optional<int> sub_state;
    std::optional<Position> mpos;
    std::optional<Position> wpos;
    std::optional<FeedSpindle> fs;
    std::optional<PinFlags> pins;
};

std::optional<StatusReport> parse_status_report(std::string_view line);

} // namespace grbl_protocol
