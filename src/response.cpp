#include "grbl_protocol/response.hpp"

#include <charconv>

namespace grbl_protocol {
namespace {

std::optional<int> parse_code(std::string_view s) {
    int v{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || ptr != s.data() + s.size()) return std::nullopt;
    return v;
}

} // namespace

std::optional<Response> parse_response(std::string_view line) {
    if (line == "ok") return Response{ResponseKind::Ok, std::nullopt};

    auto try_prefixed = [&](std::string_view prefix, ResponseKind kind) -> std::optional<Response> {
        if (line.size() <= prefix.size()) return std::nullopt;
        if (line.substr(0, prefix.size()) != prefix) return std::nullopt;
        auto code = parse_code(line.substr(prefix.size()));
        if (!code) return std::nullopt;
        return Response{kind, code};
    };

    if (auto r = try_prefixed("error:", ResponseKind::Error)) return r;
    if (auto r = try_prefixed("ALARM:", ResponseKind::Alarm)) return r;
    return std::nullopt;
}

std::string_view alarm_description(int code) {
    switch (code) {
        case 1:  return "Hard limit triggered";
        case 2:  return "Soft limit exceeded";
        case 3:  return "Cycle aborted (reset while in motion)";
        case 4:  return "Probe fail (probe already triggered before cycle)";
        case 5:  return "Probe fail (probe did not contact workpiece)";
        case 6:  return "Homing fail (reset during homing)";
        case 7:  return "Homing fail (door opened during homing)";
        case 8:  return "Homing fail (could not clear limit on pull-off)";
        case 9:  return "Homing fail (could not find limit switch)";
        case 10: return "Spindle control error";
        case 11: return "Startup pin asserted (control pin active at boot)";
        case 12: return "Homing fail (ambiguous limit switch)";
        case 13: return "Hard stop (motion stopped abnormally)";
        case 14: return "Machine not homed (homing required)";
        case 15: return "Initialization alarm";
        case 16: return "Pin expander reset";
        case 17: return "G-code error during execution";
        case 18: return "Probe triggered a hard limit";
        default: return "Unknown alarm code";
    }
}

std::string_view error_description(int code) {
    switch (code) {
        case 0:   return "No error";
        case 1:   return "Expected G-code command letter";
        case 2:   return "Bad G-code number format";
        case 3:   return "Invalid $ statement";
        case 4:   return "Negative value not allowed";
        case 5:   return "Setting disabled";
        case 6:   return "Step pulse too short";
        case 7:   return "Failed to read settings";
        case 8:   return "Command requires idle state";
        case 9:   return "G-code locked out during alarm or jog";
        case 10:  return "Soft limit error";
        case 11:  return "Line too long";
        case 12:  return "Max step rate exceeded";
        case 13:  return "Check door";
        case 14:  return "Line too long";
        case 15:  return "Max travel exceeded during jog";
        case 16:  return "Invalid jog command";
        case 17:  return "Laser mode requires PWM output";
        case 18:  return "No Homing/Cycle defined in settings";
        case 19:  return "Single-axis homing not allowed";
        case 20:  return "Unsupported G-code command";
        case 21:  return "G-code modal group violation";
        case 22:  return "G-code undefined feed rate";
        case 23:  return "G-code command value not integer";
        case 24:  return "G-code axis command conflict";
        case 25:  return "G-code word repeated";
        case 26:  return "G-code no axis words";
        case 27:  return "G-code invalid line number";
        case 28:  return "G-code value word missing";
        case 29:  return "G-code unsupported coordinate system";
        case 30:  return "G-code G53 invalid motion mode";
        case 31:  return "G-code extra axis words";
        case 32:  return "G-code no axis words in plane";
        case 33:  return "G-code invalid target";
        case 34:  return "G-code arc radius error";
        case 35:  return "G-code no offsets in plane";
        case 36:  return "G-code unused words";
        case 37:  return "G-code G43 dynamic axis error";
        case 38:  return "G-code max value exceeded";
        case 39:  return "P parameter max exceeded";
        case 40:  return "Check startup pins";
        case 60:  return "Failed to mount device";
        case 61:  return "Read failed";
        case 62:  return "Failed to open directory";
        case 63:  return "Directory not found";
        case 64:  return "File empty";
        case 65:  return "File not found";
        case 66:  return "Failed to open file";
        case 67:  return "Device is busy";
        case 68:  return "Failed to delete directory";
        case 69:  return "Failed to delete file";
        case 70:  return "Failed to rename file";
        case 80:  return "Number out of range for setting";
        case 81:  return "Invalid value for setting";
        case 82:  return "Failed to create file";
        case 83:  return "Failed to format filesystem";
        case 90:  return "Failed to send message";
        case 100: return "Failed to store setting";
        case 101: return "Failed to get setting status";
        case 110: return "Authentication failed";
        case 111: return "End of line";
        case 112: return "End of file";
        case 113: return "System reset";
        case 114: return "No data";
        case 120: return "Another interface is busy";
        case 130: return "Jog cancelled";
        case 150: return "Bad pin specification";
        case 151: return "Bad runtime config setting";
        case 152: return "Configuration is invalid (check boot messages)";
        case 160: return "File upload failed";
        case 161: return "File download failed";
        case 162: return "Read-only setting";
        case 170: return "Expression divide by zero";
        case 171: return "Expression invalid argument";
        case 172: return "Expression invalid result";
        case 173: return "Expression unknown operator";
        case 174: return "Expression argument out of range";
        case 175: return "Expression syntax error";
        case 176: return "Flow control syntax error";
        case 177: return "Flow control not executing macro";
        case 178: return "Flow control out of memory";
        case 179: return "Flow control stack overflow";
        case 180: return "Parameter assignment failed";
        case 181: return "G-code invalid word value";
        default:  return "Unknown error code";
    }
}

} // namespace grbl_protocol
