#include "FastLED.h"
#include <EEPROM.h>

#define ENABLE_BRIGHTNESS true
#define ENABLE_SERIAL false

void runAnimation(unsigned long frameDelay, boolean resetAnimation, void (*animation)(uint8_t, void*), void *params);
//void runAnimation(uint8_t frameDelay = 50, boolean resetAnimation = false);

//typedef void (*Animation)(uint8_t frame, void *params);


typedef struct {
    CRGB color;
    boolean skipOuter;
} InnerToOuterParams;

/*
 * Constants
 */

const uint8_t kNumLeds = 31;
const uint8_t kDataPin = 9;
const uint8_t kBrightnessPin = A0;

const uint8_t kModeAddress = 0;
const uint8_t kModeInterrupt = 0; // Pin 2
const uint8_t kModeInterruptPin = 2;

const uint8_t kFlashlightAddress = 1;
const uint8_t kFlashlightInterrupt = 1; // Pin 3
const uint8_t kFlashlightInterruptPin = 3;

const uint8_t kInnerLeds[]  = { 24, 17, 12, 18, 23 };
const uint8_t kMiddleLeds[] = { 28, 25, 16, 13, 7, 4, 8, 11, 19, 22, 29 };
const uint8_t kOuterLeds[]  = { 27, 26, 15, 14, 6, 5, 1, 0, 2, 3, 9, 10, 20, 21, 30 };

const uint8_t kInnerLedsLength = sizeof(kInnerLeds) / sizeof(uint8_t);
const uint8_t kMiddleLedsLength = sizeof(kMiddleLeds) / sizeof(uint8_t);
const uint8_t kOuterLedsLength = sizeof(kOuterLeds) / sizeof(uint8_t);

/*
 * Color Schemes
 */

const uint32_t kItsAllAboutLove[]  = { CRGB(137,  23,   5),
                                       CRGB(157,  33,  55),
                                       CRGB(199,  53, 105),
                                       CRGB(229, 103, 155),
                                       CRGB(239, 183, 195) };
                                      
const uint32_t kGreatReds[]        = { CRGB(190,   0,   0),
                                       CRGB(191,  13,   7),
                                       CRGB(163,   3,  17),
                                       CRGB(163,   8,  13),
                                       CRGB(158,  12, 12) };
                                      
const uint32_t kINeedToMeetYou[]   = { CRGB(102, 53,   43),
                                       CRGB(117, 44,   69),
                                       CRGB( 66,  62, 124),
                                       CRGB( 39, 148, 176),
                                       CRGB(109, 244, 227) };
                                      
const uint32_t kLavenderLovelies[] = { CRGB( 99,  34, 118),
                                       CRGB(141,  64, 162),
                                       CRGB(177, 102, 198),
                                       CRGB(198, 132, 216),
                                       CRGB(222, 169, 237) };    

/*
 * Arduino Functions
 */

CRGB leds[kNumLeds];

uint16_t sensorValue = 0;
uint8_t brightness = 0;

enum AnimationModes {
    DEMO_ANIMATION = 0,
    HEARTBEAT = 1,
    RAINBOW = 2,
    COLOR_CYCLE = 3,
    COLOR_CYCLE_RAINBOW = 4,
    THEATER_RAINBOW = 5,
#if ENABLE_EQUALIZER
    EQUALIZER = 6
#endif
};
#if ENABLE_EQUALIZER
const uint8_t MAX_MODE = EQUALIZER;
#else
const uint8_t MAX_MODE = THEATER_RAINBOW;
#endif

volatile uint8_t mode = DEMO_ANIMATION;
volatile boolean modeChanged = false;

volatile bool flashlight = false;
volatile bool flashlightChanged = false;

void setup() 
{
#if ENABLE_SERIAL
    Serial.begin(9600);
#endif
   
    FastLED.addLeds<NEOPIXEL, kDataPin>(leds, kNumLeds);

#if ENABLE_BRIGHTNESS
    // Prepare for brightness check
    pinMode(kBrightnessPin, INPUT);
#else
    FastLED.setBrightness(70);
#endif
}

unsigned long skippedFrames = 0;
int frameDurations[10];
int frameDurationIndex = 0;

