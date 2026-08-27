// yellowquadbot joystick firmware — Seeed XIAO ESP32C6
//
// Reads two analog sticks + buttons, sends a JoyPacket to the robot over
// ESP-NOW at ~50Hz. Prints its own MAC on boot in case you want to swap
// which board is "joystick" vs "robot" for bring-up/testing.

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>

#include "comms_protocol.h"
#include "joystick_config.h"

esp_now_peer_info_t peerInfo;
uint8_t seq = 0;

int8_t readAxis(int pin, bool invert) {
  int raw = analogRead(pin);
  int centered = raw - ADC_CENTER; // -2048..2047
  float pct = (float)centered / (float)ADC_CENTER * 100.0f;
  if (invert) pct = -pct;
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

void loop() {
  JoyPacket pkt;
  pkt.seq = seq++;
  pkt.lx = readAxis(PIN_LEFT_X, false);
  pkt.ly = readAxis(PIN_LEFT_Y, true); // stick "up" -> positive forward
  pkt.rx = readAxis(PIN_RIGHT_X, false);
  pkt.ry = readAxis(PIN_RIGHT_Y, true);

  pkt.buttons = 0;
  if (digitalRead(PIN_DEADMAN) == LOW) pkt.buttons |= 0x01;   // held = armed
  if (digitalRead(PIN_GAIT_TOGGLE) == LOW) pkt.buttons |= 0x02;

  esp_now_send(ROBOT_MAC, (uint8_t *)&pkt, sizeof(pkt));

  delay(20); // ~50Hz
}
