/*
  Exploration of how to best implement the button interface.
  The buttons are used to change the PID parameters, and the mode.

*/

#include <AceButton.h>
#include <LiquidCrystal_I2C.h>

// ── Buttons ──────────────────────────────────────────────
const int BTN_NEXT_PIN = 10;
const int BTN_INC_PIN  = 8;
const int BTN_DEC_PIN  = 9;

constexpr int ButtonPins[] = {BTN_NEXT_PIN, BTN_INC_PIN, BTN_DEC_PIN};
constexpr size_t NUM_BUTTONS = sizeof(ButtonPins) / sizeof(ButtonPins[0]);

ace_button::AceButton buttons[NUM_BUTTONS];

LiquidCrystal_I2C lcd(0x27,20,4);
bool lastBlinkState = true;


// Params

struct Param {
  const char* name;
  float value, min, max, step;
};
Param params[] = {
    {"Kp", 1, 0, 100, 2},
    {"Ki", 0, 0, 10, 1},
};
size_t currentParam = 0;
constexpr size_t NUM_PARAMS = sizeof(params) / sizeof(params[0]);

// UI
long lastUiUpdate = millis();

void handlePress(ace_button::AceButton*, uint8_t, uint8_t);

void setup() {
  for (size_t i = 0; i < NUM_BUTTONS; i++) {
    pinMode(ButtonPins[i], INPUT_PULLUP);
    buttons[i].init(ButtonPins[i], HIGH, i);
    buttons[i].setEventHandler(handlePress);
    ace_button::ButtonConfig* config = buttons[i].getButtonConfig();
    config->setFeature(ace_button::ButtonConfig::kFeatureRepeatPress);
    config->setFeature(ace_button::ButtonConfig::kFeatureSuppressAfterRepeatPress);
    config->setRepeatPressInterval(500);
  }
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.cursor();
  lcd.blink();
}

void loop() {
  for (size_t i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].check();
  }
  handleUI();
}


void handleUI() {
  if (millis() - lastUiUpdate > 100) {
    lcd.setCursor(0, 0);
    for (size_t i = 0; i < NUM_PARAMS; i++) {
      bool isCurrentParam = i == currentParam;
      lcd.print(params[i].value, 0);
      lcd.print(" ");
    }
    lcd.setCursor(currentParam * 2, 0);
    lastUiUpdate = millis();
  }
}

//void handlePress(int buttonId) {
void handlePress(ace_button::AceButton* button, uint8_t eventType,
    uint8_t /*buttonState*/) {
    
  int step;
  switch (eventType) {
    case ace_button::AceButton::kEventReleased:
      step = 1;
      break;
    case ace_button::AceButton::kEventRepeatPressed:
      step = 4;
      break;
    default:
      return;
  }

  switch (button->getId()) {
    case 0:
      currentParam = (currentParam + 1) % NUM_PARAMS;
      break;
    case 1: {
      auto &p = params[currentParam];
      p.value = constrain(p.value + p.step * step, p.min, p.max);
      break;
    }
    case 2: {
      auto &p = params[currentParam];
      p.value = constrain(p.value - p.step * step, p.min, p.max);
    }
  }
}
