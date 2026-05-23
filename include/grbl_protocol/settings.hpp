#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace grbl_protocol {

struct Setting {
    int number{};
    std::string value;
};

std::optional<Setting> parse_setting(std::string_view line);

} // namespace grbl_protocol
