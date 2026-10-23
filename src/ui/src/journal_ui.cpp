// Handwritten LVGL extension. Generated EEZ output in ui/ is never edited.
#include "journal_ui.h"
#include "journal_client.h"
#include "journal_tracker.h"
#include "link/controller_link.h"
#include "ui/screens.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <lvgl.h>
#include <math.h>
#include <stdlib.h>
#include <vector>

namespace {
struct Coffee { String id, label; };
std::vector<Coffee> coffees;
JsonDocument current_setup;
JournalTracker tracker;
bool enabled, brewing, have_state, setup_pending, save_pending, coffees_pending, submit_pending;
uint32_t last_state_ms, last_poll_ms, last_announce_ms, acknowledged_revision;
uint32_t editor_token, shot_token, coffee_page, coffee_total;
uint8_t since_brew = 255;
proto::ShotMsg pending_result = {};
bool want_result;
String draft_coffee, draft_label, draft_grind, shot_body, shot_id;
String main_status, editor_status, result_status;
lv_obj_t *summary_label, *main_status_label;
lv_obj_t *overlay, *editor, *picker, *result, *keyboard, *input;
lv_obj_t *coffee_label, *editor_message, *result_message, *save_button;
lv_obj_t *coffee_button, *editor_save;
enum class View { None, Editor, Picker, Result };
View view = View::None;

String number(JsonVariantConst value) {
    return value.isNull() ? "" : value.as<String>();
}
String singleLine(String s) {
    s.replace("\n", " "); s.replace("\r", " ");
    return s;
}
String coffeeLabel(JsonVariantConst c) {
    if (c.isNull()) return "No coffee selected";
    return singleLine(c["name"].as<String>()) + " / " + singleLine(c["roaster"] | "Unknown roaster") +
        " / " + String(c["roast_date"] | "Roast date unknown");
}
bool freshController() { return have_state && uint32_t(millis() - last_state_ms) < 2000; }
void statusText(const String &text) {
    main_status = text;
    if (main_status_label) lv_label_set_text(main_status_label, text.c_str());
}
void updateSummary() {
    String name = current_setup["coffee"]["name"] | "Select coffee";
    String grind = number(current_setup["grind_setting"]);
    String text = name + "  |  Grind " + (grind.length() ? grind : "--");
    lv_label_set_text(summary_label, text.c_str());
}
lv_obj_t *label(lv_obj_t *parent, const String &text, int x, int y, int w, int h = 32) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, w, h);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
    lv_label_set_text(obj, text.c_str());
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    return obj;
}
lv_obj_t *button(lv_obj_t *parent, const char *text, int x, int y, int w,
                 lv_event_cb_t callback, void *data = nullptr, int h = 52) {
    lv_obj_t *obj = lv_button_create(parent);
    lv_obj_set_pos(obj, x, y); lv_obj_set_size(obj, w, h);
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_add_event_cb(obj, callback, LV_EVENT_CLICKED, data);
    lv_obj_t *caption = lv_label_create(obj);
    lv_label_set_text(caption, text); lv_obj_center(caption);
    return obj;
}
void closeOverlay() {
    if (overlay) lv_obj_delete(overlay);
    overlay = editor = picker = result = keyboard = input = nullptr;
    coffee_label = editor_message = result_message = save_button = nullptr;
    coffee_button = editor_save = nullptr;
    view = View::None;
}
void newOverlay(View next) {
    closeOverlay();
    overlay = lv_obj_create(lv_layer_top());
    lv_obj_set_pos(overlay, 0, 0); lv_obj_set_size(overlay, 480, 480);
    lv_obj_set_style_bg_color(overlay, lv_color_hex(0x151B19), 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(overlay, lv_color_hex(0xF1F1E8), 0);
    lv_obj_set_style_text_font(overlay, &lv_font_montserrat_20, 0);
    lv_obj_set_style_radius(overlay, 0, 0); lv_obj_set_style_border_width(overlay, 0, 0);
    lv_obj_set_style_pad_all(overlay, 0, 0);
    lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    view = next;
}
bool enqueue(JournalJob *job) {
    if (enabled && journal_client_enqueue(job)) return true;
    delete job;
    return false;
}
void refreshSetup() {
    if (setup_pending || save_pending) return;
    auto *job = new JournalJob;
    job->kind = JournalRequest::Setup;
    setup_pending = enqueue(job);
    last_poll_ms = millis();
}
void showEditor();
void showPicker();
void requestCoffees(uint32_t offset) {
    if (coffees_pending) return;
    auto *job = new JournalJob;
    job->kind = JournalRequest::Coffees;
    job->offset = offset; job->token = editor_token;
    coffees_pending = enqueue(job);
}
void keyboardEvent(lv_event_t *event) {
    if (lv_event_get_code(event) == LV_EVENT_READY || lv_event_get_code(event) == LV_EVENT_CANCEL) {
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_state(input, LV_STATE_FOCUSED);
    }
}
void inputFocus(lv_event_t *) { if (keyboard) lv_obj_remove_flag(keyboard, LV_OBJ_FLAG_HIDDEN); }
void numericInput(const String &value, int y) {
    input = lv_textarea_create(overlay);
    lv_obj_set_pos(input, 220, y); lv_obj_set_size(input, 170, 52);
    lv_obj_set_style_text_font(input, &lv_font_montserrat_20, 0);
    lv_textarea_set_one_line(input, true);
    lv_textarea_set_accepted_chars(input, "0123456789.");
    lv_textarea_set_max_length(input, 24);
    lv_textarea_set_text(input, value.c_str());
    lv_obj_add_event_cb(input, inputFocus, LV_EVENT_FOCUSED, nullptr);
    lv_obj_add_event_cb(input, inputFocus, LV_EVENT_CLICKED, nullptr);
    keyboard = lv_keyboard_create(overlay);
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
    lv_keyboard_set_textarea(keyboard, input);
    lv_obj_set_align(keyboard, LV_ALIGN_TOP_LEFT);
    lv_obj_set_pos(keyboard, 80, 220); lv_obj_set_size(keyboard, 320, 180);
    lv_obj_add_event_cb(keyboard, keyboardEvent, LV_EVENT_ALL, nullptr);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
}
bool parseNumber(const String &text, bool optional, double &value) {
    if (!text.length()) return optional;
    char *end;
    value = strtod(text.c_str(), &end);
    return *end == '\0' && isfinite(value) && (optional ? value >= 0 : value > 0);
}
void chooseCoffee(lv_event_t *event) {
    intptr_t index = (intptr_t)lv_event_get_user_data(event);
    String id = index < 0 ? "" : coffees[index].id;
    if (id != draft_coffee) draft_grind = ""; // never carry another bag's grind silently
    draft_coffee = id;
    draft_label = index < 0 ? "No coffee selected" : coffees[index].label;
    showEditor();
}
void changeCoffee(lv_event_t *) {
    draft_grind = lv_textarea_get_text(input);
    showPicker(); requestCoffees(0);
}
void pickerRefresh(lv_event_t *) { requestCoffees(coffee_page); }
void pickerPrevious(lv_event_t *) { requestCoffees(coffee_page >= 20 ? coffee_page - 20 : 0); }
void pickerNext(lv_event_t *) { requestCoffees(coffee_page + 20); }
void pickerBack(lv_event_t *) { showEditor(); }
void showPicker() {
    newOverlay(View::Picker); picker = overlay;
    label(picker, "Choose coffee bag", 90, 28, 300);
    auto *list = lv_obj_create(picker);
    lv_obj_set_pos(list, 65, 75); lv_obj_set_size(list, 350, 270);
    lv_obj_set_style_pad_all(list, 6, 0);
    auto *none = button(list, "No coffee selected", 0, 0, 315, chooseCoffee, (void *)-1);
    (void)none;
    int y = 60;
    for (size_t i = 0; i < coffees.size(); ++i) {
        auto *b = button(list, "", 0, y, 315, chooseCoffee, (void *)(intptr_t)i, 70);
        auto *text = label(b, coffees[i].label, 4, 4, 305, 62);
        lv_obj_set_style_text_font(text, &lv_font_montserrat_14, 0);
        y += 78;
    }
    if (coffees.empty()) label(list, "No bags loaded. Add bags on the website, then Refresh.", 0, y, 310, 90);
    button(picker, "Back", 70, 355, 100, pickerBack);
    button(picker, "Refresh", 180, 355, 110, pickerRefresh);
    if (coffee_page) button(picker, "<", 300, 355, 48, pickerPrevious);
    if (coffee_page + 20 < coffee_total) button(picker, ">", 358, 355, 48, pickerNext);
    auto *hint = label(picker, editor_status, 130, 425, 220, 34);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
}
void cancelEditor(lv_event_t *) { closeOverlay(); }
void lockEditor(bool locked) {
    for (auto *obj : {input, coffee_button, editor_save}) {
        if (!obj) continue;
        if (locked) lv_obj_add_state(obj, LV_STATE_DISABLED);
        else lv_obj_remove_state(obj, LV_STATE_DISABLED);
    }
}
void saveSetup(lv_event_t *) {
    if (save_pending) return;
    draft_grind = lv_textarea_get_text(input);
    double grind = 0;
    if (!parseNumber(draft_grind, true, grind)) {
        lv_label_set_text(editor_message, "Enter a non-negative grind setting, or leave blank."); return;
    }
    JsonDocument body;
    if (draft_coffee.length()) body["coffee_id"] = draft_coffee; else body["coffee_id"] = nullptr;
    if (draft_grind.length()) body["grind_setting"] = grind; else body["grind_setting"] = nullptr;
    auto *job = new JournalJob;
    job->kind = JournalRequest::SaveSetup; job->token = editor_token;
    serializeJson(body, job->body);
    save_pending = enqueue(job);
    lv_label_set_text(editor_message, save_pending ? "Saving..." : "Journal busy or not configured. Retry.");
    lockEditor(save_pending);
}
void showEditor() {
    newOverlay(View::Editor); editor = overlay;
    label(editor, "Coffee & grind", 100, 35, 280);
    auto *b = button(editor, "", 70, 90, 340, changeCoffee, nullptr, 72);
    coffee_button = b;
    coffee_label = label(b, draft_label, 4, 4, 330, 62);
    lv_obj_set_style_text_font(coffee_label, &lv_font_montserrat_14, 0);
    label(editor, "Grind setting", 65, 192, 150);
    editor_message = label(editor, editor_status, 65, 245, 350, 65);
    editor_save = button(editor, "Save", 90, 335, 140, saveSetup);
    button(editor, "Cancel", 250, 335, 140, cancelEditor);
    auto *hint = label(editor, "Add new bags on the website.", 90, 405, 300, 32);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    numericInput(draft_grind, 180);
    lockEditor(save_pending);
}
void openEditor(lv_event_t *) {
    if (save_pending) { statusText("Setup save still in progress..."); return; }
    if (brewing) { statusText("Coffee settings apply before brewing."); return; }
    if (!enabled) { statusText("Configure journal server first."); return; }
    if (!current_setup["revision"].as<uint32_t>()) { refreshSetup(); statusText("Loading setup. Tap again when connected."); return; }
    ++editor_token;
    draft_coffee = current_setup["coffee"]["id"] | "";
    draft_label = coffeeLabel(current_setup["coffee"]);
    draft_grind = number(current_setup["grind_setting"]);
    editor_status = "Blank grind means unknown.";
    showEditor();
}
void skipResult(lv_event_t *) { closeOverlay(); }
void brewAgain(lv_event_t *) {
    if (!freshController()) { lv_label_set_text(result_message, "Controller unavailable."); return; }
    closeOverlay();
    controller_link_send_command(proto::CMD_BREW_START);
}
void resetShot() {
    ++shot_token; shot_body = ""; shot_id = ""; submit_pending = false; want_result = false;
    if (view == View::Result) closeOverlay();
}
void submit(const String &patch = "") {
    if (submit_pending) return;
    if (!shot_body.length()) {
        result_status = "Not saved: no setup at brew start.";
    } else {
        auto *job = new JournalJob;
        job->kind = JournalRequest::Submit; job->token = shot_token;
        job->body = shot_body; job->id = shot_id; job->patch = patch;
        submit_pending = enqueue(job);
        result_status = submit_pending ? "Saving..." : "Not saved. Journal busy or unconfigured.";
    }
    statusText(result_status);
    if (result_message) lv_label_set_text(result_message, result_status.c_str());
    if (save_button && submit_pending) lv_obj_add_state(save_button, LV_STATE_DISABLED);
}
void saveYield(lv_event_t *) {
    if (submit_pending) return;
    double value = 0;
    if (!parseNumber(lv_textarea_get_text(input), false, value)) {
        lv_label_set_text(result_message, "Enter measured yield greater than zero, in grams."); return;
    }
    JsonDocument patch; patch["yield_g"] = value;
    String text; serializeJson(patch, text);
    submit(text);
}
void retryShot(lv_event_t *) { submit(); }
void showResult(const proto::ShotMsg &shot) {
    newOverlay(View::Result); result = overlay;
    label(result, "Shot finished", 100, 35, 280);
    label(result, "Brew time: " + String(shot.duration_ms / 1000.0, 1) + " s", 90, 90, 300);
    label(result, "Actual yield (g)", 65, 150, 150, 60);
    result_message = label(result, result_status, 60, 230, 360, 64);
    save_button = button(result, "Save yield", 80, 305, 150, saveYield);
    if (submit_pending) lv_obj_add_state(save_button, LV_STATE_DISABLED);
    button(result, "Skip", 250, 305, 140, skipResult);
    button(result, "BREW", 140, 375, 200, brewAgain);
    button(result, "Retry log", 170, 430, 140, retryShot, nullptr, 32);
    numericInput("", 150); // never prefill actual yield with a target
}
void acceptResult(JournalJob &job) {
    if (job.kind == JournalRequest::Coffees) {
        coffees_pending = false;
        if (job.token != editor_token) {
            if (view == View::Picker) requestCoffees(0);
            return;
        }
        if (job.ok) {
            JsonDocument doc; deserializeJson(doc, job.response);
            coffees.clear();
            for (JsonVariantConst c : doc["items"].as<JsonArrayConst>()) {
                coffees.push_back({c["id"].as<String>(), coffeeLabel(c)});
            }
            coffee_page = job.offset; coffee_total = doc["total"].as<uint32_t>();
            editor_status = "Choose an active bag.";
        } else editor_status = job.error;
        if (view == View::Picker) showPicker();
    } else if (job.kind == JournalRequest::Setup || job.kind == JournalRequest::SaveSetup) {
        bool saved = job.kind == JournalRequest::SaveSetup;
        if (saved) save_pending = false; else setup_pending = false;
        if (job.ok) {
            deserializeJson(current_setup, job.response);
            updateSummary();
            // The controller's acknowledgement, not the HTTP response, confirms
            // that a subsequent physical/touch brew will use this revision.
            statusText("Syncing setup to controller...");
            if (saved && job.token == editor_token && view == View::Editor) closeOverlay();
        } else {
            statusText(job.error);
            if (saved && job.token == editor_token && editor_message) {
                lv_label_set_text(editor_message, job.error.c_str());
                lockEditor(false);
            }
        }
    } else if (job.token == shot_token) {
        submit_pending = false;
        result_status = job.ok ? (job.patch.length() ? "Yield saved." : "Shot saved. Enter measured yield or Skip.")
                               : "Not saved. " + job.error;
        statusText(result_status);
        if (result_message) lv_label_set_text(result_message, result_status.c_str());
        if (save_button) lv_obj_remove_state(save_button, LV_STATE_DISABLED);
    }
}
} // namespace

