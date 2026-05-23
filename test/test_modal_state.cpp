#include "grbl_protocol/modal_state.hpp"
#include "grbl_protocol/status_report.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace grbl_protocol;
using Catch::Matchers::WithinAbs;

TEST_CASE("parses full GC reply (space-separated)", "[modal_state]") {
    auto m = parse_modal_state("G0 G54 G17 G21 G90 G94 M5 M9 T0 F0 S0");
    REQUIRE(m.motion         == MotionMode::G0);
    REQUIRE(m.wcs            == WCS::G54);
    REQUIRE(m.plane          == Plane::XY);
    REQUIRE(m.units          == Units::Mm);
    REQUIRE(m.distance       == DistanceMode::Absolute);
    REQUIRE(m.feed_rate_mode == FeedRateMode::UnitsPerMinute);
    REQUIRE(m.spindle        == SpindleMode::Off);
    REQUIRE(m.coolant        == CoolantMode::Off);
    REQUIRE(m.tool           == 0);
    REQUIRE_THAT(*m.feed,           WithinAbs(0.0, 1e-6));
    REQUIRE_THAT(*m.spindle_speed,  WithinAbs(0.0, 1e-6));
}

TEST_CASE("parses status G: field (comma-separated)", "[modal_state]") {
    auto m = parse_modal_state("G1,G55,G17,G21,G91,G94,M3,M8,T2,F1200,S8000");
    REQUIRE(m.motion        == MotionMode::G1);
    REQUIRE(m.wcs           == WCS::G55);
    REQUIRE(m.distance      == DistanceMode::Incremental);
    REQUIRE(m.spindle       == SpindleMode::Cw);
    REQUIRE(m.coolant       == CoolantMode::Flood);
    REQUIRE(m.tool          == 2);
    REQUIRE_THAT(*m.feed,          WithinAbs(1200.0, 1e-3));
    REQUIRE_THAT(*m.spindle_speed, WithinAbs(8000.0, 1e-3));
}

TEST_CASE("parses extended WCS codes (G59.1/.2/.3)", "[modal_state]") {
    REQUIRE(parse_modal_state("G59.1").wcs == WCS::G59_1);
    REQUIRE(parse_modal_state("G59.2").wcs == WCS::G59_2);
    REQUIRE(parse_modal_state("G59.3").wcs == WCS::G59_3);
}

TEST_CASE("parses probe motion modes", "[modal_state]") {
    REQUIRE(parse_modal_state("G38.2").motion == MotionMode::G38_2);
    REQUIRE(parse_modal_state("G38.3").motion == MotionMode::G38_3);
    REQUIRE(parse_modal_state("G38.4").motion == MotionMode::G38_4);
    REQUIRE(parse_modal_state("G38.5").motion == MotionMode::G38_5);
}

TEST_CASE("partial input leaves unset fields nullopt", "[modal_state]") {
    auto m = parse_modal_state("G54 G21");
    REQUIRE(m.wcs   == WCS::G54);
    REQUIRE(m.units == Units::Mm);
    REQUIRE_FALSE(m.motion.has_value());
    REQUIRE_FALSE(m.spindle.has_value());
    REQUIRE_FALSE(m.feed.has_value());
}

TEST_CASE("unknown tokens are silently ignored", "[modal_state]") {
    auto m = parse_modal_state("G54 G99 X12 Q99 M44");
    REQUIRE(m.wcs == WCS::G54);
    REQUIRE_FALSE(m.motion.has_value());
}

TEST_CASE("status report G: field populates StatusReport.modals", "[modal_state]") {
    auto r = parse_status_report(
        "<Idle|MPos:0,0,0|FS:0,0|G:G1,G54,G17,G21,G90,G94,M5,M9,T0,F0,S0>");
    REQUIRE(r.has_value());
    REQUIRE(r->modals.has_value());
    REQUIRE(r->modals->motion == MotionMode::G1);
    REQUIRE(r->modals->wcs    == WCS::G54);
    REQUIRE(r->modals->units  == Units::Mm);
}
