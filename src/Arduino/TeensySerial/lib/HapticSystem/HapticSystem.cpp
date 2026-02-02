#include "HapticSystem.h"
#include <Arduino.h>

// Heartbeat LED indicator
#define HEARTBEAT_LED_PIN 2

// Global instance
HapticSystem hapticSystem;

// Wrapper function implementation
void hapticStripCallbackWrapper() {
  hapticSystem.stripCallback();
}

HapticSystem::HapticSystem()
  : initialized(false),
    drv1Ready(false),
    drv2Ready(false),
    currentState(HAPTIC_IDLE),
    previousState(HAPTIC_IDLE),
    currentEnergy(0),
    lastHeartbeat(0),
    lastUpdate(0),
    stateEffectTimer(0),
    bpmGetter(nullptr),
    lastI2CCommand(0)
{
}

bool HapticSystem::begin() {
  Serial.println("[Haptic] Initializing...");

  // Setup heartbeat LED
  pinMode(HEARTBEAT_LED_PIN, OUTPUT);
  digitalWrite(HEARTBEAT_LED_PIN, LOW);

  // Try to initialize left driver - non-blocking, just skip if fails
  drv1Ready = drv1.begin(&Wire);
  if (drv1Ready) {
    drv1.selectLibrary(6);  // LRA library
    drv1.useLRA();
    
    // Auto-calibration for LRA - CRITICAL for proper vibration strength
    // Set rated voltage (1.8V RMS typical for LRA)
    drv1.writeRegister8(DRV2605_REG_RATEDV, 0x50);  // ~1.8V
    // Set overdrive clamp voltage (2.0V max)
    drv1.writeRegister8(DRV2605_REG_CLAMPV, 0x89);  // ~2.0V
    // Set feedback control for LRA
    drv1.writeRegister8(DRV2605_REG_FEEDBACK, 0xB6);  // LRA mode, brake factor 4x, loop gain medium
    // Set control registers
    drv1.writeRegister8(DRV2605_REG_CONTROL1, 0x93);  // Drive time
    drv1.writeRegister8(DRV2605_REG_CONTROL2, 0xF5);  // Sample time, blanking, idiss
    drv1.writeRegister8(DRV2605_REG_CONTROL3, 0x80);  // LRA open loop, analog input
    
    drv1.setMode(DRV2605_MODE_INTTRIG);
    Serial.println("[Haptic] Left DRV2605 OK + Calibrated");
  } else {
    Serial.println("[Haptic] Left DRV2605 not found - continuing without");
  }

  // Try to initialize right driver - non-blocking
  drv2Ready = drv2.begin(&Wire1);
  if (drv2Ready) {
    drv2.selectLibrary(6);  // LRA library
    drv2.useLRA();
    
    // Auto-calibration for LRA - same settings as drv1
    drv2.writeRegister8(DRV2605_REG_RATEDV, 0x50);
    drv2.writeRegister8(DRV2605_REG_CLAMPV, 0x89);
    drv2.writeRegister8(DRV2605_REG_FEEDBACK, 0xB6);
    drv2.writeRegister8(DRV2605_REG_CONTROL1, 0x93);
    drv2.writeRegister8(DRV2605_REG_CONTROL2, 0xF5);
    drv2.writeRegister8(DRV2605_REG_CONTROL3, 0x80);
    
    drv2.setMode(DRV2605_MODE_INTTRIG);
    Serial.println("[Haptic] Right DRV2605 OK + Calibrated");
  } else {
    Serial.println("[Haptic] Right DRV2605 not found - continuing without");
  }

  initialized = drv1Ready || drv2Ready;
  
  if (initialized) {
    Serial.println("[Haptic] System ready");
  } else {
    Serial.println("[Haptic] No motors found - haptics disabled");
  }

  return initialized;
}

bool HapticSystem::canSendI2C() {
  // Rate limit I2C commands to prevent bus saturation
  // But allow rapid successive calls for both motors
  uint32_t now = millis();
  if (now - lastI2CCommand >= MIN_UPDATE_INTERVAL) {
    lastI2CCommand = now;
    return true;
  }
  return false;
}

void HapticSystem::triggerMotor(Adafruit_DRV2605& drv, uint8_t effect) {
  // No rate limiting here - let both motors trigger together
  drv.setWaveform(0, effect);
  drv.setWaveform(1, 0);
  drv.go();
}

