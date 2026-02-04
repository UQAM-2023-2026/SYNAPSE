/*==============================================================================
 * HapticSystem.cpp - Haptic feedback implementation
 * 
 * Clean, simple haptic system synchronized with HeartbeatSystem.
 * Non-blocking, millis()-based timing.
 *============================================================================*/

#include "HapticSystem.h"

// Global instance
HapticSystem hapticSystem;

// Wrapper for HeartbeatSystem callback
void hapticHeartbeatCallback() {
    hapticSystem.onHeartbeat();
}

/*------------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
HapticSystem::HapticSystem()
    : _initialized(false),
      _leftReady(false),
      _rightReady(false),
      _currentState(0),
      _currentEnergy(0),
      _leftConnected(false),
      _rightConnected(false),
      _generationActive(false),
      _generationStartTime(0)
{
}

/*------------------------------------------------------------------------------
 * Initialization
 *----------------------------------------------------------------------------*/
bool HapticSystem::begin() {
    Serial.println("[Haptic] Initializing...");
    
    initDriver(_drvLeft, _leftReady, "Left");
    initDriver(_drvRight, _rightReady, "Right");
    
    _initialized = _leftReady || _rightReady;
    
    if (_initialized) {
        Serial.println("[Haptic] System ready");
    } else {
        Serial.println("[Haptic] No motors found - haptics disabled");
    }
    
    return _initialized;
}

void HapticSystem::initDriver(Adafruit_DRV2605& drv, bool& ready, const char* name) {
    // Left uses Wire1, Right uses Wire (swapped from physical wiring)
    TwoWire* wire = (&drv == &_drvLeft) ? &Wire1 : &Wire;
    
    ready = drv.begin(wire);
    if (!ready) {
        Serial.print("[Haptic] ");
        Serial.print(name);
        Serial.println(" DRV2605 not found");
        return;
    }
    
    // Configure for LRA (Linear Resonant Actuator)
    drv.selectLibrary(6);
    drv.useLRA();
    
    // LRA calibration settings
    drv.writeRegister8(DRV2605_REG_RATEDV, 0x50);    // 1.8V RMS
    drv.writeRegister8(DRV2605_REG_CLAMPV, 0x89);    // 2.0V max
    drv.writeRegister8(DRV2605_REG_FEEDBACK, 0xB6);  // LRA mode, brake 4x
    drv.writeRegister8(DRV2605_REG_CONTROL1, 0x93);  // Drive time
    drv.writeRegister8(DRV2605_REG_CONTROL2, 0xF5);  // Sample/blanking
    drv.writeRegister8(DRV2605_REG_CONTROL3, 0x80);  // LRA open loop
    
    drv.setMode(DRV2605_MODE_INTTRIG);
    
    Serial.print("[Haptic] ");
    Serial.print(name);
    Serial.println(" DRV2605 OK");
}

/*------------------------------------------------------------------------------
 * Main Update Loop
 *----------------------------------------------------------------------------*/
void HapticSystem::update(float energy, uint8_t rhizomeState, bool leftConnected, bool rightConnected) {
    if (!_initialized) return;
    
    // Store current state
    _currentEnergy = energy;
    _currentState = rhizomeState;
    _leftConnected = leftConnected;
    _rightConnected = rightConnected;
    
    // Auto-stop generation when fully charged
    if (_generationActive && energy >= 100.0f) {
        stopGeneration();
    }
}

/*------------------------------------------------------------------------------
 * Heartbeat Callback (called by HeartbeatSystem)
 * 
 * Heartbeat pattern: systole (sharp) + pause + diastole (soft)
 * Like a real heart: lub-dub
 *----------------------------------------------------------------------------*/
