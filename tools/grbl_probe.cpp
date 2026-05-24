#include "grbl_protocol/modal_state.hpp"
#include "grbl_protocol/push_message.hpp"
#include "grbl_protocol/realtime.hpp"
#include "grbl_protocol/response.hpp"
#include "grbl_protocol/setting.hpp"
#include "grbl_protocol/status_report.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std::chrono_literals;
using grbl_protocol::decode_message;
using grbl_protocol::MessageKind;
using grbl_protocol::ModalState;
using grbl_protocol::parse_modal_state;
using grbl_protocol::parse_push_message;
using grbl_protocol::parse_response;
using grbl_protocol::parse_setting;
using grbl_protocol::parse_status_report;
using grbl_protocol::PinFlags;
using grbl_protocol::ResponseKind;

namespace {

// Last STATUS line we printed. Held at file scope so handle_user_line()
// can clear it on each user input — without that, dedupe makes the tool
// look frozen whenever the controller sits in a stable state.
std::string g_last_status;

// FluidNC ExecAlarm codes (1..18). The library stays flavor-agnostic;
// these descriptions diverge from classic GRBL v1.1 from code 10 on, so
// they live with the consumer that knows it's talking to FluidNC.
std::string_view alarm_description(int code) {
    switch (code) {
        case 1:  return "Hard limit triggered";
        case 2:  return "Soft limit exceeded";
        case 3:  return "Cycle aborted (reset while in motion)";
        case 4:  return "Probe fail (probe already triggered before cycle)";
        case 5:  return "Probe fail (probe did not contact workpiece)";
        case 6:  return "Homing fail (reset during homing)";
        case 7:  return "Homing fail (door opened during homing)";
        case 8:  return "Homing fail (could not clear limit on pull-off)";
        case 9:  return "Homing fail (could not find limit switch)";
        case 10: return "Spindle control error";
        case 11: return "Startup pin asserted (control pin active at boot)";
        case 12: return "Homing fail (ambiguous limit switch)";
        case 13: return "Hard stop (motion stopped abnormally)";
        case 14: return "Machine not homed (homing required)";
        case 15: return "Initialization alarm";
        case 16: return "Pin expander reset";
        case 17: return "G-code error during execution";
        case 18: return "Probe triggered a hard limit";
        default: return "Unknown alarm code";
    }
}

// FluidNC Error codes. Sparse beyond 40; falls back to "Unknown error
// code" for unmapped values. Same reason it's here and not in the lib:
// classic GRBL diverges at 5, 14, 18, etc.
std::string_view error_description(int code) {
    switch (code) {
        case 0:   return "No error";
        case 1:   return "Expected G-code command letter";
        case 2:   return "Bad G-code number format";
        case 3:   return "Invalid $ statement";
        case 4:   return "Negative value not allowed";
        case 5:   return "Setting disabled";
        case 6:   return "Step pulse too short";
        case 7:   return "Failed to read settings";
        case 8:   return "Command requires idle state";
        case 9:   return "G-code locked out during alarm or jog";
        case 10:  return "Soft limit error";
        case 11:  return "Line too long";
        case 12:  return "Max step rate exceeded";
        case 13:  return "Check door";
        case 14:  return "Line too long";
        case 15:  return "Max travel exceeded during jog";
        case 16:  return "Invalid jog command";
        case 17:  return "Laser mode requires PWM output";
        case 18:  return "No Homing/Cycle defined in settings";
        case 19:  return "Single-axis homing not allowed";
        case 20:  return "Unsupported G-code command";
        case 21:  return "G-code modal group violation";
        case 22:  return "G-code undefined feed rate";
        case 23:  return "G-code command value not integer";
        case 24:  return "G-code axis command conflict";
        case 25:  return "G-code word repeated";
        case 26:  return "G-code no axis words";
        case 27:  return "G-code invalid line number";
        case 28:  return "G-code value word missing";
        case 29:  return "G-code unsupported coordinate system";
        case 30:  return "G-code G53 invalid motion mode";
        case 31:  return "G-code extra axis words";
        case 32:  return "G-code no axis words in plane";
        case 33:  return "G-code invalid target";
        case 34:  return "G-code arc radius error";
        case 35:  return "G-code no offsets in plane";
        case 36:  return "G-code unused words";
        case 37:  return "G-code G43 dynamic axis error";
        case 38:  return "G-code max value exceeded";
        case 39:  return "P parameter max exceeded";
        case 40:  return "Check startup pins";
        case 60:  return "Failed to mount device";
        case 61:  return "Read failed";
        case 62:  return "Failed to open directory";
        case 63:  return "Directory not found";
        case 64:  return "File empty";
        case 65:  return "File not found";
        case 66:  return "Failed to open file";
        case 67:  return "Device is busy";
        case 68:  return "Failed to delete directory";
        case 69:  return "Failed to delete file";
        case 70:  return "Failed to rename file";
        case 80:  return "Number out of range for setting";
        case 81:  return "Invalid value for setting";
        case 82:  return "Failed to create file";
        case 83:  return "Failed to format filesystem";
        case 90:  return "Failed to send message";
        case 100: return "Failed to store setting";
        case 101: return "Failed to get setting status";
        case 110: return "Authentication failed";
        case 111: return "End of line";
        case 112: return "End of file";
        case 113: return "System reset";
        case 114: return "No data";
        case 120: return "Another interface is busy";
        case 130: return "Jog cancelled";
        case 150: return "Bad pin specification";
        case 151: return "Bad runtime config setting";
        case 152: return "Configuration is invalid (check boot messages)";
        case 160: return "File upload failed";
        case 161: return "File download failed";
        case 162: return "Read-only setting";
        case 170: return "Expression divide by zero";
        case 171: return "Expression invalid argument";
        case 172: return "Expression invalid result";
        case 173: return "Expression unknown operator";
        case 174: return "Expression argument out of range";
        case 175: return "Expression syntax error";
        case 176: return "Flow control syntax error";
        case 177: return "Flow control not executing macro";
        case 178: return "Flow control out of memory";
        case 179: return "Flow control stack overflow";
        case 180: return "Parameter assignment failed";
        case 181: return "G-code invalid word value";
        default:  return "Unknown error code";
    }
}

std::string format_pins(const PinFlags& p) {
    std::string s;
    if (p.x)            s += 'X';
    if (p.y)            s += 'Y';
    if (p.z)            s += 'Z';
    if (p.probe)        s += 'P';
    if (p.door)         s += 'D';
    if (p.hold)         s += 'H';
    if (p.soft_reset)   s += 'R';
    if (p.cycle_start)  s += 'S';
    if (s.empty()) s = "(none)";
    return s;
}

std::string format_position(const grbl_protocol::Position& p, const char* label) {
    std::string s = " ";
    s += label;
    s += "=(";
    char tmp[16];
    for (std::uint8_t i = 0; i < p.count; ++i) {
        if (i > 0) s += ',';
        std::snprintf(tmp, sizeof(tmp), "%.3f", p.axes[i]);
        s += tmp;
    }
    s += ')';
    return s;
}

std::string format_modals(const ModalState& m) {
    std::string s;
    char buf[64];
    auto add_kv = [&](const char* k, const char* v) {
        if (!s.empty()) s += ' ';
        s += k; s += '='; s += v;
    };
    if (m.motion)   add_kv("motion",  name(*m.motion));
    if (m.wcs)      add_kv("wcs",     name(*m.wcs));
    if (m.plane)    add_kv("plane",   name(*m.plane));
    if (m.units)    add_kv("units",   name(*m.units));
    if (m.distance) add_kv("dist",    name(*m.distance));
    if (m.spindle)  add_kv("spindle", name(*m.spindle));
    if (m.coolant)  add_kv("coolant", name(*m.coolant));
    if (m.tool)          { std::snprintf(buf, sizeof(buf), "tool=%d",  *m.tool);          if (!s.empty()) s += ' '; s += buf; }
    if (m.feed)          { std::snprintf(buf, sizeof(buf), "feed=%.0f", *m.feed);         if (!s.empty()) s += ' '; s += buf; }
    if (m.spindle_speed) { std::snprintf(buf, sizeof(buf), "S=%.0f",   *m.spindle_speed); if (!s.empty()) s += ' '; s += buf; }
    return s;
}

std::string format_accessory(const grbl_protocol::AccessoryState& a) {
    std::string s;
    if (a.spindle_cw)  s += 'S';
    if (a.spindle_ccw) s += 'C';
    if (a.flood)       s += 'F';
    if (a.mist)        s += 'M';
    if (s.empty()) s = "(none)";
    return s;
}

std::string format_status(const grbl_protocol::StatusReport& sr) {
    std::string s = "STATUS   state=";
    s += name(sr.state);
    char buf[64];
    if (sr.sub_state) {
        std::snprintf(buf, sizeof(buf), ":%d", *sr.sub_state);
        s += buf;
    }
    if (sr.mpos) s += format_position(*sr.mpos, "mpos");
    if (sr.wpos) s += format_position(*sr.wpos, "wpos");
    if (sr.wco)  s += format_position(*sr.wco,  "wco");
    if (sr.fs) {
        std::snprintf(buf, sizeof(buf), " fs=(%.0f,%.0f)", sr.fs->feed, sr.fs->spindle);
        s += buf;
    }
    if (sr.ov) {
        std::snprintf(buf, sizeof(buf), " ov=(%u,%u,%u)",
                      sr.ov->feed, sr.ov->rapid, sr.ov->spindle);
        s += buf;
    }
    if (sr.a) {
        s += " a=";
        s += format_accessory(*sr.a);
    }
    if (sr.bf) {
        std::snprintf(buf, sizeof(buf), " bf=(%u,%u)",
                      sr.bf->planner_blocks, sr.bf->rx_bytes);
        s += buf;
    }
    if (sr.line_number) {
        std::snprintf(buf, sizeof(buf), " ln=%d", *sr.line_number);
        s += buf;
    }
    if (sr.modals && sr.modals->wcs) {
        s += " wcs=";
        s += name(*sr.modals->wcs);
    }
    if (sr.pins) {
        s += " pins=";
        s += format_pins(*sr.pins);
    }
    return s;
}

int connect_tcp(const char* host, const char* port) {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    if (int rc = getaddrinfo(host, port, &hints, &res); rc != 0) {
        std::fprintf(stderr, "getaddrinfo(%s:%s): %s\n", host, port, gai_strerror(rc));
        return -1;
    }
    int fd = -1;
    for (auto* p = res; p; p = p->ai_next) {
        fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (fd < 0) continue;
        if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0) break;
        ::close(fd);
        fd = -1;
    }
    freeaddrinfo(res);
    return fd;
}

