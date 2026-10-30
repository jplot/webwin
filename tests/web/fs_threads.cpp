// File reads through the per-thread block cache, from several pthreads, the page's index faked.
// Runs under Node: node fs_threads.js
#include <webwin/web_fs.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <random>
#include <thread>
#include <vector>

namespace {

constexpr long long kBlock = 262144;
constexpr long long kBytes = 80 * kBlock + 1000;   // as in fs_threads.pre.js

std::atomic<int> g_failures{0};

uint8_t Byte(long long p) { return (uint8_t)(p * 31 + p / 251); }

void Read(int id, long long offset, int len, const char *what)
{
    std::vector<uint8_t> buf(len);
    int got = kbweb_pread(id, (double)offset, buf.data(), len);
    bool ok = got == std::min<long long>(len, kBytes - offset);
    for (int i = 0; ok && i < got; ++i) ok = buf[i] == Byte(offset + i);
    if (!ok && ++g_failures <= 10) printf("[fs_threads] %s: %d bytes at %lld gave %d\n", what, len, offset, got);
}

} // namespace

int main()
{
    int id = kbweb_open("main/iw_38.iwd");
    // Shi No Numa stopped loading on a read across the oldest of a file's 64 cached blocks.
    Read(id, 63 * kBlock, 16, "oldest block");
    for (long long b = 0; b < 63; ++b) Read(id, b * kBlock, 16, "filling the cache");
    Read(id, 64 * kBlock - 8, 16, "across the oldest block");

    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t)
        threads.emplace_back([id, t] {
            std::mt19937 rng(t + 1);
            for (int k = 0; k < 3000; ++k) {
                long long offset = rng() % kBytes;
                int len = k % 10 ? 1 + rng() % 65536 : 1 + rng() % 400000;
                Read(id, offset, len, "random read");
            }
        });
    for (std::thread &t : threads) t.join();
    printf("[fs_threads] %s: %d failures\n", g_failures ? "FAILED" : "ok", g_failures.load());
    return g_failures ? 1 : 0;
}
