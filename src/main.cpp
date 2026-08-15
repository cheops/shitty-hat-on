#include <Arduino.h>
#include <FastLED.h>

#include "data.h"

// pin numbers, the blue colored ones in doc/attiny85-guide-pinout.png
#define LED_PIN 3
// #define IR_RX_PIN 4  // defined in "data.h"
#define SCL 2
#define SDA 0

#define NUM_LEDS 10
#define BRIGHTNESS 255
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define FRAMES_PER_SECOND 60
// CRGB leds[NUM_LEDS];
CRGBArray<NUM_LEDS> leds;

void FillLEDsFromPaletteColors(const TProgmemRGBPalette16 &currentPalette, TBlendType currentBlending);

void rainbowColors();
void partyColors();
void oceanColors();
void forestColors();

void christmasSparkles();
void christmasSparklesRG();
void christmasSparklesBP();

void heart_beat_all();
void heart_beat_all_reverse();
void heart_beat_eyes_red();
void heart_beat_logo();
void heart_beat_eyes_blue();
void heart_beat_eyes_mono();

void sinelon();
void side_flip_flop();

void heart_beat(CRGBSet lubs, CRGBSet dubs, uint8_t hue, bool change_color);

void handle_ir_packet(IrDataPacket packet);

void nextPattern();

void setup()
{
    // Force clock prescaler to 1 (8MHz) even if CKDIV8 fuse is set
    CLKPR = (1 << CLKPCE);
    CLKPR = 0;

    PORTB |= (1 << PB0) | (1 << PB2); // Enables pull-up on PB0 and PB2

    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
#if BRIGHTNESS != 255
    FastLED.setBrightness(BRIGHTNESS); // default scale is already 255, skip when full
#endif

    // Enable global interrupts
    sei();
}

//---------------------------------------------------------------
// List of patterns to cycle through.  Each is defined as a separate function.
typedef void (*SimplePatternList[])();

SimplePatternList gPatterns = 
{
    rainbowColors,
    heart_beat_eyes_red,
    christmasSparkles,
    partyColors,
    heart_beat_all,
    oceanColors,
    forestColors,
    christmasSparklesRG,
    christmasSparklesBP,
    heart_beat_all_reverse,
    heart_beat_logo,
    heart_beat_eyes_blue,
    heart_beat_eyes_mono,
    sinelon,
    side_flip_flop,
};

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

// Push the leds[] buffer to the strip with the IR pin-change interrupt disabled
// during transmission (an ISR mid-transmission corrupts WS2812 timing).
static void showSafe()
{
    PCMSK &= ~0b00010000; // disable PCINT4
    FastLED.show();
    PCMSK |= 0b00010000;  // re-enable PCINT4
}

//---------------------------------------------------------------
void loop()
{
    // light effect when receiving blaster shot
    handle_ir_packet(Data.readIr());

    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
    
    showSafe();
    // slows the framerate to a modest value
    delay(1000 / FRAMES_PER_SECOND);  // use delay, to give some time to process ir interrupts
    // FastLED.delay(1000 / FRAMES_PER_SECOND); 

    // change patterns periodically (frame-counted to avoid EVERY_N_SECONDS' 32-bit
    // divide-by-1000, which is costly in flash on AVR)
    static uint16_t patternFrames = 0;
    if (++patternFrames >= FRAMES_PER_SECOND * 20) // ~20 s
    {
        patternFrames = 0;
        nextPattern();
        FastLED.clear();
    }
}

//---------------------------------------------------------------
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
void nextPattern()
{
    // add one to the current pattern number, and wrap around at the end
    if (++gCurrentPatternNumber >= ARRAY_SIZE(gPatterns))
        gCurrentPatternNumber = 0;
}

void handle_ir_packet(IrDataPacket packet)
{
    if (packet.raw != 0 && packet.action == eActionDamage)
    {
        // Map team bits to color: R=red, G=green, B=blue (matches team bit positions)
        CRGB color = CRGB(
            (packet.team & eTeamRex)    ? 255 : 0,
            (packet.team & eTeamGiggle) ? 255 : 0,
            (packet.team & eTeamBuzz)   ? 255 : 0
        );

        leds = color;
        showSafe();

        while (leds[0].getAverageLight() != 0)
        {
            delay(1000/FRAMES_PER_SECOND);
            leds.fadeToBlackBy(10);
            showSafe();
        }
        Data.readIr(); // clear buffer
        FastLED.clear();
    }
}

