#include <Arduino.h>
#include <FastLED.h>

#include "data.h"

// pin numbers, the blue colored ones in doc/attiny85-guide-pinout.png
#define LED_PIN 3
// #define IR_RX_PIN 4  // defined in "data.h"
// #define SCL 2
// #define SDA 0

#define NUM_LEDS 10
#define BRIGHTNESS 255
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define FRAMES_PER_SECOND 60
// CRGB leds[NUM_LEDS];
CRGBArray<NUM_LEDS> leds;

CRGBSet eyes(leds(4, 5));
CRGBSet logo(leds(8, 9));

void FillLEDsFromPaletteColors(CRGBPalette16 currentPalette, TBlendType currentBlending);

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
void heart_beat_eyes_blue();
void heart_beat_eyes_mono();
void heart_beat_logo();

void heart_beat(CRGBSet lubs, CRGBSet dubs, uint8_t hue, bool change_color);

void handle_ir_packet(IrDataPacket packet);

void nextPattern();

void setup()
{
    // delay( 3000 ); // power-up safety delay // does not seem necessary, start showing patterns sooner.
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);

    FastLED.clear(true);

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
    // oceanColors,
    // forestColors,
    // christmasSparklesRG,
    // christmasSparklesBP,
    // heart_beat_all_reverse,
    // heart_beat_eyes_blue, 
    // heart_beat_eyes_mono, 
    // heart_beat_logo
};

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current

//---------------------------------------------------------------
void loop()
{
    // light effect when receiving blaster shot
    handle_ir_packet(Data.readIr());

    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
    
    FastLED.show();
    // slows the framerate to a modest value
    delay(1000 / FRAMES_PER_SECOND);  // use delay, to give some time to process ir interrupts
    // FastLED.delay(1000 / FRAMES_PER_SECOND); 

    // do some periodic updates
    EVERY_N_SECONDS(20)
    {
        nextPattern();
        FastLED.clear();
    } // change patterns periodically
}

