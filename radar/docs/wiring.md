# Wiring

## Display / touch

Already wired on-board (SPI to the ILI9341 TFT, a separate SPI bus to the
XPT2046 touch controller — not used by this app) — nothing to connect
there. Pins are configured through TFT_eSPI's `User_Setup.h` (see the
README's "Building with the Arduino IDE" section for the required copy
step), not through `pins.h`.

## Servo, via PCA9685

The servo doesn't connect to the ESP32 directly — it goes through a
PCA9685 I2C PWM driver, same part used for the yellowquadbot legs
elsewhere in this repo. This board only has one free GPIO pair (the CN1
connector), so that's what the I2C bus uses.

| PCA9685 pin | Connects to |
|-------------|-------------|
| VCC (logic) | 3.3V |
| GND | GND (common with everything else) |
| SDA | GPIO22 (`I2C_SDA_PIN` in `pins.h`, the CN1 connector) |
| SCL | GPIO27 (`I2C_SCL_PIN` in `pins.h`, the CN1 connector) |
| V+ (servo power rail) | External 5V supply, **not** the board's own 5V/USB pin |
| Channel 0 (or whichever you set `SERVO_CHANNEL` to) signal pin | Servo signal wire (orange/white) |

Servo's own + and GND wires go to the PCA9685's screw terminal alongside
V+ and GND, not to the ESP32 board.

A single small servo (SG90-class) can usually run off a basic 5V USB
supply into the PCA9685's V+ terminal. If you add more servos later or
upgrade to something higher-torque, budget the supply for their combined
stall current — same reasoning as any multi-servo project.

## Ultrasonic sensor (HC-SR04)

This board has no free GPIO left after the display, touch, and the I2C
pair above — `pins.h` reclaims 2 of the 3 onboard RGB LED pins (see the
README's "This board is nearly out of GPIO" section for why that's the
right move here, not a hack).

| HC-SR04 pin | Connects to |
|-------------|-------------|
| VCC         | 5V |
| GND         | GND (common with servo and board) |
| TRIG        | GPIO4 (`ULTRASONIC_TRIG`, was the RGB LED's red channel) |
| ECHO        | GPIO17 (`ULTRASONIC_ECHO`, was the RGB LED's blue channel) **through a voltage divider** |

**Important:** the HC-SR04's ECHO pin outputs a 5V logic signal. The
ESP32's GPIOs are not 5V-tolerant — feeding ECHO straight into a GPIO
risks damaging the pin. Use a simple resistor divider: ECHO through a 1kΩ
resistor to the GPIO, then a 2kΩ resistor from that same GPIO node down to
GND. That divides 5V down to a safe ~3.3V for the ESP32 to read, without
noticeably affecting TRIG (which the ESP32 drives, not receives).

If the onboard RGB LED is still physically populated and you'd rather
keep it working, desolder it first — otherwise it'll flicker/light up as
a side effect of the ultrasonic sensor's signals toggling those pins,
harmless but visually confusing.
