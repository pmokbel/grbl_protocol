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

} // namespace grbl_protocol
