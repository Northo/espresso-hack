#include "network/display_link.h"
#include "config.h"
#include "cleaning.h"
#include "protocol.h"
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_log.h>
#include <string.h>

static const char *TAG = "DisplayLink";

constexpr unsigned long STATE_INTERVAL_MS = 250;
constexpr double SETPOINT_MIN = 20.0, SETPOINT_MAX = 160.0;

// Broadcast: no pairing needed. Set to the display's MAC to unicast instead.
static const uint8_t PEER[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct Packet {
    uint8_t len;
    uint8_t data[proto::MAX_PAYLOAD];
};

static QueueHandle_t rx_queue;
static double normal_setpoint = DEFAULT_TARGET_TEMPERATURE; // restored when leaving steam

// Runs in the WiFi task: just queue, handle in handleDisplayLink().
static void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
    if (len <= 0 || len > (int)proto::MAX_PAYLOAD) return;
    Packet p;
    p.len = len;
    memcpy(p.data, data, len);
    xQueueSend(rx_queue, &p, 0);
}

void initDisplayLink(MachineState &) {
    rx_queue = xQueueCreate(8, sizeof(Packet));

    if (esp_now_init() != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_init failed");
        return;
    }
    esp_now_register_recv_cb(onRecv);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, PEER, 6);
    peer.channel = 0; // current WiFi channel
    peer.encrypt = false;
    if (esp_now_add_peer(&peer) != ESP_OK) {
        ESP_LOGE(TAG, "esp_now_add_peer failed");
        return;
    }
    ESP_LOGI(TAG, "ESP-NOW ready on channel %d, MAC %s", WiFi.channel(), WiFi.macAddress().c_str());
}

static void applyCommand(MachineState &state, const proto::CommandMsg &cmd) {
    const bool steam = state.mode == ControlMode::BANG_BANG;
    ESP_LOGI(TAG, "command %u value %d", cmd.cmd, cmd.value);

    switch (cmd.cmd) {
    case proto::CMD_CLEAN_START:
        startCleaning(state);
        break;

    case proto::CMD_CLEAN_CANCEL:
        cancelCleaning(state);
        break;

    case proto::CMD_CLEAN_CONTINUE:
        continueCleaning(state);
        break;

    case proto::CMD_BREW_START:
        if (state.clean_state != CleanState::IDLE) break; // cleaning in progress
        if (!state.auto_brew_enabled) {
            state.auto_brew_enabled = true;
            state.auto_brew_start_time = millis();
            state.brew_hold = false;
        }
        break;

    case proto::CMD_BREW_STOP:
        state.auto_brew_enabled = false;
        state.brew_hold = false;
        state.pump_active = false;
        state.solenoid_active = false;
        break;

    case proto::CMD_BREW_HOLD_TOGGLE:
        if (state.auto_brew_enabled) state.brew_hold = !state.brew_hold;
        break;

    case proto::CMD_STEAM_TOGGLE:
        if (!steam) {
            normal_setpoint = state.target_temperature;
            state.mode = ControlMode::BANG_BANG;
            state.target_temperature = STEAM_TARGET_TEMPERATURE;
        } else {
            state.mode = ControlMode::PID;
            state.target_temperature = normal_setpoint;
        }
        break;

    case proto::CMD_SET_SETPOINT: {
        if (steam) break; // the steam target is fixed
        double t = cmd.value / 10.0;
        if (t >= SETPOINT_MIN && t <= SETPOINT_MAX) state.target_temperature = t;
        break;
    }

    case proto::CMD_SET_BREW_DURATION:
        if (cmd.value > 0) state.auto_brew_duration = (long)cmd.value * 1000;
        break;
    }
}

// Mirrors the stage timing in controller.cpp (preinfusion -> bloom -> brewing).
static void brewProgress(const MachineState &state, uint16_t &elapsed_s, uint8_t &stage) {
    elapsed_s = 0;
    stage = proto::STAGE_NONE;
    if (!state.auto_brew_enabled) return;

    unsigned long elapsed = millis() - state.auto_brew_start_time;
    elapsed_s = (uint16_t)min(elapsed / 1000UL, 65535UL);
    if (elapsed < (unsigned long)state.auto_brew_preinfusion_duration)
        stage = proto::STAGE_PREINFUSION;
    else if (elapsed < (unsigned long)(state.auto_brew_preinfusion_duration + state.auto_brew_bloom_duration))
        stage = proto::STAGE_BLOOM;
    else
        stage = proto::STAGE_BREWING;
}

static void sendState(const MachineState &state) {
    proto::StateMsg msg = {};
    msg.type = proto::MSG_STATE;
    msg.temperature_x10 = (int16_t)lround(state.current_temperature * 10);
    msg.setpoint_x10 = (int16_t)lround(state.target_temperature * 10);
    msg.brew_duration_s = (uint16_t)(state.auto_brew_duration / 1000);
    msg.flags = (state.auto_brew_enabled ? proto::FLAG_BREWING : 0) |
                (state.mode == ControlMode::BANG_BANG ? proto::FLAG_STEAM : 0) |
                (state.brew_hold ? proto::FLAG_HOLD : 0) |
                (state.clean_pump_phase && (state.clean_state == CleanState::DETERGENT ||
                                            state.clean_state == CleanState::WATER)
                     ? proto::FLAG_CLEAN_PUMP : 0);
    msg.clean_state = (uint8_t)state.clean_state;
    msg.clean_cycle = state.clean_cycle;
    uint16_t elapsed_s;
    uint8_t stage;
    brewProgress(state, elapsed_s, stage);
    msg.brew_elapsed_s = elapsed_s;
    msg.brew_stage = stage;
    esp_now_send(PEER, (const uint8_t *)&msg, sizeof(msg));
}

void handleDisplayLink(MachineState &state) {
    if (!rx_queue) return; // init failed

    Packet p;
    while (xQueueReceive(rx_queue, &p, 0) == pdTRUE) {
        if (p.len == sizeof(proto::CommandMsg) && p.data[0] == proto::MSG_COMMAND) {
            proto::CommandMsg cmd;
            memcpy(&cmd, p.data, sizeof(cmd));
            applyCommand(state, cmd);
        }
    }

    static unsigned long last_send = 0;
    if (millis() - last_send >= STATE_INTERVAL_MS) {
        last_send = millis();
        sendState(state);
    }
}
