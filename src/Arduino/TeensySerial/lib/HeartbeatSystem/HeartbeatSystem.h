/*==============================================================================
 * HeartbeatSystem.h - Centralized heartbeat management
 * Project: Synapse - Interactive Rhizome Installation
 * 
 * This library centralizes all heartbeat-related functionality:
 * - LED strip pulse brightness modulation
 * - Haptic motor vibration timing
 * - Heartbeat indicator LED (pin 2)
 * 
 * All systems are perfectly synchronized to a single heartbeat timing source.
 *============================================================================*/

#ifndef HEARTBEAT_SYSTEM_H
#define HEARTBEAT_SYSTEM_H

#include <Arduino.h>

/*------------------------------------------------------------------------------
 * Heartbeat Configuration
 *----------------------------------------------------------------------------*/
#define HEARTBEAT_LED_PIN 2   // Physical LED indicator

// BPM range based on energy level
#define HEARTBEAT_MIN_BPM 20  // BPM at 0% energy
#define HEARTBEAT_MAX_BPM 60 // BPM at 100% energy

/*------------------------------------------------------------------------------
 * Heartbeat Phase Structure
 * Shared phase data for synchronized rendering
 *----------------------------------------------------------------------------*/
struct HeartbeatPhase {
    float phase;              // 0.0 - 1.0, current position in beat cycle
    float normalizedBrightness; // 0.0 - 1.0, brightness value from heartbeat curve
    bool newBeatStarted;      // True at the start of each new beat
    uint8_t currentBPM;       // Current beats per minute
};

/*------------------------------------------------------------------------------
 * Callback Types
 *----------------------------------------------------------------------------*/
typedef void (*HeartbeatCallback)(void);        // Simple beat notification
typedef void (*HeartbeatPhaseCallback)(const HeartbeatPhase& phase); // Detailed phase info

/*------------------------------------------------------------------------------
 * HeartbeatSystem Class
 * Central manager for all heartbeat timing and synchronization
 *----------------------------------------------------------------------------*/
class HeartbeatSystem {
public:
    HeartbeatSystem();
    
    // Initialization
    void begin();
    
    // Main update - call every loop iteration
    // Returns true if a new beat started this frame
    bool update(float energy);
    
    // Get current heartbeat phase data (for LED rendering)
    const HeartbeatPhase& getPhase() const { return _phase; }
    
    // Get current BPM
    uint8_t getCurrentBPM() const { return _phase.currentBPM; }
    
    // Register callbacks - called at the START of each heartbeat
    void setHapticCallback(HeartbeatCallback callback);
    void setLedStripCallback(HeartbeatPhaseCallback callback);
    
    // Enable/disable heartbeat (e.g., when dead)
    void setEnabled(bool enabled);
    bool isEnabled() const { return _enabled; }
    
    // Manual sync (e.g., for MiddleMan mode receiving sync from another rhizome)
    void forceSync(float phase);
    void setBPM(uint8_t bpm);
    
    // Debug
    void printDebug(Stream& output = Serial);

private:
    // Phase tracking
    HeartbeatPhase _phase;
    float _previousPhase;
    uint32_t _lastUpdate;
    
    // State
    bool _enabled;
    bool _initialized;
    
    // Callbacks
    HeartbeatCallback _hapticCallback;
    HeartbeatPhaseCallback _ledStripCallback;
    
    // Internal helpers
    uint8_t calculateBPM(float energy) const;
    float calculateHeartbeatCurve(float phase) const;
    void triggerHeartbeatLed(bool newBeat);
    void notifyCallbacks();
};

// Global singleton instance
extern HeartbeatSystem heartbeatSystem;

#endif // HEARTBEAT_SYSTEM_H
