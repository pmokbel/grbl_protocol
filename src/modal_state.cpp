#include "grbl_protocol/modal_state.hpp"

#include "detail/num_parse.hpp"

namespace grbl_protocol {
namespace {

using detail::parse_float;
using detail::parse_int;

void apply_g_token(std::string_view rest, ModalState& m) {
    // Motion
    if (rest == "0")    { m.motion = MotionMode::G0;    return; }
    if (rest == "1")    { m.motion = MotionMode::G1;    return; }
    if (rest == "2")    { m.motion = MotionMode::G2;    return; }
    if (rest == "3")    { m.motion = MotionMode::G3;    return; }
    if (rest == "80")   { m.motion = MotionMode::G80;   return; }
    if (rest == "38.2") { m.motion = MotionMode::G38_2; return; }
    if (rest == "38.3") { m.motion = MotionMode::G38_3; return; }
    if (rest == "38.4") { m.motion = MotionMode::G38_4; return; }
    if (rest == "38.5") { m.motion = MotionMode::G38_5; return; }

    // Plane
    if (rest == "17") { m.plane = Plane::XY; return; }
    if (rest == "18") { m.plane = Plane::XZ; return; }
    if (rest == "19") { m.plane = Plane::YZ; return; }

    // Units
    if (rest == "20") { m.units = Units::Inches; return; }
    if (rest == "21") { m.units = Units::Mm;     return; }

    // WCS
    if (rest == "54")   { m.wcs = WCS::G54;   return; }
    if (rest == "55")   { m.wcs = WCS::G55;   return; }
    if (rest == "56")   { m.wcs = WCS::G56;   return; }
    if (rest == "57")   { m.wcs = WCS::G57;   return; }
    if (rest == "58")   { m.wcs = WCS::G58;   return; }
    if (rest == "59")   { m.wcs = WCS::G59;   return; }
    if (rest == "59.1") { m.wcs = WCS::G59_1; return; }
    if (rest == "59.2") { m.wcs = WCS::G59_2; return; }
    if (rest == "59.3") { m.wcs = WCS::G59_3; return; }

    // Distance mode
    if (rest == "90") { m.distance = DistanceMode::Absolute;    return; }
    if (rest == "91") { m.distance = DistanceMode::Incremental; return; }

    // Feed rate mode
    if (rest == "93") { m.feed_rate_mode = FeedRateMode::InverseTime;        return; }
    if (rest == "94") { m.feed_rate_mode = FeedRateMode::UnitsPerMinute;     return; }
    if (rest == "95") { m.feed_rate_mode = FeedRateMode::UnitsPerRevolution; return; }

    // Unknown G-code -- silently ignored for forward-compat.
}

void apply_m_token(std::string_view rest, ModalState& m) {
    // Program flow
    if (rest == "0")  { m.program = ProgramFlow::Pause;         return; }
    if (rest == "1")  { m.program = ProgramFlow::OptionalPause; return; }
    if (rest == "2")  { m.program = ProgramFlow::Stop;          return; }
    if (rest == "30") { m.program = ProgramFlow::Reset;         return; }

    // Spindle
    if (rest == "3") { m.spindle = SpindleMode::Cw;  return; }
    if (rest == "4") { m.spindle = SpindleMode::Ccw; return; }
    if (rest == "5") { m.spindle = SpindleMode::Off; return; }

    // Coolant
    if (rest == "7") { m.coolant = CoolantMode::Mist;  return; }
    if (rest == "8") { m.coolant = CoolantMode::Flood; return; }
    if (rest == "9") { m.coolant = CoolantMode::Off;   return; }

    // Unknown M-code -- silently ignored.
}

bool is_separator(char c) {
    return c == ',' || c == ' ' || c == '\t';
}

} // namespace

ModalState parse_modal_state(std::string_view s) {
    ModalState m{};
    while (!s.empty()) {
        while (!s.empty() && is_separator(s.front())) s.remove_prefix(1);
        if (s.empty()) break;

        std::size_t end = 0;
        while (end < s.size() && !is_separator(s[end])) ++end;
        auto tok = s.substr(0, end);
        s.remove_prefix(end);

        if (tok.size() < 2) continue;
        char head = tok.front();
        auto rest = tok.substr(1);

        switch (head) {
            case 'G': case 'g': apply_g_token(rest, m); break;
            case 'M': case 'm': apply_m_token(rest, m); break;
            case 'T': case 't': if (auto v = parse_int(rest))   m.tool          = v; break;
            case 'F': case 'f': if (auto v = parse_float(rest)) m.feed          = v; break;
            case 'S': case 's': if (auto v = parse_float(rest)) m.spindle_speed = v; break;
            default: break;
        }
    }
    return m;
}

const char* name(MotionMode m) {
    switch (m) {
        case MotionMode::G0:    return "G0";
        case MotionMode::G1:    return "G1";
        case MotionMode::G2:    return "G2";
        case MotionMode::G3:    return "G3";
        case MotionMode::G38_2: return "G38.2";
        case MotionMode::G38_3: return "G38.3";
        case MotionMode::G38_4: return "G38.4";
        case MotionMode::G38_5: return "G38.5";
        case MotionMode::G80:   return "G80";
    }
    return "?";
}

const char* name(Plane p) {
    switch (p) {
        case Plane::XY: return "XY";
        case Plane::XZ: return "XZ";
        case Plane::YZ: return "YZ";
    }
    return "?";
}

const char* name(Units u) {
    switch (u) {
        case Units::Inches: return "in";
        case Units::Mm:     return "mm";
    }
    return "?";
}

const char* name(WCS w) {
    switch (w) {
        case WCS::G54:   return "G54";
        case WCS::G55:   return "G55";
        case WCS::G56:   return "G56";
        case WCS::G57:   return "G57";
        case WCS::G58:   return "G58";
        case WCS::G59:   return "G59";
        case WCS::G59_1: return "G59.1";
        case WCS::G59_2: return "G59.2";
        case WCS::G59_3: return "G59.3";
    }
    return "?";
}

const char* name(DistanceMode d) {
    switch (d) {
        case DistanceMode::Absolute:    return "absolute";
        case DistanceMode::Incremental: return "incremental";
    }
    return "?";
}

const char* name(FeedRateMode f) {
    switch (f) {
        case FeedRateMode::InverseTime:        return "inverse-time";
        case FeedRateMode::UnitsPerMinute:     return "units-per-min";
        case FeedRateMode::UnitsPerRevolution: return "units-per-rev";
    }
    return "?";
}

const char* name(SpindleMode s) {
    switch (s) {
        case SpindleMode::Off: return "off";
        case SpindleMode::Cw:  return "CW";
        case SpindleMode::Ccw: return "CCW";
    }
    return "?";
}

const char* name(CoolantMode c) {
    switch (c) {
        case CoolantMode::Off:   return "off";
        case CoolantMode::Mist:  return "mist";
        case CoolantMode::Flood: return "flood";
    }
    return "?";
}

const char* name(ProgramFlow p) {
    switch (p) {
        case ProgramFlow::Pause:         return "pause";
        case ProgramFlow::OptionalPause: return "optional-pause";
        case ProgramFlow::Stop:          return "stop";
        case ProgramFlow::Reset:         return "reset";
    }
    return "?";
}

} // namespace grbl_protocol
