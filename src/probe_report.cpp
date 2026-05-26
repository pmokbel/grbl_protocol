#include "grbl_protocol/probe_report.hpp"

#include "detail/num_parse.hpp"
#include "detail/position_parse.hpp"

namespace grbl_protocol {

std::optional<ProbeReport> parse_probe_report(std::string_view line) {
    // Frame is "[PRB:<coords>:<success>]".
    constexpr std::string_view kPrefix = "[PRB:";
    if (line.size() < kPrefix.size() + 1) return std::nullopt;   // need at least "[PRB:" + "]"
    if (line.substr(0, kPrefix.size()) != kPrefix) return std::nullopt;
    if (line.back() != ']') return std::nullopt;

    // Body is everything between "[PRB:" and the closing ']'.
    auto body = line.substr(kPrefix.size(), line.size() - kPrefix.size() - 1);

    // Coordinates never contain a colon, so the first one separates them
    // from the trailing success bit.
    auto colon = body.find(':');
    if (colon == std::string_view::npos) return std::nullopt;    // missing :success

    auto pos = detail::parse_position(body.substr(0, colon));
    if (!pos) return std::nullopt;

    auto bit = detail::parse_int(body.substr(colon + 1));
    if (!bit) return std::nullopt;

    return ProbeReport{*pos, *bit != 0};
}

} // namespace grbl_protocol
