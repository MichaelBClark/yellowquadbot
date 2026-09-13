// yellowquadbot/radar — ultrasonic ping-radar on an ELEGOO EL-EB-009
// (ESP32-2432S028R "Cheap Yellow Display", 320x240 ILI9341), servo-swept
// HC-SR04.
//
// Display comes from TFT_eSPI, configured at the LIBRARY level via
// User_Setup.h in this folder - that file must be copied over
// <Arduino libraries>/TFT_eSPI/User_Setup.h before this will compile
// correctly. See README.md.
//
// Classic "ping radar" layout: servo sweeps 0-180 degrees, pivot at the
// bottom-center of the screen, targets plotted in the top semicircle with
// distance from center = signal strength, and a fading trail of past hits.
// Landscape orientation (320 wide x 240 tall) gives the semicircle plenty
// of horizontal room.

#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Adafruit_PWMServoDriver.h>

#include "pins.h"
#include "ultrasonic.h"
#include "servo_sweep.h"

TFT_eSPI tft = TFT_eSPI();

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_I2C_ADDR, Wire);
ServoSweep servo(pwm, SERVO_CHANNEL, 5, 175, 90.0f); // degrees/sec sweep speed
Ultrasonic sonar(ULTRASONIC_TRIG, ULTRASONIC_ECHO, 200.0f); // 200cm max range

// ---- Radar display geometry (landscape 320x240) ----
constexpr int16_t SCREEN_W = 320;
constexpr int16_t SCREEN_H = 240;
constexpr int16_t CENTER_X = SCREEN_W / 2;
constexpr int16_t CENTER_Y = SCREEN_H - 20;  // pivot near bottom of screen
constexpr int16_t MAX_RADIUS = 150;          // fits within both screen edges
constexpr float MAX_RANGE_CM = 200.0f;

const uint16_t COLOR_BG = TFT_BLACK;
const uint16_t COLOR_GRID = 0x0320;   // dim green
const uint16_t COLOR_SWEEP = TFT_GREEN;

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
  tft.fillScreen(COLOR_BG);
  // Range rings.
  for (int i = 1; i <= 4; i++) {
    int16_t r = MAX_RADIUS * i / 4;
    int16_t prevX = polarX(0, r), prevY = polarY(0, r);
    for (int a = 5; a <= 180; a += 5) {
      int16_t x = polarX(a, r), y = polarY(a, r);
      tft.drawLine(prevX, prevY, x, y, COLOR_GRID);
      prevX = x;
      prevY = y;
    }
  }
  // Angle spokes every 30 degrees.
  for (int a = 0; a <= 180; a += 30) {
    tft.drawLine(CENTER_X, CENTER_Y, polarX(a, MAX_RADIUS), polarY(a, MAX_RADIUS), COLOR_GRID);
  }
  tft.drawLine(0, CENTER_Y, SCREEN_W, CENTER_Y, COLOR_GRID); // baseline
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
    tft.fillCircle(x, y, 3, tft.color565(brightness, 0, 0));
    b.life--;
  }

  // Sweep line (drawn last so it's on top).
  tft.drawLine(CENTER_X, CENTER_Y, polarX(sweepAngleDeg, MAX_RADIUS),
               polarY(sweepAngleDeg, MAX_RADIUS), COLOR_SWEEP);
}

// Cheap insurance against a pin accidentally getting set to -1 in a future
// edit of pins.h - passing -1 to pinMode()/Wire.begin() doesn't fail
// cleanly, it corrupts GPIO/bus state and crashes with an opaque Guru
// Meditation StoreProhibited panic. Catch it here instead, before
// anything touches hardware.
void haltIfPinsUnset() {
  struct NamedPin { const char *name; int pin; };
  const NamedPin required[] = {
      {"I2C_SDA_PIN", I2C_SDA_PIN}, {"I2C_SCL_PIN", I2C_SCL_PIN},
      {"ULTRASONIC_TRIG", ULTRASONIC_TRIG},
      {"ULTRASONIC_ECHO", ULTRASONIC_ECHO},
  };

  bool missing = false;
  for (auto &p : required) {
    if (p.pin < 0) {
      Serial.printf("pins.h: %s is -1/unset\n", p.name);
      missing = true;
    }
  }
  if (!missing) return;

  Serial.println("Halting before touching any hardware - fix the pins "
                  "above in pins.h and re-flash.");
  while (true) delay(1000);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  haltIfPinsUnset();

  tft.init();
  tft.setRotation(1); // landscape, USB connector on the left
  tft.fillScreen(COLOR_BG);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
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
