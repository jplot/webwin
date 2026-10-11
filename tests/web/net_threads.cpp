// Datagrams sent from several pthreads, looped back by the page and read on another pthread.
// Runs under Node: node net_threads.js
#include <webwin/web_net.h>

#include <emscripten/em_asm.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

namespace {

constexpr int kSenders = 4;
constexpr int kPerSender = 5000;

int g_failures = 0;

void Fail(const char *what, int sender, int seq)
{
    if (++g_failures <= 10) printf("[net_threads] %s: sender %d packet %d\n", what, sender, seq);
}

size_t Length(int seq) { return 6 + (seq * 37) % 1400; }

uint8_t Byte(int seq, size_t i) { return (uint8_t)(seq * 31 + i); }

} // namespace

int main()
{
    std::atomic<int> sent{0};
    std::vector<std::thread> senders;
    for (int s = 0; s < kSenders; ++s)
        senders.emplace_back([s, &sent] {
            uint8_t packet[1406];
            const uint8_t ip[4] = {10, 0, 0, (uint8_t)(s + 1)};
            for (int seq = 0; seq < kPerSender; ++seq) {
                size_t length = Length(seq);
                packet[0] = (uint8_t)s;
                for (int b = 0; b < 4; ++b) packet[1 + b] = (uint8_t)(seq >> (8 * b));
                for (size_t i = 5; i < length; ++i) packet[i] = Byte(seq, i);
                WebNet_Send(ip, (uint16_t)(3074 + s), packet, length);
                sent.fetch_add(1);
                if (seq % 64 == 63) std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });

    int received = 0, last[kSenders];
    for (int &l : last) l = -1;
    uint8_t buffer[2048], ip[4];
    uint16_t port;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(60);
    for (;;) {
        size_t length = WebNet_Receive(ip, &port, buffer, sizeof buffer);
        if (!length) {
            if (sent.load() == kSenders * kPerSender &&
                received + MAIN_THREAD_EM_ASM_INT({ return Module['webnetDropped']; }) == kSenders * kPerSender)
                break;
            if (std::chrono::steady_clock::now() > deadline) {
                printf("[net_threads] timed out: %d received\n", received);
                ++g_failures;
                break;
            }
            std::this_thread::yield();
            continue;
        }
        ++received;
        int s = buffer[0], seq = buffer[1] | buffer[2] << 8 | buffer[3] << 16 | buffer[4] << 24;
        if (s >= kSenders) { Fail("unknown sender", s, seq); continue; }
        if (seq <= last[s]) Fail("out of order", s, seq);
        last[s] = seq;
        if (length != Length(seq)) Fail("wrong length", s, seq);
        for (size_t i = 5; i < length; ++i)
            if (buffer[i] != Byte(seq, i)) { Fail("corrupt payload", s, seq); break; }
        if (ip[0] != 10 || ip[3] != s + 1 || port != 3074 + s) Fail("wrong address", s, seq);
        // A reader now and then slower than the page, so that the ring fills.
        if (received % 997 == 0) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    for (auto &t : senders) t.join();

    int dropped = MAIN_THREAD_EM_ASM_INT({ return Module['webnetDropped']; });
    printf("[net_threads] %s: %d sent, %d received, %d dropped, %d failures\n", g_failures ? "FAILED" : "ok",
           kSenders * kPerSender, received, dropped, g_failures);
    return g_failures ? 1 : 0;
}
