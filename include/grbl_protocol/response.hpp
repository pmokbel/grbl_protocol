#pragma once

#include <optional>
#include <string_view>

namespace grbl_protocol {

enum class ResponseKind {
    Ok,
    Error,
    Alarm,
};

struct Response {
    ResponseKind kind{};
    std::optional<int> code;
};

std::optional<Response> parse_response(std::string_view line);

// Maps a GRBL v1.1 ALARM:N code to a short human-readable reason.
// Returns "Unknown alarm code" for codes outside the v1.1 1..10 range.
std::string_view alarm_description(int code);

} // namespace grbl_protocol
