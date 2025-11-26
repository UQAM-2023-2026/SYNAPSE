/*-------------------Libraries----------------------*/
#include "StripsAnimation.h"

#include <Arduino.h>
#include <FastLED.h>

#include <RhizomeStateAndID.h>
static RhizomeStateAndID *pRhizome = nullptr; // pointer to external rhizome object

// ==========================================================
// Gestion des leds - Projet Synapse
// ==========================================================

/* LED strip configuration */
#define LED_PIN     17
#define NUM_LEDS    60
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
/*--------------------------------------------------*/

CRGB leds[NUM_LEDS]; // LED array

// animation state
int state = 0;

/*------------------Leds animation variables-------------------- */
static uint8_t phase = 0;           // used with sin8 for smooth fade
static uint8_t hueOffset = 0;       // small offset for strip gradient

static uint8_t currentEnergy = 0; // 0..100
static uint8_t nbConnected = 1; // number of connected rhizomes

// connexion one-shot state
static bool connexionActive = false;
static unsigned long connexionStart = 0;
const unsigned long connexionDuration = 600; // ms

// generating animation state
static uint16_t genPos = 0;
static unsigned long genLastMove = 0;
/*--------------------------------------------------*/



void SetupStrips(RhizomeStateAndID &rh, uint8_t brightness) {
    pRhizome = &rh;
    
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.clear();
    FastLED.show();
    FastLED.setBrightness(brightness);
}

/*---------------Setters----------------------*/
void LedsSetState() {
  if (pRhizome) state = pRhizome->getState();
  else state = 0;
}
void LedsSetEnergy() {
  if (pRhizome) currentEnergy = pRhizome->getEnergy();
  else currentEnergy = 0;
}
void LedsSetNbConnected() {
  if (pRhizome) nbConnected = pRhizome->getCount();
  else nbConnected = 1;
}
/*--------------------------------------------------*/



/*----------------IDLE ANIMATION-----------------------------------*/
/* idle animation:
   - smooth fade between 20% and 100% brightness using a sine lookup (sin8)
   - hue is a smooth gradient derived from energy (red->yellow->green)
   - blink/fade speed depends on energy (low energy = slow, high = fast)
   - non-blocking (no delays); call often from StripLoop()
*/
// Helper: map energy (0-100) to hue 0..85 (red -> yellow -> green)
static uint8_t hueFromEnergy(uint8_t e) {
  return map(e, 0, 100, 0, 85);
}

void LedsIdle() {
  // compute speed from energy: lower energy -> slower phase increment
  // map energy 0..100 => speed 1..20 (phase increment per frame)
  uint8_t speed = map(currentEnergy, 100, 0, 1, 1.5);
  phase += speed; // advances waveform

  // sin8 returns 0..255; map to brightness 20%..100% (51..255)
  uint8_t sine = sin8(phase);
  uint8_t bright = map(sine, 0, 255, 51, 255); // 20% -> 100%

  // base hue controlled by energy (smooth from red to green)
  uint8_t baseHue = hueFromEnergy(currentEnergy);
  uint8_t sat = 200; // fairly saturated colors

  // create slight gradient along strip
  for (int i = 0; i < NUM_LEDS; ++i) {
    // small hue shift across LEDs for nicer look
    uint8_t hue = baseHue + (uint8_t)map(i, 0, NUM_LEDS - 1, 0, 20);
    leds[i] = CHSV(hue, sat, bright);
  }

  // optionally introduce a gentle moving tint so it's not static
  hueOffset++; // gradual change over time
  // rotate palette slightly for motion (cheap)
  if ((hueOffset & 0x07) == 0) {
    // shift hues a tiny bit every few frames
    for (int i = 0; i < NUM_LEDS; ++i) {
      leds[i].nscale8(250); // keep overall responsiveness; tiny dim to blend frames
    }
  }
  FastLED.show();
}
/*--------------------------------------------------*/


/*----------------CONNEXION ANIMATION-----------------------------------*/

// quick one-shot white burst -> fade to black, triggered when state==1
void LedsConnection() {
  for(int i = 0; i < NUM_LEDS; ++i) {
    leds[i] = CRGB::White;
  }
  FastLED.show();
}
/*--------------------------------------------------*/


/*----------------GENERATING ANIMATION-----------------------------------*/
// orange left->right pulse; speed depends on nbConnected
void LedsGenerating() {
  uint8_t baseHue = hueFromEnergy(currentEnergy);
  leds[0] = CHSV(baseHue, random8(), random8(100, 255));

  EVERY_N_MILLISECONDS(100) {
    for(int i = NUM_LEDS - 1; i > 0; i--){
      leds[i] = leds[i - 1];
    }
  }
  FastLED.show();
}
/*--------------------------------------------------*/


/*----------------GIVING TO NODE ANIMATION-----------------------------------*/
void LedsGivingToNode() {
    leds[0] = CHSV(160, random8(), random8(100,255));

    EVERY_N_MILLISECONDS(100) {
      for(int i = NUM_LEDS - 1; i > 0; i--){
        leds[i] = leds[i - 1];
      }
    }
  FastLED.show();
}
/*--------------------------------------------------*/


// Main loop for strip animation
void StripLoop() {
    switch (state) {
        case 0:
            LedsIdle();
            break;
        case 1:
            LedsConnection();
            break;
        case 2:
            LedsGenerating();
            break;
        case 3:
            LedsGivingToNode();
            break;
        default:
            LedsIdle();
            break;
    }
    LedsSetState();
    LedsSetEnergy();
    LedsSetNbConnected();
}