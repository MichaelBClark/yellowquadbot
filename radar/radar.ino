// yellowquadbot/radar — ultrasonic ping-radar on a Waveshare
// ESP32-S3-Touch-LCD-1.46 (412x412 round display), servo-swept HC-SR04.
//
// The display/touch controller is an SPD2010, which Arduino_GFX does not
// support (its QSPI classes assume a different, 8-bit-command panel
// protocol; SPD2010 needs ESP-IDF's esp_lcd_panel APIs with a 32-bit
// command width and a dedicated vendor driver). I2C_Driver.*,
// TCA9554PWR.*, Touch_SPD2010.*, esp_lcd_spd2010.*, and Display_SPD2010.*
// in this folder are copied verbatim from Waveshare's own working example
// for this exact board (waveshareteam/ESP32-S3-Touch-LCD-1.46,
// example/Arduino-3.1.1/examples/LVGL_Arduino) rather than reimplemented,
// since getting a QSPI panel's low-level init sequence subtly wrong
// produces exactly the kind of garbled-but-not-crashing output this
// project hit before finding that source.
//
// Display_SPD2010.h's LCD_addWindow() blits a full rectangular pixel
// buffer - there's no drawLine/fillCircle primitive API like Arduino_GFX
// had, so this file keeps its own tiny software framebuffer (one
// LCD_WIDTH*LCD_HEIGHT array of RGB565 pixels in PSRAM) and draws into it
// with plain Bresenham/midpoint routines, then blits the whole thing once
// per frame.
//
// Classic "ping radar" layout: servo sweeps 0-180 degrees, pivot at the
// bottom-center of the screen, targets plotted in the top semicircle with
// distance from center = signal strength, and a fading trail of past hits.

#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "Display_SPD2010.h"

#include "pins.h"
#include "ultrasonic.h"
#include "servo_sweep.h"

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(PCA9685_I2C_ADDR, Wire);
ServoSweep servo(pwm, SERVO_CHANNEL, 5, 175, 90.0f); // degrees/sec sweep speed
Ultrasonic sonar(ULTRASONIC_TRIG, ULTRASONIC_ECHO, 200.0f); // 200cm max range

// ---- Software framebuffer (RGB565), blitted via LCD_addWindow() ----
uint16_t *fb = nullptr; // EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT, from Display_SPD2010.h

inline uint16_t color565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

inline void setPixel(int16_t x, int16_t y, uint16_t c) {
  if (x < 0 || y < 0 || x >= EXAMPLE_LCD_WIDTH || y >= EXAMPLE_LCD_HEIGHT) return;
  fb[y * EXAMPLE_LCD_WIDTH + x] = c;
}

void fillScreen(uint16_t c) {
  for (int i = 0; i < EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT; i++) fb[i] = c;
}

void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c) {
  int16_t dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
  int16_t dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
  int16_t err = dx + dy;
  while (true) {
    setPixel(x0, y0, c);
    if (x0 == x1 && y0 == y1) break;
    int16_t e2 = 2 * err;
    if (e2 >= dy) { err += dy; x0 += sx; }
    if (e2 <= dx) { err += dx; y0 += sy; }
  }
}

void fillCircle(int16_t cx, int16_t cy, int16_t r, uint16_t c) {
  for (int16_t y = -r; y <= r; y++)
    for (int16_t x = -r; x <= r; x++)
      if (x * x + y * y <= r * r) setPixel(cx + x, cy + y, c);
}

// ---- Radar display geometry ----
constexpr int16_t CENTER_X = EXAMPLE_LCD_WIDTH / 2;
constexpr int16_t CENTER_Y = EXAMPLE_LCD_HEIGHT - 20;   // pivot near bottom of screen
constexpr int16_t MAX_RADIUS = EXAMPLE_LCD_HEIGHT - 40; // leaves room for pivot + label margin
constexpr float MAX_RANGE_CM = 200.0f;

const uint16_t COLOR_BG = color565(0, 0, 0);
const uint16_t COLOR_GRID = color565(0, 60, 0);
const uint16_t COLOR_SWEEP = color565(0, 255, 0);

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
  fillScreen(COLOR_BG);
  // Range rings.
  for (int i = 1; i <= 4; i++) {
    int16_t r = MAX_RADIUS * i / 4;
    int16_t prevX = polarX(0, r), prevY = polarY(0, r);
    for (int a = 5; a <= 180; a += 5) {
      int16_t x = polarX(a, r), y = polarY(a, r);
      drawLine(prevX, prevY, x, y, COLOR_GRID);
      prevX = x;
      prevY = y;
    }
  }
  // Angle spokes every 30 degrees.
  for (int a = 0; a <= 180; a += 30) {
    drawLine(CENTER_X, CENTER_Y, polarX(a, MAX_RADIUS), polarY(a, MAX_RADIUS), COLOR_GRID);
  }
  drawLine(0, CENTER_Y, EXAMPLE_LCD_WIDTH, CENTER_Y, COLOR_GRID); // baseline
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
    fillCircle(x, y, 3, color565(brightness, 0, 0));
    b.life--;
  }

  // Sweep line (drawn last so it's on top).
  drawLine(CENTER_X, CENTER_Y, polarX(sweepAngleDeg, MAX_RADIUS),
            polarY(sweepAngleDeg, MAX_RADIUS), COLOR_SWEEP);

  LCD_addWindow(0, 0, EXAMPLE_LCD_WIDTH - 1, EXAMPLE_LCD_HEIGHT - 1, fb);
}

// Cheap insurance against a pin accidentally getting set to -1 in a future
// edit of pins.h - passing -1 to pinMode()/Wire.begin() doesn't fail
// cleanly, it corrupts GPIO/bus state and crashes with an opaque Guru
// Meditation StoreProhibited panic. Catch it here instead, before
// anything touches hardware.
void haltIfPinsUnset() {
  struct NamedPin { const char *name; int pin; };
  const NamedPin required[] = {
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

  fb = (uint16_t *)ps_malloc((size_t)EXAMPLE_LCD_WIDTH * EXAMPLE_LCD_HEIGHT * sizeof(uint16_t));
  if (!fb) {
    Serial.println("Framebuffer allocation failed - PSRAM not enabled? "
                    "Check Tools > PSRAM in the Arduino IDE.");
    while (true) delay(1000);
  }

  // Same bring-up order as Waveshare's own LVGL_Arduino.ino: I2C, then the
  // IO expander (all EXIO pins as outputs), then backlight, then the LCD
  // itself (which pulses its reset line through the expander and inits
  // the touch controller internally).
  I2C_Init();
  TCA9554PWR_Init(0x00);
  Backlight_Init();
  Set_Backlight(80);
  LCD_Init();

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
