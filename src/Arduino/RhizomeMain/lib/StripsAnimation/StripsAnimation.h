#ifndef STRIPS_ANIMATION_H
#define STRIPS_ANIMATION_H

#include <Arduino.h>

void SetupStrips(uint8_t brightness);
void StripLoop();            // fixed signature to match implementation
void SetEnergy(uint8_t e);   // update energy from main (0-100)

#endif