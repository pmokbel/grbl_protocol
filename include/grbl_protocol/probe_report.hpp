#pragma once

#include "grbl_protocol/status_report.hpp"  // for Position

#include <optional>
#include <string_view>

namespace grbl_protocol {

// Result of a G38.x probing move, emitted by GRBL/FluidNC as a
// "[PRB:x,y,z:success]" push message when the move completes.
//
// Coordinates share Position's variable-axis-count layout with the status
// report (axis 0 == X, 1 == Y, 2 == Z, then ABC on 4/5/6-axis builds);
// `pos.count` says how many leading entries are real. Per the FluidNC
// docs the coordinates are in MACHINE space.
//
// `ok` reflects the trailing success bit: true when contact was made,
// false when the move completed without touching (in which case the
// controller will also raise an alarm -- reported separately).
struct ProbeReport {
    Position pos;   // contact coordinates in machine space
    bool ok{};      // true = contact made; false = move ended without touching
};

// Parses a "[PRB:...]" line. Mirrors parse_status_report's
// permissive-but-not-stupid stance: a missing/garbled success bit or a
// non-numeric coordinate fails the parse, while the axis count is handled
// exactly like the status report (2..6 real axes, anything past XYZABC
// silently truncated). The caller is expected to have stripped any
// trailing CR/LF first (see line_kind.hpp), like the rest of the
// parse_*() family. Returns nullopt on malformed input.
std::optional<ProbeReport> parse_probe_report(std::string_view line);

} // namespace grbl_protocol
