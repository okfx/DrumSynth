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
static constexpr int OLED_MOSI = 4;
static constexpr int OLED_CLK  = 5;
static constexpr int OLED_DC   = 6;
static constexpr int OLED_CS   = -1;
static constexpr int OLED_RST  = -1;

// LED shift register pins (2x 74HC595 daisy-chained, 16 LEDs)
static constexpr int LED_DATA  = 9;
static constexpr int LED_CLOCK = 10;
static constexpr int LED_LATCH = 11;

// External clock input (rising-edge interrupt via attachInterrupt)
static constexpr int EXT_CLK_PIN = 12;

// OLED display (128x64 SH1106G, software SPI)
Adafruit_SH1106G display(128, 64, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);

// LED shift register (2x 74HC595 = 16 outputs)
ShiftRegister74HC595<2> ledShiftReg(LED_DATA, LED_CLOCK, LED_LATCH);

// Analog/digital multiplexers (4-bit address → 16 channels each)
admux::Mux knobMux1      = admux::Mux(admux::Pin(14, INPUT, admux::PinType::Analog),  admux::Pinset(0, 1, 2, 3));  // Knobs 0-15
admux::Mux knobMux2      = admux::Mux(admux::Pin(16, INPUT, admux::PinType::Analog),  admux::Pinset(0, 1, 2, 3));  // Knobs 16-31
admux::Mux stepButtonsMux  = admux::Mux(admux::Pin(17, INPUT, admux::PinType::Digital), admux::Pinset(0, 1, 2, 3));  // Step buttons 0-15
admux::Mux otherButtonsMux = admux::Mux(admux::Pin(22, INPUT, admux::PinType::Digital), admux::Pinset(0, 1, 2, 3));  // Control buttons 0-9

// Hardware counts
static constexpr int knobCount         = 32;  // 2 muxes × 16 channels
static constexpr int stepButtonCount   = 16;  // 1 mux × 16 channels
static constexpr int otherButtonsCount = 10;  // 1 mux, 10 of 16 channels used
static constexpr int numSteps          = stepButtonCount;  // Sequencer step count

#endif