void classify(std::string_view line) {
    if (line.empty()) return;
    if (auto r = parse_response(line)) {
        switch (r->kind) {
            case ResponseKind::Ok:
                std::printf("RESPONSE ok\n");
                break;
            case ResponseKind::Error: {
                auto desc = error_description(*r->code);
                std::printf("RESPONSE error code=%d -- %.*s\n",
                            *r->code,
                            static_cast<int>(desc.size()), desc.data());
                break;
            }
            case ResponseKind::Alarm: {
                auto desc = alarm_description(*r->code);
                std::printf("RESPONSE alarm code=%d -- %.*s\n",
                            *r->code,
                            static_cast<int>(desc.size()), desc.data());
                break;
            }
        }
        return;
    }
    if (auto s = parse_setting(line)) {
        std::printf("SETTING  $%.*s=%.*s\n",
                    static_cast<int>(s->key.size()),   s->key.data(),
                    static_cast<int>(s->value.size()), s->value.data());
        return;
    }
    if (auto m = parse_push_message(line)) {
        if (m->key == "GC") {
            auto modals = parse_modal_state(m->value);
            std::printf("MODALS   %s\n", format_modals(modals).c_str());
            return;
        }
        if (m->key == "MSG") {
            auto dm = decode_message(m->value);
            const char* tag = "MSG";
            switch (dm.kind) {
                case MessageKind::Welcome:   tag = "WELCOME";    break;
                case MessageKind::AlarmHint: tag = "ALARM-HINT"; break;
                case MessageKind::Unlocked:  tag = "UNLOCKED";   break;
                case MessageKind::Homed:     tag = "HOMED";      break;
                case MessageKind::Error:     tag = "ERR";        break;
                case MessageKind::Info:      tag = "INFO";       break;
                case MessageKind::Warning:   tag = "WARN";       break;
                case MessageKind::Debug:     tag = "DBG";        break;
                case MessageKind::Critical:  tag = "CRIT";       break;
                case MessageKind::Other:     tag = "MSG";        break;
            }
            if (dm.body.empty()) {
                std::printf("PUSH     [%s]\n", tag);
            } else {
                std::printf("PUSH     [%s] %.*s\n",
                            tag,
                            static_cast<int>(dm.body.size()), dm.body.data());
            }
        } else {
            std::printf("PUSH     [%.*s] %.*s\n",
                        static_cast<int>(m->key.size()), m->key.data(),
                        static_cast<int>(m->value.size()), m->value.data());
        }
        return;
    }
    if (auto sr = parse_status_report(line)) {
        auto formatted = format_status(*sr);
        if (formatted != g_last_status) {
            std::printf("%s\n", formatted.c_str());
            g_last_status = std::move(formatted);
        }
        return;
    }
    std::printf("UNKNOWN  %.*s\n", static_cast<int>(line.size()), line.data());
}

