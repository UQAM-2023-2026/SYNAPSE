// ANIMATION, TRANSFERT D'ÉNERGIE À UN NOEUD

#include <Plaquette.h>
#include <PqLEDStrip.h>

Wave sawtooth(TRIANGLE, 1, 1);

// ONE LED
const int LED_PIN = 17;
const int NUM_LEDS = 30;

DigitalOut led(13);

LEDStripWS281X<LED_PIN, GRB, NUM_LEDS> strip;

DEFINE_GRADIENT_PALETTE(customGradient){
  0,   0,   0,   0,
  255, 100,   17, 177,
  0,   0,   80,  0,
};

CRGBPalette16 customPalette = customGradient;

Wave blink(SQUARE, 0.5, 0.1);

TimeSliceField<16> timeSliceField(0.5);
Wave animation(SINE, 0.15);

// --- NEW STATE FOR CYCLES + FADE ---
static float lastSaw = 0.0f;
static int   cycleCount = 0;
static float fadeStartTime = -1.0f;

void begin() {
  strip.palette(customPalette, NOBLEND);
  strip.brightness(1.0f);
}

void step() {
  // --- 1) COMPTER LES CYCLES DE LA SAWTOOTH ---
  float currentSaw = sawtooth;  // valeur entre 0 et 1

  // wrap détecté: on passe de ~1 à ~0
  if (currentSaw < 0.1f && lastSaw > 0.9f) {
    cycleCount++;
  }
  lastSaw = currentSaw;

  // --- 2) CALCULER LA BRIGHTNESS GLOBALE ---
  float brightness = 1.0f;

  if (cycleCount >= 1) {
    if (fadeStartTime < 0.0f) {
      fadeStartTime = seconds();     // moment où on commence le fade
    }

    float dt = seconds() - fadeStartTime;
    const float fadeDuration = 1.0f; // durée du fade en secondes

    if (dt >= fadeDuration) {
      brightness = 0.0f;             // complètement éteint
    } else {
      brightness = 0.2f - (dt / fadeDuration); // descend de 1 → 0
    }
  }

  strip.brightness(brightness);

  // --- 3) ANIMATION ORIGINALE (inchangée) ---
  animation >> timeSliceField;

  if (timeSliceField.updated()) {
    animation.phase(sawtooth);
    strip.draw(timeSliceField);
  }
}
