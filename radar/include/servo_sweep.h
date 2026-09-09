#pragma once
#include <Arduino.h>

// Drives a standard hobby servo directly from the ESP32-S3's LEDC PWM
// peripheral (no external servo library needed) and sweeps it back and
// forth between two angles, like a classic ping-radar head.
class ServoSweep {
public:
  ServoSweep(int pin, int minDeg = 15, int maxDeg = 165, float degPerSec = 60.0f)
      : pin_(pin), minDeg_(minDeg), maxDeg_(maxDeg), degPerSec_(degPerSec),
        angle_(minDeg), direction_(1) {}

  void begin() {
    ledcAttach(pin_, 50 /* Hz */, 14 /* bit resolution */);
    writeAngle(angle_);
  }

  // Call every loop iteration; advances the sweep angle based on elapsed
  // time and returns the current commanded angle in degrees.
  float update() {
    uint32_t now = micros();
    if (lastUpdateUs_ == 0) lastUpdateUs_ = now;
    float dtSec = (now - lastUpdateUs_) / 1000000.0f;
    lastUpdateUs_ = now;

    angle_ += direction_ * degPerSec_ * dtSec;
    if (angle_ >= maxDeg_) {
      angle_ = maxDeg_;
      direction_ = -1;
    } else if (angle_ <= minDeg_) {
      angle_ = minDeg_;
      direction_ = 1;
    }

    writeAngle(angle_);
    return angle_;
  }

  float currentAngle() const { return angle_; }

private:
  void writeAngle(float deg) {
    deg = constrain(deg, 0.0f, 180.0f);
    // Standard hobby servo: 500-2500us pulse over a 20ms (50Hz) period.
    uint32_t pulseUs = 500 + (uint32_t)((deg / 180.0f) * 2000.0f);
    uint32_t maxDuty = (1 << 14) - 1;
    uint32_t duty = (uint32_t)(((uint64_t)pulseUs * maxDuty) / 20000);
    ledcWrite(pin_, duty);
  }

  int pin_;
  int minDeg_, maxDeg_;
  float degPerSec_;
  float angle_;
  int direction_;
  uint32_t lastUpdateUs_ = 0;
};