bool send_all(int fd, std::string_view data) {
    while (!data.empty()) {
        auto n = ::send(fd, data.data(), data.size(), 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            std::perror("send");
            return false;
        }
        data.remove_prefix(static_cast<size_t>(n));
    }
    return true;
}

void handle_user_line(int fd, std::string_view line) {
    if (line.empty()) return;
    // Force the next STATUS to print regardless of whether the controller's
    // state actually changes: gives the user immediate visible confirmation
    // that their command landed, even when state is stable (e.g. in Hold).
    g_last_status.clear();
    if (line.size() == 1 && grbl_protocol::realtime::is_realtime(static_cast<std::uint8_t>(line[0]))) {
        send_all(fd, line);
        return;
    }
    std::string buf(line);
    buf.push_back('\n');
    send_all(fd, buf);
}

void drain_socket(int fd, std::string& in_buf) {
    char chunk[1024];
    while (true) {
        auto n = ::recv(fd, chunk, sizeof(chunk), MSG_DONTWAIT);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) return;
            if (errno == EINTR) continue;
            std::perror("recv");
            return;
        }
        if (n == 0) {
            std::fprintf(stderr, "connection closed by peer\n");
            std::exit(0);
        }
        in_buf.append(chunk, static_cast<size_t>(n));
        size_t pos;
        while ((pos = in_buf.find('\n')) != std::string::npos) {
            std::string_view line(in_buf.data(), pos);
            if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
            classify(line);
            in_buf.erase(0, pos + 1);
        }
    }
}

