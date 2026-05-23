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
    if (ec != std::errc{} || ptr != s.data() + s.size()) return std::nullopt;
    return v;
}

std::optional<Position> parse_position(std::string_view s) {
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
    // Axes beyond 6 are silently truncated -- forward-compat with any
    // future fork that extends past XYZABC.
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

PinFlags parse_pins(std::string_view s) {
    PinFlags p{};
    for (char c : s) {
        switch (c) {
            case 'X': p.x = true;            break;
            case 'Y': p.y = true;            break;
            case 'Z': p.z = true;            break;
            case 'P': p.probe = true;        break;
            case 'D': p.door = true;         break;
            case 'H': p.hold = true;         break;
            case 'R': p.soft_reset = true;   break;
            case 'S': p.cycle_start = true;  break;
            default: break;
        }
    }
    return p;
}

AccessoryState parse_accessory(std::string_view s) {
    AccessoryState a{};
    for (char c : s) {
        switch (c) {
            case 'S': a.spindle_cw  = true; break;
            case 'C': a.spindle_ccw = true; break;
            case 'F': a.flood       = true; break;
            case 'M': a.mist        = true; break;
            default: break;
        }
    }
    return a;
}

std::optional<Overrides> parse_overrides(std::string_view s) {
    auto c1 = s.find(',');
    if (c1 == std::string_view::npos) return std::nullopt;
    auto c2 = s.find(',', c1 + 1);
    if (c2 == std::string_view::npos) return std::nullopt;
    if (s.find(',', c2 + 1) != std::string_view::npos) return std::nullopt;

    auto f  = parse_int(s.substr(0, c1));
    auto r  = parse_int(s.substr(c1 + 1, c2 - c1 - 1));
    auto sp = parse_int(s.substr(c2 + 1));
    if (!f || !r || !sp) return std::nullopt;
    return Overrides{
        static_cast<std::uint8_t>(*f),
        static_cast<std::uint8_t>(*r),
        static_cast<std::uint8_t>(*sp),
    };
}

std::optional<BufferState> parse_buffer(std::string_view s) {
    auto comma = s.find(',');
    if (comma == std::string_view::npos) return std::nullopt;
    if (s.find(',', comma + 1) != std::string_view::npos) return std::nullopt;

    auto planner = parse_int(s.substr(0, comma));
    auto rx      = parse_int(s.substr(comma + 1));
    if (!planner || !rx) return std::nullopt;
    return BufferState{
        static_cast<std::uint16_t>(*planner),
        static_cast<std::uint16_t>(*rx),
    };
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
            } else if (key == "Pn") {
                report.pins = parse_pins(val);
            } else if (key == "WCO") {
                auto w = parse_position(val);
                if (!w) return std::nullopt;
                report.wco = w;
            } else if (key == "Ov") {
                auto o = parse_overrides(val);
                if (!o) return std::nullopt;
                report.ov = o;
            } else if (key == "A") {
                report.a = parse_accessory(val);
            } else if (key == "Bf") {
                auto b = parse_buffer(val);
                if (!b) return std::nullopt;
                report.bf = b;
            } else if (key == "Ln") {
                auto n = parse_int(val);
                if (!n) return std::nullopt;
                report.line_number = n;
            } else if (key == "G") {
                report.modals = parse_modal_state(val);
            }
        }

        if (bar == std::string_view::npos) break;
        line.remove_prefix(bar + 1);
    }

    if (!saw_state) return std::nullopt;
    return report;
}

const char* name(MachineState s) {
    switch (s) {
        case MachineState::Idle:  return "Idle";
        case MachineState::Run:   return "Run";
        case MachineState::Hold:  return "Hold";
        case MachineState::Jog:   return "Jog";
        case MachineState::Alarm: return "Alarm";
        case MachineState::Door:  return "Door";
        case MachineState::Check: return "Check";
        case MachineState::Home:  return "Home";
        case MachineState::Sleep: return "Sleep";
    }
    return "?";
}

} // namespace grbl_protocol
