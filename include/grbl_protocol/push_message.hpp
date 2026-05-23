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

// Semantic classification of a [MSG:...] payload (the `value` returned
// by parse_push_message when `key == "MSG"`). Add new kinds here as new
// FluidNC patterns are observed in the wild.
enum class MessageKind {
    Welcome,    // "Reset to continue"
    AlarmHint,  // "'$H'|'$X' to unlock"
    Unlocked,   // "Caution: Unlocked"
    Homed,      // "Homed:<axes>"
    Error,      // "ERR: <text>"
    Info,       // "INFO: <text>"
    Warning,    // "WARN: <text>"
    Debug,      // "DBG: <text>"
    Critical,   // "CRIT: <text>" or "Critical Error: <text>"
    Other,      // anything not recognized; body is the original value
};

// Backed by the caller's input buffer — same lifetime rule as PushMessage.
struct DecodedMessage {
    MessageKind kind{};
    std::string_view body;
};

DecodedMessage decode_message(std::string_view msg_value);

} // namespace grbl_protocol
