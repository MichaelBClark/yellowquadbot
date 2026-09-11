// yellowquadbot/radar — ultrasonic ping-radar on a Waveshare
// ESP32-S3-Touch-LCD-1.46 (412x412 round display), servo-swept HC-SR04.
//
// Pin numbers in pins.h are confirmed from Waveshare's own docs
// (docs.waveshare.com/ESP32-S3-Touch-LCD-1.46). One open item: LCD_RST is
// wired through an onboard I2C GPIO expander, not driven directly by this
// code yet - see the comment above the Arduino_SH8601 constructor below.
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
//
// !! LCD_RST is wired through the board's I2C GPIO expander (EXIO2), not a
// plain ESP32 pin - see the big comment on LCD_RST_EXIO_PIN in pins.h. This
// code passes GFX_NOT_DEFINED (no reset line) to Arduino_GFX rather than
// silently getting that wrong, which means the display relies on its
// power-on reset instead of an explicit one. That's usually fine for a
// clean boot, but if gfx->begin() fails or the panel stays blank, the
// expander-driven reset is the most likely reason and needs adding
// (Arduino_GFX has expander-aware bus/reset classes - search its examples
// for this exact board, "ESP32-S3-Touch-LCD-1.46" or "1.46 AMOLED", for
// the expander chip's I2C address and correct wiring, rather than
// guessing).
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_GFX *gfx = new Arduino_SH8601(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */,
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

// Cheap insurance against a pin accidentally getting set to -1 (or left
// unset) in a future edit of pins.h - passing -1 to pinMode()/Wire.begin()/
// the QSPI display driver doesn't fail cleanly, it corrupts GPIO/bus state
// and crashes with an opaque Guru Meditation StoreProhibited panic. Catch
// it here instead, before anything touches hardware.
void haltIfPinsUnset() {
  struct NamedPin { const char *name; int pin; };
  const NamedPin required[] = {
      {"LCD_SDIO0", LCD_SDIO0}, {"LCD_SDIO1", LCD_SDIO1},
      {"LCD_SDIO2", LCD_SDIO2}, {"LCD_SDIO3", LCD_SDIO3},
      {"LCD_SCLK", LCD_SCLK},   {"LCD_CS", LCD_CS}, {"LCD_BL", LCD_BL},
      {"PCA9685_SDA", PCA9685_SDA}, {"PCA9685_SCL", PCA9685_SCL},
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

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH); // backlight on

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
