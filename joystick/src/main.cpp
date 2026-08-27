// yellowquadbot joystick firmware — Seeed XIAO ESP32C6
//
// Reads a 4-switch arcade joystick (up/down/left/right) plus turn/deadman/
// gait buttons, and sends a JoyPacket to the robot over ESP-NOW at ~50Hz.
// All inputs are digital (active-low, internal pullups) — no analog sticks.
// Prints its own MAC on boot for bring-up/testing.

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#include "comms_protocol.h"
#include "joystick_config.h"

esp_now_peer_info_t peerInfo;
uint8_t seq = 0;

inline bool pressed(int pin) { return digitalRead(pin) == LOW; }

void onDataSent(const uint8_t *mac, esp_now_send_status_t status) {
  // Optional: blink an LED or log on failure. Kept quiet to avoid serial spam.
}

void setup() {
  Serial.begin(115200);
  delay(200);

  const int inputPins[] = {PIN_STICK_UP, PIN_STICK_DOWN, PIN_STICK_LEFT, PIN_STICK_RIGHT,
                            PIN_DEADMAN, PIN_TURN_LEFT, PIN_TURN_RIGHT, PIN_GAIT_TOGGLE};
  for (int pin : inputPins) pinMode(pin, INPUT_PULLUP);

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
uint8_t gaitToggleEdge = 0;

void loop() {
  JoyPacket pkt;
  pkt.seq = seq++;

  // Digital directions collapse to -100/0/100, same range the controller's
  // deadzone/stride math already expects from an analog stick.
  pkt.ly = pressed(PIN_STICK_UP) ? 100 : (pressed(PIN_STICK_DOWN) ? -100 : 0);
  pkt.lx = pressed(PIN_STICK_RIGHT) ? 100 : (pressed(PIN_STICK_LEFT) ? -100 : 0);
  pkt.rx = pressed(PIN_TURN_RIGHT) ? 100 : (pressed(PIN_TURN_LEFT) ? -100 : 0);
  pkt.ry = 0; // no body-height input on this build

  bool gaitButtonDown = pressed(PIN_GAIT_TOGGLE);
  gaitToggleEdge = (gaitButtonDown && !gaitButtonWasDown) ? 1 : 0;
  gaitButtonWasDown = gaitButtonDown;

  pkt.buttons = 0;
  if (pressed(PIN_DEADMAN)) pkt.buttons |= 0x01;
  if (gaitToggleEdge) pkt.buttons |= 0x02;

  esp_now_send(ROBOT_MAC, (uint8_t *)&pkt, sizeof(pkt));

  delay(20); // ~50Hz
}
