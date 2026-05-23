#include "grbl_protocol/push_message.hpp"

namespace grbl_protocol {

std::optional<PushMessage> parse_push_message(std::string_view line) {
    if (line.size() < 3 || line.front() != '[' || line.back() != ']') return std::nullopt;
    auto body = line.substr(1, line.size() - 2);
    auto colon = body.find(':');
    if (colon == std::string_view::npos || colon == 0) return std::nullopt;
    return PushMessage{body.substr(0, colon), body.substr(colon + 1)};
}

} // namespace grbl_protocol
