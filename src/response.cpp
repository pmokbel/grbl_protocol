#include "grbl_protocol/response.hpp"

#include <charconv>

namespace grbl_protocol {
namespace {

std::optional<int> parse_code(std::string_view s) {
    int v{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), v);
    if (ec != std::errc{} || ptr != s.data() + s.size()) return std::nullopt;
    return v;
}

} // namespace

std::optional<Response> parse_response(std::string_view line) {
    if (line == "ok") return Response{ResponseKind::Ok, std::nullopt};

    auto try_prefixed = [&](std::string_view prefix, ResponseKind kind) -> std::optional<Response> {
        if (line.size() <= prefix.size()) return std::nullopt;
        if (line.substr(0, prefix.size()) != prefix) return std::nullopt;
        auto code = parse_code(line.substr(prefix.size()));
        if (!code) return std::nullopt;
        return Response{kind, code};
    };

    if (auto r = try_prefixed("error:", ResponseKind::Error)) return r;
    if (auto r = try_prefixed("ALARM:", ResponseKind::Alarm)) return r;
    return std::nullopt;
}

} // namespace grbl_protocol
