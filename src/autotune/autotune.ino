#include "PID_AutoTune_v0.h"
#include <Adafruit_MAX31855.h>
#include <LiquidCrystal_I2C.h>

// Run results
// setpoint 95, output 100, step 100: kp: 21.07, ki: 0.18, kd: 632.8
// setpoint 95, output 600, step 600: kp: 25.12, ki: 0.07, kd: 2257
// setpoint 95, output 250, step 250: kp: 28.29, ki: 0.09, kd: 2115
// 

// Sensors and IO

#define MAXDO   3   // MISO
#define MAXCS   4   // Chip Select
#define MAXCLK  5   // Clock

Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Working parameters

double input, output;
long lastTemperatureRead = 0;
long windowStartTime = 0;
long lastUIWrite = 0;
int finished = 0;

// Config
double aTuneStartSetpoint = 95;
double aTuneStartOutput = 250;
double aTuneOStep = 250;
double aTuneLookback = 60; // seconds
double aNoiseBand = 0.5;
int controlType = 1; // 0 = PI, 1 = PID

long windowSize = 1000;

int relayPin = 7;

int temperatureReadInterval = 250;
int writeUIInterval = 1000;

PID_ATune aTune(&input, &output);

void readTemperature() {
  long now = millis();
  if (now - lastTemperatureRead > temperatureReadInterval) {
    input = thermocouple.readCelsius();
    lastTemperatureRead = now;
  }
}

void writeUI() {
  long now = millis();
  if (now - lastUIWrite > writeUIInterval) {
    lcd.setCursor(0, 0);
    lcd.print(input);
    lcd.print(" ");
    lcd.print(output);

    Serial.print("Input:");
    Serial.print(input);
    Serial.print(", Output:");
    Serial.println(output);
    lastUIWrite = now;
  }
}

void writeRelay() {
  // Time-proportioning control for relay
  long now = millis();
  if (now - windowStartTime > windowSize) {
    windowStartTime += windowSize;
  }

  if (output > (now - windowStartTime)) {
    digitalWrite(relayPin, LOW);  // LOW == on
  } else {
    digitalWrite(relayPin, HIGH);
  }
}

void hasFinished() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(aTune.GetKp());
  lcd.print(" ");
  lcd.print(aTune.GetKi());
  lcd.print(" ");
  lcd.print(aTune.GetKd());
  lcd.print(" ");

  Serial.print("Kp: ");
  Serial.println(aTune.GetKp());
  Serial.print("Ki: ");
  Serial.println(aTune.GetKi());
  Serial.print("Kd: ");
  Serial.println(aTune.GetKd());

  while (true) {};
}

void setup() {
  Serial.begin(9600);

  Serial.println("Thermocouple begin");
  thermocouple.begin();

  Serial.println("lcd begin");
  lcd.init();
  lcd.backlight();

  Serial.println("atune begin");
  aTune.SetOutputStep(aTuneOStep);
  aTune.SetControlType(controlType);
	aTune.SetLookbackSec(aTuneLookback);
	aTune.SetNoiseBand(aNoiseBand);
  input = aTuneStartSetpoint;
  output = aTuneStartOutput;

  Serial.println("relay begin");
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // HIGH = off

}

void loop() {
  finished = aTune.Runtime();

  if (finished) hasFinished();
  readTemperature();
  writeUI();
  writeRelay();
}
