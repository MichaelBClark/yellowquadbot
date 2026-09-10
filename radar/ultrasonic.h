#pragma once
#include <Arduino.h>

// HC-SR04-style ultrasonic sensor: trigger a 10us pulse, measure the echo
// pulse width, convert to distance using the speed of sound.
class Ultrasonic {
public:
  Ultrasonic(int trigPin, int echoPin, float maxRangeCm = 400.0f)
      : trig_(trigPin), echo_(echoPin), maxRangeCm_(maxRangeCm) {}

  void begin() {
    pinMode(trig_, OUTPUT);
    pinMode(echo_, INPUT);
    digitalWrite(trig_, LOW);
  }

  // Returns distance in cm, or -1 if no echo was received (out of range /
  // no reflective surface) within the timeout window.
  float readCm() {
    digitalWrite(trig_, LOW);
    delayMicroseconds(2);
    digitalWrite(trig_, HIGH);
    delayMicroseconds(10);
    digitalWrite(trig_, LOW);

    // Timeout sized for maxRangeCm plus margin: sound round-trip is
    // ~58us/cm, add slack for sensors that read a bit long.
    uint32_t timeoutUs = (uint32_t)(maxRangeCm_ * 60.0f) + 2000;
    uint32_t durationUs = pulseIn(echo_, HIGH, timeoutUs);
    if (durationUs == 0) return -1.0f;

    float cm = durationUs / 58.0f;
    if (cm > maxRangeCm_) return -1.0f;
    return cm;
  }

private:
  int trig_, echo_;
  float maxRangeCm_;
};
