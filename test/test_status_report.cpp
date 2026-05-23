#include "grbl_protocol/status_report.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace grbl_protocol;
using Catch::Matchers::WithinAbs;

TEST_CASE("parses minimal Idle report", "[status_report]") {
    auto r = parse_status_report("<Idle|MPos:0.000,0.000,0.000|FS:0,0>");
    REQUIRE(r.has_value());
    REQUIRE(r->state == MachineState::Idle);
    REQUIRE_FALSE(r->sub_state.has_value());

    REQUIRE(r->mpos.has_value());
    REQUIRE_THAT(r->mpos->x, WithinAbs(0.0, 1e-6));
    REQUIRE_THAT(r->mpos->y, WithinAbs(0.0, 1e-6));
    REQUIRE_THAT(r->mpos->z, WithinAbs(0.0, 1e-6));

    REQUIRE(r->fs.has_value());
    REQUIRE_THAT(r->fs->feed,    WithinAbs(0.0, 1e-6));
    REQUIRE_THAT(r->fs->spindle, WithinAbs(0.0, 1e-6));
}

TEST_CASE("parses Hold with substate, WPos, and feed/spindle", "[status_report]") {
    auto r = parse_status_report("<Hold:0|WPos:1.234,-5.678,9.000|FS:500,8000>");
    REQUIRE(r.has_value());
    REQUIRE(r->state == MachineState::Hold);
    REQUIRE(r->sub_state == 0);

    REQUIRE(r->wpos.has_value());
    REQUIRE_THAT(r->wpos->x, WithinAbs(1.234, 1e-4));
    REQUIRE_THAT(r->wpos->y, WithinAbs(-5.678, 1e-4));
    REQUIRE_THAT(r->wpos->z, WithinAbs(9.0,    1e-6));

    REQUIRE(r->fs.has_value());
    REQUIRE_THAT(r->fs->feed,    WithinAbs(500.0,  1e-3));
    REQUIRE_THAT(r->fs->spindle, WithinAbs(8000.0, 1e-3));
}

TEST_CASE("ignores unknown fields like Ov and Pn", "[status_report]") {
    auto r = parse_status_report("<Run|MPos:0,0,0|FS:0,0|Ov:100,100,100|Pn:XYZ>");
    REQUIRE(r.has_value());
    REQUIRE(r->state == MachineState::Run);
    REQUIRE(r->mpos.has_value());
    REQUIRE(r->fs.has_value());
}

TEST_CASE("rejects malformed reports", "[status_report]") {
    REQUIRE_FALSE(parse_status_report("Idle|MPos:0,0,0").has_value());      // no angle brackets
    REQUIRE_FALSE(parse_status_report("<>").has_value());                   // empty body
    REQUIRE_FALSE(parse_status_report("<Bogus|MPos:0,0,0>").has_value());   // unknown state
    REQUIRE_FALSE(parse_status_report("<Idle|MPos:0,0>").has_value());      // too few axes
    REQUIRE_FALSE(parse_status_report("<Idle|MPos:a,b,c>").has_value());    // non-numeric
    REQUIRE_FALSE(parse_status_report("<Hold:x|FS:0,0>").has_value());      // bad substate
}
