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

struct StatusReport {
    MachineState state{};
    std::optional<int> sub_state;
    std::optional<Position> mpos;
    std::optional<Position> wpos;
    std::optional<FeedSpindle> fs;
};

std::optional<StatusReport> parse_status_report(std::string_view line);

} // namespace grbl_protocol
