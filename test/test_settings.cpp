#include "grbl_protocol/settings.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace grbl_protocol;

TEST_CASE("parses int setting", "[settings]") {
    auto s = parse_setting("$10=1");
    REQUIRE(s.has_value());
    REQUIRE(s->number == 10);
    REQUIRE(s->value == "1");
}

TEST_CASE("parses float setting and preserves formatting", "[settings]") {
    auto s = parse_setting("$130=1000.000");
    REQUIRE(s.has_value());
    REQUIRE(s->number == 130);
    REQUIRE(s->value == "1000.000");
}

TEST_CASE("rejects $N0 startup-block form (non-numeric key)", "[settings]") {
    REQUIRE_FALSE(parse_setting("$N0=G54").has_value());
}

TEST_CASE("rejects non-setting lines", "[settings]") {
    REQUIRE_FALSE(parse_setting("ok").has_value());
    REQUIRE_FALSE(parse_setting("$$").has_value());
    REQUIRE_FALSE(parse_setting("$=5").has_value());
    REQUIRE_FALSE(parse_setting("$abc=1").has_value());
    REQUIRE_FALSE(parse_setting("10=1").has_value());
}