void HapticSystem::triggerMotorSequence(Adafruit_DRV2605& drv, uint8_t* effects, uint8_t count) {
  // No rate limiting here - let both motors trigger together
  for (uint8_t i = 0; i < count && i < 8; i++) {
    drv.setWaveform(i, effects[i]);
  }
  drv.go();
}

void HapticSystem::update(float energy, uint8_t rhizomeState) {
  if (!initialized) return;

  // Store current energy
  currentEnergy = energy;

  // Update state if changed
  if (rhizomeState != currentState) {
    previousState = currentState;
    currentState = (HapticState)rhizomeState;
    stateEffectTimer = millis(); // Reset state effect timer on state change
  }

  // Don't run haptics too frequently
  uint32_t now = millis();
  if (now - lastUpdate < 20) return; // Max 50 updates/sec
  lastUpdate = now;

  // Heartbeat is now handled by stripCallback() called from LED animation
  // Only handle state-specific effects here
  handleStateEffects(energy);
}

void HapticSystem::handleHeartbeat(float energy) {
  uint8_t currentBPM = getCurrentBPM(energy);

  if (shouldTriggerHeartbeat(currentBPM)) {
    // Map energy to intensity - softer when low energy
    uint8_t intensity = mapIntensity(energy);
    
    // Create heartbeat sequence: lub-dub
    uint8_t sequence[] = {intensity, 66, intensity, 0}; // effect, pause, effect, end
    
    // Trigger BOTH motors simultaneously for synchronized heartbeat
    if (drv1Ready) {
      triggerMotorSequence(drv1, sequence, 4);
    }
    if (drv2Ready) {
      triggerMotorSequence(drv2, sequence, 4);
    }
    
    lastHeartbeat = millis();
  }
}

void HapticSystem::handleStateEffects(float energy) {
  // POWER-SAFE: Disable all extra state effects to minimize power draw
  // Only the heartbeat in stripCallback() will trigger vibrations
  return;
  
  /*
  uint32_t now = millis();
  uint32_t timeSinceStateEffect = now - stateEffectTimer;

  switch (currentState) {
    case HAPTIC_IDLE:
      // Occasional gentle invite pulse every 5 seconds
      if (timeSinceStateEffect > 5000) {
        uint8_t intensity = mapIntensity(energy);
        if (drv2Ready) {
          uint8_t sequence[] = {(uint8_t)(intensity / 2), 66, (uint8_t)(intensity / 2), 0};
          triggerMotorSequence(drv2, sequence, 4);
        }
        stateEffectTimer = now;
      }
      break;

    case HAPTIC_GENERATING:
      // Slow rumble every 3 seconds
      if (timeSinceStateEffect > 3000) {
        uint8_t intensity = mapIntensity(energy);
        if (drv1Ready) triggerMotor(drv1, intensity);
        if (drv2Ready) triggerMotor(drv2, intensity);
        stateEffectTimer = now;
      }
      break;

    case HAPTIC_GIVING:
      // Energy transfer pulse every 2 seconds
      if (timeSinceStateEffect > 2000) {
        uint8_t intensity = mapIntensity(energy);
        // Descending intensity to simulate energy leaving
        uint8_t sequence[] = {intensity, 66, (uint8_t)(intensity * 2/3), 66, (uint8_t)(intensity / 3), 0};
        if (drv1Ready) triggerMotorSequence(drv1, sequence, 6);
        if (drv2Ready) triggerMotorSequence(drv2, sequence, 6);
        stateEffectTimer = now;
      }
      break;

    case HAPTIC_MIDDLEMAN:
      // Flow-through vibration every 1.5 seconds
      if (timeSinceStateEffect > 1500) {
        uint8_t intensity = mapIntensity(50); // Medium intensity for middleman
        if (drv1Ready) triggerMotor(drv1, intensity);
        if (drv2Ready) triggerMotor(drv2, intensity);
        stateEffectTimer = now;
      }
      break;

    case HAPTIC_DEAD:
      // No effects when dead
      break;

    case HAPTIC_INVITE:
      // Double tap invite every 4 seconds
      if (timeSinceStateEffect > 4000) {
        uint8_t sequence[] = {15, 66, 15, 0}; // Light double tap
        if (drv2Ready) triggerMotorSequence(drv2, sequence, 4);
        stateEffectTimer = now;
      }
      break;
  }
  */
}

