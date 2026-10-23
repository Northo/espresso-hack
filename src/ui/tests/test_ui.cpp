// Real LVGL widgets and production UI logic; mock only Arduino/network/transport.
// Include in this TU to inspect private transient state without test hooks in firmware.
#include "../src/journal_ui.cpp"
#include <cassert>
#include <deque>
#include <fstream>
#include <iostream>

objects_t objects = {};
static uint32_t clock_ms = 100;
unsigned long millis() { return clock_ms; }
static std::deque<JournalJob *> requested, completed;
static uint32_t sent_revision;
static proto::CommandId sent_command;
bool journal_client_begin() { return true; }
bool journal_client_enqueue(JournalJob *job) { requested.push_back(job); return true; }
JournalJob *journal_client_result() {
    if (completed.empty()) return nullptr;
    auto *job = completed.front(); completed.pop_front(); return job;
}
void controller_link_send_setup(uint32_t revision) { sent_revision = revision; }
void controller_link_send_command(proto::CommandId cmd, int16_t) { sent_command = cmd; }
static JournalJob *pop(JournalRequest kind) {
    assert(!requested.empty());
    auto *job = requested.front(); requested.pop_front(); assert(job->kind == kind); return job;
}
static void complete(JournalJob *job, bool ok, const char *response = "{}") {
    job->ok = ok; job->response = response; job->error = "Server unavailable";
    completed.push_back(job); journal_ui_tick();
}
static void flush(lv_display_t *display, const lv_area_t *, uint8_t *) { lv_display_flush_ready(display); }
static std::vector<uint8_t> pixels(480 * 480 * 2);
static void screenshot(const char *name) {
    lv_tick_inc(40);
    lv_timer_handler();
    lv_refr_now(nullptr);
    std::ofstream file(name, std::ios::binary);
    file << "P6\n480 480\n255\n";
    for (size_t p = 0; p < pixels.size(); p += 2) {
        uint16_t rgb = pixels[p] | (uint16_t(pixels[p + 1]) << 8);
        file.put((rgb >> 11) * 255 / 31);
        file.put(((rgb >> 5) & 63) * 255 / 63);
        file.put((rgb & 31) * 255 / 31);
    }
}

