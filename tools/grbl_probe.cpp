#include "grbl_protocol/push_message.hpp"
#include "grbl_protocol/response.hpp"
#include "grbl_protocol/settings.hpp"
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
using grbl_protocol::alarm_description;
using grbl_protocol::decode_message;
using grbl_protocol::error_description;
using grbl_protocol::MachineState;
using grbl_protocol::MessageKind;
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

const char* state_name(MachineState s) {
    switch (s) {
        case MachineState::Idle:  return "Idle";
        case MachineState::Run:   return "Run";
        case MachineState::Hold:  return "Hold";
        case MachineState::Jog:   return "Jog";
        case MachineState::Alarm: return "Alarm";
        case MachineState::Door:  return "Door";
        case MachineState::Check: return "Check";
        case MachineState::Home:  return "Home";
        case MachineState::Sleep: return "Sleep";
    }
    return "?";
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

std::string format_status(const grbl_protocol::StatusReport& sr) {
    std::string s = "STATUS   state=";
    s += state_name(sr.state);
    char buf[64];
    if (sr.sub_state) {
        std::snprintf(buf, sizeof(buf), ":%d", *sr.sub_state);
        s += buf;
    }
    if (sr.mpos) {
        std::snprintf(buf, sizeof(buf), " mpos=(%.3f,%.3f,%.3f)",
                      sr.mpos->x, sr.mpos->y, sr.mpos->z);
        s += buf;
    }
    if (sr.wpos) {
        std::snprintf(buf, sizeof(buf), " wpos=(%.3f,%.3f,%.3f)",
                      sr.wpos->x, sr.wpos->y, sr.wpos->z);
        s += buf;
    }
    if (sr.fs) {
        std::snprintf(buf, sizeof(buf), " fs=(%.0f,%.0f)", sr.fs->feed, sr.fs->spindle);
        s += buf;
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
        std::printf("SETTING  $%d=%s\n", s->number, s->value.c_str());
        return;
    }
    if (auto m = parse_push_message(line)) {
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

// Single-char lines matching these are sent raw (no \n) so they're
// interpreted as GRBL real-time commands rather than regular g-code lines.
bool is_realtime(char c) {
    return c == '?' || c == '!' || c == '~' || c == '\x18';
}

void handle_user_line(int fd, std::string_view line) {
    if (line.empty()) return;
    // Force the next STATUS to print regardless of whether the controller's
    // state actually changes: gives the user immediate visible confirmation
    // that their command landed, even when state is stable (e.g. in Hold).
    g_last_status.clear();
    if (line.size() == 1 && is_realtime(line[0])) {
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
