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
        if (_femaleReady) stopBuzz(_drvFemale);
        Serial.println("[HapticFeedback] Generation buzz stopped (100%)");
    }
}

/*------------------------------------------------------------------------------
 * Event Handlers
 *----------------------------------------------------------------------------*/
void HapticFeedback::onStateChange(RhizomeState oldState, RhizomeState newState) {
    if (!_initialized) return;
    
    // Entering GENERATING - double click + start buzz
    if (newState == RhizomeState::GENERATING && oldState != RhizomeState::GENERATING) {
        Serial.println("[HapticFeedback] GENERATING start - double click + buzz");
        
        if (_maleReady) playDoubleClick(_drvMale);
        if (_femaleReady) playDoubleClick(_drvFemale);
        
        _generationActive = true;
        
        // Start buzz after short delay for clicks
        delay(100);
        if (_maleReady) startBuzz(_drvMale);
        if (_femaleReady) startBuzz(_drvFemale);
    }
    
    // Leaving GENERATING - stop buzz
    if (oldState == RhizomeState::GENERATING && newState != RhizomeState::GENERATING) {
        _generationActive = false;
        if (_maleReady) stopBuzz(_drvMale);
        if (_femaleReady) stopBuzz(_drvFemale);
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
    
    // Double click celebration
    if (_maleReady) playDoubleClick(_drvMale);
    if (_femaleReady) playDoubleClick(_drvFemale);
}

void HapticFeedback::onHeartbeat() {
    if (!_initialized) return;
    if (millis() < STARTUP_DELAY_MS) return;
    if (!shouldPlayHeartbeat()) return;
    
    // Heartbeat on unconnected ports
    // In IDLE (both disconnected): both play
    // Otherwise: only unconnected port plays
    
    bool playMale = !_maleConnected;
    bool playFemale = !_femaleConnected;
    
    // If both disconnected (IDLE), both play
    if (!_maleConnected && !_femaleConnected) {
        playMale = true;
        playFemale = true;
    }
    
    if (playMale && _maleReady) {
        playHeartbeat(_drvMale);
    }
    if (playFemale && _femaleReady) {
        playHeartbeat(_drvFemale);
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
void HapticFeedback::playHeartbeat(Adafruit_DRV2605& drv) {
    drv.setWaveform(0, HapticEffects::SYSTOLE);
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
