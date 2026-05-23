#pragma once

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

struct Position {
    float x{};
    float y{};
    float z{};
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
