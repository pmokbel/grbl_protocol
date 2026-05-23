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
    REQUIRE(r->mpos->count == 3);
    REQUIRE_THAT(r->mpos->axes[0], WithinAbs(0.0, 1e-6));
    REQUIRE_THAT(r->mpos->axes[1], WithinAbs(0.0, 1e-6));
    REQUIRE_THAT(r->mpos->axes[2], WithinAbs(0.0, 1e-6));

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
    REQUIRE(r->wpos->count == 3);
    REQUIRE_THAT(r->wpos->axes[0], WithinAbs(1.234, 1e-4));
    REQUIRE_THAT(r->wpos->axes[1], WithinAbs(-5.678, 1e-4));
    REQUIRE_THAT(r->wpos->axes[2], WithinAbs(9.0,    1e-6));

    REQUIRE(r->fs.has_value());
    REQUIRE_THAT(r->fs->feed,    WithinAbs(500.0,  1e-3));
    REQUIRE_THAT(r->fs->spindle, WithinAbs(8000.0, 1e-3));
}

TEST_CASE("parses 4-axis report (XYZA)", "[status_report]") {
    auto r = parse_status_report("<Idle|MPos:1.0,2.0,3.0,4.0|FS:0,0>");
    REQUIRE(r.has_value());
    REQUIRE(r->mpos.has_value());
    REQUIRE(r->mpos->count == 4);
    REQUIRE_THAT(r->mpos->axes[0], WithinAbs(1.0, 1e-6));
    REQUIRE_THAT(r->mpos->axes[3], WithinAbs(4.0, 1e-6));
}

TEST_CASE("parses 6-axis report (XYZABC)", "[status_report]") {
    auto r = parse_status_report("<Idle|MPos:1.0,2.0,3.0,4.0,5.0,6.0>");
    REQUIRE(r.has_value());
    REQUIRE(r->mpos.has_value());
    REQUIRE(r->mpos->count == 6);
    REQUIRE_THAT(r->mpos->axes[5], WithinAbs(6.0, 1e-6));
}

TEST_CASE("parses 2-axis report (XY only)", "[status_report]") {
    auto r = parse_status_report("<Idle|MPos:10.0,20.0>");
    REQUIRE(r.has_value());
    REQUIRE(r->mpos.has_value());
    REQUIRE(r->mpos->count == 2);
    REQUIRE_THAT(r->mpos->axes[0], WithinAbs(10.0, 1e-6));
    REQUIRE_THAT(r->mpos->axes[1], WithinAbs(20.0, 1e-6));
}

TEST_CASE("truncates axes beyond 6", "[status_report]") {
    auto r = parse_status_report("<Idle|MPos:1,2,3,4,5,6,7,8>");
    REQUIRE(r.has_value());
    REQUIRE(r->mpos.has_value());
    REQUIRE(r->mpos->count == 6);
    REQUIRE_THAT(r->mpos->axes[5], WithinAbs(6.0, 1e-6));
}

TEST_CASE("parses Pn pins and ignores unknown fields like Ov", "[status_report]") {
    auto r = parse_status_report("<Run|MPos:0,0,0|FS:0,0|Ov:100,100,100|Pn:XYZP>");
    REQUIRE(r.has_value());
    REQUIRE(r->state == MachineState::Run);
    REQUIRE(r->mpos.has_value());
    REQUIRE(r->fs.has_value());

    REQUIRE(r->pins.has_value());
    REQUIRE(r->pins->x);
    REQUIRE(r->pins->y);
    REQUIRE(r->pins->z);
    REQUIRE(r->pins->probe);
    REQUIRE_FALSE(r->pins->door);
    REQUIRE_FALSE(r->pins->hold);
    REQUIRE_FALSE(r->pins->soft_reset);
    REQUIRE_FALSE(r->pins->cycle_start);
}

TEST_CASE("parses alarm with single tripped pin", "[status_report]") {
    auto r = parse_status_report("<Alarm|MPos:0.000,0.000,0.000|Pn:Z>");
    REQUIRE(r.has_value());
    REQUIRE(r->state == MachineState::Alarm);
    REQUIRE(r->pins.has_value());
    REQUIRE_FALSE(r->pins->x);
    REQUIRE_FALSE(r->pins->y);
    REQUIRE(r->pins->z);
}

TEST_CASE("Pn ignores unknown pin characters", "[status_report]") {
    auto r = parse_status_report("<Idle|MPos:0,0,0|Pn:XQ>");
    REQUIRE(r.has_value());
    REQUIRE(r->pins.has_value());
    REQUIRE(r->pins->x);
    REQUIRE_FALSE(r->pins->y);
}

TEST_CASE("missing Pn leaves pins unset", "[status_report]") {
    auto r = parse_status_report("<Idle|MPos:0,0,0|FS:0,0>");
    REQUIRE(r.has_value());
    REQUIRE_FALSE(r->pins.has_value());
}

TEST_CASE("rejects malformed reports", "[status_report]") {
    REQUIRE_FALSE(parse_status_report("Idle|MPos:0,0,0").has_value());      // no angle brackets
    REQUIRE_FALSE(parse_status_report("<>").has_value());                   // empty body
    REQUIRE_FALSE(parse_status_report("<Bogus|MPos:0,0,0>").has_value());   // unknown state
    REQUIRE_FALSE(parse_status_report("<Idle|MPos:>").has_value());         // empty axis list
    REQUIRE_FALSE(parse_status_report("<Idle|MPos:a,b,c>").has_value());    // non-numeric
    REQUIRE_FALSE(parse_status_report("<Hold:x|FS:0,0>").has_value());      // bad substate
}
