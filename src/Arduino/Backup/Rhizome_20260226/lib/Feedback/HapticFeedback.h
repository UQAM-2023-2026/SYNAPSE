/*==============================================================================
 * HapticFeedback.h - Haptic feedback system
 * 
 * State-driven haptic feedback using RhizomeState directly.
 * Synchronized with HeartbeatSystem for pulse timing.
 * 
 * Behaviors per state:
 * - IDLE: Heartbeat on unconnected port(s)
 * - DISCOVERING: Heartbeat on unconnected port
 * - GENERATING: Double-click start, continuous buzz until 100%
 * - GIVING: "Glou glou" pulsed haptic (like liquid emptying from bottle)
 * - MIDDLEMAN: No vibration
 * - DEAD: No vibration
 *============================================================================*/

#ifndef HAPTIC_FEEDBACK_H
#define HAPTIC_FEEDBACK_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>
#include <RhizomeState.h>

// DRV2605 Waveform IDs
namespace HapticEffects {
    constexpr uint8_t CLICK = 1;              // Strong click 100% - for heartbeat
    constexpr uint8_t SOFT_CLICK = 3;         // Strong click 30% - for glou-glou (less power)
    constexpr uint8_t SHARP_CLICK = 4;        // Sharp click 100%
    constexpr uint8_t TICK = 47;              // Tick 1 - 100% - for connection feedback
    constexpr uint8_t SOFT_BUZZ = 14;         // Soft Fuzz 60% - lighter buzz for generation
    constexpr uint8_t END = 0;                // Waveform terminator
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
    void onMaleConnected();     // Connection feedback
    void onFemaleConnected();   // Connection feedback
    
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
    unsigned long _lastBuzzRefresh;  // For periodic buzz refresh
    static constexpr unsigned long BUZZ_REFRESH_MS = 800;  // Refresh buzz interval
    
    // Alternation for power safety (never both motors at once)
    bool _useFirstMotor;
    
    // Glou-glou pulse state (for GIVING)
    bool _glouGlouActive;
    unsigned long _lastGlouGlouPulse;
    
    // Glou-glou timing constants (energy-based: high=slow, low=fast)
    static constexpr unsigned long GLOU_MIN_INTERVAL_MS = 250;    // At 0% energy (fast) - was 150, increased for stability
    static constexpr unsigned long GLOU_MAX_INTERVAL_MS = 1500;   // At 100% energy (slow)
    static constexpr unsigned long GLOU_INITIAL_DELAY_MS = 300;   // Delay before first pulse
    static constexpr bool GLOU_ENABLED = true;                    // Re-enabled with softer settings
    
    // Startup protection
    static constexpr uint32_t STARTUP_DELAY_MS = 2000;
    
    // Helpers
    void initDriver(Adafruit_DRV2605& drv, TwoWire* wire, bool& ready, const char* name);
    void playClick(Adafruit_DRV2605& drv);
    void playSoftClick(Adafruit_DRV2605& drv);  // Lower power for glou-glou
    void playTick(Adafruit_DRV2605& drv);        // Connection feedback
    void playDoubleClick(Adafruit_DRV2605& drv);
    void startBuzz(Adafruit_DRV2605& drv);
    void stopBuzz(Adafruit_DRV2605& drv);
    
    bool shouldPlayHeartbeat() const;
    unsigned long getGlouGlouInterval(float energy) const;  // Energy-based interval
};

// Global instance
extern HapticFeedback hapticFeedback;

// Callback wrapper for HeartbeatSystem
void hapticHeartbeatCallback();

#endif // HAPTIC_FEEDBACK_H
