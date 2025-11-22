//ANIMATION, TRANSFERT D'ÉNERGIE À UN NOEUD

#include <Plaquette.h>
#include <PqLEDStrip.h>

Wave sawtooth (TRIANGLE,1,1);

// ONE LED
const int LED_PIN = 17;
const int NUM_LEDS = 30;

DigitalOut led(13);

LEDStripWS281X<LED_PIN, GRB, NUM_LEDS> strip;

DEFINE_GRADIENT_PALETTE(customGradient){
  0, 0, 0, 0,
  255, 150, 47, 0,
  0, 100, 20, 0,
};

CRGBPalette16 customPalette = customGradient;

Wave blink(SQUARE, 0.5, 0.1);

TimeSliceField<16> timeSliceField(0.01);
Wave animation(SINE, 0.05);

void begin() {
  strip.palette(customPalette, NOBLEND);
}

void step() {
  animation >> timeSliceField;

  if(timeSliceField.updated()){
    animation.phase(sawtooth);
    strip.draw(timeSliceField);
  }
}