// Read the palette straight from PROGMEM. Taking a CRGBPalette16 here (or a
// local copy in palettePattern) would put a 48-byte palette on the stack, and
// with only ~115 bytes of free RAM the IR pin-change ISR nesting on top of it
// overflows the stack into leds[], corrupting LEDs and causing flicker.
void FillLEDsFromPaletteColors(const TProgmemRGBPalette16 &currentPalette, TBlendType currentBlending)
{

    static uint8_t startIndex = 0;
    startIndex = startIndex + 1; /* motion speed */

    uint8_t index = startIndex;

    for (uint8_t i = 0; i < NUM_LEDS; ++i)
    {
        leds[i] = ColorFromPalette(currentPalette, index, BRIGHTNESS, currentBlending);
        index += 3;
    }
}

void palettePattern(uint8_t index);

void rainbowColors() { palettePattern(0); }
void partyColors()   { palettePattern(1); }
void oceanColors()   { palettePattern(2); }
void forestColors()  { palettePattern(3); }

const TProgmemRGBPalette16* const paletteList[] PROGMEM = {
    &RainbowColors_p,
    &PartyColors_p,
    &OceanColors_p,
    &ForestColors_p,
};

void palettePattern(uint8_t index)
{
    const TProgmemRGBPalette16* pal_ptr = (const TProgmemRGBPalette16*)pgm_read_word(&paletteList[index]);
    FillLEDsFromPaletteColors(*pal_ptr, LINEARBLEND);
}



//===============================================================
// The different patterns to choose from...
//===============================================================
//---------------------------------------------------------------
uint8_t ledsData[NUM_LEDS][4];    // for Sparkles, array to store RGB data and an extra value
uint8_t pick;                     // for Sparkles, stores a temporary pixel number

// Sparkle color definitions stored in PROGMEM {hue, saturation, value}
// All sparkle colors in one contiguous PROGMEM array
const uint8_t PROGMEM sparkleAllColors[][3] = {
    // Christmas multi-color (index 0-4)
    {178, 244, 210}, // blue
    { 10, 255, 240}, // red
    {  0,  25, 255}, // white-ish
    { 35, 235, 245}, // orange
    {190, 255, 238}, // purple
    // Red and Green (index 5-6)
    { 16, 253, 242}, // red
    { 96, 230, 255}, // green
    // Blues and Purple (index 7-9)
    {165, 180, 230}, // blue
    {200, 170, 240}, // pink-light-purple
    {130, 200, 255}, // light blue
};

// Sparkle preset config: bgH, bgS, bgV, chance, life, fadeRate, colorOffset, numColors
struct SparklePreset { uint8_t bgH, bgS, bgV, chance, life, fadeRate, colorOfs, numColors; };
const SparklePreset PROGMEM sparklePresets[] = {
    { 50,  30,  40,  60, 35, 245, 0, 5}, // Christmas
    {  0,   0,   0, 110, 65, 253, 5, 2}, // Red-Green
    { 96, 185,  30, 170, 20, 242, 7, 3}, // Blues-Purple
};

void sparkles(const SparklePreset* preset_p)
{
    SparklePreset p;
    memcpy_P(&p, preset_p, sizeof(p));

    EVERY_N_MILLISECONDS(40)
    {
        if (random8() < p.chance)
        {
            pick = random8(NUM_LEDS);
            if (ledsData[pick][3] == 0)
            {
                ledsData[pick][3] = p.life;
                uint8_t idx = p.colorOfs + random8(p.numColors);
                ledsData[pick][0] = pgm_read_byte(&sparkleAllColors[idx][0]);
                ledsData[pick][1] = pgm_read_byte(&sparkleAllColors[idx][1]);
                ledsData[pick][2] = pgm_read_byte(&sparkleAllColors[idx][2]);
                leds[pick] = CHSV(ledsData[pick][0], ledsData[pick][1], ledsData[pick][2]);
            }
        }
        for (uint8_t i = 0; i < NUM_LEDS; i++)
        {
            if (ledsData[i][3] == 0)
            {
                leds[i] = CHSV(p.bgH, p.bgS, p.bgV);
            }
            else
            {
                EVERY_N_MILLISECONDS(38) { ledsData[i][0] = ledsData[i][0] - 1; }
                ledsData[i][2] = scale8(ledsData[i][2], p.fadeRate);
                leds[i] = CHSV(ledsData[i][0], ledsData[i][1], ledsData[i][2]);
                ledsData[i][3] = ledsData[i][3] - 1;
            }
        }
    }
}

void christmasSparkles()   { sparkles(&sparklePresets[0]); }
void christmasSparklesRG() { sparkles(&sparklePresets[1]); }
void christmasSparklesBP() { sparkles(&sparklePresets[2]); }

