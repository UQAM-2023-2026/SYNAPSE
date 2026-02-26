/*==============================================================================
 * RhizomeState.h - Rhizome state definitions
 * 
 * Single source of truth for all rhizome states.
 * States are mutually exclusive.
 *============================================================================*/

#ifndef RHIZOME_STATE_H
#define RHIZOME_STATE_H

#include <Arduino.h>

enum class RhizomeState : uint8_t {
    IDLE,           // No port connected, waiting
    DISCOVERING,    // One or both ports connected, building ID bank
    GENERATING,     // Loop closed, generating energy
    GIVING,         // Connected to node via MALE, draining energy
    MIDDLEMAN,      // Relaying between node (MALE) and rhizome (FEMALE)
    DEAD            // Energy depleted to 0%
};

// String conversion for debugging
inline const char* stateToString(RhizomeState state) {
    switch (state) {
        case RhizomeState::IDLE:        return "IDLE";
        case RhizomeState::DISCOVERING: return "DISCOVERING";
        case RhizomeState::GENERATING:  return "GENERATING";
        case RhizomeState::GIVING:      return "GIVING";
        case RhizomeState::MIDDLEMAN:   return "MIDDLEMAN";
        case RhizomeState::DEAD:        return "DEAD";
        default:                        return "UNKNOWN";
    }
}

#endif // RHIZOME_STATE_H
