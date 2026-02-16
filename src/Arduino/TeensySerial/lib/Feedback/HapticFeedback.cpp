/*==============================================================================
 * HapticFeedback.cpp - Haptic feedback implementation
 *============================================================================*/

#include "HapticFeedback.h"
#include <HardwarePins.h>

// Global instance
HapticFeedback hapticFeedback;

// Callback wrapper for HeartbeatSystem
void hapticHeartbeatCallback() {
    hapticFeedback.onHeartbeat();
}

/*------------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
HapticFeedback::HapticFeedback()
    : _initialized(false)
    , _maleReady(false)
    , _femaleReady(false)
    , _currentState(RhizomeState::IDLE)
    , _currentEnergy(0)
    , _maleConnected(false)
    , _femaleConnected(false)
    , _generationActive(false)
    , _useFirstMotor(true)
{}

/*------------------------------------------------------------------------------
 * Initialization
 *----------------------------------------------------------------------------*/
bool HapticFeedback::begin() {
    Serial.println("[HapticFeedback] Initializing...");
    
    // MALE uses Wire (SDA 18 / SCL 19)
    initDriver(_drvMale, &Wire, _maleReady, "MALE");
    
    // FEMALE uses Wire1 (SDA 17 / SCL 16)
    initDriver(_drvFemale, &Wire1, _femaleReady, "FEMALE");
    
    _initialized = _maleReady || _femaleReady;
    
    if (_initialized) {
        Serial.println("[HapticFeedback] Ready");
    } else {
        Serial.println("[HapticFeedback] No motors found - disabled");
    }
    
    return _initialized;
}

void HapticFeedback::initDriver(Adafruit_DRV2605& drv, TwoWire* wire, bool& ready, const char* name) {
    ready = drv.begin(wire);
    if (!ready) {
        Serial.print("[HapticFeedback] ");
        Serial.print(name);
        Serial.println(" DRV2605 not found");
        return;
    }
    
    // Configure for LRA (Linear Resonant Actuator)
    drv.selectLibrary(6);
    drv.useLRA();
    
    // LRA calibration settings
    drv.writeRegister8(DRV2605_REG_RATEDV, 0x50);
    drv.writeRegister8(DRV2605_REG_CLAMPV, 0x89);
    drv.writeRegister8(DRV2605_REG_FEEDBACK, 0xB6);
    drv.writeRegister8(DRV2605_REG_CONTROL1, 0x93);
    drv.writeRegister8(DRV2605_REG_CONTROL2, 0xF5);
    drv.writeRegister8(DRV2605_REG_CONTROL3, 0x80);
    
    drv.setMode(DRV2605_MODE_INTTRIG);
    
    Serial.print("[HapticFeedback] ");
    Serial.print(name);
    Serial.println(" DRV2605 OK");
}

/*------------------------------------------------------------------------------
 * Main Update
 *----------------------------------------------------------------------------*/
void HapticFeedback::update(RhizomeState state, float energy, bool maleConnected, bool femaleConnected) {
    if (!_initialized) return;
    
    _currentState = state;
    _currentEnergy = energy;
    _maleConnected = maleConnected;
    _femaleConnected = femaleConnected;
    
    // Auto-stop generation buzz when fully charged
    if (_generationActive && energy >= 100.0f) {
        _generationActive = false;
        if (_maleReady) stopBuzz(_drvMale);
        Serial.println("[HapticFeedback] Generation buzz stopped (100%)");
    }
}

/*------------------------------------------------------------------------------
 * Event Handlers
 *----------------------------------------------------------------------------*/
void HapticFeedback::onStateChange(RhizomeState oldState, RhizomeState newState) {
    if (!_initialized) return;
    
    // Entering GENERATING - start continuous buzz on ONE motor only (power safety)
    if (newState == RhizomeState::GENERATING && oldState != RhizomeState::GENERATING) {
        Serial.println("[HapticFeedback] GENERATING start - buzz on");
        _generationActive = true;
        // Only male motor buzzes to avoid power spike from both motors
        if (_maleReady) startBuzz(_drvMale);
    }
    
    // Leaving GENERATING - stop buzz
    if (oldState == RhizomeState::GENERATING && newState != RhizomeState::GENERATING) {
        _generationActive = false;
        if (_maleReady) stopBuzz(_drvMale);
        Serial.println("[HapticFeedback] GENERATING ended - buzz stopped");
    }
}

void HapticFeedback::onAllRhizomesFull() {
    if (!_initialized) return;
    
    Serial.println("[HapticFeedback] All rhizomes full - celebration!");
    
    // Stop generation buzz
    _generationActive = false;
    if (_maleReady) stopBuzz(_drvMale);
    if (_femaleReady) stopBuzz(_drvFemale);
    
    // Single click on male motor only (avoid power spike)
    if (_maleReady) playClick(_drvMale);
}

void HapticFeedback::onHeartbeat() {
    if (!_initialized) return;
    if (millis() < STARTUP_DELAY_MS) return;
    if (!shouldPlayHeartbeat()) return;
    
    // Determine which ports should play heartbeat
    bool playMale = !_maleConnected;
    bool playFemale = !_femaleConnected;
    
    // If both disconnected (IDLE), alternate between motors to avoid power spike
    if (playMale && playFemale) {
        if (_useFirstMotor && _maleReady) {
            playClick(_drvMale);
        } else if (!_useFirstMotor && _femaleReady) {
            playClick(_drvFemale);
        }
        _useFirstMotor = !_useFirstMotor;
    }
    // If only one disconnected, play on that one
    else if (playMale && _maleReady) {
        playClick(_drvMale);
    }
    else if (playFemale && _femaleReady) {
        playClick(_drvFemale);
    }
}

bool HapticFeedback::shouldPlayHeartbeat() const {
    // No heartbeat when dead
    if (_currentState == RhizomeState::DEAD) return false;
    
    // No heartbeat when middleman
    if (_currentState == RhizomeState::MIDDLEMAN) return false;
    
    // No heartbeat when generating (continuous buzz instead)
    if (_generationActive) return false;
    
    // No heartbeat at 0% energy
    if (_currentEnergy <= 0) return false;
    
    return true;
}

/*------------------------------------------------------------------------------
 * Motor Control
 *----------------------------------------------------------------------------*/
void HapticFeedback::playClick(Adafruit_DRV2605& drv) {
    drv.setWaveform(0, HapticEffects::CLICK);
    drv.setWaveform(1, HapticEffects::END);
    drv.go();
}

void HapticFeedback::playDoubleClick(Adafruit_DRV2605& drv) {
    drv.setWaveform(0, HapticEffects::SHARP_CLICK);
    drv.setWaveform(1, HapticEffects::SHARP_CLICK);
    drv.setWaveform(2, HapticEffects::END);
    drv.go();
}

void HapticFeedback::startBuzz(Adafruit_DRV2605& drv) {
    drv.setWaveform(0, HapticEffects::LONG_BUZZ);
    drv.setWaveform(1, HapticEffects::END);
    drv.go();
}

void HapticFeedback::stopBuzz(Adafruit_DRV2605& drv) {
    drv.stop();
}
