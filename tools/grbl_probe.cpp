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
using grbl_protocol::parse_push_message;
using grbl_protocol::parse_response;
using grbl_protocol::parse_setting;
using grbl_protocol::parse_status_report;
using grbl_protocol::ResponseKind;

namespace {

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
            case ResponseKind::Ok:    std::printf("RESPONSE ok\n"); break;
            case ResponseKind::Error: std::printf("RESPONSE error code=%d\n", *r->code); break;
            case ResponseKind::Alarm: std::printf("RESPONSE alarm code=%d\n", *r->code); break;
        }
        return;
    }
    if (auto s = parse_setting(line)) {
        std::printf("SETTING  $%d=%s\n", s->number, s->value.c_str());
        return;
    }
    if (auto m = parse_push_message(line)) {
        std::printf("PUSH     [%.*s] %.*s\n",
                    static_cast<int>(m->key.size()), m->key.data(),
                    static_cast<int>(m->value.size()), m->value.data());
        return;
    }
    if (auto sr = parse_status_report(line)) {
        std::printf("STATUS   state=%d\n", static_cast<int>(sr->state));
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

// Reads until poll() times out with no new bytes AND we've seen a terminating
// ok/error since the last command, or until the overall deadline elapses.
void drain_until_ok(int fd, std::chrono::milliseconds idle_ms, std::chrono::milliseconds deadline_ms) {
    auto start = std::chrono::steady_clock::now();
    std::string buf;
    bool got_terminator = false;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (now - start > deadline_ms) break;
        if (got_terminator) break;

        pollfd pfd{fd, POLLIN, 0};
        int rc = ::poll(&pfd, 1, static_cast<int>(idle_ms.count()));
        if (rc < 0) {
            if (errno == EINTR) continue;
            std::perror("poll");
            return;
        }
        if (rc == 0) continue;

        char chunk[1024];
        auto n = ::recv(fd, chunk, sizeof(chunk), 0);
        if (n <= 0) {
            if (n < 0) std::perror("recv");
            return;
        }
        buf.append(chunk, static_cast<size_t>(n));

        size_t pos;
        while ((pos = buf.find('\n')) != std::string::npos) {
            std::string_view line(buf.data(), pos);
            if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
            classify(line);
            if (auto r = parse_response(line); r && r->kind != ResponseKind::Alarm) {
                got_terminator = true;
            }
            buf.erase(0, pos + 1);
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

    drain_until_ok(fd, 300ms, 2000ms);

    if (!send_all(fd, "$I\n")) { ::close(fd); return 1; }
    drain_until_ok(fd, 300ms, 3000ms);

    if (!send_all(fd, "$$\n")) { ::close(fd); return 1; }
    drain_until_ok(fd, 300ms, 5000ms);

    ::close(fd);
    return 0;
}
