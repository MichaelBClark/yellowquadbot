// yellowquadbot/radar — ultrasonic ping-radar on a Waveshare
// ESP32-S3-Touch-LCD-1.46 (412x412 round display), servo-swept HC-SR04.
//
// Pin numbers in pins.h are confirmed from Waveshare's own docs
// (docs.waveshare.com/ESP32-S3-Touch-LCD-1.46). LCD_RST/TP_RST are wired
// through an onboard I2C GPIO expander rather than plain ESP32 pins,
// driven here via io_expander.h - see resetDisplayAndTouch() below. If the
// display still doesn't come up cleanly, check the boot-time I2C scan
// output against IO_EXPANDER_I2C_ADDR in pins.h; that address is a common
// default, not a confirmed one.
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
#include "io_expander.h"

// ---- Display bring-up (QSPI SH8601 round AMOLED) ----
//
// LCD_RST is wired through the board's I2C GPIO expander (EXIO2), not a
// plain ESP32 pin. It's pulsed via IoExpander in resetDisplayAndTouch(),
// called from setup() before gfx->begin() - GFX_NOT_DEFINED here just
// means "Arduino_GFX itself doesn't drive a reset pin", not that nothing
// resets the panel.
Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_GFX *gfx = new Arduino_SH8601(bus, GFX_NOT_DEFINED /* RST */, 0 /* rotation */,
                                       false /* IPS */, LCD_WIDTH, LCD_HEIGHT);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_I2C_ADDR, Wire);
ServoSweep servo(pwm, SERVO_CHANNEL, 5, 175, 90.0f); // degrees/sec sweep speed
Ultrasonic sonar(ULTRASONIC_TRIG, ULTRASONIC_ECHO, 200.0f); // 200cm max range
IoExpander expander(Wire, IO_EXPANDER_I2C_ADDR);

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

// Prints every I2C address that responds on the bus. Run this once to
// confirm IO_EXPANDER_I2C_ADDR in pins.h (and PCA9685_I2C_ADDR, and the
// touch controller's address if you care) against what's actually on your
// board, rather than trusting a guessed default.
void scanI2CBus() {
  Serial.println("Scanning I2C bus...");
  int found = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  found device at 0x%02X\n", addr);
      found++;
    }
  }
  if (found == 0) Serial.println("  no I2C devices found - check TOUCH_SDA/TOUCH_SCL wiring/pins");
}

void resetDisplayAndTouch() {
  expander.pinModeOutput(LCD_RST_EXIO_PIN);
  expander.pinModeOutput(TOUCH_RST_EXIO_PIN);
  expander.pulseResetLow(LCD_RST_EXIO_PIN);
  expander.pulseResetLow(TOUCH_RST_EXIO_PIN);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  haltIfPinsUnset();

  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH); // backlight on

  Wire.begin(PCA9685_SDA, PCA9685_SCL);
  scanI2CBus();
  resetDisplayAndTouch();

  if (!gfx->begin()) {
    Serial.println("Display init failed - check pins.h against Waveshare's demo pin_config.h");
    while (true) delay(1000);
  }
  gfx->fillScreen(COLOR_BG);

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
