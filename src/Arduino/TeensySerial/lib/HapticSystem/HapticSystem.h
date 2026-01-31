#ifndef HAPTIC_SYSTEM_H
#define HAPTIC_SYSTEM_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_DRV2605.h>

// Haptic states matching rhizome states
enum HapticState {
  HAPTIC_IDLE = 0,
  HAPTIC_GENERATING = 1,
  HAPTIC_GIVING = 2,
  HAPTIC_MIDDLEMAN = 3,
  HAPTIC_DEAD = 4,
  HAPTIC_INVITE = 5
};

// Function pointer type for BPM access
typedef uint8_t (*BPMGetterFunc)(void);

class HapticSystem {
public:
  HapticSystem();

  // Initialize haptic drivers - returns false if init fails (non-blocking)
  bool begin();

  // Main update function - call in loop() - completely non-blocking
  void update(float energy, uint8_t rhizomeState);

  // Public API for external triggering
  void triggerEvent(bool isRight, uint8_t effect);
  void triggerLeftConnection();   // Vibrate left motor on left pogo connection
  void triggerRightConnection();  // Vibrate right motor on right pogo connection
  void setState(uint8_t state);
  void setBPMGetter(BPMGetterFunc getter);

  // Helper function to get current BPM
  uint8_t getCurrentBPM(float energy) const;

  // Callback for StripAnimation to trigger haptics (synchronized with LED heartbeat)
  void stripCallback();

  // Check if system is ready
  bool isReady() const { return initialized; }

private:
  // Driver instances
  Adafruit_DRV2605 drv1; // Left haptic driver (pins 18/19)
  Adafruit_DRV2605 drv2; // Right haptic driver (pins 16/17)

  // Initialization status
  bool initialized;
  bool drv1Ready;
  bool drv2Ready;

  // Current state
  HapticState currentState;
  HapticState previousState;
  float currentEnergy;

  // Timing variables - all non-blocking
  uint32_t lastHeartbeat;
  uint32_t lastUpdate;
  uint32_t stateEffectTimer;

  // BPM getter function pointer
  BPMGetterFunc bpmGetter;

  // Rate limiting to prevent I2C bus saturation
  static const uint32_t MIN_UPDATE_INTERVAL = 50; // ms between I2C commands
  uint32_t lastI2CCommand;

  // Private helper functions - all non-blocking
  void handleHeartbeat(float energy);
  void handleStateEffects(float energy);

  // Safe motor trigger with rate limiting
  void triggerMotor(Adafruit_DRV2605& drv, uint8_t effect);
  void triggerMotorSequence(Adafruit_DRV2605& drv, uint8_t* effects, uint8_t count);

  // Utility functions
  uint8_t mapBPM(float energy) const;
  uint8_t mapIntensity(float energy) const;
  bool shouldTriggerHeartbeat(uint8_t currentBPM);
  bool canSendI2C();
};

// Global instance
extern HapticSystem hapticSystem;

// Wrapper function for callback
void hapticStripCallbackWrapper();

#endif // HAPTIC_SYSTEM_H