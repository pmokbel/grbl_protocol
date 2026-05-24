#include "grbl_protocol/line_kind.hpp"

#include <catch2/catch_test_macros.hpp>

namespace gp = grbl_protocol;

TEST_CASE("classify recognizes status reports", "[line_kind]") {
    REQUIRE(gp::classify("<Idle|MPos:0,0,0|FS:0,0>") == gp::LineKind::Status);
    REQUIRE(gp::classify("<Run|MPos:1.0,2.0,3.0|FS:500,8000|Pn:P>")
            == gp::LineKind::Status);
    // Shape-only: malformed body still classifies as Status.
    REQUIRE(gp::classify("<garbage")    == gp::LineKind::Status);
    REQUIRE(gp::classify("<")           == gp::LineKind::Status);
}

TEST_CASE("classify recognizes push messages", "[line_kind]") {
    REQUIRE(gp::classify("[MSG:Reset to continue]") == gp::LineKind::PushMessage);
    REQUIRE(gp::classify("[VER:1.1f.20170801:]")    == gp::LineKind::PushMessage);
    REQUIRE(gp::classify("[GC:G0 G54 G17 G21 G90]") == gp::LineKind::PushMessage);
    REQUIRE(gp::classify("[")                       == gp::LineKind::PushMessage);
}

TEST_CASE("classify recognizes responses", "[line_kind]") {
    REQUIRE(gp::classify("ok")        == gp::LineKind::Response);
    REQUIRE(gp::classify("error:1")   == gp::LineKind::Response);
    REQUIRE(gp::classify("error:33")  == gp::LineKind::Response);
    REQUIRE(gp::classify("ALARM:1")   == gp::LineKind::Response);
    REQUIRE(gp::classify("ALARM:10")  == gp::LineKind::Response);
}

TEST_CASE("classify recognizes settings", "[line_kind]") {
    REQUIRE(gp::classify("$22=1")                          == gp::LineKind::Setting);
    REQUIRE(gp::classify("$0=10")                          == gp::LineKind::Setting);
    REQUIRE(gp::classify("$Config/Filename=config.yaml")   == gp::LineKind::Setting);
    REQUIRE(gp::classify("$")                              == gp::LineKind::Setting);
}

TEST_CASE("classify returns Unknown for non-protocol lines", "[line_kind]") {
    REQUIRE(gp::classify("")            == gp::LineKind::Unknown);
    REQUIRE(gp::classify("G0 X10")      == gp::LineKind::Unknown);
    REQUIRE(gp::classify(">G54")        == gp::LineKind::Unknown);
    REQUIRE(gp::classify("Grbl 1.1f")   == gp::LineKind::Unknown);
    REQUIRE(gp::classify("Ok")          == gp::LineKind::Unknown);  // case-sensitive
    REQUIRE(gp::classify("OK")          == gp::LineKind::Unknown);
    REQUIRE(gp::classify("error")       == gp::LineKind::Unknown);  // missing ':'
    REQUIRE(gp::classify("ALARM")       == gp::LineKind::Unknown);
    REQUIRE(gp::classify("alarm:1")     == gp::LineKind::Unknown);  // case-sensitive
}

TEST_CASE("classify is constexpr-evaluable", "[line_kind]") {
    static_assert(gp::classify("<Idle>") == gp::LineKind::Status);
    static_assert(gp::classify("[MSG:x]") == gp::LineKind::PushMessage);
    static_assert(gp::classify("ok") == gp::LineKind::Response);
    static_assert(gp::classify("$22=1") == gp::LineKind::Setting);
    static_assert(gp::classify("") == gp::LineKind::Unknown);
    static_assert(gp::classify("G0") == gp::LineKind::Unknown);
    SUCCEED();
}