void HapticSystem::onHeartbeat() {
    if (!_initialized) return;
    if (millis() < STARTUP_DELAY_MS) return;
    if (!shouldPlayHeartbeat()) return;
    
    // IDLE: Both motors play heartbeat
    // ONE END CONNECTED: Only unconnected end plays
    
    bool playLeft = !_leftConnected;
    bool playRight = !_rightConnected;
    
    // In pure IDLE (nothing connected), both play
    if (!_leftConnected && !_rightConnected) {
        playLeft = true;
        playRight = true;
    }
    
    if (playLeft && _leftReady) {
        playHeartbeat(_drvLeft);
    }
    if (playRight && _rightReady) {
        playHeartbeat(_drvRight);
    }
}

bool HapticSystem::shouldPlayHeartbeat() const {
    // No heartbeat when dead
    if (_currentState == 4) return false;  // DEAD
    
    // No heartbeat when middleman
    if (_currentState == 3) return false;  // MIDDLEMAN
    
    // No heartbeat when generating (continuous buzz instead)
    if (_generationActive) return false;
    
    // No heartbeat at 0% energy
    if (_currentEnergy <= 0) return false;
    
    return true;
}

/*------------------------------------------------------------------------------
 * Event Handlers
 *----------------------------------------------------------------------------*/
void HapticSystem::onGenerationStart() {
    if (!_initialized) return;
    
    Serial.println("[Haptic] Generation start - double click + buzz");
    
    // Double click confirmation on both ends
    if (_leftReady) playDoubleClick(_drvLeft);
    if (_rightReady) playDoubleClick(_drvRight);
    
    // Start continuous buzz after a short delay for the clicks
    _generationActive = true;
    _generationStartTime = millis();
    
    // Start the long buzz on both motors
    if (_leftReady) startBuzz(_drvLeft);
    if (_rightReady) startBuzz(_drvRight);
}

void HapticSystem::onFullyCharged() {
    if (!_initialized) return;
    
    Serial.println("[Haptic] Fully charged - double click");
    
    // Stop any ongoing generation
    stopGeneration();
    
    // Double click celebration on both ends
    if (_leftReady) playDoubleClick(_drvLeft);
    if (_rightReady) playDoubleClick(_drvRight);
}

void HapticSystem::stopGeneration() {
    if (!_generationActive) return;
    
    Serial.println("[Haptic] Stopping generation buzz");
    
    _generationActive = false;
    
    // Stop the motors
    if (_leftReady) stopBuzz(_drvLeft);
    if (_rightReady) stopBuzz(_drvRight);
}

/*------------------------------------------------------------------------------
 * Motor Control Helpers
 *----------------------------------------------------------------------------*/
void HapticSystem::playEffect(Adafruit_DRV2605& drv, uint8_t effect) {
    drv.setWaveform(0, effect);
    drv.setWaveform(1, HapticEffects::END);
    drv.go();
}

void HapticSystem::playHeartbeat(Adafruit_DRV2605& drv) {
    // Heartbeat sequence: systole (strong click) + short pause + diastole (softer pulse)
    // Musical analogy: dotted eighth note, rest, eighth note
    drv.setWaveform(0, HapticEffects::SYSTOLE);   // Strong click - lub
    drv.setWaveform(1, 66);                        // Short pause (~60ms)
    drv.setWaveform(2, HapticEffects::DIASTOLE);  // Softer pulse - dub
    drv.setWaveform(3, HapticEffects::END);
    drv.go();
}

void HapticSystem::playDoubleClick(Adafruit_DRV2605& drv) {
    // Two sharp clicks for confirmation
    drv.setWaveform(0, HapticEffects::SHARP_CLICK);
    drv.setWaveform(1, 66);  // Short pause
    drv.setWaveform(2, HapticEffects::SHARP_CLICK);
    drv.setWaveform(3, HapticEffects::END);
    drv.go();
}

void HapticSystem::startBuzz(Adafruit_DRV2605& drv) {
    // Long continuous buzz - effect 118 runs until stopped
    drv.setWaveform(0, HapticEffects::LONG_BUZZ);
    drv.setWaveform(1, HapticEffects::END);
    drv.go();
}

void HapticSystem::stopBuzz(Adafruit_DRV2605& drv) {
    // Stop the motor by sending empty sequence
    drv.setWaveform(0, HapticEffects::END);
    drv.go();
}
