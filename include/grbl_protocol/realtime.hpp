#pragma once

#include <cstdint>

// GRBL v1.1 / FluidNC real-time command bytes. These are sent as
// single bytes WITHOUT a newline and are executed by the controller
// immediately, out-of-band from the gcode line buffer. Values verified
// against FluidNC/src/RealtimeCmd.h.
//
// Use:
//   send_bytes(fd, &realtime::FeedHold, 1);
//   if (realtime::is_realtime(b)) ...
//
// All values are universal across classic GRBL, grblHAL, and FluidNC.

namespace grbl_protocol::realtime {

// Standard GRBL v1.1 (printable ASCII + one control byte).
inline constexpr std::uint8_t Reset        = 0x18;  // Ctrl-X, soft reset
inline constexpr std::uint8_t StatusReport = '?';   // 0x3F
inline constexpr std::uint8_t CycleStart   = '~';   // 0x7E
inline constexpr std::uint8_t FeedHold     = '!';   // 0x21

// Extended commands (high-bit range, GRBL v1.1+).
inline constexpr std::uint8_t SafetyDoor   = 0x84;
inline constexpr std::uint8_t JogCancel    = 0x85;
inline constexpr std::uint8_t DebugReport  = 0x86;

inline constexpr std::uint8_t Macro0 = 0x87;
inline constexpr std::uint8_t Macro1 = 0x88;
inline constexpr std::uint8_t Macro2 = 0x89;
inline constexpr std::uint8_t Macro3 = 0x8A;

// Feed-rate override.
inline constexpr std::uint8_t FeedOvrReset       = 0x90;
inline constexpr std::uint8_t FeedOvrCoarsePlus  = 0x91;  // +10%
inline constexpr std::uint8_t FeedOvrCoarseMinus = 0x92;  // -10%
inline constexpr std::uint8_t FeedOvrFinePlus    = 0x93;  // +1%
inline constexpr std::uint8_t FeedOvrFineMinus   = 0x94;  // -1%

// Rapid override (discrete steps, not increments).
inline constexpr std::uint8_t RapidOvrReset    = 0x95;  // 100%
inline constexpr std::uint8_t RapidOvrMedium   = 0x96;  // 50%
inline constexpr std::uint8_t RapidOvrLow      = 0x97;  // 25%
inline constexpr std::uint8_t RapidOvrExtraLow = 0x98;  // (FluidNC extension)

// Spindle override.
inline constexpr std::uint8_t SpindleOvrReset       = 0x99;
inline constexpr std::uint8_t SpindleOvrCoarsePlus  = 0x9A;  // +10%
inline constexpr std::uint8_t SpindleOvrCoarseMinus = 0x9B;  // -10%
inline constexpr std::uint8_t SpindleOvrFinePlus    = 0x9C;  // +1%
inline constexpr std::uint8_t SpindleOvrFineMinus   = 0x9D;  // -1%
inline constexpr std::uint8_t SpindleOvrStop        = 0x9E;

// Coolant toggles.
inline constexpr std::uint8_t CoolantFloodOvrToggle = 0xA0;
inline constexpr std::uint8_t CoolantMistOvrToggle  = 0xA1;

// Returns true if the byte is a GRBL/FluidNC real-time command (must
// be sent raw, no newline). The high-bit range (0x80..0xFF) is treated
// as realtime in its entirety -- catches the FluidNC extensions above
// AND any future codes added to the same reserved range.
constexpr bool is_realtime(std::uint8_t b) {
    switch (b) {
        case Reset:
        case StatusReport:
        case CycleStart:
        case FeedHold:
            return true;
        default:
            return b >= 0x80;
    }
}

} // namespace grbl_protocol::realtime
