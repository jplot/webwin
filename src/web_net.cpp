// web_net.cpp — datagrams between pages (see web_net.h). Outgoing ones run Module.webnet.send on
// the page's main thread, where the page keeps its connections; incoming ones wait in a ring the
// page fills and the program's network thread empties, one producer and one consumer.
#ifdef __EMSCRIPTEN__

#include <webwin/web_net.h>

#include <emscripten.h>
#ifdef __EMSCRIPTEN_PTHREADS__
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#endif

#include <atomic>
#include <cstdlib>
#include <cstring>

namespace {

constexpr uint32_t kSlots = 256;
constexpr uint32_t kSlotData = 4096;

struct Slot {
    uint32_t length;
    uint8_t ip[4];
    uint16_t port;
    uint8_t data[kSlotData];
};

Slot g_slots[kSlots];
std::atomic<uint32_t> g_head{0}, g_tail{0};

struct Packet {
    uint32_t ip, port, length;
    uint8_t data[];
};

EM_JS(void, webnet_forward, (uint32_t ip, uint32_t port, const uint8_t *data, uint32_t length), {
    const net = Module['webnet'];
    if (net) net.send([ip >>> 24, (ip >>> 16) & 255, (ip >>> 8) & 255, ip & 255].join('.'), port,
                      HEAPU8.slice(data, data + length));
});

EM_JS(void, webnet_install, (), {
    Module['webnetDeliver'] = (address, port, bytes) => {
        const data = _webnet_slot(bytes.length);
        if (!data) return false;
        HEAPU8.set(bytes, data);
        _webnet_commit(address.split('.').reduce((value, part) => value * 256 + Number(part), 0), port, bytes.length);
        return true;
    };
});

void Forward(void *p)
{
    Packet *packet = static_cast<Packet *>(p);
    webnet_forward(packet->ip, packet->port, packet->data, packet->length);
    free(packet);
}

// Global constructors run on the page's main thread, before main().
const bool g_installed = (webnet_install(), true);

} // namespace

extern "C" EMSCRIPTEN_KEEPALIVE uint8_t *webnet_slot(uint32_t length)
{
    uint32_t head = g_head.load(std::memory_order_relaxed);
    if (length > kSlotData || head - g_tail.load(std::memory_order_acquire) >= kSlots) return nullptr;
    return g_slots[head % kSlots].data;
}

extern "C" EMSCRIPTEN_KEEPALIVE void webnet_commit(uint32_t ip, uint32_t port, uint32_t length)
{
    uint32_t head = g_head.load(std::memory_order_relaxed);
    Slot &slot = g_slots[head % kSlots];
    slot.length = length;
    slot.ip[0] = ip >> 24, slot.ip[1] = ip >> 16, slot.ip[2] = ip >> 8, slot.ip[3] = ip;
    slot.port = (uint16_t)port;
    g_head.store(head + 1, std::memory_order_release);
}

void WebNet_Send(const uint8_t ip[4], uint16_t port, const void *data, size_t length)
{
    Packet *packet = static_cast<Packet *>(malloc(sizeof(Packet) + length));
    packet->ip = (uint32_t)ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
    packet->port = port;
    packet->length = (uint32_t)length;
    memcpy(packet->data, data, length);
#ifdef __EMSCRIPTEN_PTHREADS__
    if (!emscripten_is_main_runtime_thread()) {
        emscripten_proxy_async(emscripten_proxy_get_system_queue(), emscripten_main_runtime_thread_id(), Forward,
                               packet);
        return;
    }
#endif
    Forward(packet);
}

size_t WebNet_Receive(uint8_t ip[4], uint16_t *port, void *buffer, size_t capacity)
{
    for (;;) {
        uint32_t tail = g_tail.load(std::memory_order_relaxed);
        if (tail == g_head.load(std::memory_order_acquire)) return 0;
        const Slot &slot = g_slots[tail % kSlots];
        size_t length = slot.length;
        if (length <= capacity) {
            memcpy(ip, slot.ip, 4);
            *port = slot.port;
            memcpy(buffer, slot.data, length);
        }
        g_tail.store(tail + 1, std::memory_order_release);
        if (length <= capacity) return length;
    }
}

#endif // __EMSCRIPTEN__
