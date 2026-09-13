// TFT_eSPI configuration for the ELEGOO EL-EB-009 / ESP32-2432S028R
// ("Cheap Yellow Display"). TFT_eSPI is configured at the LIBRARY level,
// not per-sketch - this file must be COPIED to overwrite
// <Arduino libraries folder>/TFT_eSPI/User_Setup.h, not left sitting in
// this sketch folder. See the README for the exact steps.
//
// Pins verified against github.com/witnessmenow/ESP32-Cheap-Yellow-Display
// (PINS.md / DisplayConfig/User_Setup.h in that repo), trimmed to just
// what this board needs.

#define USER_SETUP_INFO "ELEGOO_EL-EB-009_CYD"

#define ILI9341_2_DRIVER     // this board's specific ILI9341 variant

#define TFT_WIDTH  240
#define TFT_HEIGHT 320

#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1   // tied to the board's own reset, not a separate pin

#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

// This sketch doesn't use TFT_eSPI's built-in touch support (the XPT2046
// touch controller is on a separate SPI bus from the display on this
// board, so it needs its own library/wiring, not this one) - TOUCH_CS is
// defined only to silence a library warning, not wired to anything real.
#define TOUCH_CS -1

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT

#define SPI_FREQUENCY  40000000
#define SPI_READ_FREQUENCY  20000000

#define USE_HSPI_PORT
