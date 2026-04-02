#include "network/http_server.h"
#include <WebServer.h>
#include <ArduinoJson.h>

static WebServer server(80);

static void sendStatus(MachineState& machine_state) {
    StaticJsonDocument<320> doc;
    doc["temperature"].set(machine_state.current_temperature);
    doc["output"].set(machine_state.heater_power);
    doc["setpoint"].set(machine_state.target_temperature);
    doc["mode"].set(controlModeToString(machine_state.mode));

    doc["kp"].set(machine_state.kp);
    doc["ki"].set(machine_state.ki);
    doc["kd"].set(machine_state.kd);
    doc["pid_sample_time"].set(machine_state.pid_sample_time);

    doc["relay_window_size"].set(machine_state.relay_window_size);
    doc["temperature_ema_alpha"].set(machine_state.temperature_ema_alpha);

    doc["manual_power"].set(machine_state.manual_power);

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
};

static void handleConfig(MachineState& machine_state) {
        StaticJsonDocument<320> doc;
        DeserializationError error = deserializeJson(doc, server.arg("plain"));
        if (error) {
            server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
            return;
        }


        if (doc.containsKey("mode")) {
            String mode_str = doc["mode"].as<String>();
            if (mode_str == "PID") {
                machine_state.mode = ControlMode::PID;
            } else if (mode_str == "MANUAL") {
                machine_state.mode = ControlMode::MANUAL_MODE;
            } else {
                server.send(400, "application/json", "{\"error\":\"Invalid mode\"}");
                return;
            }
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
        
}

static void handleNotFound() {
    server.send(404, "application/json", "{\"error\":\"not found\"}");
}

void initHTTPServer(MachineState& machine_state) {
    server.enableCORS(true);
    server.on("/api/config", HTTP_GET, [&machine_state]() {
        sendStatus(machine_state);
    });
    server.on("/api/config", HTTP_PATCH, [&machine_state]() {
        handleConfig(machine_state);
        sendStatus(machine_state);
    });
    // Serve 204 no content on OPTIONS request, to fix CORS preflight.
    server.on("/api/config", HTTP_OPTIONS, []() {
        server.send(204);
    });
    server.onNotFound(handleNotFound);
    server.begin();
};

void handleHTTPServer() {
    server.handleClient();
};
