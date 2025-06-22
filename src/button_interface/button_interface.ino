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

class ParameterTuner {
public:
  static constexpr size_t kParameterCount = 2;

  ParameterTuner()
    : buttonManager_(this, 8, 9, 10), parameters_({ { 1.0, 0.0, 10.0, 1.0 }, { 1.0, 0.0, 10.0, 1.0 } }){};

  void setParameter(int parameterId, float value) {
    auto& parameter = parameters_[parameterId];
    parameter.value = value;  //constrain(value, parameter.min, parameter.max);
  };

  const Parameter* getParameter(int parameterId) {
    return &parameters_[parameterId];
  };

  void updateParameter(int parameterId, float change) {
    auto& parameter = parameters_[parameterId];
    parameter.value = constrain(parameter.value + change * parameter.step, parameter.min, parameter.max);
  }

  int getCurrentParam() {
    return currentParam;
  }

  void setCurrentParam(int param) {
    currentParam = param;
  }

  void nextPram() {
    setCurrentParam((getCurrentParam() + 1) % kParameterCount);
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
      Serial.print("Setting paramaeter ");
      Serial.print(currentParam);
      Serial.print(" to ");
      Serial.println(parameterTuner->getParameter(currentParam)->value);
      break;
    }
    case 0:
    {
      parameterTuner->nextPram();
      Serial.print("Current param: ");
      Serial.println(parameterTuner->getCurrentParam());
      break;
    }
  }
}

ParameterTuner tuner;

void setup() {
  Serial.begin(9600);
  tuner.begin();
}

void loop() {
  tuner.check();
}
