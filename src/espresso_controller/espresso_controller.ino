#include <PID_v1.h>
#include <Adafruit_MAX31855.h>
#include <LiquidCrystal_I2C.h>

// Pin configuration for MAX31855
#define MAXDO   3   // MISO
#define MAXCS   4   // Chip Select
#define MAXCLK  5   // Clock

// Mechanical relay pin
#define RELAY_PIN 7

// Button input pin
#define BUTTON_PIN 2
int buttonState;
int lastButtonState;

// PID variables
double Input;
double FilteredInput;
double Output;
double Setpoint;  // Desired setpoint in °C
double alpha = 0.1;  // Smoothing

// MAX31855 sensor
Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);

// LCD display
LiquidCrystal_I2C lcd(0x27, 16, 2);

#define AGGKP                                 62     // PID Kp (regular phase)
#define AGGTN                                 0     // PID Tn (regular phase)
#define AGGTV                                 20   // PID Tv (regular phase)

double aggKp = AGGKP;
#if AGGTN == 0
double aggKi = 0;
#else
double aggKi = AGGKP / AGGTN;
#endif
double aggKd = AGGTV * AGGKP;

bool use_p_on_m = false;

PID myPID(&FilteredInput, &Output, &Setpoint, aggKp, aggKi, aggKd, use_p_on_m ? P_ON_M : P_ON_E, DIRECT);

unsigned int sampleTime = 2000;

// Relay control window
const int WindowSize = 1000;  // in milliseconds
unsigned long windowStartTime = 0;
unsigned long lastTempRead = 0;

enum MODE {
  PID_MODE = AUTOMATIC,
  MONITOR_MODE = MANUAL,
};

enum MODE currentMode = PID_MODE;

void setup() {
  Serial.begin(9600);
  
  setupThermocouple();
  setupRelay();
  setupLCD();
  setupButton();

  windowStartTime = millis();

  Setpoint = 94;
  //tell the PID to range between 0 and the full window size
  myPID.SetOutputLimits(0, WindowSize);
  myPID.SetSampleTime(sampleTime);

  Input = FilteredInput = thermocouple.readCelsius();

  //turn the PID on
  myPID.SetMode(currentMode);
}

void setupButton() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  buttonState = digitalRead(BUTTON_PIN);
  lastButtonState = buttonState;
}

void setupThermocouple() {
  if (!thermocouple.begin()) {
    Serial.println("MAX31855 not found. Check wiring!");
    while (1);
  }
}

void setupRelay() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);  // NB HIGH = off
}

void setupLCD() {
  lcd.init(); // initialize the lcd
  lcd.backlight();
}

void loop() {
  unsigned long now = millis();

  if (now - lastTempRead >= 1000) {
    // Throttled loop
    lastTempRead = now;
    Input = thermocouple.readCelsius();
    FilteredInput = (1 - alpha) * Input + alpha * FilteredInput;
    printLCD();

    writeSerial();
  }

  myPID.SetMode(currentMode);
  if (currentMode == MONITOR_MODE) {
    Output = 0;
  }
  myPID.Compute();
  writeOutput(now);

  handleButtonPress();
}

void writeSerial() {
  char tempStr[10];
  char outStr[10];
  char setpointStr[10];
  char logline[100];

  dtostrf(Input, 0, 2, tempStr);   // Input is a pointer → dereference
  dtostrf(Output, 0, 2, outStr);    // Output is a float/double → use directly
  dtostrf(Setpoint, 0, 2, setpointStr);

  snprintf(logline, sizeof(logline),
           "TempC:%s,Output:%s,Setpoint:%s,Mode:%d",
           tempStr,
           outStr,
           setpointStr,
           currentMode
  );
  Serial.print(logline);
  Serial.print(",Kp:"); Serial.print(aggKp, 2);
  Serial.print(",Ki:"); Serial.print(aggKi, 2);
  Serial.print(",Kd:"); Serial.print(aggKd, 2);
  Serial.print(",FilteredInput:"); Serial.print(FilteredInput, 2);
  Serial.print(",alpha:"); Serial.println(alpha, 2);
}

void writeOutput(unsigned long now) {
  // Time-proportioning control for relay
  if (now - windowStartTime > WindowSize) {
    windowStartTime += WindowSize;
  }

  if (Output > (now - windowStartTime)) {
    digitalWrite(RELAY_PIN, LOW);  // LOW == on
  } else {
    digitalWrite(RELAY_PIN, HIGH);
  }
}

void printLCD() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(Input);
    lcd.print("  ");
    lcd.print(Setpoint);
    if (currentMode == PID_MODE) {
      lcd.print(" *");
    }
    lcd.setCursor(0, 1);
    lcd.print(int(Output));
    lcd.print(" ");
    lcd.print(aggKp);
    lcd.print(" ");
    lcd.print(aggKi);
    lcd.print(" ");
    lcd.print(aggKd);
}

void handleButtonPress() {
  // Check if button was pressed, and if so, toggle mode
  // Button has pullup, so check for low
  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && buttonState != lastButtonState) {
    // We just clicked the button
    currentMode = currentMode == PID_MODE ? MONITOR_MODE : PID_MODE;
  }
  lastButtonState = buttonState;
}