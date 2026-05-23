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

// Wire token for the response kind: "ok", "error", or "ALARM" (note the
// ALARM uppercase to match what the controller emits). Returned pointer
// is to a string literal; safe to store, never null.
const char* name(ResponseKind k);

} // namespace grbl_protocol
