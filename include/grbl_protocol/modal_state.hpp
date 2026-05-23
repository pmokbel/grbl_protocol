#pragma once

#include <optional>
#include <string_view>

namespace grbl_protocol {

enum class MotionMode {
    G0,     // rapid
    G1,     // linear feed
    G2,     // arc CW
    G3,     // arc CCW
    G38_2,  // probe toward, error if no contact
    G38_3,  // probe toward, no error
    G38_4,  // probe away, error if no break
    G38_5,  // probe away, no error
    G80,    // motion canceled
};

enum class Plane {
    XY,  // G17
    XZ,  // G18
    YZ,  // G19
};

enum class Units {
    Inches,  // G20
    Mm,      // G21
};

enum class WCS {
    G54,
    G55,
    G56,
    G57,
    G58,
    G59,
    G59_1,
    G59_2,
    G59_3,
};

enum class DistanceMode {
    Absolute,     // G90
    Incremental,  // G91
};

enum class FeedRateMode {
    InverseTime,         // G93
    UnitsPerMinute,      // G94
    UnitsPerRevolution,  // G95
};

enum class SpindleMode {
    Off,  // M5
    Cw,   // M3
    Ccw,  // M4
};

enum class CoolantMode {
    Off,    // M9
    Mist,   // M7
    Flood,  // M8
};

enum class ProgramFlow {
    Pause,          // M0
    OptionalPause,  // M1
    Stop,           // M2
    Reset,          // M30
};

// Decoded snapshot of GRBL/FluidNC modal state. Sources on the wire:
//  - `[GC:G0 G54 G17 G21 G90 G94 M5 M9 T0 F0 S0]` push, replied to `$G`
//  - `G:G0,G54,G17,...` field inside a `?` status report (when enabled by
//    the controller's status mask)
// Both use the same set of tokens but different separators (space vs
// comma) -- parse_modal_state handles either.
//
// Every field is optional because a single line may only carry a subset;
// nullopt means "this report didn't mention it," not "the controller
// doesn't have this state."
struct ModalState {
    std::optional<MotionMode>    motion;
    std::optional<Plane>         plane;
    std::optional<Units>         units;
    std::optional<WCS>           wcs;
    std::optional<DistanceMode>  distance;
    std::optional<FeedRateMode>  feed_rate_mode;
    std::optional<SpindleMode>   spindle;
    std::optional<CoolantMode>   coolant;
    std::optional<ProgramFlow>   program;
    std::optional<int>           tool;
    std::optional<float>         feed;
    std::optional<float>         spindle_speed;
};

ModalState parse_modal_state(std::string_view s);

} // namespace grbl_protocol