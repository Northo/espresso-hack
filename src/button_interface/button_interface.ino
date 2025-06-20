#include <AceButton.h>
#include <LiquidCrystal_I2C.h>
#include <Array.h>
#include <avr/pgmspace.h>

using namespace ace_button;

// ── Pin Definitions ──────────────────────────────────────
constexpr uint8_t BTN_NEXT_PIN = 10;
constexpr uint8_t BTN_INC_PIN  = 8;
constexpr uint8_t BTN_DEC_PIN  = 9;
Array<uint8_t, 3> ButtonPins = {{ BTN_NEXT_PIN, BTN_INC_PIN, BTN_DEC_PIN }};
constexpr size_t NUM_BUTTONS = 3;

// ── Parameter Struct & Count ─────────────────────────────
struct Param {
  float value, min, max, step;
};
constexpr size_t NUM_PARAMS = 2;

// ── Names in Flash (PROGMEM) ─────────────────────────────
const char kp_name[] PROGMEM = "Kp";
const char ki_name[] PROGMEM = "Ki";
const char* const ParamNames[NUM_PARAMS] PROGMEM = { kp_name, ki_name };

// ── Runtime Parameter Store ─────────────────────────────
Array<Param, NUM_PARAMS> params;

void initParams() {
  params[0] = { 1.0f, 0.0f, 10.0f, 1.0f };
  params[1] = { 0.0f, 0.0f, 10.0f, 1.0f };
}
size_t currentParam = 0;

// Forward declaration for button events
void handleButtonEvent(AceButton*, uint8_t, uint8_t);

// ── Button Manager ───────────────────────────────────────
class ButtonManager {
public:
  ButtonManager() {
    for (size_t i = 0; i < NUM_BUTTONS; ++i) {
      buttons_[i].init(ButtonPins[i], HIGH, i);
      buttons_[i].setEventHandler(handleButtonEvent);
    }
    auto* cfg = buttons_[0].getButtonConfig();
    cfg->setFeature(ButtonConfig::kFeatureRepeatPress);
    cfg->setFeature(ButtonConfig::kFeatureSuppressAfterRepeatPress);
    cfg->setRepeatPressInterval(500);
  }

  void begin() {
    // Simple range-based pinMode setup
    for (auto pin : ButtonPins) {
      pinMode(pin, INPUT_PULLUP);
    }
  }

  void poll() {
    for (size_t i = 0; i < NUM_BUTTONS; ++i) {
      buttons_[i].check();
    }
  }

private:
  Array<AceButton, NUM_BUTTONS> buttons_;
};

// ── Display Manager ──────────────────────────────────────
class Display {
public:
  Display() : lcd_(0x27, 20, 4) {}

  void begin() {
    lcd_.init();
    lcd_.backlight();
  }

  void showAll() {
    lcd_.clear();
    for (size_t i = 0; i < NUM_PARAMS; ++i) {
      // Fetch name from PROGMEM
      char nameBuf[6];
      strcpy_P(nameBuf, (char*)pgm_read_word(&(ParamNames[i])));

      lcd_.setCursor(i * 7, 0);
      lcd_.print(nameBuf);
      lcd_.print(params[i].value, 0);

      if (i == currentParam) {
        lcd_.setCursor(i * 7 + strlen(nameBuf), 1);
        lcd_.print("^");
      }
    }
  }

private:
  LiquidCrystal_I2C lcd_;
};

// ── Globals ──────────────────────────────────────────────
ButtonManager buttonManager;
Display       display;

// Timing for display updates (avoid redrawing every loop)
unsigned long lastDisplayUpdate = 0;
constexpr unsigned long DISPLAY_INTERVAL = 200;

// ── Arduino Standard Hooks ───────────────────────────────────────────────
void setup() {
  initParams();          // initialize parameter values
  buttonManager.begin();
  display.begin();
}

void loop() {
  buttonManager.poll();
  unsigned long now = millis();
  if (now - lastDisplayUpdate >= DISPLAY_INTERVAL) {
    display.showAll();
    lastDisplayUpdate = now;
  }
}

// ── Button Event Handler ─────────────────────────────────────────────────
void handleButtonEvent(AceButton* button, uint8_t eventType, uint8_t /*state*/) {
  int step = 0;
  if (eventType == AceButton::kEventReleased) {
    step = 1;
  } else if (eventType == AceButton::kEventRepeatPressed) {
    step = 4;
  }
  if (!step) return;

  switch (button->getId()) {
    case 0:
      currentParam = (currentParam + 1) % NUM_PARAMS;
      break;
    case 1: {
      auto& p = params[currentParam];
      p.value = constrain(p.value + p.step * step, p.min, p.max);
      break;
    }
    case 2: {
      auto& p = params[currentParam];
      p.value = constrain(p.value - p.step * step, p.min, p.max);
      break;
    }
  }
}