uint8_t HapticSystem::mapIntensity(float energy) const {
  // Using BUZZ effects (more perceptible than clicks)
  // Library 6 (LRA) effect reference:
  // 47 = Strong Click - 100%
  // 48 = Strong Click - 60%
  // 49 = Strong Click - 30%
  // 14 = Strong Buzz - 100%
  // 15 = Strong Buzz - 75%
  // 16 = Strong Buzz - 50%
  // 52 = Pulsing Strong 1
  // 58 = Transition Ramp Up Short Sharp 1
  
  if (energy < 30) {
    return 50;  // Strong Buzz 50%
  } else if (energy < 60) {
    return 48;  // Strong Buzz 75%
  } else {
    return 47;  // Strong Buzz 100%
  }
}

uint8_t HapticSystem::getCurrentBPM(float energy) const {
  if (bpmGetter != nullptr) {
    return bpmGetter();
  }
  return mapBPM(energy);
}

uint8_t HapticSystem::mapBPM(float energy) const {
  // Same mapping as LED system: 30 BPM at 0% energy, 60 BPM at 100% energy
  return map(constrain(energy, 0, 100), 0, 100, 30, 60);
}

bool HapticSystem::shouldTriggerHeartbeat(uint8_t currentBPM) {
  uint32_t now = millis();
  uint32_t interval = 60000 / currentBPM;
  return (now - lastHeartbeat >= interval);
}

void HapticSystem::stripCallback() {
  // Called by LED pulse layer at the start of each heartbeat
  // This ensures PERFECT synchronization with LEDs
  
  // Toggle heartbeat LED (even if haptics disabled)
  static bool ledState = false;
  ledState = !ledState;
  digitalWrite(HEARTBEAT_LED_PIN, ledState ? HIGH : LOW);
  
  if (!initialized) return;
  
  // No heartbeat when dead
  if (currentState == HAPTIC_DEAD) return;
  
  // POWER-SAFE: Don't trigger if system just started (wait 2 seconds)
  if (millis() < 2000) return;
  
  uint8_t intensity = mapIntensity(currentEnergy);
  
  // POWER-SAFE: Alternate between motors, never both at once
  static bool useMotor1 = true;
  
  // Single pulse for power safety
  if (useMotor1 && drv1Ready) {
    drv1.setWaveform(0, intensity);
    drv1.setWaveform(1, 0);
    drv1.go();
  } else if (!useMotor1 && drv2Ready) {
    drv2.setWaveform(0, intensity);
    drv2.setWaveform(1, 0);
    drv2.go();
  }
  
  useMotor1 = !useMotor1;  // Alternate for next time
  lastHeartbeat = millis();
}

void HapticSystem::triggerEvent(bool isRight, uint8_t effect) {
  if (!initialized) return;
  
  Adafruit_DRV2605* drv = isRight ? &drv2 : &drv1;
  bool* ready = isRight ? &drv2Ready : &drv1Ready;
  
  if (!*ready) return;
  
  switch (effect) {
    case 1: // Connection
      triggerMotor(*drv, 10);
      break;
    case 2: // Disconnection
      triggerMotor(*drv, 15);
      break;
    case 3: // Full energy celebration
      {
        uint8_t sequence[] = {35, 66, 35, 66, 25, 0};
        triggerMotorSequence(*drv, sequence, 6);
      }
      break;
    default:
      triggerMotor(*drv, effect);
      break;
  }
}

void HapticSystem::setState(uint8_t state) {
  if (state != currentState) {
    previousState = currentState;
    currentState = (HapticState)state;
    stateEffectTimer = millis();
  }
}

void HapticSystem::setBPMGetter(BPMGetterFunc getter) {
  bpmGetter = getter;
}

void HapticSystem::triggerLeftConnection() {
  if (!initialized || !drv1Ready) return;
  
  // Distinct connection feedback - double tap
  // Effect 10 = Double Click - 100%
  drv2.setWaveform(0, 47);  // Double click
  drv2.setWaveform(1, 0);
  drv2.go();
  
  Serial.println("[Haptic] Left connection vibration");
}

void HapticSystem::triggerRightConnection() {
  if (!initialized || !drv2Ready) return;
  
  // Distinct connection feedback - double tap
  // Effect 10 = Double Click - 100%
  drv1.setWaveform(0, 47);  // Double click
  drv1.setWaveform(1, 0);
  drv1.go();
  
  Serial.println("[Haptic] Right connection vibration");
}
