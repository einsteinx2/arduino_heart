#include "FastLED.h"
#include <EEPROM.h>

#define ENABLE_BRIGHTNESS true
#define ENABLE_SERIAL false

void runAnimation(unsigned long frameDelay, boolean resetAnimation, void (*animation)(uint8_t*, void*), void *params);
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
const uint8_t kBrightnessPin = A3;

const uint8_t kModeAddress = 0;
const uint8_t kFlashlightAddress = 10;
const uint8_t kButtonInterrupt = 1; // Pin 3
const uint8_t kButtonInterruptPin = 3;

const uint8_t kInnerLeds[]  = { 6, 7, 12, 13, 18 };
const uint8_t kMiddleLeds[] = { 1, 2, 5, 8, 11, 14, 17, 19, 22, 23, 26 };
const uint8_t kOuterLeds[]  = { 0, 3, 4, 9, 10, 15, 16, 20, 21, 24, 25, 27, 28, 29, 30 };

const uint8_t kInnerLedsLength = sizeof(kInnerLeds) / sizeof(uint8_t);
const uint8_t kMiddleLedsLength = sizeof(kMiddleLeds) / sizeof(uint8_t);
const uint8_t kOuterLedsLength = sizeof(kOuterLeds) / sizeof(uint8_t);

/*
 * Color Schemes
 */

const CRGB kItsAllAboutLove[]  = { CRGB(137,  23,   5),
                                   CRGB(157,  33,  55),
                                   CRGB(199,  53, 105),
                                   CRGB(229, 103, 155),
                                   CRGB(239, 183, 195) };
                                      
const CRGB kGreatReds[]        = { CRGB(190,   0,   0),
                                   CRGB(191,  13,   7),
                                   CRGB(163,   3,  17),
                                   CRGB(163,   8,  13),
                                   CRGB(158,  12, 12) };
                                      
const CRGB kINeedToMeetYou[]   = { CRGB(102, 53,   43),
                                   CRGB(117, 44,   69),
                                   CRGB( 66,  62, 124),
                                   CRGB( 39, 148, 176),
                                   CRGB(109, 244, 227) };
                                      
const CRGB kLavenderLovelies[] = { CRGB( 99,  34, 118),
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
    //DEMO_ANIMATION,
    HEARTBEAT,
    RAINBOW,
    COLOR_CYCLE,
    COLOR_CYCLE_RAINBOW,
    THEATER_RAINBOW,
#if ENABLE_EQUALIZER
    EQUALIZER
#endif
};
#if ENABLE_EQUALIZER
const uint8_t MAX_MODE = EQUALIZER;
#else
const uint8_t MAX_MODE = THEATER_RAINBOW;
#endif

volatile uint8_t currentMode = 0;
volatile bool flashlight = false;
uint8_t lastMode = 0;

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

    attachButtonInterrupt();

    readModeStateFromEEPROM();
}

unsigned long skippedFrames = 0;
int frameDurations[10];
int frameDurationIndex = 0;

void loop() 
{
#if ENABLE_BRIGHTNESS
    checkBrightness();
#endif

    if (flashlight)
    {
        runFlashlight();
    }
    else
    {
        bool modeChanged = false;
        if (currentMode != lastMode)
        {
            modeChanged = true;
            lastMode = currentMode;
            writeModeStateToEEPROM(currentMode);
        }
    
        // Run the animation for this mode  
        switch(currentMode)
        {
            //case DEMO_ANIMATION:      runDemoAnimation(modeChanged);     break;
            case HEARTBEAT:           runHeartbeat(modeChanged);         break;
            case RAINBOW:             runRainbow(modeChanged);           break;
            case COLOR_CYCLE:         runColorCycle(modeChanged);        break;
            case COLOR_CYCLE_RAINBOW: runColorCycleRainbow(modeChanged); break;
            case THEATER_RAINBOW:     runTheaterRainbow(modeChanged);    break;
#if ENABLE_EQUALIZER
            case EQUALIZER:           runEqualizer(modeChanged);         break;
#endif
        
            //default: runDemoAnimation(modeChanged); break;
            default: runHeartbeat(modeChanged); break;
        }
    }
}

