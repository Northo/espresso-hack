#include <AceButton.h>
#include <PID_v1.h>
#include <Adafruit_MAX31855.h>
#include <LiquidCrystal_I2C.h>

namespace Config {
int BTN_NEXT_PIN = 8, BTN_INC_PIN = 10, BTN_DEC_PIN = 9;

int RELAY_PIN = 7;

int REPEAT_PRESS_DELAY = 700, REPEAT_PRESS_INTERVAL = 400;

int singlePressStep = 1, repeatPressStep = 5;

int temperatureReadInterval = 500, pidInterval = 1000;

int MAXDO = 3, MAXCS = 4, MAXCLK = 5;

float kP = 28, kI=0.1, kD=2100;
};

struct Parameter {
  float value, min, max, step;
};

class Controller {
public:
  Controller(LiquidCrystal_I2C *lcd, int relayPin)
    : pid_(&filteredTemperature_, &output_, &setpoint_, 0.0, 0.0, 0.0, DIRECT), thermocouple_(Config::MAXCLK, Config::MAXCS, Config::MAXDO), relayPin_(relayPin), lcd_(lcd){};

  void begin() {
    thermocouple_.begin();
    pinMode(relayPin_, OUTPUT);

    pid_.SetOutputLimits(0, WindowSize_);
    pid_.SetSampleTime(pidSampleTime_);
    pid_.SetMode(AUTOMATIC);

    lcd_->setCursor(8, 1);
    lcd_->print((int)setpoint_);
  }

  void check() {
    readTemp();
    pid_.Compute();
    setRelay();
    writeSerial();
  }

  void setTuningParameter(int index, float value) {
    float currentValues[] = { pid_.GetKp(), pid_.GetKi(), pid_.GetKd() };
    currentValues[index] = value;
    pid_.SetTunings(currentValues[0], currentValues[1], currentValues[2]);
  }

  PID pid_;

private:
  Adafruit_MAX31855 thermocouple_;
  LiquidCrystal_I2C *lcd_;
  double temperature_ = 0;
  double filteredTemperature_ = 0;
  double output_ = 0;
  double setpoint_ = 94;
  int relayPin_;

  int pidSampleTime_ = Config::pidInterval;

  unsigned long lastSerialWrite_ = 0;
  int serialWriteInterval_ = 3000;

  const int WindowSize_ = 1000;
  unsigned long windowStartTime_ = 0;
  unsigned long lastTempRead_ = 0;
  const int readInterval_ = Config::temperatureReadInterval;

  const float filterAlpha_ = 0.8;

  void writeSerial() {
    if (millis() - lastSerialWrite_ >= serialWriteInterval_) {
      char tempStr[10];
      char outStr[10];
      char setpointStr[10];
      char logline[100];

      dtostrf(temperature_, 0, 2, tempStr);   // Input is a pointer → dereference
      dtostrf(output_, 0, 2, outStr);    // Output is a float/double → use directly
      dtostrf(setpoint_, 0, 2, setpointStr);

      snprintf(logline, sizeof(logline),
              "TempC:%s,Output:%s,Setpoint:%s",
              tempStr,
              outStr,
              setpointStr
      );
      Serial.print(logline);
      Serial.print(",Kp:");
      Serial.print(pid_.GetKp(), 2);
      Serial.print(",Ki:");
      Serial.print(pid_.GetKi(), 2);
      Serial.print(",Kd:");
      Serial.print(pid_.GetKd(), 2);
      Serial.print(",FilteredTemp:");
      Serial.print(filteredTemperature_, 2);
      Serial.print(",alpha:");
      Serial.println(filterAlpha_, 2);

      lastSerialWrite_ = millis();
    };
  }

  void readTemp() {
    if (millis() - lastTempRead_ >= readInterval_) {
      temperature_ = thermocouple_.readCelsius();
      filteredTemperature_ = filterAlpha_ * filteredTemperature_ + (1 - filterAlpha_) * temperature_;
      lcd_->setCursor(0, 1);
      lcd_->print(filteredTemperature_);
      lcd_->setCursor(12, 1);
      lcd_->print((int)output_);
      lcd_->print("   ");
      lastTempRead_ = millis();
    }
  }

  void setRelay() {
    unsigned long now = millis();
    if ((now - windowStartTime_) > WindowSize_) {
      windowStartTime_ += WindowSize_;
    }

    if (output_ > (now - windowStartTime_)) {
      digitalWrite(relayPin_, LOW);  // LOW == on
    } else {
      digitalWrite(relayPin_, HIGH);
    }
  }
};