void loop() 
{
#if ENABLE_BRIGHTNESS
    checkBrightness();
#endif
    
    runAnimation(25, false, &rainbow, NULL);
    //theaterChaseRainbow(50);
    
    //InnerToOuterParams params = { .color = CRGB(255, 0, 0), .skipOuter = false };
    //runAnimation(200, false, &innerToOuter, &params);

//    static int i = 0;
//    if (i > 49)
//    {
//        int totalFrameDuration = 0;
//        int averageFrameDuration = 0;
//        for (int j = 0; j < 10; j++)
//        {
//            totalFrameDuration += frameDurations[j];
//        }
//        averageFrameDuration = totalFrameDuration / 10;
//        
//        Serial.print("average frame duration: ");
//        Serial.println(averageFrameDuration);
//        i = 0;
//    }
//    i++;
}

void runAnimation(unsigned long frameDelay, boolean resetAnimation, void (*animation)(uint8_t, void*), void *params)
{
    static uint8_t frame = 0;
    static unsigned long lastCallTime = millis();
    
    if (resetAnimation)
    {
        frame = 0;
        lastCallTime = millis();
    }

    unsigned long currentTime = millis();
    unsigned long timeDiff = currentTime - lastCallTime;
    if (timeDiff < frameDelay)
    {
        return;
    }
    /* Turned off frame skipping because it looks better without it
    else if (timeDiff > frameDelay)
    {
        // Skip frames
        int numFrames = (timeDiff - frameDelay) / frameDelay;
        frame += numFrames;

        skippedFrames = numFrames;
    }*/

    // Call the animation frame
    animation(frame, params);

    frame++;

    lastCallTime = currentTime;
}

/*
 * Color Pattern Functions
 */

// Fills the heart with color, ring by ring, starting from the inner ring
void innerToOuter(uint8_t frame, void *params)
{
    const InnerToOuterParams *p = (InnerToOuterParams*)params;
    
    const uint8_t numRings = p->skipOuter ? 2 : 3;
    const uint8_t ringIndex = frame % numRings;
//
//    Serial.print("frame: ");
//    Serial.println(frame);
//    Serial.print("numRings: ");
//    Serial.println(numRings);
//    Serial.print("ringIndex: ");
//    Serial.println(ringIndex);
//    Serial.flush();
    
    for (uint8_t i = 0; i < kInnerLedsLength; i++)
    {
        const uint8_t pixel = kInnerLeds[i];
        leds[pixel] = ringIndex == 0 ? p->color : CRGB::Black;
    }

    for (uint8_t i = 0; i < kMiddleLedsLength; i++)
    {
        const uint8_t pixel = kMiddleLeds[i];
        leds[pixel] = ringIndex == 1 ? p->color : CRGB::Black;
    }

    for (uint8_t i = 0; i < kOuterLedsLength; i++)
    {
        const uint8_t pixel = kOuterLeds[i];
        leds[pixel] = ringIndex == 2 ? p->color : CRGB::Black;
    }

    FastLED.show();
}

// Fades through the colors (average frame duration 26 ms)
void rainbow(uint8_t frame, void *params)
{
    for (int i = 0; i < kNumLeds; i++)
    {
        leds[i] = colorWheel((i + frame) & 255);
    }
    
    FastLED.show();
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t frame, void *params)
{
    for (int i = 0; i < kNumLeds; i++) 
    {
        leds[i] = colorWheel(((i * 256 / kNumLeds) + frame) & 255);
    }

    FastLED.show();
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t frame, void *params)
{
    const uint8_t offset = frame % 3;
    const boolean on = frame % 2;

    for (int i = 0; i < kNumLeds; i = i + 3) 
    {
        // Turn every third pixel on or off
        CRGB color = on ? colorWheel((i + frame) % 255) : CRGB::Black;
        leds[i + offset] = color;    
    }
    
    FastLED.show();
}

//// Theatre-style crawling lights with rainbow effect
//void theaterChaseRainbow(uint8_t wait)
//{
//    // Cycle all 256 colors in the wheel
//    for (int j = 0; j < 256; j+=2)
//    {
//        for (int q = 0; q < 3; q++)
//        {
//            for (int i = 0; i < kNumLeds; i = i + 3)
//            {
//                // Turn every third pixel on
//                //strip.setPixelColor(i + q, colorWheel((i + j) % 255));
//                leds[i + q] = colorWheel((i + j) % 255);
//            }
//            FastLED.show();
//            
//            delay(wait);
//            
//            for (int i = 0; i < kNumLeds; i = i + 3)
//            {
//                // Turn every third pixel off
//                //strip.setPixelColor(i + q, 0);
//                leds[i + q] = CRGB::Black;
//            }
//        }
//    }
//}

/*
 * Data Persistance
 */

