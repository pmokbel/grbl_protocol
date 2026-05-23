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
