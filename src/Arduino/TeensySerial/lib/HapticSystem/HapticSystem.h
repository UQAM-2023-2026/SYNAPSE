/*==============================================================================
 * HapticSystem.h - Haptic feedback for Synapse Rhizome
 * 
 * Provides vibration feedback synchronized with the centralized HeartbeatSystem.
 * 
 * Behaviors:
 * - IDLE: Heartbeat on both motors (if energy > 0)
 * - ONE END CONNECTED: Heartbeat only on unconnected end
 * - GENERATING (chain): 2 clicks at start, then continuous buzz until 100%
 * - FULLY CHARGED: 2 clicks on both ends
 * - MIDDLEMAN: No vibration
 * - DEAD: No vibration
 *============================================================================*/

#ifndef HAPTIC_SYSTEM_H
#define HAPTIC_SYSTEM_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

// DRV2605 Waveform IDs used in this system
namespace HapticEffects {
    // Heartbeat
    constexpr uint8_t SYSTOLE = 52;          // sharp systole 17
    constexpr uint8_t DIASTOLE = 0;        // softer diastole 24
    
    // Confirmation clicks
    constexpr uint8_t SHARP_CLICK = 4;      // Sharp click 100%
    
    // Continuous generation buzz
    constexpr uint8_t LONG_BUZZ = 118;      // Long buzz for programmatic stopping
    
    // Waveform sequence terminator
    constexpr uint8_t END = 0;
}

class HapticSystem {
public:
    HapticSystem();
    
    // Initialize both DRV2605 drivers
    bool begin();
    
    // Main update - call in loop() with current state info
    void update(float energy, uint8_t rhizomeState, bool leftConnected, bool rightConnected);
    
    // Called by HeartbeatSystem at the start of each heartbeat
    void onHeartbeat();
    
    // Event triggers (called externally)
    void onGenerationStart();    // Chain formed, start generating
    void onFullyCharged();       // Loop reached 100% energy
    void stopGeneration();       // Stop continuous buzz
    
    // Status
    bool isReady() const { return _initialized; }

private:
    // Hardware
    Adafruit_DRV2605 _drvLeft;   // Left motor (Wire, pins 18/19)
    Adafruit_DRV2605 _drvRight;  // Right motor (Wire1, pins 16/17)
    bool _initialized;
    bool _leftReady;
    bool _rightReady;
    
    // State tracking
    uint8_t _currentState;
    float _currentEnergy;
    bool _leftConnected;
    bool _rightConnected;
    
    // Generation buzz state
    bool _generationActive;
    uint32_t _generationStartTime;
    
    // Startup protection
    static constexpr uint32_t STARTUP_DELAY_MS = 2000;
    
    // Helper functions
    void initDriver(Adafruit_DRV2605& drv, bool& ready, const char* name);
    void playEffect(Adafruit_DRV2605& drv, uint8_t effect);
    void playHeartbeat(Adafruit_DRV2605& drv);
    void playDoubleClick(Adafruit_DRV2605& drv);
    void startBuzz(Adafruit_DRV2605& drv);
    void stopBuzz(Adafruit_DRV2605& drv);
    
    bool shouldPlayHeartbeat() const;
};

// Global instance
extern HapticSystem hapticSystem;

// Wrapper for HeartbeatSystem callback
void hapticHeartbeatCallback();

#endif // HAPTIC_SYSTEM_H