#pragma once

#include "num_parse.hpp"

#include "grbl_protocol/status_report.hpp"  // Position

#include <optional>
#include <string_view>

namespace grbl_protocol::detail {

// Parses GRBL/FluidNC's positional, comma-separated coordinate list
// ("x,y,z" up to "x,y,z,a,b,c"). Shared by the status report
// (MPos/WPos/WCO) and the probe report so the variable-axis-count
// handling lives in exactly one place: 2..6 real axes recorded in
// Position::count, anything past XYZABC silently truncated for
// forward-compat. Returns nullopt on a non-numeric coordinate or an
// empty list.
inline std::optional<Position> parse_position(std::string_view s) {
    Position p{};
    while (!s.empty() && p.count < p.axes.size()) {
        auto comma = s.find(',');
        auto tok = comma == std::string_view::npos ? s : s.substr(0, comma);
        auto v = parse_float(tok);
        if (!v) return std::nullopt;
        p.axes[p.count++] = *v;
        if (comma == std::string_view::npos) break;
        s.remove_prefix(comma + 1);
    }
    if (p.count == 0) return std::nullopt;
    return p;
}

} // namespace grbl_protocol::detail
