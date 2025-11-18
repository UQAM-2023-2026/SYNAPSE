// ANIMATION, TRANSFERT D'ÉNERGIE À UN NOEUD

#include <Plaquette.h>
#include <PqLEDStrip.h>

Wave sawtooth(SINE, 0.8, 1);   // 1s period

const int LED_PIN  = 17;
const int NUM_LEDS = 30;

DigitalOut led(13);

LEDStripWS281X<LED_PIN, GRB, NUM_LEDS> strip;

DEFINE_GRADIENT_PALETTE(customGradient){
  0,   0,   0,   0,
  255, 150, 47,  0,
  0,   100, 20,  0,
};

CRGBPalette16 customPalette = customGradient;


void begin() {
  strip.palette(customPalette, NOBLEND);
  strip.brightness(1.0f);
}

void step() {
  // Wave value in [0,1]
  float posNorm = sawtooth;

  // Reversed direction: 0..NUM_LEDS-1
  float center = (1.0f - posNorm) * (NUM_LEDS - 1);

  // How far the "influence" spreads from the center
  const float fadeRadius = 6.0f;  // bigger = wider glow

  // Base color of the energy
  const uint8_t baseR = 150;
  const uint8_t baseG = 47;
  const uint8_t baseB = 0;

  for (int i = 0; i < NUM_LEDS; ++i) {
    // Distance from this LED to the moving center
    float d = i - center;
    if (d < 0) d = -d;   // abs

    // Map distance → factor in [0,1]
    float t = 1.0f - (d / fadeRadius);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // Make the center stronger and edges softer (ease curve)
    float w = t * t;   // quadratic falloff

    if (w > 0.0f) {
      // Scale base color by w
      uint8_t r = (uint8_t)(baseR * w);
      uint8_t g = (uint8_t)(baseG * w);
      uint8_t b = (uint8_t)(baseB * w);

      strip.setPixel(i, CRGB(r, g, b));
    } else {
      // Very far from center → completely off
      strip.setPixel(i, CRGB::Black);
    }
  }
}