void runFlashlight()
{
    FastLED.showColor(CRGB::White);
}

void runDemoAnimation(boolean resetAnimation)
{
    
}

void runHeartbeat(boolean resetAnimation)
{
    heartBeat(81, CRGB::Red, true, CRGB(255, 15, 15));
}

void runRainbow(boolean resetAnimation)
{
    runAnimation(25, resetAnimation, &rainbow, NULL);
}

void runColorCycle(boolean resetAnimation)
{
    colorCycleInnerToOuter(75, kItsAllAboutLove, 5);
}

void runColorCycleRainbow(boolean resetAnimation)
{
    colorCycleInnerToOuterRainbow(75);
}

void runTheaterRainbow(boolean resetAnimation)
{
    runAnimation(30, resetAnimation, &theaterChaseRainbow, NULL);
}

void runAnimation(unsigned long frameDelay, boolean resetAnimation, void (*animation)(uint8_t*, void*), void *params)
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
    animation(&frame, params);

    frame++;

    lastCallTime = currentTime;
}

/*
 * Color Pattern Functions
 */

// Cycle colors, ring by ring, starting from the center
// NEEDS TO BE REWRITTEN USING FRAME ANIMATION
void colorCycleInnerToOuter(double msDelay, const CRGB colors[], uint8_t length)
{            
    for (int i = 0; i < length; i++)
    {
        const CRGB color = colors[i];
        innerToOuter(msDelay, color, false, false);
    }
}

// NEEDS TO BE REWRITTEN USING FRAME ANIMATION
void colorCycleInnerToOuterRainbow(double msDelay)
{
    for (int i = 0; i < 256; i+=50)
        {
            const CRGB color = colorWheel(i);
            innerToOuter(msDelay, color, false, false);
        }
}

// Quick double beats like a heart, with msDelay between beat groups
// NEEDS TO BE REWRITTEN USING FRAME ANIMATION
void heartBeat(double bpm, CRGB color1, boolean doubleBeat, CRGB color2)
{
    double beatsPerSecond = bpm / 60.0;
    double iterationLength = 1 / beatsPerSecond;
    double msDelay = (iterationLength * 1000) - (35 * 5) - (doubleBeat ? 50 - (25 * 3) : 0);
  
    innerToOuter(35, color1, true, false);
  
    if (doubleBeat)
    {
        delay(50);
        innerToOuter(25, color2, true, true);
    } 
  
    delay(msDelay);
}

// Fills the heart with color, ring by ring, starting from the inner ring
// NEEDS TO BE REWRITTEN USING FRAME ANIMATION
void innerToOuter(uint16_t msDelay, CRGB color, boolean reverse, boolean skipOuter)
{
    for (uint8_t i = 0; i < kInnerLedsLength; i++)
    {
        const uint8_t pixel = kInnerLeds[i];
        leds[pixel] = color;
    } 
    FastLED.show();
    delay(msDelay);
    
    for (uint8_t i = 0; i < kMiddleLedsLength; i++)
    {
        const uint8_t pixel = kMiddleLeds[i];
        leds[pixel] = color;
    }
    FastLED.show(); 
    delay(msDelay);
    
    if (!skipOuter)
    {
        for (uint8_t i = 0; i < kOuterLedsLength; i++)
        {
            const uint8_t pixel = kOuterLeds[i];
            leds[pixel] = color;
        }
        FastLED.show();
    }
    
    if (reverse)
    {
        delay(msDelay);
        outerToInner(msDelay, CRGB::Black, false, skipOuter);
    }
}

// Fills the heart with color, ring by ring, starting from the outer ring
// NEEDS TO BE REWRITTEN USING FRAME ANIMATION
void outerToInner(uint16_t msDelay, CRGB color, boolean reverse, boolean skipOuter)
{
    if (!skipOuter)
    {
        for (uint8_t i = 0; i < kOuterLedsLength; i++)
        {
            const uint8_t pixel = kOuterLeds[i];
            leds[pixel] = color;
        }
        FastLED.show(); 
        delay(msDelay);
    }
    
    for (uint8_t i = 0; i < kMiddleLedsLength; i++)
    {
        const uint8_t pixel = kMiddleLeds[i];
        leds[pixel] = color;
    }
    FastLED.show(); 
    delay(msDelay);
    
    for (uint8_t i = 0; i < kInnerLedsLength; i++)
    {
        const uint8_t pixel = kInnerLeds[i];
        leds[pixel] = color;
    }
    FastLED.show();
    
    if (reverse)
    {
        innerToOuter(msDelay, CRGB::Black, false, skipOuter);
    }
}

