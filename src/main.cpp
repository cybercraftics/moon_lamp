#include <Arduino.h>
#include <CapacitiveSensor.h>

#define LED_CHANNEL_RED 11
#define LED_CHANNEL_GREEN 10
#define LED_CHANNEL_BLUE 9
#define CAPACITIVE_SENSOR_SOURCE_PIN 4
#define CAPACITIVE_SENSOR_SINK_PIN 2
#define PIEZO_PIN A1
#define LIGHT_SENSOR_PIN A2

const int MAX_BRIGHTNESS = 150;
const int TOUCH_TIME_OUT = 500;

const int CAPACITIVE_SENSOR_THRESHOLD = 60;
const int TAP_SENSOR_THRESHOLD = 4;
const int LIGHT_SENSOR_THRESHOLD = 700;
const long DOUBLE_TAP_MAX_INTERVAL = 500;

CapacitiveSensor cs = CapacitiveSensor(CAPACITIVE_SENSOR_SOURCE_PIN, CAPACITIVE_SENSOR_SINK_PIN);

int state = 0;
int currentColor = 0;
int autoMode = 0;
bool calibrated = false;

unsigned long lastTapTime = 0;
unsigned long lastTouchTime = 0;
unsigned long currentTime = 0;
unsigned long lastBrighnessAdjustment = 0;
bool singleTapDetected = false;

long capacitiveSensorValue = 0;
int lightSensorValue = 0;
int tapSensorValue = 0;
int tapSensorBaseValue = 0;
int capacitiveSensorBaseValue = 0;
int brightness = 0;

void sensorsCalibration();
void toggleState();
void turnOn();
void turnOff();
void setLEDs();
void onSingleTap();
void onDoubleTap();
void signalizeAutoMode();
int determineOptimalBrightness();

void setup() {
  Serial.begin(9600);
  pinMode(LED_CHANNEL_RED, OUTPUT);
  pinMode(LED_CHANNEL_GREEN, OUTPUT);
  pinMode(LED_CHANNEL_BLUE, OUTPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(PIEZO_PIN, INPUT);
}

void loop() {
  sensorsCalibration();
  currentTime = millis();
  capacitiveSensorValue = cs.capacitiveSensor(100);
  lightSensorValue = analogRead(LIGHT_SENSOR_PIN);
  tapSensorValue = analogRead(PIEZO_PIN);
  brightness = determineOptimalBrightness();

  if (!calibrated) {
    return;
  }

  if (capacitiveSensorValue > (capacitiveSensorBaseValue + CAPACITIVE_SENSOR_THRESHOLD) && (currentTime - lastTouchTime) > TOUCH_TIME_OUT) {
    toggleState();
    lastTouchTime = currentTime;
  }

  if (tapSensorValue > (tapSensorBaseValue + TAP_SENSOR_THRESHOLD)) {
    Serial.println("Tap Sensor Value: " + String(tapSensorValue));
    if (singleTapDetected) {
      if (currentTime - lastTapTime < DOUBLE_TAP_MAX_INTERVAL) {
        Serial.println("Double Tap Detected");
        singleTapDetected = false;
        onDoubleTap();
      } else {
        Serial.println("Single Tap Detected");
        singleTapDetected = false;
        onSingleTap();
      }
    } else {
      singleTapDetected = true;
      lastTapTime = currentTime;
    }

    // Debounce delay to prevent multiple readings from a single tap
    delay(200);
  }

  // If no second tap is detected within the threshold, consider it a single tap
  if (singleTapDetected && (millis() - lastTapTime) >= DOUBLE_TAP_MAX_INTERVAL) {
    Serial.println("Single Tap Detected");
    singleTapDetected = false;
    onSingleTap();
  }

  // detect darkness and turn on if auto mode is enabled
  if (lightSensorValue < LIGHT_SENSOR_THRESHOLD && autoMode == 1 && state == 0) {
    turnOn();
  } else if (lightSensorValue > LIGHT_SENSOR_THRESHOLD && autoMode == 1 && state == 1) {
    turnOff();
  }

  // Adjust brightness every 5 seconds
  if (lastBrighnessAdjustment >= 5000 && state == 1) {
    setLEDs();
    lastBrighnessAdjustment = 0;
  }

  lastBrighnessAdjustment += 100;
  
  delay(100);
}

void turnOn() {
  state = 1;
  setLEDs();
}

void turnOff() {
  state = 0;

  analogWrite(LED_CHANNEL_RED, 0);
  analogWrite(LED_CHANNEL_GREEN, 0);
  analogWrite(LED_CHANNEL_BLUE, 0);
}

void toggleState() {
  if (state == 0) {
    turnOn();
  } else {
    turnOff();
  }
}

void setLEDs() {
  if (currentColor == 0) {  // white
    analogWrite(LED_CHANNEL_RED, brightness);
    analogWrite(LED_CHANNEL_GREEN, brightness);
    analogWrite(LED_CHANNEL_BLUE, brightness);
  } if (currentColor == 1) {  // red
    analogWrite(LED_CHANNEL_RED, brightness);
    analogWrite(LED_CHANNEL_GREEN, 0);
    analogWrite(LED_CHANNEL_BLUE, 0);
  } else if (currentColor == 2) {  // green
    analogWrite(LED_CHANNEL_RED, 0);
    analogWrite(LED_CHANNEL_GREEN, brightness);
    analogWrite(LED_CHANNEL_BLUE, 0);
  } else if (currentColor == 3) {  // blue
    analogWrite(LED_CHANNEL_RED, 0);
    analogWrite(LED_CHANNEL_GREEN, 0);
    analogWrite(LED_CHANNEL_BLUE, brightness);
  } else if (currentColor == 4) {  // yellow
    analogWrite(LED_CHANNEL_RED, brightness);
    analogWrite(LED_CHANNEL_GREEN, brightness);
    analogWrite(LED_CHANNEL_BLUE, 0);
  } else if (currentColor == 5) { // cyan
    analogWrite(LED_CHANNEL_RED, 0);
    analogWrite(LED_CHANNEL_GREEN, brightness);
    analogWrite(LED_CHANNEL_BLUE, brightness);
  } else if (currentColor == 6) { // magenta
    analogWrite(LED_CHANNEL_RED, brightness);
    analogWrite(LED_CHANNEL_GREEN, 0);
    analogWrite(LED_CHANNEL_BLUE, brightness);
  }
}

void onSingleTap() {
  currentColor = (currentColor + 1) % 7;
  setLEDs();
}

void onDoubleTap() {
  autoMode = (autoMode + 1) % 2;
  Serial.println("Auto Mode: " + String(autoMode));
  signalizeAutoMode();
}

void signalizeAutoMode() {
  for (int i = 0; i < (3 - autoMode); i++) {
    turnOn();
    delay(500);
    turnOff();
    delay(500);
  }
}

void sensorsCalibration() {
  if (currentTime < 2000) {
    Serial.println("Calibrating sensors..." + String(currentTime));
    if (tapSensorBaseValue < tapSensorValue) {
      tapSensorBaseValue = tapSensorValue;
    }
    if (capacitiveSensorBaseValue < capacitiveSensorValue) {
      capacitiveSensorBaseValue = capacitiveSensorValue;
    }
  } else {
    calibrated = true;
  }
}

int determineOptimalBrightness() {
  switch (lightSensorValue)
  {
  case 0 ... 590:
    return MAX_BRIGHTNESS - 100;
    break;
  case 591 ... 750:
    return MAX_BRIGHTNESS - 50;
    break;
  default:
    return MAX_BRIGHTNESS;
    break;
  }
}