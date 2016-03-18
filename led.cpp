/*
    *  Flybrix Flight Controller -- Copyright 2015 Flying Selfie Inc.
    *
    *  License and other details available at: http://www.flybrix.com/firmware
*/

#include "led.h"
#include "board.h"
#include "state.h"

#define NO_LEDS  // Instead of looking at actual LEDs, we look at the serial debug for feedback
#ifdef NO_LEDS
#include "debug.h"
#endif

namespace {
class LEDDriver {
   public:
    LEDDriver();
    void setPattern(LED::Pattern pattern);
    void setColor(CRGB color, board::led::Position lower_left = {-128, -128}, board::led::Position upper_right = {127, 127});
    void set(LED::Pattern pattern, CRGB color);
    void update();  // fire this off at 30Hz

   private:
    void writeToDisplay();
    void updateBeacon();     // 2sec periodic double pulse
    void updateFlash();      //~3Hz flasher
    void updateBreathe();    // 4sec periodic breathe
    void updateAlternate();  // 4sec on - 4sec off alternating TODO: use the proper left-right alternating
    void updateSolid();      // maintain constant light level

    uint8_t cycleIndex{0};
    uint8_t pattern;
    uint8_t scale{0};
    CRGB leds[board::led::COUNT];
    bool hasChanges{true};
} LED_driver;

LEDDriver::LEDDriver() {
    setColor(CRGB::Black);
    setPattern(LED::SOLID);
    FastLED.addLeds<WS2812B, board::led::DATA_PIN>(leds, board::led::COUNT);
}

inline bool isInside(const board::led::Position& p, const board::led::Position& p_min, const board::led::Position& p_max) {
    return p.x >= p_min.x && p.y >= p_min.y && p.x <= p_max.x && p.y <= p_max.y;
}

void LEDDriver::setColor(CRGB color, board::led::Position lower_left, board::led::Position upper_right) {
    for (size_t idx = 0; idx < board::led::COUNT; ++idx) {
        if (!isInside(board::led::POSITION[idx], lower_left, upper_right))
            continue;
        if (leds[idx].red == color.red && leds[idx].green == color.green && leds[idx].blue == color.blue)
            continue;
        hasChanges = true;
        leds[idx] = color;
    }
}

void LEDDriver::setPattern(LED::Pattern pattern) {
    if (pattern == this->pattern)
        return;
    this->pattern = pattern;
    hasChanges = true;
    cycleIndex = 255;
}

void LEDDriver::set(LED::Pattern pattern, CRGB color) {
    setColor(color);
    setPattern(pattern);
}

void LEDDriver::update() {
    ++cycleIndex;
    writeToDisplay();
    if (!hasChanges)
        return;
    FastLED.show(scale);
#ifdef NO_LEDS
    DebugPrintf("%d %d %d | %d %d %d | %d %d %d | %d %d %d | %d", leds[0].red, leds[0].green, leds[0].blue, leds[1].red, leds[1].green, leds[1].blue, leds[2].red, leds[2].green, leds[2].blue,
                leds[3].red, leds[3].green, leds[3].blue, scale);
#endif
    hasChanges = false;
}

void LEDDriver::updateFlash() {
    if (cycleIndex & 3)
        return;
    scale = (cycleIndex & 4) ? 0 : 255;
    hasChanges = true;
}

void LEDDriver::updateBeacon() {
    switch ((cycleIndex & 63) >> 2) {  // two second period
        case 1:
        case 4:
            scale = 255;
            hasChanges = true;
            break;
        case 2:
        case 5:
            scale = 0;
            hasChanges = true;
            break;
        default:
            break;
    }
}

void LEDDriver::updateBreathe() {
    uint16_t multiplier = cycleIndex & 127;
    if (multiplier > 31)
        return;
    scale = multiplier;
    if (scale > 15)
        scale = 31 - scale;
    scale <<= 4;
    hasChanges = true;
}

void LEDDriver::updateAlternate() {
    if (cycleIndex & 127)
        return;
    scale = (cycleIndex & 128) ? 0 : 255;
    hasChanges = true;
}

void LEDDriver::updateSolid() {
    if (scale == 255)
        return;
    scale = 255;
    hasChanges = true;
}

void LEDDriver::writeToDisplay() {
    switch (pattern) {
        case LED::FLASH:
            updateFlash();
            break;
        case LED::BEACON:
            updateBeacon();
            break;
        case LED::BREATHE:
            updateBreathe();
            break;
        case LED::ALTERNATE:
            updateAlternate();
            break;
        case LED::SOLID:
            updateSolid();
            break;
    }
}
}  // namespace