//---------------------------------------------------------------
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
void nextPattern()
{
    // add one to the current pattern number, and wrap around at the end
    gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void handle_ir_packet(IrDataPacket packet)
{
    if (packet.raw != 0 && packet.action == eActionDamage)
    {
        CRGB color = CRGB::Black;

        switch (packet.team)
        {
        case eTeamRex:
            color = CRGB::Red;
            break;

        case eTeamBuzz:
            color = CRGB::Blue;
            break;

        case eTeamGiggle:
            color = CRGB::Green;
            break;

        case eTeamYellow:
            color = CRGB::Yellow;
            break;

        case eTeamMagenta:
            color = CRGB::Magenta;
            break;

        case eTeamCyan:
            color = CRGB::Cyan;
            break;

        case eTeamWhite:
            color = CRGB::White;
            break;

        default:
            color = CRGB::Black;
            break;
        }

        leds = color;
        while (leds[0].getAverageLight() != 0)
        {
            FastLED.show();
            delay(1000/FRAMES_PER_SECOND);
            leds.fadeToBlackBy(10);
        }
        delay(100);
        Data.readIr(); // clear buffer
        FastLED.clear();
    }
}

void FillLEDsFromPaletteColors(CRGBPalette16 currentPalette, TBlendType currentBlending)
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

void rainbowColors() { FillLEDsFromPaletteColors(RainbowColors_p, LINEARBLEND); }
void partyColors()   { FillLEDsFromPaletteColors(PartyColors_p,   LINEARBLEND); }
void oceanColors()   { FillLEDsFromPaletteColors(OceanColors_p,   LINEARBLEND); }
void forestColors()  { FillLEDsFromPaletteColors(ForestColors_p,  LINEARBLEND); }



//===============================================================
// The different patterns to choose from...
//===============================================================
//---------------------------------------------------------------
uint8_t ledsData[NUM_LEDS][4];    // for Sparkles, array to store RGB data and an extra value
uint8_t pick;                     // for Sparkles, stores a temporary pixel number

void christmasSparkles()
{
    //"Background" color for non-sparkling pixels.
    CRGB sparkleBgColor = CHSV(50, 30, 40); // dim white
    // CRGB sparkleBgColor = CHSV(96, 200, 30);  // dim green

    EVERY_N_MILLISECONDS(40)
    {
        if (random8() < 60)
        { // How much to sparkle!  Higher number is more.
            pick = random8(NUM_LEDS);
            if (ledsData[pick][3] == 0)
            {
                ledsData[pick][3] = 35; // Used to tag pixel as sparkling
                uint8_t randomPick = random8(5);
                if (randomPick == 0)
                {
                    ledsData[pick][0] = 178; // sparkle hue (blue)
                    ledsData[pick][1] = 244; // sparkle saturation
                    ledsData[pick][2] = 210; // sparkle value
                }
                if (randomPick == 1)
                {
                    ledsData[pick][0] = 10;  // sparkle hue (red)
                    ledsData[pick][1] = 255; // sparkle saturation
                    ledsData[pick][2] = 240; // sparkle value
                }
                if (randomPick == 2)
                {
                    ledsData[pick][0] = 0;   // sparkle hue (white-ish)
                    ledsData[pick][1] = 25;  // sparkle saturation
                    ledsData[pick][2] = 255; // sparkle value
                }
                if (randomPick == 3)
                {
                    ledsData[pick][0] = 35;  // sparkle hue (orange)
                    ledsData[pick][1] = 235; // sparkle saturation
                    ledsData[pick][2] = 245; // sparkle value
                }
                if (randomPick == 4)
                {
                    ledsData[pick][0] = 190; // sparkle hue (purple)
                    ledsData[pick][1] = 255; // sparkle saturation
                    ledsData[pick][2] = 238; // sparkle value
                }
                leds[pick] = CHSV(ledsData[pick][0], ledsData[pick][1], ledsData[pick][2]);
            }
        }
        for (uint8_t i = 0; i < NUM_LEDS; i++)
        {
            if (ledsData[i][3] == 0)
            { // if not sparkling, set to "back ground" color
                leds[i] = sparkleBgColor;
            }
            else
            {
                CHSV hsv = rgb2hsv_approximate(leds[i]);                   // Used to get approximate Hue
                EVERY_N_MILLISECONDS(38) { ledsData[i][0] = hsv.hue - 1; } // slightly shift hue
                ledsData[i][2] = scale8(ledsData[i][2], 245);              // slowly darken
                leds[i] = CHSV(ledsData[i][0], ledsData[i][1], ledsData[i][2]);
                ledsData[i][3] = ledsData[i][3] - 1; // countdown sparkle tag
            }
        }
    }
} // end christmasSparkles

//---------------------------------------------------------------
void christmasSparklesRG()
{ // Red and Green only
    //"Background" color for non-sparkling pixels.  Can be set to black for no bg color.
    CRGB sparkleBgColor = CHSV(0, 0, 0); // black
    // CRGB sparkleBgColor = CHSV(50, 30, 30);  // dim white

    EVERY_N_MILLISECONDS(40)
    {
        if (random8() < 110)
        { // How much to sparkle!  Higher number is more.
            pick = random8(NUM_LEDS);
            if (ledsData[pick][3] == 0)
            {
                ledsData[pick][3] = 65; // Used to tag pixel as sparkling
                uint8_t randomPick = random8(2);
                if (randomPick == 0)
                {
                    ledsData[pick][0] = 16;  // sparkle hue (red)
                    ledsData[pick][1] = 253; // sparkle saturation
                    ledsData[pick][2] = 242; // sparkle value
                }
                if (randomPick == 1)
                {
                    ledsData[pick][0] = 96;  // sparkle hue (green)
                    ledsData[pick][1] = 230; // sparkle saturation
                    ledsData[pick][2] = 255; // sparkle value
                }
                leds[pick] = CHSV(ledsData[pick][0], ledsData[pick][1], ledsData[pick][2]);
            }
        }
        for (uint8_t i = 0; i < NUM_LEDS; i++)
        {
            if (ledsData[i][3] == 0)
            { // if not sparkling, set to "back ground" color
                leds[i] = sparkleBgColor;
            }
            else
            {
                CHSV hsv = rgb2hsv_approximate(leds[i]);                   // Used to get approximate Hue
                EVERY_N_MILLISECONDS(50) { ledsData[i][0] = hsv.hue - 1; } // slightly shift hue
                ledsData[i][2] = scale8(ledsData[i][2], 253);              // slowly darken
                leds[i] = CHSV(ledsData[i][0], ledsData[i][1], ledsData[i][2]);
                ledsData[i][3] = ledsData[i][3] - 1; // countdown sparkle tag
            }
        }
    }
} // end christmasSparklesRG

//---------------------------------------------------------------
void christmasSparklesBP()
{ // Blues and Purple only
    //"Background" color for non-sparkling pixels.
    CRGB sparkleBgColor = CHSV(96, 185, 30); // green

    EVERY_N_MILLISECONDS(40)
    {
        if (random8() < 170)
        { // How much to sparkle!  Higher number is more.
            pick = random8(NUM_LEDS);
            if (ledsData[pick][3] == 0)
            {
                ledsData[pick][3] = 20; // Used to tag pixel as sparkling
                uint8_t randomPick = random8(3);
                if (randomPick == 0)
                {
                    ledsData[pick][0] = 165; // sparkle hue (blue)
                    ledsData[pick][1] = 180; // sparkle saturation
                    ledsData[pick][2] = 230; // sparkle value
                }
                if (randomPick == 1)
                {
                    ledsData[pick][0] = 200; // sparkle hue (pink-light-purple)
                    ledsData[pick][1] = 170; // sparkle saturation
                    ledsData[pick][2] = 240; // sparkle value
                }
                if (randomPick == 2)
                {
                    ledsData[pick][0] = 130; // sparkle hue (light blue)
                    ledsData[pick][1] = 200; // sparkle saturation
                    ledsData[pick][2] = 255; // sparkle value
                }
                leds[pick] = CHSV(ledsData[pick][0], ledsData[pick][1], ledsData[pick][2]);
            }
        }
        for (uint8_t i = 0; i < NUM_LEDS; i++)
        {
            if (ledsData[i][3] == 0)
            { // if not sparkling, set to "back ground" color
                leds[i] = sparkleBgColor;
            }
            else
            {
                CHSV hsv = rgb2hsv_approximate(leds[i]);                   // Used to get approximate Hue
                EVERY_N_MILLISECONDS(20) { ledsData[i][0] = hsv.hue - 1; } // slightly shift hue
                ledsData[i][2] = scale8(ledsData[i][2], 242);              // slowly darken
                leds[i] = CHSV(ledsData[i][0], ledsData[i][1], ledsData[i][2]);
                ledsData[i][3] = ledsData[i][3] - 1; // countdown sparkle tag
            }
        }
    }
} // end christmasSparklesBP

void heart_beat_all()         { heart_beat(leds(0, 4), leds(5, 9), HUE_RED , true) ; }
void heart_beat_all_reverse() { heart_beat(leds(9, 5), leds(4, 0), HUE_RED , true ); }
void heart_beat_eyes_red()    { heart_beat(leds(4, 4), leds(5, 5), HUE_RED , false); }
void heart_beat_eyes_blue()   { heart_beat(leds(4, 4), leds(5, 5), HUE_BLUE, false); }
void heart_beat_eyes_mono()   { heart_beat(leds(4, 5), leds(4, 5), HUE_RED , false); }
void heart_beat_logo()        { heart_beat(leds(8, 8), leds(9, 9), HUE_RED , true ); }

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