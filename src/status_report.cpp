#include "grbl_protocol/status_report.hpp"

#include <charconv>
#include <system_error>

namespace grbl_protocol {
namespace {

std::optional<MachineState> parse_state(std::string_view s) {
    if (s == "Idle")  return MachineState::Idle;
    if (s == "Run")   return MachineState::Run;
    if (s == "Hold")  return MachineState::Hold;
    if (s == "Jog")   return MachineState::Jog;
    if (s == "Alarm") return MachineState::Alarm;
    if (s == "Door")  return MachineState::Door;
    if (s == "Check") return MachineState::Check;
    if (s == "Home")  return MachineState::Home;
    if (s == "Sleep") return MachineState::Sleep;
    return std::nullopt;
}

std::optional<float> parse_float(std::string_view s) {
    float v{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{}) return std::nullopt;
    return v;
}

std::optional<int> parse_int(std::string_view s) {
    int v{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{}) return std::nullopt;
    return v;
}

std::optional<Position> parse_position(std::string_view s) {
    Position p{};
    float* axes[] = { &p.x, &p.y, &p.z };
    int i = 0;
    while (!s.empty() && i < 3) {
        auto comma = s.find(',');
        auto tok = comma == std::string_view::npos ? s : s.substr(0, comma);
        auto v = parse_float(tok);
        if (!v) return std::nullopt;
        *axes[i++] = *v;
        if (comma == std::string_view::npos) break;
        s.remove_prefix(comma + 1);
    }
    if (i < 3) return std::nullopt;
    return p;
}

std::optional<FeedSpindle> parse_fs(std::string_view s) {
    auto comma = s.find(',');
    if (comma == std::string_view::npos) return std::nullopt;
    auto f = parse_float(s.substr(0, comma));
    auto sp = parse_float(s.substr(comma + 1));
    if (!f || !sp) return std::nullopt;
    return FeedSpindle{*f, *sp};
}

} // namespace

std::optional<StatusReport> parse_status_report(std::string_view line) {
    if (line.size() < 2 || line.front() != '<' || line.back() != '>') {
        return std::nullopt;
    }
    line.remove_prefix(1);
    line.remove_suffix(1);

    StatusReport report{};
    bool saw_state = false;

    while (!line.empty()) {
        auto bar = line.find('|');
        auto field = bar == std::string_view::npos ? line : line.substr(0, bar);

        if (!saw_state) {
            auto colon = field.find(':');
            auto name = colon == std::string_view::npos ? field : field.substr(0, colon);
            auto state = parse_state(name);
            if (!state) return std::nullopt;
            report.state = *state;
            if (colon != std::string_view::npos) {
                auto sub = parse_int(field.substr(colon + 1));
                if (!sub) return std::nullopt;
                report.sub_state = sub;
            }
            saw_state = true;
        } else {
            auto colon = field.find(':');
            if (colon == std::string_view::npos) return std::nullopt;
            auto key = field.substr(0, colon);
            auto val = field.substr(colon + 1);
            if (key == "MPos") {
                auto p = parse_position(val);
                if (!p) return std::nullopt;
                report.mpos = p;
            } else if (key == "WPos") {
                auto p = parse_position(val);
                if (!p) return std::nullopt;
                report.wpos = p;
            } else if (key == "FS") {
                auto fs = parse_fs(val);
                if (!fs) return std::nullopt;
                report.fs = fs;
            }
        }

        if (bar == std::string_view::npos) break;
        line.remove_prefix(bar + 1);
    }

    if (!saw_state) return std::nullopt;
    return report;
}

} // namespace grbl_protocol
