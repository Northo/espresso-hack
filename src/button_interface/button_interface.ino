#include <AceButton.h>

namespace Config {

};

class ParameterTuner;

class ButtonManager : public ace_button::IEventHandler {
public:
  ButtonManager(ParameterTuner* parameterTuner, int nextButtonPin, int incButtonPin, int decButtonPin)
    : parameterTuner(parameterTuner) {
      auto *buttonConfig = ace_button::ButtonConfig::getSystemButtonConfig();

      buttonConfig->setIEventHandler(this);
      nextButton.init(buttonConfig, nextButtonPin, HIGH, 0);
      incButton.init(buttonConfig, incButtonPin, HIGH, 1);
      decButton.init(buttonConfig, decButtonPin, HIGH, 2);

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

template <class T>
class Callback : public CallbackInterface {
public:
  Callback(T* instance, void (T::*func)())
  : instance(instance), memberFunc(func) {};

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
    : buttonManager_(this, 8, 9, 10), parameters_({ { 1.0, 0.0, 10.0, 1.0 }, { 1.0, 0.0, 10.0, 1.0 } }){};

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

  void addCallback(CallbackInterface *callback) {
    callback_ = callback;
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
  CallbackInterface *callback_;
  
  void notify() {
    callback_->invoke();
  }
};

void ButtonManager::handleEvent(ace_button::AceButton* button, uint8_t event_type, uint8_t /* buttonState */) {
  if (event_type != ace_button::AceButton::kEventReleased) return;

  const auto id = button->getId();
  switch (id) {
    case 1:
    case 2:
    {
      const auto currentParam = parameterTuner->getCurrentParam();
      parameterTuner->updateParameter(currentParam, (id == 1) ? 1 : -1);
      break;
    }
    case 0:
    {
      parameterTuner->nextPram();
      break;
    }
  }
}

class SerialLogger {
public:
  SerialLogger (ParameterTuner *tuner)
  : tuner_(tuner) {};

  void logTunerState() {
      Serial.print("Current param: ");
      Serial.print(tuner_->getCurrentParam());
      Serial.print(" . Value: ");
      Serial.println(tuner_->getParameter(tuner_->getCurrentParam())->value);
  }

private:
  ParameterTuner *tuner_;
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
