#include <AceButton.h>

namespace Config {

};

class ParameterTuner;

class ButtonManager : public ace_button::IEventHandler {
public:
  ButtonManager(ParameterTuner* parameterTuner, int nextButtonPin, int incButtonPin, int decButtonPin)
    : parameterTuner(parameterTuner) {

    nextButtonConfig.setIEventHandler(this);
    repeatButtonConfig.setIEventHandler(this);
    repeatButtonConfig.setFeature(ace_button::ButtonConfig::kFeatureRepeatPress);
    repeatButtonConfig.setFeature(ace_button::ButtonConfig::kFeatureSuppressAfterRepeatPress);
    repeatButtonConfig.setRepeatPressDelay(700);
    repeatButtonConfig.setRepeatPressInterval(300);

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
    : buttonManager_(this, 8, 9, 10), parameters_({ { 1.0, 0.0, 400.0, 2.4 }, { 1.0, 0.0, 10.0, 1.0 } }){};

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

ParameterTuner tuner;

SerialLogger serialLogger(&tuner);

Callback<SerialLogger> serialLoggerCallback(&serialLogger, &SerialLogger::logTunerState);

void setup() {
  Serial.begin(9600);
  tuner.begin();
  tuner.addCallback(&serialLoggerCallback);
}

void loop() {
  tuner.check();
}
