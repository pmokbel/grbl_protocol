#include "grbl_protocol/response.hpp"

#include "detail/num_parse.hpp"

namespace grbl_protocol {

std::optional<Response> parse_response(std::string_view line) {
    if (line == "ok") return Response{ResponseKind::Ok, std::nullopt};

    auto try_prefixed = [&](std::string_view prefix, ResponseKind kind) -> std::optional<Response> {
        if (line.size() <= prefix.size()) return std::nullopt;
        if (line.substr(0, prefix.size()) != prefix) return std::nullopt;
        auto code = detail::parse_int(line.substr(prefix.size()));
        if (!code) return std::nullopt;
        return Response{kind, code};
    };

    if (auto r = try_prefixed("error:", ResponseKind::Error)) return r;
    if (auto r = try_prefixed("ALARM:", ResponseKind::Alarm)) return r;
    return std::nullopt;
}

const char* name(ResponseKind k) {
    switch (k) {
        case ResponseKind::Ok:    return "ok";
        case ResponseKind::Error: return "error";
        case ResponseKind::Alarm: return "ALARM";
    }
    return "?";
}

} // namespace grbl_protocol
