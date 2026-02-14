#ifndef HW_SETUP_H
#define HW_SETUP_H

// ============================================================================
//  Hardware Pin Assignments and Peripheral Setup
//
//  Teensy 4.0 pin map:
//    0-3   : MUX address lines (shared by all four muxes)
//    4-6   : OLED SPI (MOSI, CLK, DC)
//    9-11  : LED shift register (DATA, CLOCK, LATCH)
//    12    : External clock input (rising-edge interrupt)
//    14    : Knob MUX 1 analog input (16 knobs)
//    16    : Knob MUX 2 analog input (16 knobs)
//    17    : Step button MUX digital input (16 buttons)
//    22    : Control button MUX digital input (10 buttons)
// ============================================================================

// OLED SPI pins (software SPI — no CS/RST needed)
#define OLED_MOSI 4
#define OLED_CLK  5
#define OLED_DC   6
#define OLED_CS   -1
#define OLED_RST  -1

// LED shift register pins (2x 74HC595 daisy-chained, 16 LEDs)
#define LED_DATA  9
#define LED_CLOCK 10
#define LED_LATCH 11

// External clock input (rising-edge interrupt via attachInterrupt)
#define EXT_CLK_PIN 12

// OLED display (128x64 SH1106G, software SPI)
Adafruit_SH1106G display(128, 64, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

// LED shift register (2x 74HC595 = 16 outputs)
ShiftRegister74HC595<2> sr(LED_DATA, LED_CLOCK, LED_LATCH);

// Analog/digital multiplexers (4-bit address → 16 channels each)
using namespace admux;

Mux knobMux1      = Mux(Pin(14, INPUT, PinType::Analog),  Pinset(0, 1, 2, 3));  // Knobs 0-15
Mux knobMux2      = Mux(Pin(16, INPUT, PinType::Analog),  Pinset(0, 1, 2, 3));  // Knobs 16-31
Mux stepButtonsMux  = Mux(Pin(17, INPUT, PinType::Digital), Pinset(0, 1, 2, 3));  // Step buttons 0-15
Mux otherButtonsMux = Mux(Pin(22, INPUT, PinType::Digital), Pinset(0, 1, 2, 3));  // Control buttons 0-9

// Hardware counts
const int knobCount         = 32;  // 2 muxes × 16 channels
const int stepButtonCount   = 16;  // 1 mux × 16 channels
const int otherButtonsCount = 10;  // 1 mux, 10 of 16 channels used
const int numSteps          = 16;  // Sequencer step count (matches step buttons)

#endif