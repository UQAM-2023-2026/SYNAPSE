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

// Haptic callback registration (for one-shot events like full energy)
void StripSetHapticCallback(void (*callback)(void));
// Note: Heartbeat callbacks are now managed by HeartbeatSystem
// Use heartbeatSystem.setHapticCallback() for heartbeat-synced haptics

// MiddleMan synchronization
void StripSetFlowSync(uint8_t speed);
void StripDisableFlowSync();

// Debug
void StripPrintDebug();

#endif