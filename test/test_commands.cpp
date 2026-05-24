#include "grbl_protocol/commands.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <string_view>

namespace cmd = grbl_protocol::commands;

TEST_CASE("bare command strings match GRBL wire format", "[commands]") {
    REQUIRE(cmd::Help              == "$");
    REQUIRE(cmd::ListSettings      == "$$");
    REQUIRE(cmd::ListParameters    == "$#");
    REQUIRE(cmd::ModalQuery        == "$G");
    REQUIRE(cmd::BuildInfo         == "$I");
    REQUIRE(cmd::ListStartupBlocks == "$N");
    REQUIRE(cmd::Home              == "$H");
    REQUIRE(cmd::Unlock            == "$X");
    REQUIRE(cmd::CheckModeToggle   == "$C");
    REQUIRE(cmd::Sleep             == "$SLP");
}

TEST_CASE("reset commands match GRBL wire format", "[commands]") {
    REQUIRE(cmd::ResetSettings   == "$RST=$");
    REQUIRE(cmd::ResetParameters == "$RST=#");
    REQUIRE(cmd::ResetAll        == "$RST=*");
}

TEST_CASE("bare commands are usable in constexpr contexts", "[commands]") {
    static_assert(cmd::ModalQuery.size() == 2);
    static_assert(cmd::ModalQuery[0] == '$');
    static_assert(cmd::Sleep == std::string_view{"$SLP"});
    SUCCEED();
}

namespace {
std::string_view view(const char* buf, std::size_t n) {
    return {buf, n};
}
}

TEST_CASE("format_jog emits incremental mm jog with defaults", "[commands][jog]") {
    char buf[64];
    cmd::JogParams p;
    p.x = 10.0f;
    p.feed = 500.0f;
    auto n = cmd::format_jog(buf, sizeof(buf), p);
    REQUIRE(n > 0);
    REQUIRE(view(buf, n) == "$J=G91G21X10.000F500.000");
}

TEST_CASE("format_jog honors absolute, inch, and machine flags", "[commands][jog]") {
    char buf[64];
    cmd::JogParams p;
    p.x = 1.5f;
    p.y = -2.25f;
    p.feed = 100.0f;
    p.relative = false;   // G90
    p.mm       = false;   // G20
    p.machine  = true;    // G53
    auto n = cmd::format_jog(buf, sizeof(buf), p);
    REQUIRE(n > 0);
    REQUIRE(view(buf, n) == "$J=G90G20G53X1.500Y-2.250F100.000");
}

TEST_CASE("format_jog supports all six axes", "[commands][jog]") {
    char buf[128];
    cmd::JogParams p;
    p.x = 1.0f; p.y = 2.0f; p.z = 3.0f;
    p.a = 4.0f; p.b = 5.0f; p.c = 6.0f;
    p.feed = 1000.0f;
    auto n = cmd::format_jog(buf, sizeof(buf), p);
    REQUIRE(n > 0);
    REQUIRE(view(buf, n) ==
            "$J=G91G21X1.000Y2.000Z3.000A4.000B5.000C6.000F1000.000");
}

TEST_CASE("format_jog rejects invalid input", "[commands][jog]") {
    char buf[64];
    cmd::JogParams p;
    // no axes
    p.feed = 500.0f;
    REQUIRE(cmd::format_jog(buf, sizeof(buf), p) == 0);

    // no feed
    p.x = 1.0f;
    p.feed = 0.0f;
    REQUIRE(cmd::format_jog(buf, sizeof(buf), p) == 0);

    // negative feed
    p.feed = -1.0f;
    REQUIRE(cmd::format_jog(buf, sizeof(buf), p) == 0);
}

TEST_CASE("format_jog returns 0 when buffer is too small", "[commands][jog]") {
    char tiny[8];
    cmd::JogParams p;
    p.x = 10.0f;
    p.feed = 500.0f;
    REQUIRE(cmd::format_jog(tiny, sizeof(tiny), p) == 0);

    REQUIRE(cmd::format_jog(tiny, 0, p) == 0);
    REQUIRE(cmd::format_jog(nullptr, 64, p) == 0);
}

TEST_CASE("format_startup_block writes a slot assignment", "[commands][startup]") {
    char buf[64];
    auto n = cmd::format_startup_block(buf, sizeof(buf), 0, "G21G90");
    REQUIRE(n > 0);
    REQUIRE(view(buf, n) == "$N0=G21G90");

    n = cmd::format_startup_block(buf, sizeof(buf), 1, "G54");
    REQUIRE(n > 0);
    REQUIRE(view(buf, n) == "$N1=G54");
}

TEST_CASE("format_startup_block clears a slot with an empty line", "[commands][startup]") {
    char buf[16];
    auto n = cmd::format_startup_block(buf, sizeof(buf), 0, "");
    REQUIRE(n > 0);
    REQUIRE(view(buf, n) == "$N0=");
}

TEST_CASE("format_startup_block rejects invalid slots and buffers", "[commands][startup]") {
    char buf[64];
    REQUIRE(cmd::format_startup_block(buf, sizeof(buf), -1, "G21") == 0);
    REQUIRE(cmd::format_startup_block(buf, sizeof(buf),  2, "G21") == 0);

    char tiny[4];
    REQUIRE(cmd::format_startup_block(tiny, sizeof(tiny), 0, "G21G90G54") == 0);
    REQUIRE(cmd::format_startup_block(buf, 0, 0, "G21") == 0);
    REQUIRE(cmd::format_startup_block(nullptr, 64, 0, "G21") == 0);
}
