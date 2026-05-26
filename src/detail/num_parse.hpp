#pragma once

#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>
#include <system_error>

namespace grbl_protocol::detail {

// std::from_chars wrapper shared by every parser in this library.
// Succeeds only when the WHOLE view is consumed, so trailing junk
// ("1.5x", "12a") is rejected -- the "not stupid" half of the parsers'
// permissive stance. For the unsigned-integer instantiations,
// std::from_chars reports values that don't fit the target type as
// result_out_of_range, which gives range-checking for free (e.g.
// parse_u8("999") == nullopt rather than a silent wrap to 231).
template <typename T>
std::optional<T> from_chars_exact(std::string_view s) {
    T v{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || ptr != s.data() + s.size()) return std::nullopt;
    return v;
}

inline std::optional<int>           parse_int(std::string_view s)   { return from_chars_exact<int>(s); }
inline std::optional<std::uint8_t>  parse_u8(std::string_view s)    { return from_chars_exact<std::uint8_t>(s); }
inline std::optional<std::uint16_t> parse_u16(std::string_view s)   { return from_chars_exact<std::uint16_t>(s); }
inline std::optional<float>         parse_float(std::string_view s) { return from_chars_exact<float>(s); }

} // namespace grbl_protocol::detail
