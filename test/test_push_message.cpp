#include "grbl_protocol/push_message.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace grbl_protocol;

TEST_CASE("parses [MSG:...] push", "[push_message]") {
    auto m = parse_push_message("[MSG:Machine: Default (Test Drive)]");
    REQUIRE(m.has_value());
    REQUIRE(m->key == "MSG");
    REQUIRE(m->value == "Machine: Default (Test Drive)");
}

TEST_CASE("parses [VER:...] and [OPT:...]", "[push_message]") {
    auto ver = parse_push_message("[VER:4.0 FluidNC v4.0.3 (esp32s3-wifi) :]");
    REQUIRE(ver.has_value());
    REQUIRE(ver->key == "VER");
    REQUIRE(ver->value == "4.0 FluidNC v4.0.3 (esp32s3-wifi) :");

    auto opt = parse_push_message("[OPT:PHS]");
    REQUIRE(opt.has_value());
    REQUIRE(opt->key == "OPT");
    REQUIRE(opt->value == "PHS");
}

TEST_CASE("rejects non-push lines", "[push_message]") {
    REQUIRE_FALSE(parse_push_message("ok").has_value());
    REQUIRE_FALSE(parse_push_message("<Idle>").has_value());
    REQUIRE_FALSE(parse_push_message("[no-colon]").has_value());
    REQUIRE_FALSE(parse_push_message("[:value]").has_value());
    REQUIRE_FALSE(parse_push_message("[MSG:unterminated").has_value());
}

TEST_CASE("decode_message recognizes welcome and alarm hint", "[push_message]") {
    auto w = decode_message("Reset to continue");
    REQUIRE(w.kind == MessageKind::Welcome);
    REQUIRE(w.body.empty());

    auto h = decode_message("'$H'|'$X' to unlock");
    REQUIRE(h.kind == MessageKind::AlarmHint);
    REQUIRE(h.body.empty());

    auto u = decode_message("Caution: Unlocked");
    REQUIRE(u.kind == MessageKind::Unlocked);
    REQUIRE(u.body.empty());
}

TEST_CASE("decode_message extracts Homed axes", "[push_message]") {
    auto z = decode_message("Homed:Z");
    REQUIRE(z.kind == MessageKind::Homed);
    REQUIRE(z.body == "Z");

    auto xy = decode_message("Homed:XY");
    REQUIRE(xy.kind == MessageKind::Homed);
    REQUIRE(xy.body == "XY");
}

TEST_CASE("decode_message handles severity-tagged messages", "[push_message]") {
    auto e = decode_message("ERR: Setting disabled");
    REQUIRE(e.kind == MessageKind::Error);
    REQUIRE(e.body == "Setting disabled");

    REQUIRE(decode_message("INFO: probe complete").kind == MessageKind::Info);
    REQUIRE(decode_message("INFO: probe complete").body == "probe complete");

    REQUIRE(decode_message("WARN: low voltage").kind   == MessageKind::Warning);
    REQUIRE(decode_message("DBG: spindle on").kind     == MessageKind::Debug);
    REQUIRE(decode_message("CRIT: fault").kind         == MessageKind::Critical);
    REQUIRE(decode_message("Critical Error: stack").kind == MessageKind::Critical);
    REQUIRE(decode_message("Critical Error: stack").body == "stack");
}

TEST_CASE("decode_message falls back to Other for unknown payloads", "[push_message]") {
    auto m = decode_message("Machine: Default (Test Drive)");
    REQUIRE(m.kind == MessageKind::Other);
    REQUIRE(m.body == "Machine: Default (Test Drive)");
}
