#include "network/http_server.h"
#include <WebServer.h>
#include <ArduinoJson.h>

static WebServer server(80);

// --- GET /api/status ---
// Live, read-only machine state: sensor readings and active outputs.

static void handleGetStatus(MachineState& machine_state) {
    StaticJsonDocument<128> doc;
    doc["temperature"].set(machine_state.current_temperature);
    doc["heater_power"].set(machine_state.heater_power);
    doc["mode"].set(controlModeToString(machine_state.mode));
    doc["pump_active"].set(machine_state.pump_active);
    doc["solenoid_active"].set(machine_state.solenoid_active);

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// --- GET /api/config ---
// All tunable configuration parameters.

static void handleGetConfig(MachineState& machine_state) {
    StaticJsonDocument<384> doc;
    doc["target_temperature"].set(machine_state.target_temperature);

    doc["kp"].set(machine_state.kp);
    doc["ki"].set(machine_state.ki);
    doc["kd"].set(machine_state.kd);
    doc["pid_sample_time"].set(machine_state.pid_sample_time);

    doc["relay_window_size"].set(machine_state.relay_window_size);
    doc["temperature_ema_alpha"].set(machine_state.temperature_ema_alpha);

    doc["manual_power"].set(machine_state.manual_power);

    doc["feed_forward_duration"].set(machine_state.feed_forward_duration);
    doc["feed_forward_power"].set(machine_state.feed_forward_power);
    doc["feed_forward_factor"].set(machine_state.feed_forward_factor);

    doc["auto_brew_preinfusion_duration"].set(machine_state.auto_brew_preinfusion_duration);
    doc["auto_brew_duration"].set(machine_state.auto_brew_duration);
    doc["auto_brew_bloom_duration"].set(machine_state.auto_brew_bloom_duration);

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

// --- PATCH /api/config ---
// Update any subset of tunable configuration parameters.

static void handlePatchConfig(MachineState& machine_state) {
    StaticJsonDocument<384> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    if (doc.containsKey("target_temperature")) {
        machine_state.target_temperature = doc["target_temperature"].as<double>();
    }
    if (doc.containsKey("kp")) {
        machine_state.kp = doc["kp"].as<double>();
    }
    if (doc.containsKey("ki")) {
        machine_state.ki = doc["ki"].as<double>();
    }
    if (doc.containsKey("kd")) {
        machine_state.kd = doc["kd"].as<double>();
    }
    if (doc.containsKey("pid_sample_time")) {
        machine_state.pid_sample_time = doc["pid_sample_time"].as<int>();
    }
    if (doc.containsKey("relay_window_size")) {
        machine_state.relay_window_size = doc["relay_window_size"].as<int>();
    }
    if (doc.containsKey("temperature_ema_alpha")) {
        machine_state.temperature_ema_alpha = doc["temperature_ema_alpha"].as<float>();
    }
    if (doc.containsKey("manual_power")) {
        machine_state.manual_power = doc["manual_power"].as<double>();
    }
    if (doc.containsKey("feed_forward_duration")) {
        machine_state.feed_forward_duration = doc["feed_forward_duration"].as<long>();
    }
    if (doc.containsKey("feed_forward_power")) {
        machine_state.feed_forward_power = doc["feed_forward_power"].as<double>();
    }
    if (doc.containsKey("feed_forward_factor")) {
        machine_state.feed_forward_factor = doc["feed_forward_factor"].as<double>();
    }
    if (doc.containsKey("auto_brew_preinfusion_duration")) {
        machine_state.auto_brew_preinfusion_duration = doc["auto_brew_preinfusion_duration"].as<long>();
    }
    if (doc.containsKey("auto_brew_duration")) {
        machine_state.auto_brew_duration = doc["auto_brew_duration"].as<long>();
    }
    if (doc.containsKey("auto_brew_bloom_duration")) {
        machine_state.auto_brew_bloom_duration = doc["auto_brew_bloom_duration"].as<long>();
    }

    handleGetConfig(machine_state);
}

// --- POST /api/mode ---
// Switch the active control mode.

static void handlePostMode(MachineState& machine_state) {
    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc.containsKey("mode")) {
        server.send(400, "application/json", "{\"error\":\"Missing field: mode\"}");
        return;
    }

    String mode_str = doc["mode"].as<String>();
    if (mode_str == "PID") {
        machine_state.mode = ControlMode::PID;
    } else if (mode_str == "MANUAL") {
        machine_state.mode = ControlMode::MANUAL_MODE;
    } else if (mode_str == "BANG_BANG") {
        machine_state.mode = ControlMode::BANG_BANG;
    } else {
        server.send(400, "application/json", "{\"error\":\"Invalid mode\"}");
        return;
    }

    handleGetStatus(machine_state);
}

// --- POST /api/brew ---
// Start or cancel an auto-brew cycle.

static void handlePostBrew(MachineState& machine_state) {
    StaticJsonDocument<64> doc;
    DeserializationError error = deserializeJson(doc, server.arg("plain"));
    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc.containsKey("active")) {
        server.send(400, "application/json", "{\"error\":\"Missing field: active\"}");
        return;
    }

    machine_state.auto_brew_enabled = doc["active"].as<bool>();

    handleGetStatus(machine_state);
}

// --- 404 ---

static void handleNotFound() {
    server.send(404, "application/json", "{\"error\":\"not found\"}");
}

// --- Init ---

void initHTTPServer(MachineState& machine_state) {
    server.enableCORS(true);

    server.on("/api/status", HTTP_GET, [&machine_state]() {
        handleGetStatus(machine_state);
    });
    server.on("/api/status", HTTP_OPTIONS, []() { server.send(204); });

    server.on("/api/config", HTTP_GET, [&machine_state]() {
        handleGetConfig(machine_state);
    });
    server.on("/api/config", HTTP_PATCH, [&machine_state]() {
        handlePatchConfig(machine_state);
    });
    server.on("/api/config", HTTP_OPTIONS, []() { server.send(204); });

    server.on("/api/mode", HTTP_POST, [&machine_state]() {
        handlePostMode(machine_state);
    });
    server.on("/api/mode", HTTP_OPTIONS, []() { server.send(204); });

    server.on("/api/brew", HTTP_POST, [&machine_state]() {
        handlePostBrew(machine_state);
    });
    server.on("/api/brew", HTTP_OPTIONS, []() { server.send(204); });

    server.onNotFound(handleNotFound);
    server.begin();
}

void handleHTTPServer() {
    server.handleClient();
}
