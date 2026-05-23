#include "grbl_protocol/response.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace grbl_protocol;

TEST_CASE("parses ok", "[response]") {
    auto r = parse_response("ok");
    REQUIRE(r.has_value());
    REQUIRE(r->kind == ResponseKind::Ok);
    REQUIRE_FALSE(r->code.has_value());
}

TEST_CASE("parses error code", "[response]") {
    auto r = parse_response("error:9");
    REQUIRE(r.has_value());
    REQUIRE(r->kind == ResponseKind::Error);
    REQUIRE(r->code == 9);
}

TEST_CASE("parses ALARM code", "[response]") {
    auto r = parse_response("ALARM:3");
    REQUIRE(r.has_value());
    REQUIRE(r->kind == ResponseKind::Alarm);
    REQUIRE(r->code == 3);
}

TEST_CASE("rejects non-response lines", "[response]") {
    REQUIRE_FALSE(parse_response("OK").has_value());
    REQUIRE_FALSE(parse_response("error:").has_value());
    REQUIRE_FALSE(parse_response("error:x").has_value());
    REQUIRE_FALSE(parse_response("ALARM").has_value());
    REQUIRE_FALSE(parse_response("$10=1").has_value());
}

TEST_CASE("alarm_description maps FluidNC ExecAlarm codes", "[response]") {
    REQUIRE(alarm_description(1)  == "Hard limit triggered");
    REQUIRE(alarm_description(5)  == "Probe fail (probe did not contact workpiece)");
    REQUIRE(alarm_description(9)  == "Homing fail (could not find limit switch)");
    REQUIRE(alarm_description(10) == "Spindle control error");
    REQUIRE(alarm_description(14) == "Machine not homed (homing required)");
    REQUIRE(alarm_description(18) == "Probe triggered a hard limit");
}

TEST_CASE("alarm_description falls back for unknown codes", "[response]") {
    REQUIRE(alarm_description(0)   == "Unknown alarm code");
    REQUIRE(alarm_description(19)  == "Unknown alarm code");
    REQUIRE(alarm_description(-1)  == "Unknown alarm code");
    REQUIRE(alarm_description(999) == "Unknown alarm code");
}

TEST_CASE("error_description maps common FluidNC codes", "[response]") {
    REQUIRE(error_description(0)   == "No error");
    REQUIRE(error_description(5)   == "Setting disabled");
    REQUIRE(error_description(18)  == "No Homing/Cycle defined in settings");
    REQUIRE(error_description(20)  == "Unsupported G-code command");
    REQUIRE(error_description(152) == "Configuration is invalid (check boot messages)");
    REQUIRE(error_description(181) == "G-code invalid word value");
}

TEST_CASE("error_description falls back for unmapped codes", "[response]") {
    REQUIRE(error_description(41)  == "Unknown error code");
    REQUIRE(error_description(59)  == "Unknown error code");
    REQUIRE(error_description(200) == "Unknown error code");
    REQUIRE(error_description(-1)  == "Unknown error code");
}