void readModeStateFromEEPROM()
{
    // Read the mode value from persistant storage
    uint8_t value = EEPROM.read(kModeAddress);
  
    // Check if the mode is out of range
    if (value > MAX_MODE)
    {
        // Out of range, so choose the first mode
        value = 0;
        writeModeStateToEEPROM(value);
    }

    mode = value;
}

void writeModeStateToEEPROM(uint8_t state)
{
    EEPROM.write(kModeAddress, state);
}

void readFlashlightStateFromEEPROM()
{
    // Read the mode value from persistant storage
    uint8_t value = EEPROM.read(kFlashlightAddress);
  
    // Check if the mode is out of range
    if (value > 1)
    {
        // Out of range, so choose false
        value = 0;
        writeFlashlightStateToEEPROM(false);
    }

    flashlight = (bool)value;
}

void writeFlashlightStateToEEPROM(bool state)
{
    EEPROM.write(kModeAddress, state);
}

/*
 * Button Handling
 */

void attachModeButtonInterrupt()
{
    pinMode(kModeInterruptPin, INPUT_PULLUP);
    attachInterrupt(kModeInterrupt, modeButtonPressed, RISING);
}

void modeButtonPressed()
{
    static unsigned long lastInterruptTime = 0;
  
    // Rate limit to every 200 ms
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > 200)
    {
        // Go to the next mode
        uint8_t newMode = mode + 1;

        // Let the animation functions know the mode changed
        modeChanged = true;
  
        // Cycle back to the first mode if necessary
        if (newMode > MAX_MODE)
        {
            newMode = 0;
        }
  
        // Persist the value
        mode = newMode;
        writeModeStateToEEPROM(mode);
  
        lastInterruptTime = interruptTime;
    }
}

void attachFlashlightButtonInterrupt()
{
    pinMode(kFlashlightInterruptPin, INPUT_PULLUP);
    attachInterrupt(kFlashlightInterrupt, flashlightButtonPressed, RISING);
}

void flashlightButtonPressed()
{
    static unsigned long lastInterruptTime = 0;
  
    // Rate limit to every 200 ms
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > 200)
    {
        // Go to the next mode
        bool newFlashlight = !flashlight;

        // Let the animation functions know the mode changed
        flashlightChanged = true;
  
        // Persist the value
        flashlight = newFlashlight;
        writeFlashlightStateToEEPROM(newFlashlight);
  
        lastInterruptTime = interruptTime;
    }
}

/*
 * Helper Functions
 */

#if ENABLE_BRIGHTNESS
void checkBrightness()
{
    // Read the raw value from the potentiometer
    uint16_t sensorValueNew = analogRead(kBrightnessPin);
  
    // Clip the values to 1 - 650
    const uint16_t minSensorValue = 1;
    const uint16_t maxSensorValue = 650;
    sensorValueNew = constrain(sensorValueNew, 1, 650);

    // Remove jitter
    const uint16_t acceptedRange = 10; 
    uint16_t range = abs(sensorValueNew - sensorValue);

#if ENABLE_SERIAL
    Serial.print("sensor value old: ");
    Serial.print(sensorValue);
    Serial.print("sensor value new: ");
    Serial.print(sensorValueNew);
    Serial.print("range: ");
    Serial.println(range);
#endif

    bool closeToBottom = sensorValue != minSensorValue && sensorValue < range;
    bool closeToTop = sensorValue != maxSensorValue && sensorValue < maxSensorValue - range;
    if (brightness == 0 || closeToBottom || closeToTop || range > acceptedRange)
    {
        // Map the sensor values into the "night to day" brightness range
        int brightnessNew = map(sensorValueNew, 1, 650, 20, 255);

#if ENABLE_SERIAL
        Serial.print(" old value: ");
        Serial.print(brightness);
        Serial.print(" new value: ");
        Serial.println(brightnessNew);
#endif

        // If the brightness changed, save and set it
        FastLED.setBrightness(brightnessNew);
        brightness = brightnessNew;
        sensorValue = sensorValueNew;
    }
}
#endif

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
CRGB colorWheel(byte wheelPos) 
{
    wheelPos = 255 - wheelPos;
    if (wheelPos < 85) 
    {
        return CRGB(255 - wheelPos * 3, 0, wheelPos * 3);
    } 
    else if (wheelPos < 170) 
    {
        wheelPos -= 85;
        return CRGB(0, wheelPos * 3, 255 - wheelPos * 3);
    }
    else 
    {
        wheelPos -= 170;
        return CRGB(wheelPos * 3, 255 - wheelPos * 3, 0);
    }
}
