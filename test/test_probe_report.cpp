#include "grbl_protocol/probe_report.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <string_view>

using namespace grbl_protocol;
using Catch::Matchers::WithinAbs;

TEST_CASE("parses 3-axis probe with contact made", "[probe_report]") {
    auto r = parse_probe_report("[PRB:0.000,0.000,-23.456:1]");
    REQUIRE(r.has_value());
    REQUIRE(r->ok);
    REQUIRE(r->pos.count == 3);
    REQUIRE_THAT(r->pos.x(), WithinAbs(0.0,     1e-6));
    REQUIRE_THAT(r->pos.y(), WithinAbs(0.0,     1e-6));
    REQUIRE_THAT(r->pos.z(), WithinAbs(-23.456, 1e-4));
}

TEST_CASE("parses 3-axis probe that made no contact", "[probe_report]") {
    auto r = parse_probe_report("[PRB:120.500,85.000,5.000:0]");
    REQUIRE(r.has_value());
    REQUIRE_FALSE(r->ok);
    REQUIRE(r->pos.count == 3);
    REQUIRE_THAT(r->pos.x(), WithinAbs(120.5, 1e-4));
    REQUIRE_THAT(r->pos.y(), WithinAbs(85.0,  1e-4));
    REQUIRE_THAT(r->pos.z(), WithinAbs(5.0,   1e-6));
}

TEST_CASE("parses 6-axis probe (XYZABC)", "[probe_report]") {
    auto r = parse_probe_report("[PRB:1.0,2.0,3.0,4.0,5.0,6.0:1]");
    REQUIRE(r.has_value());
    REQUIRE(r->ok);
    REQUIRE(r->pos.count == 6);
    REQUIRE_THAT(r->pos.a(), WithinAbs(4.0, 1e-6));
    REQUIRE_THAT(r->pos.b(), WithinAbs(5.0, 1e-6));
    REQUIRE_THAT(r->pos.c(), WithinAbs(6.0, 1e-6));
}

TEST_CASE("success bit toggles ok", "[probe_report]") {
    REQUIRE(parse_probe_report("[PRB:0,0,0:1]")->ok);
    REQUIRE_FALSE(parse_probe_report("[PRB:0,0,0:0]")->ok);
}

TEST_CASE("handles negative and fractional coordinates", "[probe_report]") {
    auto r = parse_probe_report("[PRB:-1.5,2.25,-0.001:1]");
    REQUIRE(r.has_value());
    REQUIRE_THAT(r->pos.x(), WithinAbs(-1.5,   1e-6));
    REQUIRE_THAT(r->pos.y(), WithinAbs(2.25,   1e-6));
    REQUIRE_THAT(r->pos.z(), WithinAbs(-0.001, 1e-6));
}

TEST_CASE("rejects malformed probe reports", "[probe_report]") {
    REQUIRE_FALSE(parse_probe_report("PRB:0,0,0:1").has_value());       // no brackets
    REQUIRE_FALSE(parse_probe_report("[PRB:0,0,0:1").has_value());      // no closing bracket
    REQUIRE_FALSE(parse_probe_report("[PRB:0,0,0]").has_value());       // missing :success bit
    REQUIRE_FALSE(parse_probe_report("[PRB:0,0,0:]").has_value());      // empty success bit
    REQUIRE_FALSE(parse_probe_report("[PRB::1]").has_value());          // empty coordinate list
    REQUIRE_FALSE(parse_probe_report("[PRB:a,b,c:1]").has_value());     // non-numeric coordinate
    REQUIRE_FALSE(parse_probe_report("[PRB:0,0,0:x]").has_value());     // non-numeric success bit
    REQUIRE_FALSE(parse_probe_report("[MSG:something]").has_value());   // wrong message type
    REQUIRE_FALSE(parse_probe_report("<Idle|MPos:0,0,0>").has_value()); // not a push message
    REQUIRE_FALSE(parse_probe_report("[PRB:0,0,0:1]\r").has_value());   // caller must strip CR/LF
}
