// yellowquadbot/radar — ultrasonic ping-radar on a Waveshare ESP32-S3
// 1.46" round AMOLED (412x412), servo-swept HC-SR04.
//
// !! Before flashing: fill in the real display/touch pins in
// include/pins.h from Waveshare's own demo code (see the comment block at
// the top of that file) and the servo/ultrasonic pins for whichever free
// GPIOs you wired them to. This file assumes those are correct.
//
// Classic "ping radar" layout: servo sweeps 0-180 degrees, pivot at the
// bottom-center of the screen, targets plotted in the top semicircle with
// distance from center = signal strength, and a fading trail of past hits.

#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <Arduino_GFX_Library.h>
#include <Adafruit_PWMServoDriver.h>

#include "pins.h"
#include "ultrasonic.h"
#include "servo_sweep.h"

// ---- Display bring-up (QSPI SH8601 round AMOLED) ----
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_GFX *gfx = new Arduino_SH8601(bus, LCD_RST, 0 /* rotation */,
                                       false /* IPS */, LCD_WIDTH, LCD_HEIGHT);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_I2C_ADDR, Wire);
ServoSweep servo(pwm, SERVO_CHANNEL, 5, 175, 90.0f); // degrees/sec sweep speed
Ultrasonic sonar(ULTRASONIC_TRIG, ULTRASONIC_ECHO, 200.0f); // 200cm max range

// ---- Radar display geometry ----
constexpr int16_t CENTER_X = LCD_WIDTH / 2;
constexpr int16_t CENTER_Y = LCD_HEIGHT - 20;   // pivot near bottom of screen
constexpr int16_t MAX_RADIUS = LCD_HEIGHT - 40; // leaves room for pivot + label margin
constexpr float MAX_RANGE_CM = 200.0f;

constexpr uint16_t COLOR_BG = 0x0000;      // black
constexpr uint16_t COLOR_GRID = 0x0320;    // dim green
constexpr uint16_t COLOR_SWEEP = 0x07E0;   // bright green
constexpr uint16_t COLOR_BLIP = 0xF800;    // red

// Ring of recent hits, each fades out over FADE_FRAMES sweeps then is reused.
struct Blip {
  float angleDeg;
  float rangeCm;
  int8_t life; // frames remaining, 0 = empty slot
};
constexpr int MAX_BLIPS = 64;
constexpr int8_t FADE_FRAMES = 40;
Blip blips[MAX_BLIPS];
int nextBlipSlot = 0;

int16_t polarX(float angleDeg, float radiusPx) {
  return CENTER_X + (int16_t)(radiusPx * cosf(angleDeg * DEG_TO_RAD));
}
int16_t polarY(float angleDeg, float radiusPx) {
  return CENTER_Y - (int16_t)(radiusPx * sinf(angleDeg * DEG_TO_RAD));
}

void drawGrid() {
  gfx->fillScreen(COLOR_BG);
  // Range rings.
  for (int i = 1; i <= 4; i++) {
    int16_t r = MAX_RADIUS * i / 4;
    // Approximate a semicircle with short line segments (cheap, no
    // per-pixel arc primitive needed for a static grid).
    int16_t prevX = polarX(0, r), prevY = polarY(0, r);
    for (int a = 5; a <= 180; a += 5) {
      int16_t x = polarX(a, r), y = polarY(a, r);
      gfx->drawLine(prevX, prevY, x, y, COLOR_GRID);
      prevX = x;
      prevY = y;
    }
  }
  // Angle spokes every 30 degrees.
  for (int a = 0; a <= 180; a += 30) {
    gfx->drawLine(CENTER_X, CENTER_Y, polarX(a, MAX_RADIUS), polarY(a, MAX_RADIUS), COLOR_GRID);
  }
  gfx->drawLine(0, CENTER_Y, LCD_WIDTH, CENTER_Y, COLOR_GRID); // baseline
}

void addBlip(float angleDeg, float rangeCm) {
  blips[nextBlipSlot] = {angleDeg, rangeCm, FADE_FRAMES};
  nextBlipSlot = (nextBlipSlot + 1) % MAX_BLIPS;
}

void drawFrame(float sweepAngleDeg) {
  drawGrid();

  // Fading blips, dimmer the older they are.
  for (auto &b : blips) {
    if (b.life <= 0) continue;
    float radiusPx = (b.rangeCm / MAX_RANGE_CM) * MAX_RADIUS;
    if (radiusPx > MAX_RADIUS) radiusPx = MAX_RADIUS;
    int16_t x = polarX(b.angleDeg, radiusPx);
    int16_t y = polarY(b.angleDeg, radiusPx);
    uint8_t brightness = (uint16_t)b.life * 255 / FADE_FRAMES;
    uint16_t color = gfx->color565(brightness, 0, 0);
    gfx->fillCircle(x, y, 3, color);
    b.life--;
  }

  // Sweep line (drawn last so it's on top).
  gfx->drawLine(CENTER_X, CENTER_Y, polarX(sweepAngleDeg, MAX_RADIUS),
                polarY(sweepAngleDeg, MAX_RADIUS), COLOR_SWEEP);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  if (!gfx->begin()) {
    Serial.println("Display init failed - check pins.h against Waveshare's demo pin_config.h");
    while (true) delay(1000);
  }
  gfx->fillScreen(COLOR_BG);

  Wire.begin(PCA9685_SDA, PCA9685_SCL);
  pwm.begin();
  pwm.setPWMFreq(50); // standard hobby servo rate

  servo.begin();
  sonar.begin();
}

void loop() {
  float angle = servo.update();

  // Let the servo settle a touch before pinging, otherwise readings taken
  // mid-slew read as noisy/inconsistent angles.
  static uint32_t lastPingMs = 0;
  uint32_t now = millis();
  if (now - lastPingMs >= 60) { // ~16 pings/sec, matches servo sweep cadence
    lastPingMs = now;
    float cm = sonar.readCm();
    if (cm > 0) addBlip(angle, cm);
  }

  drawFrame(angle);
}
