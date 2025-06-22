#include <AceButton.h>
#include <LiquidCrystal_I2C.h>

namespace Config {
  // Pin definitions
  constexpr int NEXT_BUTTON_PIN = 8;
  constexpr int INC_BUTTON_PIN = 9;
  constexpr int DEC_BUTTON_PIN = 10;

  constexpr int LCD_PARAM_WIDTH = 3; // How many characters for displaying a parameter value

  // Button behaviour
  constexpr int REPEAT_PRESS_DELAY_MS = 700;
  constexpr int REPEAT_PRESS_INTERVAL_MS = 300;
}

class ParameterTuner;

class ButtonManager : public ace_button::IEventHandler {
public:
  ButtonManager(ParameterTuner* parameterTuner, int nextButtonPin, int incButtonPin, int decButtonPin)
    : parameterTuner(parameterTuner) {

    nextButtonConfig.setIEventHandler(this);
    repeatButtonConfig.setIEventHandler(this);
    repeatButtonConfig.setFeature(ace_button::ButtonConfig::kFeatureRepeatPress);
    repeatButtonConfig.setFeature(ace_button::ButtonConfig::kFeatureSuppressAfterRepeatPress);
    repeatButtonConfig.setRepeatPressDelay(Config::REPEAT_PRESS_DELAY_MS);
    repeatButtonConfig.setRepeatPressInterval(Config::REPEAT_PRESS_INTERVAL_MS);

    nextButton.init(&nextButtonConfig, nextButtonPin, HIGH, 0);
    incButton.init(&repeatButtonConfig, incButtonPin, HIGH, 1);
    decButton.init(&repeatButtonConfig, decButtonPin, HIGH, 2);
  };

  void begin() {
    pinMode(nextButton.getPin(), INPUT_PULLUP);
    pinMode(incButton.getPin(), INPUT_PULLUP);
    pinMode(decButton.getPin(), INPUT_PULLUP);
  }

  void check() {
    nextButton.check();
    incButton.check();
    decButton.check();
  }
private:
  ParameterTuner* parameterTuner;
  ace_button::AceButton nextButton;
  ace_button::AceButton incButton;
  ace_button::AceButton decButton;

  ace_button::ButtonConfig repeatButtonConfig;
  ace_button::ButtonConfig nextButtonConfig;

  void handleEvent(ace_button::AceButton* /* button */, uint8_t event_type, uint8_t /* buttonState */);
};

struct Parameter {
  float value;
  float min;
  float max;
  float step;
};

class CallbackInterface {
public:
  virtual ~CallbackInterface() {}
  virtual void invoke() = 0;
};

template<class T>
class Callback : public CallbackInterface {
public:
  Callback(T* instance, void (T::*func)())
    : instance(instance), memberFunc(func){};

  void invoke() {
    (instance->*memberFunc)();
  }

private:
  T* instance;
  void (T::*memberFunc)();
};


class ParameterTuner {
public:
  static constexpr size_t kParameterCount = 2;

  ParameterTuner()
    : buttonManager_(this, Config::NEXT_BUTTON_PIN, Config::INC_BUTTON_PIN, Config::DEC_BUTTON_PIN), parameters_({ { 1.0, 0.0, 400.0, 2.4 }, { 1.0, 0.0, 10.0, 1.0 } }){};

  void setParameter(int parameterId, float value) {
    auto& parameter = parameters_[parameterId];
    parameter.value = constrain(value, parameter.min, parameter.max);
    notify();
  };

  const Parameter* getParameter(int parameterId) {
    return &parameters_[parameterId];
  };

  void updateParameter(int parameterId, float change) {
    auto& parameter = parameters_[parameterId];
    setParameter(parameterId, parameter.value + change * parameter.step);
  }

  int getCurrentParam() {
    return currentParam;
  }

  void setCurrentParam(int param) {
    currentParam = param;
    notify();
  }

  void nextPram() {
    setCurrentParam((getCurrentParam() + 1) % kParameterCount);
  }

