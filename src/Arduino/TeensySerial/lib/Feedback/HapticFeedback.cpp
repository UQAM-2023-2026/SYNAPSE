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
    , _lastGlouGlouPulse(0)
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
        if (_maleReady) stopContinuousBuzz(_drvMale);
        if (_femaleReady) stopContinuousBuzz(_drvFemale);
        Serial.println("[HapticFeedback] Generation buzz stopped (100%)");
    }
    
    // GIVING state: update glou glou effect
    if (state == RhizomeState::GIVING) {
        updateGiving();
    }
}

/*------------------------------------------------------------------------------
 * Event Handlers
 *----------------------------------------------------------------------------*/
void HapticFeedback::onStateChange(RhizomeState oldState, RhizomeState newState) {
    if (!_initialized) return;
    
    // Entering GENERATING - start continuous buzz using RTP mode on both motors
    if (newState == RhizomeState::GENERATING && oldState != RhizomeState::GENERATING) {
        Serial.println("[HapticFeedback] GENERATING start - RTP continuous buzz on both motors");
        _generationActive = true;
        if (_maleReady) startContinuousBuzz(_drvMale);
        if (_femaleReady) startContinuousBuzz(_drvFemale);
    }
    
    // Leaving GENERATING - stop buzz on both motors
    if (oldState == RhizomeState::GENERATING && newState != RhizomeState::GENERATING) {
        _generationActive = false;
        if (_maleReady) stopContinuousBuzz(_drvMale);
        if (_femaleReady) stopContinuousBuzz(_drvFemale);
        Serial.println("[HapticFeedback] GENERATING ended - buzz stopped");
    }
    
    // Entering GIVING - reset pulse timer
    if (newState == RhizomeState::GIVING && oldState != RhizomeState::GIVING) {
        _lastGlouGlouPulse = millis();
    }
}

void HapticFeedback::onMaleConnect() {
    if (!_initialized) return;
    if (millis() < STARTUP_DELAY_MS) return;
    
    // Play connection effect on MALE motor only
    if (_maleReady) {
        playConnectionEffect(_drvMale);
        Serial.println("[HapticFeedback] MALE connection - effect 47");
    }
}

void HapticFeedback::onFemaleConnect() {
    if (!_initialized) return;
    if (millis() < STARTUP_DELAY_MS) return;
    
    // Play connection effect on FEMALE motor only
    if (_femaleReady) {
        playConnectionEffect(_drvFemale);
        Serial.println("[HapticFeedback] FEMALE connection - effect 47");
    }
}

void HapticFeedback::onAllRhizomesFull() {
    if (!_initialized) return;
    
    Serial.println("[HapticFeedback] All rhizomes full - celebration!");
    
    // Stop generation buzz
    _generationActive = false;
    if (_maleReady) stopContinuousBuzz(_drvMale);
    if (_femaleReady) stopContinuousBuzz(_drvFemale);
    
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
    // No heartbeat in IDLE - per specification
    if (_currentState == RhizomeState::IDLE) return false;
    
    // No heartbeat when dead
    if (_currentState == RhizomeState::DEAD) return false;
    
    // No heartbeat when middleman
    if (_currentState == RhizomeState::MIDDLEMAN) return false;
    
    // No heartbeat when generating (continuous buzz instead)
    if (_generationActive) return false;
    
    // No heartbeat when GIVING (glou glou effect instead)
    if (_currentState == RhizomeState::GIVING) return false;
    
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

void HapticFeedback::playConnectionEffect(Adafruit_DRV2605& drv) {
    drv.setWaveform(0, HapticEffects::CONNECTION);
    drv.setWaveform(1, HapticEffects::END);
    drv.go();
}

void HapticFeedback::playGlouGlouPulse(Adafruit_DRV2605& drv) {
    // Soft short pulse for glou glou effect
    drv.setWaveform(0, HapticEffects::CLICK);
    drv.setWaveform(1, HapticEffects::END);
    drv.go();
}

void HapticFeedback::startContinuousBuzz(Adafruit_DRV2605& drv) {
    // Switch to RTP (Real-Time Playback) mode for truly continuous vibration
    drv.setMode(DRV2605_MODE_REALTIME);
    drv.setRealtimeValue(RTP_BUZZ_AMPLITUDE);
}

void HapticFeedback::stopContinuousBuzz(Adafruit_DRV2605& drv) {
    // Stop RTP and restore to internal trigger mode
    drv.setRealtimeValue(0);
    drv.setMode(DRV2605_MODE_INTTRIG);
}

/*------------------------------------------------------------------------------
 * GIVING State: Glou Glou Effect
 *----------------------------------------------------------------------------*/
void HapticFeedback::updateGiving() {
    if (!_initialized) return;
    
    uint32_t now = millis();
    uint32_t interval = calculateGlouGlouInterval();
    
    if (now - _lastGlouGlouPulse >= interval) {
        _lastGlouGlouPulse = now;
        
        // Play pulse on male motor (connected to node)
        if (_maleReady) {
            playGlouGlouPulse(_drvMale);
        }
    }
}

uint32_t HapticFeedback::calculateGlouGlouInterval() const {
    // Smooth ease-in interpolation based on energy
    // 100% -> long interval (slow pulses)
    // 0% -> short interval (fast pulses)
    
    float normalized = _currentEnergy / 100.0f;
    if (normalized < 0.0f) normalized = 0.0f;
    if (normalized > 1.0f) normalized = 1.0f;
    
    // Ease-in: use quadratic curve (more pulses as energy drops)
    float eased = normalized * normalized;
    
    // Map to interval range
    uint32_t interval = GLOU_MIN_INTERVAL_MS + 
        (uint32_t)(eased * (float)(GLOU_MAX_INTERVAL_MS - GLOU_MIN_INTERVAL_MS));
    
    return interval;
}
