#include <Arduino.h>
#include <CapacitiveSensor.h>

#define LED_CHANNEL_RED 11
#define LED_CHANNEL_GREEN 10
#define LED_CHANNEL_BLUE 9
#define CAPACITIVE_SENSOR_SOURCE_PIN 4
#define CAPACITIVE_SENSOR_SINK_PIN 2
#define PIEZO_PIN A1
#define LIGHT_SENSOR_PIN A2

const int MAX_BRIGHTNESS = 80;
const String COLORS[] = {"red", "green", "blue", "yellow", "cyan", "magenta", "white"};
const int TOUCH_TIME_OUT = 500;

const int CAPACITIVE_SENSOR_THRESHOLD = 100;
const int TAP_SENSOR_THRESHOLD = 8;
const long LIGHT_SENSOR_THRESHOLD = 70;
const long DOUBLE_TAP_MAX_INTERVAL = 500;

CapacitiveSensor cs = CapacitiveSensor(CAPACITIVE_SENSOR_SOURCE_PIN, CAPACITIVE_SENSOR_SINK_PIN);

int state = 0;
int currentColor = 0;
int autoMode = 0;
bool calibrated = false;

unsigned int touchTimeOutCounter = 0;
unsigned long lastTapTime = 0;
unsigned long lastTouchTime = 0;
unsigned long currentTime = 0;
bool singleTapDetected = false;

long capacitiveSensorValue = 0;
int lightSensorValue = 0;
int tapSensorValue = 0;
int tapSensorBaseValue = 0;
int capacitiveSensorBaseValue = 0;

void sensorsCalibration();
void toggleState();
void turnOn();
void turnOff();
void changeColor(String color);
void onSingleTap();
void onDoubleTap();
void onTouch();
void onDarkness();
void signalizeAutoMode();

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

  if (!calibrated) {
    return;
  }

  // detect touch and turn on/off
  if (capacitiveSensorValue > (capacitiveSensorBaseValue + CAPACITIVE_SENSOR_THRESHOLD) && (currentTime - lastTouchTime) > TOUCH_TIME_OUT) {
    Serial.println("Capacitive Sensor Value: " + String(capacitiveSensorValue));
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
    delay(150);
  }

  // If no second tap is detected within the threshold, consider it a single tap
  if (singleTapDetected && (millis() - lastTapTime) >= DOUBLE_TAP_MAX_INTERVAL) {
    Serial.println("Single Tap Detected");
    singleTapDetected = false;
    onSingleTap();
  }

  // detect darkness and turn on if auto mode is enabled
  if (lightSensorValue < LIGHT_SENSOR_THRESHOLD && autoMode == 1) {
    turnOn();
  } else if (lightSensorValue > LIGHT_SENSOR_THRESHOLD && autoMode == 1) {
    turnOff();
  }

  delay(100);
}

void turnOn() {
  state = 1;
  analogWrite(LED_CHANNEL_RED, MAX_BRIGHTNESS);
  analogWrite(LED_CHANNEL_GREEN, MAX_BRIGHTNESS);
  analogWrite(LED_CHANNEL_BLUE, MAX_BRIGHTNESS);
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

void changeColor(String color) {
  if (color == "red") {
    analogWrite(LED_CHANNEL_RED, MAX_BRIGHTNESS);
    analogWrite(LED_CHANNEL_GREEN, 0);
    analogWrite(LED_CHANNEL_BLUE, 0);
  } else if (color == "green") {
    analogWrite(LED_CHANNEL_RED, 0);
    analogWrite(LED_CHANNEL_GREEN, MAX_BRIGHTNESS);
    analogWrite(LED_CHANNEL_BLUE, 0);
  } else if (color == "blue") {
    analogWrite(LED_CHANNEL_RED, 0);
    analogWrite(LED_CHANNEL_GREEN, 0);
    analogWrite(LED_CHANNEL_BLUE, MAX_BRIGHTNESS);
  } else if (color == "yellow") {
    analogWrite(LED_CHANNEL_RED, MAX_BRIGHTNESS);
    analogWrite(LED_CHANNEL_GREEN, MAX_BRIGHTNESS);
    analogWrite(LED_CHANNEL_BLUE, 0);
  } else if (color == "cyan") {
    analogWrite(LED_CHANNEL_RED, 0);
    analogWrite(LED_CHANNEL_GREEN, MAX_BRIGHTNESS);
    analogWrite(LED_CHANNEL_BLUE, MAX_BRIGHTNESS);
  } else if (color == "magenta") {
    analogWrite(LED_CHANNEL_RED, MAX_BRIGHTNESS);
    analogWrite(LED_CHANNEL_GREEN, 0);
    analogWrite(LED_CHANNEL_BLUE, MAX_BRIGHTNESS);
  } else if (color == "white") {
    analogWrite(LED_CHANNEL_RED, MAX_BRIGHTNESS);
    analogWrite(LED_CHANNEL_GREEN, MAX_BRIGHTNESS);
    analogWrite(LED_CHANNEL_BLUE, MAX_BRIGHTNESS);
  }

  Serial.println("Color: " + color);
}

void onSingleTap() {
  currentColor = (currentColor + 1) % 7;
  changeColor(COLORS[currentColor]);
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