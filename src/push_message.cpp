#include "grbl_protocol/push_message.hpp"

namespace grbl_protocol {
namespace {

bool starts_with(std::string_view s, std::string_view prefix) {
    return s.size() >= prefix.size() && s.substr(0, prefix.size()) == prefix;
}

} // namespace

std::optional<PushMessage> parse_push_message(std::string_view line) {
    if (line.size() < 3 || line.front() != '[' || line.back() != ']') return std::nullopt;
    auto body = line.substr(1, line.size() - 2);
    auto colon = body.find(':');
    if (colon == std::string_view::npos || colon == 0) return std::nullopt;
    return PushMessage{body.substr(0, colon), body.substr(colon + 1)};
}

DecodedMessage decode_message(std::string_view v) {
    if (v == "Reset to continue")    return {MessageKind::Welcome,   {}};
    if (v == "'$H'|'$X' to unlock")  return {MessageKind::AlarmHint, {}};
    if (v == "Caution: Unlocked")    return {MessageKind::Unlocked,  {}};

    if (starts_with(v, "Homed:"))           return {MessageKind::Homed,    v.substr(6)};
    if (starts_with(v, "ERR: "))            return {MessageKind::Error,    v.substr(5)};
    if (starts_with(v, "INFO: "))           return {MessageKind::Info,     v.substr(6)};
    if (starts_with(v, "WARN: "))           return {MessageKind::Warning,  v.substr(6)};
    if (starts_with(v, "DBG: "))            return {MessageKind::Debug,    v.substr(5)};
    if (starts_with(v, "CRIT: "))           return {MessageKind::Critical, v.substr(6)};
    if (starts_with(v, "Critical Error: ")) return {MessageKind::Critical, v.substr(16)};

    return {MessageKind::Other, v};
}

} // namespace grbl_protocol
