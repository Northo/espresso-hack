#ifndef LINK_USE_SERIAL
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "transport.h"
#include "protocol.h"

// Must match the WiFi channel the controller is on (it joins your AP, so it uses the AP's channel).
#ifndef LINK_ESPNOW_CHANNEL
#define LINK_ESPNOW_CHANNEL 1
#endif
// Peer MAC as a comma-separated list, e.g. -DLINK_PEER_MAC=0x24,0x6F,0x28,0x00,0x00,0x01. Default: broadcast.
#ifndef LINK_PEER_MAC
#define LINK_PEER_MAC 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
#endif

namespace {

struct Packet {
    uint8_t len;
    uint8_t data[proto::MAX_PAYLOAD];
};

QueueHandle_t g_queue;
const uint8_t kPeer[6] = {LINK_PEER_MAC};

// Runs in the WiFi task: only queue, handle in poll().
void onRecv(const uint8_t *mac, const uint8_t *data, int len)
{
    if (len <= 0 || len > (int)proto::MAX_PAYLOAD) return;
    Packet p;
    p.len = len;
    memcpy(p.data, data, len);
    xQueueSend(g_queue, &p, 0);
}

class EspNowTransport : public Transport {
public:
    bool begin() override
    {
        g_queue = xQueueCreate(8, sizeof(Packet));
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        esp_wifi_set_channel(LINK_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
        if (esp_now_init() != ESP_OK) return false;
        esp_now_register_recv_cb(onRecv);

        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, kPeer, 6);
        peer.channel = 0; // current channel
        peer.encrypt = false;
        return esp_now_add_peer(&peer) == ESP_OK;
    }

    void poll() override
    {
        Packet p;
        while (xQueueReceive(g_queue, &p, 0) == pdTRUE) {
            if (rx_) rx_(p.data, p.len);
        }
    }

    bool send(const uint8_t *data, size_t len) override
    {
        return esp_now_send(kPeer, data, len) == ESP_OK;
    }
};

} // namespace

Transport *createTransport()
{
    static EspNowTransport t;
    return &t;
}
#endif
