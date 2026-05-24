#include "grbl_protocol/realtime.hpp"

#include <catch2/catch_test_macros.hpp>

namespace rt = grbl_protocol::realtime;

TEST_CASE("standard realtime byte values match GRBL v1.1", "[realtime]") {
    REQUIRE(rt::Reset        == 0x18);
    REQUIRE(rt::StatusReport == 0x3F);
    REQUIRE(rt::StatusReport == '?');
    REQUIRE(rt::CycleStart   == 0x7E);
    REQUIRE(rt::CycleStart   == '~');
    REQUIRE(rt::FeedHold     == 0x21);
    REQUIRE(rt::FeedHold     == '!');
}

TEST_CASE("extended realtime bytes match FluidNC RealtimeCmd.h", "[realtime]") {
    REQUIRE(rt::SafetyDoor          == 0x84);
    REQUIRE(rt::JogCancel           == 0x85);
    REQUIRE(rt::FeedOvrReset        == 0x90);
    REQUIRE(rt::FeedOvrFineMinus    == 0x94);
    REQUIRE(rt::RapidOvrReset       == 0x95);
    REQUIRE(rt::RapidOvrExtraLow    == 0x98);
    REQUIRE(rt::SpindleOvrReset     == 0x99);
    REQUIRE(rt::SpindleOvrCoarsePlus == 0x9A);
    REQUIRE(rt::SpindleOvrStop      == 0x9E);
    REQUIRE(rt::CoolantFloodOvrToggle == 0xA0);
    REQUIRE(rt::CoolantMistOvrToggle  == 0xA1);
}

TEST_CASE("is_realtime() accepts standard and extended bytes", "[realtime]") {
    REQUIRE(rt::is_realtime(rt::Reset));
    REQUIRE(rt::is_realtime(rt::StatusReport));
    REQUIRE(rt::is_realtime(rt::FeedHold));
    REQUIRE(rt::is_realtime(rt::CycleStart));
    REQUIRE(rt::is_realtime(rt::SafetyDoor));
    REQUIRE(rt::is_realtime(rt::JogCancel));
    REQUIRE(rt::is_realtime(rt::FeedOvrReset));
    REQUIRE(rt::is_realtime(rt::SpindleOvrCoarsePlus));
    REQUIRE(rt::is_realtime(0xFF));  // forward-compat: any 0x80+ byte
}

TEST_CASE("is_realtime() rejects ordinary gcode bytes", "[realtime]") {
    REQUIRE_FALSE(rt::is_realtime(0x00));
    REQUIRE_FALSE(rt::is_realtime('G'));
    REQUIRE_FALSE(rt::is_realtime('0'));
    REQUIRE_FALSE(rt::is_realtime(' '));
    REQUIRE_FALSE(rt::is_realtime('\n'));
    REQUIRE_FALSE(rt::is_realtime('$'));
    REQUIRE_FALSE(rt::is_realtime(0x7F));  // DEL -- just below the 0x80 line
}

TEST_CASE("is_realtime() is constexpr-evaluable", "[realtime]") {
    static_assert(rt::is_realtime(rt::Reset));
    static_assert(rt::is_realtime(rt::FeedOvrReset));
    static_assert(!rt::is_realtime('G'));
    SUCCEED();
}
