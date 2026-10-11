// web_net.h — datagrams between pages. The page carries them (WebRTC, a relay, another tab) through
// Module.webnet.send(address, port, bytes) and hands incoming ones to Module.webnetDeliver(address,
// port, bytes); the program only sees IPv4 addresses and ports. Emscripten only.
#ifndef WEBWIN_WEB_NET_H
#define WEBWIN_WEB_NET_H
#ifdef __EMSCRIPTEN__
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
// Queues a datagram for the page, from any thread; dropped, as UDP would, if the page has no transport.
void   WebNet_Send(const uint8_t ip[4], uint16_t port, const void *data, size_t length);
// The oldest datagram the page delivered: its length, 0 when there is none.
size_t WebNet_Receive(uint8_t ip[4], uint16_t *port, void *buffer, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif // __EMSCRIPTEN__
#endif
