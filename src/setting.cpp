#include "grbl_protocol/setting.hpp"

namespace grbl_protocol {

std::optional<Setting> parse_setting(std::string_view line) {
    if (line.size() < 3 || line.front() != '$') return std::nullopt;
    auto eq = line.find('=');
    if (eq == std::string_view::npos || eq < 2) return std::nullopt;

    auto key = line.substr(1, eq - 1);
    // Reject keys with embedded whitespace — the GRBL/FluidNC wire format
    // never has them, and accepting would mask malformed lines like "$ x=1".
    for (char c : key) {
        if (c == ' ' || c == '\t') return std::nullopt;
    }

    return Setting{key, line.substr(eq + 1)};
}

} // namespace grbl_protocol