  bool addCallback(CallbackInterface* callback) {
    if (numCallbacks == kMaxCallbacks) {
      return false;
    }
    callbacks_[numCallbacks] = callback;
    numCallbacks++;
  }

  void begin() {
    buttonManager_.begin();
  }

  void check() {
    buttonManager_.check();
  }

private:
  ButtonManager buttonManager_;
  Parameter parameters_[kParameterCount];
  int currentParam = 0;
  static constexpr size_t kMaxCallbacks = 2;
  int numCallbacks = 0;
  CallbackInterface* callbacks_[kMaxCallbacks] = {nullptr};

  void notify() {
    for (int i = 0; i < numCallbacks; i++) {
      callbacks_[i]->invoke();
    }
  }
};

void ButtonManager::handleEvent(ace_button::AceButton* button, uint8_t event_type, uint8_t /* buttonState */) {
  const auto id = button->getId();
  switch (id) {
    case 1:
    case 2:
      {
        if (!(event_type == ace_button::AceButton::kEventReleased
              || event_type == ace_button::AceButton::kEventRepeatPressed)) return;

        const auto currentParam = parameterTuner->getCurrentParam();
        int direction = (id == 1) ? 1 : -1;
        int magnitude = (event_type == ace_button::AceButton::kEventReleased) ? 1 : 5;
        parameterTuner->updateParameter(currentParam, direction * magnitude);
        break;
      }
    case 0:
      {
        if (event_type != ace_button::AceButton::kEventReleased) return;
        parameterTuner->nextPram();
        break;
      }
  }
}

class SerialLogger {
public:
  SerialLogger(ParameterTuner* tuner)
    : tuner_(tuner){};

  void logTunerState() {
    Serial.print("Current param: ");
    Serial.print(tuner_->getCurrentParam());
    Serial.print(" . Value: ");
    Serial.println(tuner_->getParameter(tuner_->getCurrentParam())->value);
  }

private:
  ParameterTuner* tuner_;
};

class DisplayManager {
public:
  DisplayManager(ParameterTuner* tuner, int maxWidth = 3)
  : tuner_(tuner), lcd(0x27,  16, 2), maxWidth(maxWidth) {};

  void begin() {
    lcd.init();
    lcd.backlight();
    lcd.cursor();
    lcd.blink();
  };

  void draw() {
    for (int i = 0; i < tuner_->kParameterCount; i++) {
      drawValueAtLocation(tuner_->getParameter(i)->value, i);
    }
    lcd.setCursor(tuner_->getCurrentParam() * (maxWidth + 1) + maxWidth - 1, 0);
  };

  void drawValueAtLocation(int value, int location) {
    lcd.setCursor(location * (maxWidth + 1), 0);
    char valBuf[maxWidth];
    dtostrf(value, maxWidth, 0, valBuf);
    lcd.print(valBuf);
  }

private:
  int maxWidth;
  ParameterTuner* tuner_;
  LiquidCrystal_I2C lcd;
};

class SmokerUI {
public:
  SmokerUI ()
  : tuner_()
  , displayManager_(&tuner_)
  , serialLogger_(&tuner_)
  , serialLoggerCallback_(&serialLogger_, &SerialLogger::logTunerState)
  , displayManagerCallback_(&displayManager_, &DisplayManager::draw) 
  {} ;

  void begin() {
    tuner_.begin();
    tuner_.addCallback(&serialLoggerCallback_);
    tuner_.addCallback(&displayManagerCallback_);

    displayManager_.begin();
    displayManager_.draw();
  }

  void check() {
    tuner_.check();
  }

private:
  ParameterTuner tuner_;
  DisplayManager displayManager_;
  SerialLogger serialLogger_;
  Callback<SerialLogger> serialLoggerCallback_;
  Callback<DisplayManager> displayManagerCallback_;
};

SmokerUI ui;

void setup() {
  Serial.begin(9600);
  ui.begin();
}

void loop() {
  ui.check();
}