void journal_ui_begin() {
    enabled = journal_client_begin();
    auto *summary = button(objects.main, "", 95, 270, 290, openEditor, nullptr, 42);
    summary_label = label(summary, "Select coffee | Grind --", 0, 0, 265, 27);
    lv_label_set_long_mode(summary_label, LV_LABEL_LONG_DOT);
    main_status_label = label(objects.main, "", 85, 394, 310, 32);
    lv_label_set_long_mode(main_status_label, LV_LABEL_LONG_DOT);
    statusText(enabled ? "Connecting to journal..." : "Configure journal server.");
    if (enabled) refreshSetup();
}
void journal_ui_tick() {
    while (auto *job = journal_client_result()) { acceptResult(*job); delete job; }
    const uint32_t now = millis();
    if (want_result && !brewing && freshController() && since_brew < 60) {
        showResult(pending_result);
        want_result = false;
    }
    if (enabled && !brewing && view == View::None && now - last_poll_ms >= 5000) refreshSetup();
    uint32_t revision = current_setup["revision"] | uint32_t(0);
    if (revision && freshController() && now - last_announce_ms >= 500) {
        controller_link_send_setup(revision);
        last_announce_ms = now;
    }
}
void journal_on_state(const proto::StateMsg &state) {
    bool next_brewing = state.flags & proto::FLAG_BREWING;
    if (next_brewing && !brewing) closeOverlay();
    brewing = next_brewing;
    have_state = true; last_state_ms = millis(); since_brew = state.since_brew_s;
}
void journal_on_status(const proto::JournalStatusMsg &status) {
    bool next = tracker.boot_id != status.boot_id || status.sequence > tracker.sequence;
    tracker.status(status);
    if (next) resetShot();
    acknowledged_revision = status.setup_revision;
    if (main_status == "Syncing setup to controller..." && acknowledged_revision == current_setup["revision"].as<uint32_t>())
        statusText("Setup ready.");
}
void journal_on_shot(const proto::ShotMsg &shot) {
    bool next = shot.sequence > tracker.sequence;
    if (!tracker.accept(shot)) return;
    if (next) resetShot(); // the preceding status packet may have been lost
    if (shot.phase != 2) return; // aborted before main stage: no journal record
    char id[37];
    snprintf(id, sizeof(id), "%08lx-%04lx-4%03lx-8000-%012lx",
             (unsigned long)(shot.boot_id >> 32), (unsigned long)((shot.boot_id >> 16) & 0xFFFF),
             (unsigned long)(shot.boot_id & 0xFFF), (unsigned long)shot.sequence);
    shot_id = id;
    if (shot.setup_revision) {
        JsonDocument body;
        body["id"] = shot_id;
        body["setup_revision"] = shot.setup_revision;
        body["duration_s"] = shot.duration_ms / 1000.0;
        body["configured_preinfusion_s"] = shot.preinfusion_s;
        body["configured_bloom_s"] = shot.bloom_s;
        body["configured_brew_s"] = shot.configured_brew_s;
        body["temperature_setpoint_c"] = shot.setpoint_x10 / 10.0;
        if (shot.end_temperature_x10 != INT16_MIN) body["end_temperature_c"] = shot.end_temperature_x10 / 10.0;
        serializeJson(body, shot_body);
    }
    submit();
    pending_result = shot;
    want_result = true; // wait for the idle state, even if its packet was lost
}
