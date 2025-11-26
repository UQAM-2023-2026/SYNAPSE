#ifndef HAPTIC_FEEDBACK_H
#define HAPTIC_FEEDBACK_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

extern Adafruit_DRV2605 drv;

void SetupHaptic();
void HapticLoop();

#endif
