#pragma once

#include <string_view>

// Cheap shape classifier for incoming lines from a GRBL/FluidNC
// controller. Recognizes the four frame shapes this library parses:
//
//   "<...>"             status report
//   "[...]"             push message (incl. [MSG:...])
//   "ok"/"error:"/...   response
//   "$key=value"        setting
//
// This is a *shape* check, not a *validity* check -- classify("<oops")
// still returns LineKind::Status. Use it to dispatch to the right
// parse_*() function, or to filter unsolicited push frames out of a
// streamed response body. The parse_*() functions do the actual
// validation.
//
// Caller should strip the trailing CR/LF before calling; empty lines
// classify as Unknown.

namespace grbl_protocol {

enum class LineKind {
    Status,
    PushMessage,
    Response,
    Setting,
    Unknown,
};

constexpr LineKind classify(std::string_view line) {
    if (line.empty()) return LineKind::Unknown;
    switch (line.front()) {
        case '<': return LineKind::Status;
        case '[': return LineKind::PushMessage;
        case '$': return LineKind::Setting;
        default:  break;
    }
    if (line == "ok") return LineKind::Response;
    if (line.size() >= 6) {
        auto prefix = line.substr(0, 6);
        if (prefix == "error:" || prefix == "ALARM:") return LineKind::Response;
    }
    return LineKind::Unknown;
}

} // namespace grbl_protocol
