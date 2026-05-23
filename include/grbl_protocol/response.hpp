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

// FluidNC-flavored ALARM:N description (ExecAlarm enum, codes 1..18).
// Diverges from classic GRBL v1.1 starting at code 10 (SpindleControl,
// not EStop) and extends beyond it. Returns "Unknown alarm code" for
// codes outside the known set.
std::string_view alarm_description(int code);

// FluidNC-flavored error:N description (Error enum). Covers the dense
// 0..40 block and the sparse FS / NVS / expression / flow-control
// regions. Returns "Unknown error code" for unmapped values.
std::string_view error_description(int code);

} // namespace grbl_protocol