int main(int argc, char **argv) {
    lv_init();
    auto *display = lv_display_create(480, 480);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, pixels.data(), nullptr, pixels.size(), LV_DISPLAY_RENDER_MODE_DIRECT);
    lv_display_set_flush_cb(display, flush);
    objects.main = lv_obj_create(nullptr);
    lv_screen_load(objects.main);
    journal_ui_begin();
    complete(pop(JournalRequest::Setup), true,
        R"({"revision":12,"coffee":{"id":"bag-a","name":"Fazenda Esperanca","roaster":"Local roaster","roast_date":"2026-10-01"},"grind_setting":16.5,"dose_g":18,"target_yield_g":36})");
    proto::StateMsg state = {}; state.since_brew_s = 255;
    journal_on_state(state);
    clock_ms = 600; journal_ui_tick(); assert(sent_revision == 12);
    proto::JournalStatusMsg status = {proto::MSG_JOURNAL_STATUS, 123, 0, 12};
    journal_on_status(status); assert(main_status == "Setup ready.");

    // Coffee/grind editor, bag pagination, and switching never carry over grind.
    openEditor(nullptr); assert(view == View::Editor);
    assert(String(lv_textarea_get_text(input)) == "16.5");
    changeCoffee(nullptr);
    auto *list_request = pop(JournalRequest::Coffees);
    complete(list_request, true,
        R"({"total":21,"items":[{"id":"bag-b","name":"New bag","roaster":"Another roaster","roast_date":"2026-10-02"}]})");
    assert(view == View::Picker && coffee_total == 21);
    if (argc > 1) screenshot((String(argv[1]) + "/picker.ppm").c_str());
    auto *list = lv_obj_get_child(picker, 1);
    lv_obj_send_event(lv_obj_get_child(list, 1), LV_EVENT_CLICKED, nullptr);
    assert(view == View::Editor && draft_coffee == "bag-b");
    assert(String(lv_textarea_get_text(input)).empty());
    lv_textarea_set_text(input, "17.25"); saveSetup(nullptr);
    auto *save = pop(JournalRequest::SaveSetup);
    assert(lv_obj_has_state(input, LV_STATE_DISABLED));
    assert(lv_obj_has_state(editor_save, LV_STATE_DISABLED));
    JsonDocument saved; deserializeJson(saved, save->body);
    assert(saved["coffee_id"].as<String>() == "bag-b" && saved["grind_setting"] == 17.25);
    complete(save, false); assert(view == View::Editor);
    assert(!lv_obj_has_state(input, LV_STATE_DISABLED));
    assert(String(lv_textarea_get_text(input)) == "17.25"); // failure keeps draft
    if (argc > 1) {
        screenshot((String(argv[1]) + "/editor.ppm").c_str());
        inputFocus(nullptr);
        screenshot((String(argv[1]) + "/keyboard.ppm").c_str());
        lv_obj_send_event(keyboard, LV_EVENT_CANCEL, nullptr);
    }
    inputFocus(nullptr);
    lv_obj_update_layout(overlay);
    lv_area_t keyboard_area; lv_obj_get_coords(keyboard, &keyboard_area);
    assert(keyboard_area.x1 == 80 && keyboard_area.y1 == 220);
    assert(keyboard_area.x2 < 480 && keyboard_area.y2 < 480);
    assert(!lv_obj_has_flag(keyboard, LV_OBJ_FLAG_HIDDEN));
    cancelEditor(nullptr);

    // Result arrives before the idle state (dropped/reordered radio packet).
    status.sequence = 1; journal_on_status(status);
    state.flags = proto::FLAG_BREWING; journal_on_state(state);
    proto::ShotMsg shot = {proto::MSG_SHOT, 123, 1, 1, 12, 0, 2, 5, 30, 930, INT16_MIN};
    journal_on_shot(shot);
    shot.phase = 2; shot.duration_ms = 28400; shot.end_temperature_x10 = 923;
    journal_on_shot(shot);
    auto *upload = pop(JournalRequest::Submit);
    String original_body = upload->body;
    JsonDocument creation; deserializeJson(creation, original_body);
    assert(std::abs(creation["duration_s"].as<double>() - 28.4) < .001);
    assert(creation["setup_revision"] == 12);
    assert(creation["yield_g"].isNull());
    journal_ui_tick(); assert(view != View::Result);
    state.flags = 0; state.since_brew_s = 0; journal_on_state(state);
    journal_ui_tick(); assert(view == View::Result);
    assert(String(lv_textarea_get_text(input)).empty()); // not target yield
    assert(lv_obj_has_state(save_button, LV_STATE_DISABLED));
    lv_textarea_set_text(input, "36.7");
    complete(upload, true); assert(!lv_obj_has_state(save_button, LV_STATE_DISABLED));
    saveYield(nullptr);
    auto *yield = pop(JournalRequest::Submit);
    assert(yield->body == original_body);
    JsonDocument patch; deserializeJson(patch, yield->patch);
    assert(std::abs(patch["yield_g"].as<double>() - 36.7) < .001);
    complete(yield, false); assert(String(lv_textarea_get_text(input)) == "36.7");
    assert(result_status.find("Not saved") != String::npos);
    if (argc > 1) screenshot((String(argv[1]) + "/result.ppm").c_str());
    saveYield(nullptr); yield = pop(JournalRequest::Submit);
    assert(yield->body == original_body);
    complete(yield, true); assert(result_status == "Yield saved.");
    journal_on_shot(shot); assert(requested.empty()); // repeated result is not uploaded again

    // New brew is never gated by networking; old HTTP responses cannot change its UI.
    retryShot(nullptr); auto *old = pop(JournalRequest::Submit);
    brewAgain(nullptr); assert(sent_command == proto::CMD_BREW_START && view == View::None);
    status.sequence = 2; journal_on_status(status);
    result_status = "new shot";
    complete(old, false); assert(result_status == "new shot");
    shot.sequence = 3; shot.phase = 2; // next status packet missing entirely
    auto generation = shot_token;
    journal_on_shot(shot); assert(shot_token != generation);
    complete(pop(JournalRequest::Submit), true);
    closeOverlay();
    std::cout << "LVGL coffee/grind, measured yield, retries, stale responses and lost-state tests passed.\n";
}