void interactive_loop(int fd, std::chrono::milliseconds poll_interval) {
    std::string in_buf;
    std::string out_buf;
    auto next_poll = std::chrono::steady_clock::now() + poll_interval;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto until_poll = std::chrono::duration_cast<std::chrono::milliseconds>(next_poll - now);
        int timeout_ms = until_poll.count() < 0 ? 0 : static_cast<int>(until_poll.count());

        pollfd pfds[2] = {
            {fd, POLLIN, 0},
            {STDIN_FILENO, POLLIN, 0},
        };
        int rc = ::poll(pfds, 2, timeout_ms);
        if (rc < 0) {
            if (errno == EINTR) continue;
            std::perror("poll");
            return;
        }

        if (std::chrono::steady_clock::now() >= next_poll) {
            send_all(fd, "?");
            next_poll = std::chrono::steady_clock::now() + poll_interval;
        }

        if (pfds[0].revents & POLLIN) {
            drain_socket(fd, in_buf);
        }
        if (pfds[0].revents & (POLLHUP | POLLERR)) {
            std::fprintf(stderr, "socket closed/error\n");
            return;
        }

        if (pfds[1].revents & POLLIN) {
            char chunk[256];
            auto n = ::read(STDIN_FILENO, chunk, sizeof(chunk));
            if (n <= 0) {
                std::fprintf(stderr, "stdin EOF, exiting\n");
                return;
            }
            out_buf.append(chunk, static_cast<size_t>(n));
            size_t pos;
            while ((pos = out_buf.find('\n')) != std::string::npos) {
                std::string_view line(out_buf.data(), pos);
                if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
                handle_user_line(fd, line);
                out_buf.erase(0, pos + 1);
            }
        }
        if (pfds[1].revents & POLLHUP) {
            std::fprintf(stderr, "stdin closed, exiting\n");
            return;
        }
    }
}

} // namespace

int main(int argc, char** argv) {
    const char* host = argc > 1 ? argv[1] : "cnc.local";
    const char* port = argc > 2 ? argv[2] : "23";

    int fd = connect_tcp(host, port);
    if (fd < 0) {
        std::fprintf(stderr, "connect %s:%s failed\n", host, port);
        return 1;
    }
    std::fprintf(stderr, "connected to %s:%s\n", host, port);
    std::fprintf(stderr,
                 "interactive console. Ctrl-D to quit. "
                 "Single-char lines ?/!/~/<Ctrl-X> are sent as GRBL real-time bytes; "
                 "anything else gets a newline appended. ? auto-polled every 250ms.\n");

    interactive_loop(fd, 250ms);

    ::close(fd);
    return 0;
}
