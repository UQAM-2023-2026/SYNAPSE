#ifndef STRIPS_ANIMATION_H
#define STRIPS_ANIMATION_H

#include <RhizomeStateAndID.h>

// Core functions
void SetupStrips(RhizomeStateAndID &rh, uint8_t brightness);
void StripLoop();

// Event triggers (call from external code when events occur)
void StripEventFullEnergy();
void StripEventEmptyEnergy();
void StripEventConnection();
void StripEventDisconnection();

// Haptic callback registration
void StripSetHapticCallback(void (*callback)(void));

// MiddleMan synchronization
void StripSetFlowSync(uint8_t speed);
void StripDisableFlowSync();

// Debug
void StripPrintDebug();

#endif