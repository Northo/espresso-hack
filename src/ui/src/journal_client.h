#pragma once
#include <Arduino.h>

enum class JournalRequest { Setup, Coffees, SaveSetup, Submit };
struct JournalJob {
    JournalRequest kind;
    uint32_t token = 0;
    uint32_t offset = 0;
    String body;
    String patch;
    String id;
    String response;
    String error;
    bool ok = false;
};

// Ownership: enqueue succeeds -> worker owns it until journal_client_result().
// Only the worker touches HTTP. Results/LVGL are handled on the Arduino loop.
bool journal_client_begin();
bool journal_client_enqueue(JournalJob *job);
JournalJob *journal_client_result();
