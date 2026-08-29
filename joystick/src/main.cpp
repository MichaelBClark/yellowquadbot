// yellowquadbot joystick firmware — Seeed XIAO ESP32C6
//
// Reads two proportional (potentiometer) RC-gimbal joysticks plus deadman/
// gait buttons, and sends a JoyPacket to the robot over ESP-NOW at ~50Hz.
// Unbranded RC-surplus sticks are rarely mechanically centered, so each
// axis's rest voltage is measured at boot and used as that axis's zero
// point instead of assuming the ADC midpoint. Prints its own MAC on boot
// for bring-up/testing.

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#include "comms_protocol.h"
#include "joystick_config.h"

esp_now_peer_info_t peerInfo;
uint8_t seq = 0;

struct AxisCal {
  int pin;
  bool invert;
  int center;
};

AxisCal axisLX{PIN_LEFT_X, INVERT_LEFT_X, 0};
AxisCal axisLY{PIN_LEFT_Y, INVERT_LEFT_Y, 0};
AxisCal axisRX{PIN_RIGHT_X, INVERT_RIGHT_X, 0};
AxisCal axisRY{PIN_RIGHT_Y, INVERT_RIGHT_Y, 0};

int measureCenter(int pin) {
  // Average several samples of the resting position, taken at boot.
  // IMPORTANT: keep sticks untouched while the board boots.
  long sum = 0;
  const int samples = 32;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return (int)(sum / samples);
}

int8_t readAxis(AxisCal &axis) {
  int raw = analogRead(axis.pin);
  int centered = raw - axis.center;

  // Scale using whichever side of center is larger, so travel that isn't
  // symmetric around the measured rest point still reaches +/-100 at full
  // deflection instead of clipping early on the short side.
  int span = centered >= 0 ? (ADC_MAX - axis.center) : axis.center;
  if (span <= 0) span = ADC_MAX / 2;

  float pct = (float)centered / (float)span * 100.0f;
  if (axis.invert) pct = -pct;
  if (pct > 100.0f) pct = 100.0f;
  if (pct < -100.0f) pct = -100.0f;
  return (int8_t)pct;
}

void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  // Optional: blink an LED or log on failure. Kept quiet to avoid serial spam.
}

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(PIN_DEADMAN, INPUT_PULLUP);
  pinMode(PIN_GAIT_TOGGLE, INPUT_PULLUP);

  Serial.println("Calibrating stick centers, don't touch the sticks...");
  axisLX.center = measureCenter(axisLX.pin);
  axisLY.center = measureCenter(axisLY.pin);
  axisRX.center = measureCenter(axisRX.pin);
  axisRY.center = measureCenter(axisRY.pin);
  Serial.printf("Centers: LX=%d LY=%d RX=%d RY=%d\n", axisLX.center, axisLY.center,
                axisRX.center, axisRY.center);

  WiFi.mode(WIFI_STA);
  Serial.print("Joystick MAC address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed, halting.");
    while (true) delay(1000);
  }
  esp_now_register_send_cb(onDataSent);

  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, ROBOT_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add robot as ESP-NOW peer.");
  }
}

// Simple debounce for the gait-toggle button: only latch a press once per
// physical push, so holding it doesn't spam mode changes.
bool gaitButtonWasDown = false;

void loop() {
  JoyPacket pkt;
  pkt.seq = seq++;
  pkt.lx = readAxis(axisLX);
  pkt.ly = readAxis(axisLY);
  pkt.rx = readAxis(axisRX);
  pkt.ry = readAxis(axisRY);

  bool gaitButtonDown = digitalRead(PIN_GAIT_TOGGLE) == LOW;
  uint8_t gaitToggleEdge = (gaitButtonDown && !gaitButtonWasDown) ? 1 : 0;
  gaitButtonWasDown = gaitButtonDown;

  pkt.buttons = 0;
  if (digitalRead(PIN_DEADMAN) == LOW) pkt.buttons |= 0x01;
  if (gaitToggleEdge) pkt.buttons |= 0x02;

  esp_now_send(ROBOT_MAC, (uint8_t *)&pkt, sizeof(pkt));

  delay(20); // ~50Hz
}
