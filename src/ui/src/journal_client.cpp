#include "journal_client.h"
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#if __has_include("journal_config.h")
#include "journal_config.h"
#endif
#ifndef ESPRESSO_API_URL
#define ESPRESSO_API_URL ""
#endif

static QueueHandle_t requests, results;
static String base_url;

static bool request(JournalJob &job, const char *method, const String &path,
                    const String &body, JsonDocument &response) {
    if (WiFi.status() != WL_CONNECTED) {
        job.error = "Not saved: WiFi unavailable. Retry when connected.";
        return false;
    }
    WiFiClient socket;
    HTTPClient http;
    http.setConnectTimeout(1500);
    http.setTimeout(2000);
    http.useHTTP10(true); // stream JSON without HTTP chunk framing
    if (!http.begin(socket, base_url + "/api/v1" + path)) {
        job.error = "Cannot connect to journal server.";
        return false;
    }
    http.addHeader("Content-Type", "application/json");
    const int code = http.sendRequest(method, body);
    if (code <= 0) {
        job.error = "Server unavailable. Retry when connected.";
        http.end();
        return false;
    }
    // Stream/filter the response: never buffer bag notes or entire shot histories
    // on the display. Active bags are fetched in pages of 20.
    JsonDocument filter;
    for (const char *key : {"revision", "id", "name", "roaster", "roast_date", "archived",
                            "grind_setting", "dose_g", "target_yield_g", "yield_g", "total", "detail"}) {
        filter[key] = true;
    }
    for (const char *key : {"id", "name", "roaster", "roast_date"}) filter["coffee"][key] = true;
    for (const char *key : {"id", "name", "roaster", "roast_date"}) filter["items"][0][key] = true;
    auto error = deserializeJson(response, http.getStream(), DeserializationOption::Filter(filter),
                                 DeserializationOption::NestingLimit(12));
    http.end();
    if (code < 200 || code >= 300) {
        job.error = "Journal error " + String(code) + ". ";
        job.error += response["detail"].is<const char *>()
            ? response["detail"].as<String>() : "Check settings and retry.";
        return false;
    }
    if (error) {
        job.error = "Invalid server response. Check server address.";
        return false;
    }
    return true;
}

static bool getSetup(JournalJob &job, JsonDocument &doc) {
    if (!request(job, "GET", "/setup", "", doc)) return false;
    if (!doc["revision"].is<uint32_t>() || doc["revision"].as<uint32_t>() == 0) {
        job.error = "Invalid setup revision from server.";
        return false;
    }
    return true;
}

static void perform(JournalJob &job) {
    JsonDocument doc;
    if (job.kind == JournalRequest::Setup) {
        job.ok = getSetup(job, doc);
    } else if (job.kind == JournalRequest::Coffees) {
        job.ok = request(job, "GET", "/coffees?limit=20&offset=" + String(job.offset), "", doc);
        if (job.ok && (!doc["items"].is<JsonArray>() || !doc["total"].is<uint32_t>())) {
            job.ok = false;
            job.error = "Invalid coffee list from server.";
        }
    } else if (job.kind == JournalRequest::SaveSetup) {
        // PUT replaces all fields. Preserve the latest website-managed dose and
        // target instead of clearing them or sending stale editor-open values.
        if (!getSetup(job, doc)) return;
        JsonDocument body;
        if (deserializeJson(body, job.body)) { job.error = "Invalid setup input."; return; }
        body["dose_g"] = doc["dose_g"];
        body["target_yield_g"] = doc["target_yield_g"];
        String text;
        serializeJson(body, text);
        doc.clear();
        job.ok = request(job, "PUT", "/setup", text, doc);
    } else {
        // Re-use the EXACT creation body/UUID on retry, even when the user has
        // since entered yield. A timed-out POST may already have committed.
        if (!request(job, "POST", "/shots", job.body, doc)) return;
        if (doc["id"].as<String>() != job.id) { job.error = "Unexpected shot response."; return; }
        job.ok = true;
        if (job.patch.length()) {
            doc.clear();
            job.ok = request(job, "PATCH", "/shots/" + job.id, job.patch, doc);
        }
    }
    if (job.ok) serializeJson(doc, job.response);
}

static void worker(void *) {
    JournalJob *job;
    for (;;) {
        if (xQueueReceive(requests, &job, portMAX_DELAY) == pdTRUE) {
            perform(*job);
            xQueueSend(results, &job, portMAX_DELAY);
        }
    }
}

bool journal_client_begin() {
    base_url = ESPRESSO_API_URL;
    while (base_url.endsWith("/")) base_url.remove(base_url.length() - 1);
    if (!base_url.startsWith("http://")) return false;
    requests = xQueueCreate(6, sizeof(JournalJob *));
    results = xQueueCreate(6, sizeof(JournalJob *));
    if (requests && results &&
        xTaskCreatePinnedToCore(worker, "journal-http", 12288, nullptr, 1, nullptr, 0) == pdPASS) return true;
    if (requests) vQueueDelete(requests);
    if (results) vQueueDelete(results);
    requests = results = nullptr;
    return false;
}

bool journal_client_enqueue(JournalJob *job) {
    return requests && xQueueSend(requests, &job, 0) == pdTRUE;
}
JournalJob *journal_client_result() {
    JournalJob *job = nullptr;
    if (results) xQueueReceive(results, &job, 0);
    return job;
}
