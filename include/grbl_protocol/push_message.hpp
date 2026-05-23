#pragma once

#include <optional>
#include <string_view>

namespace grbl_protocol {

// Backed by the caller's input buffer — view, do not store past it.
struct PushMessage {
    std::string_view key;
    std::string_view value;
};

std::optional<PushMessage> parse_push_message(std::string_view line);

} // namespace grbl_protocol
