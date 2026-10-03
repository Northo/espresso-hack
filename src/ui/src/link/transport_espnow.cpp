#ifndef LINK_USE_SERIAL
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "transport.h"
#include "protocol.h"

// Only used with -DLINK_NO_OTA. Otherwise the display joins the router (for OTA), which puts ESP-NOW
// on the router's channel automatically, same as the controller.
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
#ifdef LINK_DEBUG
volatile uint32_t g_rx_count, g_rx_bad;
#endif

void onRecv(const uint8_t *mac, const uint8_t *data, int len)
{
#ifdef LINK_DEBUG
    g_rx_count++;
#endif
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
#ifdef LINK_NO_OTA
        // Not joining the router: pin the channel to the one the controller is on.
        WiFi.disconnect();
        esp_wifi_set_channel(LINK_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
#endif
        if (esp_now_init() != ESP_OK) return false;
        esp_now_register_recv_cb(onRecv);

        esp_now_peer_info_t peer = {};
        memcpy(peer.peer_addr, kPeer, 6);
        peer.channel = 0; // current channel
        peer.encrypt = false;
        bool ok = esp_now_add_peer(&peer) == ESP_OK;
#ifdef LINK_DEBUG
        Serial.printf("[link] init ok=%d mac=%s channel=%d\n", ok, WiFi.macAddress().c_str(), WiFi.channel());
        int n = WiFi.scanNetworks();
        for (int i = 0; i < n; i++)
            Serial.printf("[link] AP '%s' ch %d rssi %d\n", WiFi.SSID(i).c_str(), WiFi.channel(i), WiFi.RSSI(i));
#ifdef LINK_NO_OTA
        esp_wifi_set_channel(LINK_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE); // scan may have moved it
#endif
#endif
        return ok;
    }

    void poll() override
    {
#ifdef LINK_DEBUG
        static uint32_t last = 0;
        if (millis() - last > 2000) {
            last = millis();
            uint8_t ch; wifi_second_chan_t sc;
            esp_wifi_get_channel(&ch, &sc);
            Serial.printf("[link] channel=%u rx=%u\n", ch, (unsigned)g_rx_count);
        }
#endif
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
