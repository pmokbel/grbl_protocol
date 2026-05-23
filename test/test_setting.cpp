#include "grbl_protocol/setting.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace grbl_protocol;

TEST_CASE("parses numeric-key setting", "[settings]") {
    auto s = parse_setting("$10=1");
    REQUIRE(s.has_value());
    REQUIRE(s->key == "10");
    REQUIRE(s->value == "1");
}

TEST_CASE("preserves value formatting verbatim", "[settings]") {
    auto s = parse_setting("$130=1000.000");
    REQUIRE(s.has_value());
    REQUIRE(s->key == "130");
    REQUIRE(s->value == "1000.000");
}

TEST_CASE("parses FluidNC string-named settings", "[settings]") {
    auto a = parse_setting("$Config/Filename=config.yaml");
    REQUIRE(a.has_value());
    REQUIRE(a->key == "Config/Filename");
    REQUIRE(a->value == "config.yaml");

    auto b = parse_setting("$LocalFS/Show=/preferences.json");
    REQUIRE(b.has_value());
    REQUIRE(b->key == "LocalFS/Show");
    REQUIRE(b->value == "/preferences.json");

    auto c = parse_setting("$Spindle/Type=PWM");
    REQUIRE(c.has_value());
    REQUIRE(c->key == "Spindle/Type");
}

TEST_CASE("accepts classic GRBL startup-block form", "[settings]") {
    auto s = parse_setting("$N0=G54");
    REQUIRE(s.has_value());
    REQUIRE(s->key == "N0");
    REQUIRE(s->value == "G54");
}

TEST_CASE("accepts empty value", "[settings]") {
    auto s = parse_setting("$Foo=");
    REQUIRE(s.has_value());
    REQUIRE(s->key == "Foo");
    REQUIRE(s->value.empty());
}

TEST_CASE("rejects non-setting lines", "[settings]") {
    REQUIRE_FALSE(parse_setting("ok").has_value());
    REQUIRE_FALSE(parse_setting("$$").has_value());     // no '='
    REQUIRE_FALSE(parse_setting("$=5").has_value());    // empty key
    REQUIRE_FALSE(parse_setting("10=1").has_value());   // no leading '$'
    REQUIRE_FALSE(parse_setting("$ x=1").has_value()); // whitespace in key
}