class ParameterManager : public ace_button::IEventHandler {
public:
  static constexpr size_t kNumParameters = 3;
  ParameterManager(Controller *controller, LiquidCrystal_I2C *lcd)
    : parameters_({
      { Config::kP, 0, 200, 1 },
      { Config::kI, 0, 1, 0.01 },
      { Config::kD, 0, 5000, 5 }
    }), controller_(controller), lcd_(lcd) {
    nextButtonConfig.setIEventHandler(this);
    incDecButtonConfig.setIEventHandler(this);
    incDecButtonConfig.setFeature(ace_button::ButtonConfig::kFeatureRepeatPress);
    incDecButtonConfig.setFeature(ace_button::ButtonConfig::kFeatureRepeatPress);
    incDecButtonConfig.setFeature(ace_button::ButtonConfig::kFeatureSuppressAfterRepeatPress);
    incDecButtonConfig.setRepeatPressInterval(Config::REPEAT_PRESS_INTERVAL);
    incDecButtonConfig.setRepeatPressDelay(Config::REPEAT_PRESS_DELAY);
  };

  void begin() {
    pinMode(Config::BTN_NEXT_PIN, INPUT_PULLUP);
    pinMode(Config::BTN_INC_PIN, INPUT_PULLUP);
    pinMode(Config::BTN_DEC_PIN, INPUT_PULLUP);

    nextButton.init(&nextButtonConfig, Config::BTN_NEXT_PIN, HIGH, 0);
    incButton.init(&incDecButtonConfig, Config::BTN_INC_PIN, HIGH, 1);
    decButton.init(&incDecButtonConfig, Config::BTN_DEC_PIN, HIGH, 2);

    controller_->pid_.SetTunings(parameters_[0].value, parameters_[1].value, parameters_[2].value);

    beginLcd();
  }

  void beginLcd() {
    lcd_->cursor();
    lcd_->blink();
    drawLcd();
  }

  void drawLcd() {
    // total characters = each value width + 1 space  * number of params
    const int charsPerValue = lcdMaxWidth + 1;
    const int lineWidth = kNumParameters * charsPerValue;
    char lineBuf[lineWidth + 1];  // +1 for '\0'
    int idx = 0;

    // build the line: [val0][sp][val1][sp]...[valN][sp]
    for (int i = 0; i < kNumParameters; i++) {
      // temp buffer for this parameter’s value
      char valBuf[lcdMaxWidth + 1];  // +1 for '\0'
      dtostrf(parameters_[i].value, lcdMaxWidth, parameters_[i].value > 10 ? 0 : 2, valBuf);

      // copy the fixed-width number
      for (int j = 0; j < lcdMaxWidth; j++) {
        lineBuf[idx++] = valBuf[j];
      }
      // separator (you can omit for the last one if you like)
      lineBuf[idx++] = ' ';
    }

    lineBuf[lineWidth] = '\0';  // terminate

    // send it all in one go
    lcd_->setCursor(0, 0);
    lcd_->print(lineBuf);

    // reposition cursor under the “current” parameter
    int cur = getCurrentParameter();
    int cursorCol = cur * charsPerValue + (lcdMaxWidth - 1);
    lcd_->setCursor(cursorCol, 0);
  }

  void check() {
    nextButton.check();
    incButton.check();
    decButton.check();
  }

  int getCurrentParameter() {
    return currentParameter_;
  };
  void nextParameter() {
    currentParameter_ = (currentParameter_ + 1) % kNumParameters;
    drawLcd();
  }
  void setParameter(int parameterId, float value) {
    auto &parameter = parameters_[parameterId];
    parameter.value = constrain(value, parameter.min, parameter.max);
    drawLcd();
    controller_->setTuningParameter(parameterId, parameter.value);
  }

  void stepParameter(int parameterId, float change) {
    auto &parameter = parameters_[parameterId];
    setParameter(parameterId, parameter.value + change * parameter.step);
  }

  void handleEvent(ace_button::AceButton *button, uint8_t event_type, uint8_t /* buttonState */) {
    Serial.println(event_type);
    const auto id = button->getId();
    switch (id) {
      case 1:
      case 2:
        {
          if (!(event_type == ace_button::AceButton::kEventReleased
                || event_type == ace_button::AceButton::kEventRepeatPressed)) return;

          const auto currentParam = getCurrentParameter();
          int direction = (id == 1) ? 1 : -1;
          int magnitude = (event_type == ace_button::AceButton::kEventReleased) ? Config::singlePressStep : Config::repeatPressStep;
          stepParameter(currentParam, direction * magnitude);
          break;
        }
      case 0:
        {
          if (event_type != ace_button::AceButton::kEventReleased) return;
          nextParameter();
          break;
        }
    }
  }

private:
  Parameter parameters_[kNumParameters];
  Controller *controller_;
  ace_button::AceButton nextButton;
  ace_button::AceButton incButton;
  ace_button::AceButton decButton;
  ace_button::ButtonConfig incDecButtonConfig;
  ace_button::ButtonConfig nextButtonConfig;
  LiquidCrystal_I2C *lcd_;
  int currentParameter_ = 0;
  int lcdMaxWidth = 4;
};

LiquidCrystal_I2C lcd(0x27, 16, 2);

Controller controller(&lcd, Config::RELAY_PIN);
ParameterManager parameterManager(&controller, &lcd);


void setup() {
  Serial.begin(9600);
  Serial.println("Hello");
  lcd.init();
  lcd.backlight();

  parameterManager.begin();
  controller.begin();
}

void loop() {
  parameterManager.check();
  controller.check();
}