void heart_beat_all()         { heart_beat(leds(0, 4), leds(5, 9), HUE_RED , true) ; }
void heart_beat_all_reverse() { heart_beat(leds(9, 5), leds(4, 0), HUE_RED , true ); }
void heart_beat_eyes_red()    { heart_beat(leds(4, 4), leds(5, 5), HUE_RED , false); }
void heart_beat_logo()        { heart_beat(leds(8, 8), leds(9, 9), HUE_RED , true ); }
void heart_beat_eyes_blue()   { heart_beat(leds(4, 4), leds(5, 5), HUE_BLUE, false); }
void heart_beat_eyes_mono()   { heart_beat(leds(4, 5), leds(4, 5), HUE_RED , false); }

// A colored dot sweeping back and forth with a fading trail.
// Frame-counted (loop runs at ~60fps): no EVERY_N_MILLISECONDS objects/guards and
// no millis() math, so it stays cheap on both RAM and flash.
void sinelon()
{
    static uint8_t hue = 0, pos = 0, tick = 0;
    static int8_t dir = 1;
    leds.fadeToBlackBy(20);
    if (++tick >= 4) // advance ~every 4 frames (~64 ms)
    {
        tick = 0;
        hue += 3;
        pos += dir;
        if (pos == 0 || pos == NUM_LEDS - 1)
            dir = -dir;
    }
    leds[pos] += CHSV(hue, 255, 192);
}

// Alternate the two sides of the hat (0-4 vs 5-9), crossfading, hue shifts per flip.
void side_flip_flop()
{
    static uint8_t tick = 0, side = 0, hue = 0;
    leds.fadeToBlackBy(40);
    CRGB c = CHSV(hue, 255, 255);
    uint8_t start = side ? 5 : 0;
    for (uint8_t i = 0; i < 5; i++)
        leds[start + i] = c;
    if (++tick >= 20) // flip ~every 20 frames (~0.33 s)
    {
        tick = 0;
        side ^= 1;
        hue += 32;
    }
}

//---------------------------------------------------------------
// Heart beat function
#define LUB_TIME 1100 // Time between main lubs [milliseconds]
#define DUB_DELAY 120 // Short delay for when secondary dub starts [milliseconds]

void heart_beat(CRGBSet lubs, CRGBSet dubs, uint8_t hue, bool change_color)
{

    static boolean lubRunning = 0;                  // Is lub running? [1=true/0=false]
    static boolean dubRunning = 0;                  // Is dub running? [1=true/0=false]
    static boolean dubTrigger = 0;                  // Is dub triggered? [1=true/0=false]
    static CEveryNMilliseconds dubTimer(DUB_DELAY); // Create timer for dub delay
    static uint8_t lubValue, dubValue;
    static uint8_t heart_hue = HUE_RED;
    if (!change_color)
    {
        heart_hue = hue;
    }

    // CRGBSet lubs(leds(4, 4)); // Pixels for lub (first) part of heart beat
    // CRGBSet dubs(leds(5, 5)); // Pixels for dub (second) part of heart beat

    //---------------------------------
    // Regularly fade out the heart beat pixels
    EVERY_N_MILLISECONDS(5)
    {                           // How often to do the fade
        lubs.fadeToBlackBy(21); // Amount to fade [use smaller number for slower fade]
        dubs.fadeToBlackBy(18);
    }

    //---------------------------------
    // Timing of heart beat
    EVERY_N_MILLISECONDS(LUB_TIME)
    {
        lubRunning = 1;
        lubValue = 20; // Starting value when ramping up [Use 1 or greater]
        dubTrigger = 1;
        dubTimer.reset(); // Reset dub timer
    }
    if (dubTrigger && dubTimer)
    {
        dubRunning = 1;
        dubValue = 1;   // Starting value when ramping up [Use 1 or greater]
        dubTrigger = 0; // Reset trigger
    }

    //---------------------------------
    // Assign pixel data
    if (lubRunning)
    {
        EVERY_N_MILLISECONDS(7)
        {
            lubValue = brighten8_video(lubValue);
        }
        lubs = CHSV(heart_hue, 255, lubValue);
        if (lubValue >= 250)
        {
            lubRunning = 0; // Reset
        }
    }

    if (dubRunning)
    {
        EVERY_N_MILLISECONDS(7)
        {
            dubValue = brighten8_video(dubValue);
        }

        dubs = CHSV(heart_hue, 255, dubValue);
        if (dubValue >= 250)
        {
            dubRunning = 0; // Reset
        }
    }

    //---------------------------------
    // Just for fun... Uncomment for rainbow heart beats!
    if (change_color)
    {
        EVERY_N_MILLISECONDS(DUB_DELAY)
        {
            heart_hue = heart_hue + random8(32, 65);
        }
    }

} // end heart_beat

//---------------------------------------------------------------