LED::LED(State* __state) : state(__state) {
    // indicator leds are not inverted
    pinMode(board::GREEN_LED, OUTPUT);
    pinMode(board::RED_LED, OUTPUT);
}

void LED::set(Pattern pattern, uint8_t red_a, uint8_t green_a, uint8_t blue_a, uint8_t red_b, uint8_t green_b, uint8_t blue_b, bool red_indicator, bool green_indicator) {
    override = pattern != LED::NO_OVERRIDE;
    oldStatus = 0;
    if (!override)
        return;
    red_indicator ? indicatorRedOn() : indicatorRedOff();
    green_indicator ? indicatorGreenOn() : indicatorGreenOff();
    LED_driver.setPattern(pattern);
    LED_driver.setColor({red_a, green_a, blue_a}, {0, -128}, {127, 127});
    LED_driver.setColor({red_b, green_b, blue_b}, {-128, -128}, {0, 127});
}

void LED::update() {
    if (!override && oldStatus != state->status) {
        oldStatus = state->status;
        changeLights();
    }
    LED_driver.update();
}

void LED::changeLights() {
    if (state->is(STATUS_MPU_FAIL)) {
        LED_driver.setPattern(LED::SOLID);
        LED_driver.setColor(CRGB::Black);
        LED_driver.setColor(CRGB::Red, {-128, -128}, {0, 127});
        indicatorRedOn();
    } else if (state->is(STATUS_BMP_FAIL)) {
        LED_driver.setPattern(LED::SOLID);
        LED_driver.setColor(CRGB::Black);
        LED_driver.setColor(CRGB::Red, {0, -128}, {127, 127});
        indicatorRedOn();
    } else if (state->is(STATUS_BOOT)) {
        LED_driver.set(LED::SOLID, CRGB::Green);
    } else if (state->is(STATUS_UNPAIRED)) {
        // use(LED::FLASH, 255,180,20); //orange
        LED_driver.set(LED::FLASH, CRGB::White);  // for pcba testing purposes -- a "good" board will end up in this state
        indicatorGreenOn();                       // for pcba testing purposes -- a "good" board will end up in this state
    } else if (state->is(STATUS_RX_FAIL)) {
        LED_driver.set(LED::FLASH, CRGB::Red);  // red
    } else if (state->is(STATUS_FAIL_STABILITY) || state->is(STATUS_FAIL_ANGLE)) {
        LED_driver.set(LED::FLASH, CRGB::Yellow);  // yellow
    } else if (state->is(STATUS_OVERRIDE)) {
        LED_driver.set(LED::BEACON, CRGB::Red);  // red
    } else if (state->is(STATUS_ENABLING)) {
        LED_driver.set(LED::FLASH, CRGB::Blue);  // blue
    } else if (state->is(STATUS_ENABLED)) {
        LED_driver.set(LED::BEACON, CRGB::Blue);  // blue for enable
    } else if (state->is(STATUS_TEMP_WARNING)) {
        LED_driver.set(LED::FLASH, CRGB::Yellow);  // yellow
    } else if (state->is(STATUS_LOG_FULL)) {
        LED_driver.set(LED::FLASH, CRGB::Blue);
    } else if (state->is(STATUS_BATTERY_LOW)) {
        LED_driver.set(LED::BEACON, CRGB::Orange);
    } else if (state->is(STATUS_IDLE)) {
        indicatorRedOff();                         // clear boot test
        LED_driver.set(LED::BEACON, CRGB::Green);  // breathe instead?
    } else {
        // ERROR: ("NO STATUS BITS SET???");
    }
}

void LED::indicatorRedOn() {
    digitalWriteFast(board::RED_LED, HIGH);
}

void LED::indicatorGreenOn() {
    digitalWriteFast(board::GREEN_LED, HIGH);
}

void LED::indicatorRedOff() {
    digitalWriteFast(board::RED_LED, LOW);
}

void LED::indicatorGreenOff() {
    digitalWriteFast(board::GREEN_LED, LOW);
}
