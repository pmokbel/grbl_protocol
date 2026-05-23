#include "grbl_protocol/settings.hpp"

#include <charconv>

namespace grbl_protocol {

std::optional<Setting> parse_setting(std::string_view line) {
    if (line.size() < 4 || line.front() != '$') return std::nullopt;
    auto eq = line.find('=');
    if (eq == std::string_view::npos || eq < 2) return std::nullopt;

    auto num_sv = line.substr(1, eq - 1);
    int number{};
    auto [ptr, ec] = std::from_chars(num_sv.data(), num_sv.data() + num_sv.size(), number);
    if (ec != std::errc{} || ptr != num_sv.data() + num_sv.size()) return std::nullopt;

    return Setting{number, std::string(line.substr(eq + 1))};
}

} // namespace grbl_protocol
