#include "StripsAnimation.h"

#include <FastLED.h>
#include <Arduino.h>

#define LED_PIN     18
#define NUM_LEDS    30
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

void SetupStrips(uint8_t brightness) {
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.clear();
    FastLED.show();
    FastLED.setBrightness(brightness);
}

/*void anim1() {
    if (!fadingOut) {
    // Allume une LED de plus à chaque cycle
    leds[currentLED] = CRGB(0, 100, 255); // couleur cyan
    FastLED.show();
    currentLED++;

    if (currentLED >= NUM_LEDS) {
      fadingOut = true;
      delay(500); // petit délai avant le fade-out
    }
    delay(10); // vitesse de la "chenille"
  } 
  else {
    // Fade global
    for (int i = 0; i < NUM_LEDS; i++) {
      leds[i].fadeToBlackBy(50); // valeur entre 1 et 50 pour régler la vitesse
    }
    FastLED.show();

    // Vérifie si tout est noir
    bool allBlack = true;
    for (int i = 0; i < NUM_LEDS; i++) {
      if (leds[i]) { // tant qu’il reste de la lumière
        allBlack = false;
        break;
      }
    }

    if (allBlack) {
      fadingOut = false;
      currentLED = 0;
      FastLED.clear();
    }

    delay(50); // vitesse du fade
  }
}*/

void StripLoop(bool state) {
  
  if(state){
    leds[0] = CHSV(160, random8(), random8(100,255));

    EVERY_N_MILLISECONDS(100) {
      for(int i = NUM_LEDS - 1; i > 0; i--){
        leds[i] = leds[i - 1];
      }
    }
  } else {
    // Quand le bouton n'est pas pressé, on diminue progressivement chaque LED
    fadeToBlackBy(leds, NUM_LEDS, 1);  // 10 = vitesse du fade (1-255)
  }

 
  FastLED.show();
}
