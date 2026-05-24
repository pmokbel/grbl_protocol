# grbl_protocol

A C++17 library for the GRBL v1.1 / FluidNC ASCII wire protocol — parsers for incoming frames, canonical constants for outgoing commands, and small builders for the parameterized ones. Controller-agnostic — the same surface works against classic GRBL, grblHAL, or FluidNC because they all share the v1.1 wire format.

**Status:** pre-1.0. Existing surface (parsers, constants, builders) is stable; expect new modules to land as protocol coverage grows.

## What it knows about

**Parsing incoming frames:**

- **Status reports** — `<State|MPos|WPos|WCO|FS|Pn|Ov|A|Bf|Ln>` with all v1.1 fields, 1–6 axes (XYZABC).
- **Responses** — `ok`, `error:N`, `ALARM:N`.
- **Settings** — `$<key>=<value>` for both classic numeric keys (`$22=1`) and FluidNC string-named keys (`$Config/Filename=config.yaml`).
- **Push messages** — `[KEY:VALUE]` framing, plus a semantic decoder for `[MSG:…]` payloads (Welcome, AlarmHint, Unlocked, Homed, and severity-tagged Error/Info/Warning/Debug/Critical).
- **Line shape classifier** — `classify(line) -> LineKind` for cheap dispatch / filtering streamed payloads.

**Outgoing wire constants and builders:**

- **Realtime byte commands** — `realtime::FeedHold`, `realtime::StatusReport`, the override and macro bytes, and `is_realtime(b)` for the out-of-band channel.
- **System commands** — `commands::ModalQuery`, `commands::Home`, `commands::Unlock`, the `$RST=` variants, etc., as canonical strings.
- **Parameterized builders** — `commands::format_jog(...)` and `commands::format_startup_block(...)` write into a caller buffer, no allocation.

## Scope

Pure protocol library — knows the wire shapes but not the transport. Callers own serial/socket I/O, line framing, retry logic, and any controller-flavor-specific lookup tables. Numeric alarm/error code descriptions in particular are caller-side because they diverge across GRBL flavors (e.g. FluidNC alarm 10 is `SpindleControl`, classic GRBL v1.1 alarm 10 is `EStop`); the `grbl_probe` tool in `tools/` ships a FluidNC-flavored table as a reference.

## Build & test

```sh
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Requires CMake 3.20+ and a C++17 compiler.

## Library use

```cpp
#include <grbl_protocol/status_report.hpp>
#include <cstdio>

using grbl_protocol::parse_status_report;

int main() {
    auto r = parse_status_report(
        "<Run|MPos:1.234,5.678,9.000|FS:500,8000|Pn:P>");
    if (!r || !r->mpos) return 1;
    std::printf("axes=%u  pos=(%.3f, %.3f, %.3f)  pin_probe=%d\n",
                r->mpos->count,
                r->mpos->axes[0], r->mpos->axes[1], r->mpos->axes[2],
                r->pins && r->pins->probe);
}
```

Each `parse_*` function returns `std::optional<T>`; `nullopt` means "this line is not of that type." All parsers borrow from the caller's input buffer — the resulting `string_view` fields are valid only while the source line is alive.

Sending side:

```cpp
#include <grbl_protocol/commands.hpp>
#include <grbl_protocol/realtime.hpp>

namespace cmd = grbl_protocol::commands;
namespace rt  = grbl_protocol::realtime;

send_line(cmd::Home);                 // "$H\n"
send_byte(rt::FeedHold);              // 0x21, raw (no newline)

char buf[64];
cmd::JogParams p;
p.x = 10.0f; p.feed = 500.0f;
if (auto n = cmd::format_jog(buf, sizeof(buf), p)) {
    send_line({buf, n});              // "$J=G91G21X10.000F500.000\n"
}
```

## Available as

- **CMake library** — the default. `add_subdirectory()` it or install and `find_package(grbl_protocol)`.
- **ESP-IDF component** — the root `CMakeLists.txt` has an `if(ESP_PLATFORM)` short-circuit that registers via `idf_component_register()`. Drop the repo into your project's `components/` directory (or add to `EXTRA_COMPONENT_DIRS`) and `REQUIRES grbl_protocol` from your consumer. Published to the IDF Component Registry; metadata in `idf_component.yml`.
- **PlatformIO library** — `library.json` declares it; consume with `lib_deps = grbl_protocol`. Works on ESP32, RP2040, SAMD, STM32, and nRF52 platforms. Classic 8-bit AVR (Arduino Uno/Mega) is not supported — the library uses `<string_view>` / `<charconv>` / `<optional>` which AVR's libstdc++ does not ship.

## The `grbl_probe` tool

`tools/grbl_probe` is an interactive TCP console for talking to a live controller. It connects to a controller (default `cnc.local:23`), classifies every incoming line (STATUS / RESPONSE / SETTING / PUSH / UNKNOWN), and forwards your input. Single-character `?` / `!` / `~` / Ctrl-X are sent as raw GRBL real-time bytes; longer lines get `\n` appended. A `?` status poll fires every 250 ms and dedups identical reports so the terminal only shows actual state changes.

```sh
./build/tools/grbl_probe cnc.local 23
```

The tool is host-only (uses POSIX sockets) and is built only when this repo is the top-level CMake project — it's excluded automatically when consumed as an ESP-IDF component or a PlatformIO library.

## License

Apache-2.0. See `LICENSE`.