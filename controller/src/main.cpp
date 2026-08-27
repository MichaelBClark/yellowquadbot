// yellowquadbot controller firmware — ESP32 WROOM + PCA9685
//
// Receives JoyPacket over ESP-NOW from the joystick, turns stick input into
// a trot gait, solves per-leg IK, and drives all 12 servos via the PCA9685.
// If the deadman button isn't held (or packets stop arriving), legs relax
// to the safe standing pose instead of continuing to walk.

#include <Arduino.h>
#include <Wire.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Adafruit_PWMServoDriver.h>

#include "comms_protocol.h"
#include "robot_config.h"
#include "leg_ik.h"
#include "gait.h"

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40, Wire);

volatile JoyPacket latestPacket = {0, 0, 0, 0, 0, 0};
volatile uint32_t lastPacketMillis = 0;
volatile bool havePacket = false;

float gaitPhase = 0.0f;
uint8_t lastSeq = 0;

// Leg mount offsets from body center (mm), used only for logging/expansion;
// gait math below works entirely in each leg's own local frame.
constexpr float DEADBAND = 8.0f; // stick units, out of 100

float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(JoyPacket)) return;
  JoyPacket pkt;
  memcpy(&pkt, data, sizeof(JoyPacket));
  noInterrupts();
  latestPacket = pkt;
  lastPacketMillis = millis();
  havePacket = true;
  interrupts();
}

uint16_t angleToTick(float angleDeg) {
  // Map -90..+90 degrees (servo neutral = 90) onto the calibrated pulse range.
  float servoDeg = clampf(angleDeg + 90.0f, 0.0f, 180.0f);
  float t = servoDeg / 180.0f;
  return (uint16_t)(SERVO_MIN_TICK + t * (SERVO_MAX_TICK - SERVO_MIN_TICK));
}

void writeLeg(LegId leg, LegAngles a) {
  const float raw[NUM_JOINTS] = {a.coxa, a.femur, a.tibia};
  for (int j = 0; j < NUM_JOINTS; j++) {
    float deg = raw[j] + SERVO_TRIM_DEG[leg][j];
    if (SERVO_INVERT[leg][j]) deg = -deg;
    pwm.setPWM(SERVO_CHANNEL[leg][j], 0, angleToTick(deg));
  }
}

void driveStandingPose() {
  for (int leg = 0; leg < NUM_LEGS; leg++) {
    LegAngles a = solveLegIK(Geometry::STAND_X, Geometry::STAND_Y, Geometry::STAND_Z);
    if (a.valid) writeLeg((LegId)leg, a);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  pwm.begin();
  pwm.setPWMFreq(PCA_FREQ_HZ);

  WiFi.mode(WIFI_STA);
  Serial.print("Robot MAC address: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed, halting.");
    while (true) delay(1000);
  }
  esp_now_register_recv_cb(onDataRecv);

  driveStandingPose();
}

void loop() {
  JoyPacket pkt;
  uint32_t since;
  bool has;
  noInterrupts();
  pkt = latestPacket;
  since = millis() - lastPacketMillis;
  has = havePacket;
  interrupts();

  bool deadmanHeld = has && (pkt.buttons & 0x01);
  bool packetFresh = has && (since < PACKET_TIMEOUT_MS);

  if (!deadmanHeld || !packetFresh) {
    driveStandingPose();
    gaitPhase = 0.0f;
    delay(20);
    return;
  }

  // Deadzone the sticks.
  float ly = fabsf(pkt.ly) > DEADBAND ? pkt.ly : 0.0f;
  float lx = fabsf(pkt.lx) > DEADBAND ? pkt.lx : 0.0f;
  float rx = fabsf(pkt.rx) > DEADBAND ? pkt.rx : 0.0f;
  float ryIn = fabsf(pkt.ry) > DEADBAND ? pkt.ry : 0.0f;

  // Stick -> stride length (mm) and turn contribution.
  float strideY = (ly / 100.0f) * 35.0f;                 // forward/back stride
  float strideX = (lx / 100.0f) * 25.0f + (rx / 100.0f) * 20.0f; // strafe + turn blended into x-sweep
  float heightTrim = (ryIn / 100.0f) * 20.0f;             // body height adjust

  // Advance gait phase proportional to commanded speed; idle (near-zero
  // stick) still lets legs settle back toward stance rather than freezing
  // mid-swing.
  float speedMag = clampf((fabsf(ly) + fabsf(lx) + fabsf(rx)) / 100.0f, 0.0f, 1.0f);
  gaitPhase += 0.02f * (0.15f + speedMag);
  if (gaitPhase >= 1.0f) gaitPhase -= 1.0f;

  for (int leg = 0; leg < NUM_LEGS; leg++) {
    Gait::FootOffset off = Gait::legOffset(leg, gaitPhase, strideY, strideX);
    float x = Geometry::STAND_X + off.dx;
    float y = Geometry::STAND_Y + off.dy;
    float z = Geometry::STAND_Z + heightTrim + off.dz;

    LegAngles a = solveLegIK(x, y, z);
    if (a.valid) writeLeg((LegId)leg, a);
    // If unreachable, hold the previous commanded angle (skip write) rather
    // than snapping to a garbage pose.
  }

  delay(20); // ~50Hz update
}
