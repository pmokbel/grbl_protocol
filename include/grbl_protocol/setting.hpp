#pragma once

#include <optional>
#include <string_view>

namespace grbl_protocol {

// Backed by the caller's input buffer — view, do not store past it.
// `key` is the raw text between `$` and `=`; callers that want a numeric
// setting id (classic GRBL $22, $130 etc.) parse it themselves via
// std::from_chars on `key`.
struct Setting {
    std::string_view key;
    std::string_view value;
};

std::optional<Setting> parse_setting(std::string_view line);

} // namespace grbl_protocol