// Fades through the colors (average frame duration 26 ms)
void rainbow(uint8_t *frame, void *params)
{
    for (int i = 0; i < kNumLeds; i++)
    {
        leds[i] = colorWheel((i + *frame) & 255);
    }
    
    FastLED.show();
}

// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t *frame, void *params)
{
    for (int i = 0; i < kNumLeds; i++) 
    {
        leds[i] = colorWheel(((i * 256 / kNumLeds) + *frame) & 255);
    }

    FastLED.show();
}

// Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t *frame, void *params)
{
    const uint8_t offset = *frame % 3;
    const boolean on = *frame % 2;

    for (int i = 0; i < kNumLeds; i = i + 3) 
    {
        // Turn every third pixel on or off
        CRGB color = on ? colorWheel((i + *frame) % 255) : CRGB::Black;
        leds[i + offset] = color;    
    }
    
    FastLED.show();
}

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

    currentMode = value;
}

void writeModeStateToEEPROM(uint8_t state)
{
    EEPROM.write(kModeAddress, state);
}

/*
 * Button Handling
 */

void attachButtonInterrupt()
{
    pinMode(kButtonInterruptPin, INPUT_PULLUP);
    attachInterrupt(kButtonInterrupt, buttonPressed, CHANGE);
}

void buttonPressed()
{
    const unsigned long longPressDuration = 500; // milliseconds

    static unsigned long lastLowInterruptTime = 0;

    bool value = digitalRead(kButtonInterruptPin);
    unsigned long interruptTime = millis();
    unsigned long pressedTime = interruptTime - lastLowInterruptTime;

    // Handle the button press
    if (value == HIGH)
    {
        if (pressedTime > longPressDuration)
        {
            flashlight = !flashlight;
        }
        else
        {
            // Go to the next mode
            uint8_t newMode = currentMode + 1;
    
            // Cycle back to the first mode if necessary
            if (newMode > MAX_MODE)
            {
                newMode = 0;
            }
      
            currentMode = newMode;
        }
    }

    if (value == LOW)
    {
        lastLowInterruptTime = interruptTime;
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
  
//    // Clip the values to 1 - 650
    const int16_t minSensorValue = 1;
    const int16_t maxSensorValue = 1000;

    // Invert the brightness value as the potentiometer is upside down
    sensorValueNew = maxSensorValue - constrain(sensorValueNew, minSensorValue, maxSensorValue);

    // Remove jitter
    const int16_t acceptedRange = 10; 
    int16_t range = sensorValueNew - sensorValue;
    range = abs(range);

#if ENABLE_SERIAL
    static uint8_t serialCounter = 0;
    if (serialCounter == 10)
    {
        Serial.print("sensor value old: ");
        Serial.print(sensorValue);
        Serial.print(" sensor value new: ");
        Serial.print(sensorValueNew);
        Serial.print(" range: ");
        Serial.print(range);
    }
#endif

    bool closeToBottom = sensorValue != minSensorValue && sensorValue < range;
    bool closeToTop = sensorValue != maxSensorValue && sensorValue < maxSensorValue - range;
    if (brightness == 0 || closeToBottom || closeToTop || range > acceptedRange)
    {
        // Map the sensor values into the "night to day" brightness range
        uint8_t brightnessNew = map(sensorValueNew, minSensorValue, maxSensorValue, 10, 255);

#if ENABLE_SERIAL
        if (serialCounter == 10)
        {
            Serial.print(" old value: ");
            Serial.print(brightness);
            Serial.print(" new value: ");
            Serial.println(brightnessNew);
        }

        serialCounter++;
        if (serialCounter > 10)
        {
            serialCounter = 0;
        }
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
