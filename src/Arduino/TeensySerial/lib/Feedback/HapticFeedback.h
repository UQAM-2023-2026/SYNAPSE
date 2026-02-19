/*==============================================================================
 * HapticFeedback.h - Haptic feedback system
 * 
 * State-driven haptic feedback using RhizomeState directly.
 * Synchronized with HeartbeatSystem for pulse timing.
 * 
 * Behaviors per state:
 * - IDLE: No vibration
 * - DISCOVERING: No vibration
 * - GENERATING: Continuous buzz until 100%, refreshed periodically
 * - GIVING (to node): Progressive "glou glou" effect based on energy
 * - MIDDLEMAN: No vibration
 * - DEAD: No vibration
 * 
 * Connection events: Effect 47 on the motor of the connected port
 *============================================================================*/

#ifndef HAPTIC_FEEDBACK_H
#define HAPTIC_FEEDBACK_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>
#include <RhizomeState.h>

// DRV2605 Waveform IDs
namespace HapticEffects {
    constexpr uint8_t CLICK = 1;          // Strong click 100% - clean, short
    constexpr uint8_t CONNECTION = 47;    // Soft buzz 60% - connection feedback
    constexpr uint8_t END = 0;            // Waveform terminator
}

class HapticFeedback {
public:
    HapticFeedback();
    
    // Initialize DRV2605 drivers
    bool begin();
    
    // Called every loop with current state info
    void update(RhizomeState state, float energy, bool maleConnected, bool femaleConnected);
    
    // Event handlers
    void onStateChange(RhizomeState oldState, RhizomeState newState);
    void onAllRhizomesFull();  // Triggered by FullEnergySync
    void onHeartbeat();         // Triggered by HeartbeatSystem
    void onMaleConnect();       // Connection event - male port
    void onFemaleConnect();     // Connection event - female port
    
    bool isReady() const { return _initialized; }
    
private:
    // Hardware
    Adafruit_DRV2605 _drvMale;   // MALE motor (Wire, pins 18/19)
    Adafruit_DRV2605 _drvFemale; // FEMALE motor (Wire1, pins 16/17)
    bool _initialized;
    bool _maleReady;
    bool _femaleReady;
    
    // State tracking
    RhizomeState _currentState;
    float _currentEnergy;
    bool _maleConnected;
    bool _femaleConnected;
    
    // Generation buzz state
    bool _generationActive;
    
    // GIVING state: glou glou effect tracking
    uint32_t _lastGlouGlouPulse;  // When we last played a pulse
    
    // Alternation for power safety (never both motors at once)
    bool _useFirstMotor;
    
    // Startup protection
    static constexpr uint32_t STARTUP_DELAY_MS = 2000;
    
    // RTP mode amplitude for continuous buzz (0-127)
    static constexpr uint8_t RTP_BUZZ_AMPLITUDE = 80;
    
    // GIVING pulse timing (energy-based interpolation)
    static constexpr uint32_t GLOU_MIN_INTERVAL_MS = 150;   // At 0% energy
    static constexpr uint32_t GLOU_MAX_INTERVAL_MS = 1500;  // At 100% energy
    
    // Helpers
    void initDriver(Adafruit_DRV2605& drv, TwoWire* wire, bool& ready, const char* name);
    void playClick(Adafruit_DRV2605& drv);
    void playConnectionEffect(Adafruit_DRV2605& drv);
    void playGlouGlouPulse(Adafruit_DRV2605& drv);
    void startContinuousBuzz(Adafruit_DRV2605& drv);  // RTP mode continuous vibration
    void stopContinuousBuzz(Adafruit_DRV2605& drv);   // Stop and restore mode
    void updateGiving();  // GIVING state glou glou logic
    
    uint32_t calculateGlouGlouInterval() const;  // Energy-based interval
    
    bool shouldPlayHeartbeat() const;
};

// Global instance
extern HapticFeedback hapticFeedback;

// Callback wrapper for HeartbeatSystem
void hapticHeartbeatCallback();

#endif // HAPTIC_FEEDBACK_H
