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
    , _lastBuzzRefresh(0)
    , _useFirstMotor(true)
    , _glouGlouActive(false)
    , _lastGlouGlouPulse(0)
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
    
    unsigned long now = millis();
    
    // Auto-stop generation buzz when fully charged
    if (_generationActive && energy >= 100.0f) {
        _generationActive = false;
        if (_maleReady) stopBuzz(_drvMale);
        if (_femaleReady) stopBuzz(_drvFemale);
    }
    
    // Periodic buzz refresh during GENERATING (waveform-based, safe)
    if (_generationActive && (now - _lastBuzzRefresh >= BUZZ_REFRESH_MS)) {
        if (_maleReady) startBuzz(_drvMale);
        if (_femaleReady) startBuzz(_drvFemale);
        _lastBuzzRefresh = now;
    }
    
    // Update glou-glou pulsing during GIVING state
    // Interval based on energy: high energy = slow, low energy = fast
    if (GLOU_ENABLED && _glouGlouActive && _maleReady) {
        unsigned long interval = getGlouGlouInterval(energy);
        if (now - _lastGlouGlouPulse >= interval) {
            playSoftClick(_drvMale);  // Use soft click to reduce power draw
            _lastGlouGlouPulse = now;
        }
    }
}

/*------------------------------------------------------------------------------
 * Event Handlers
 *----------------------------------------------------------------------------*/
void HapticFeedback::onStateChange(RhizomeState oldState, RhizomeState newState) {
    if (!_initialized) return;
    
    // Entering GENERATING - start buzz on both motors
    if (newState == RhizomeState::GENERATING && oldState != RhizomeState::GENERATING) {
        _generationActive = true;
        _lastBuzzRefresh = millis();
        if (_maleReady) startBuzz(_drvMale);
        if (_femaleReady) startBuzz(_drvFemale);
    }
    
    // Leaving GENERATING - stop buzz on both motors
    if (oldState == RhizomeState::GENERATING && newState != RhizomeState::GENERATING) {
        _generationActive = false;
        if (_maleReady) stopBuzz(_drvMale);
        if (_femaleReady) stopBuzz(_drvFemale);
    }
    
    // Entering GIVING - start glou-glou pulsing (with initial delay to avoid power spike)
    if (newState == RhizomeState::GIVING && oldState != RhizomeState::GIVING) {
        _glouGlouActive = true;
        _lastGlouGlouPulse = millis();  // Initial delay handled by interval calculation
    }
    
    // Leaving GIVING - stop glou-glou immediately
    if (oldState == RhizomeState::GIVING && newState != RhizomeState::GIVING) {
        _glouGlouActive = false;
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

void HapticFeedback::onMaleConnected() {
    if (!_initialized || !_maleReady) return;
    playTick(_drvMale);
}

void HapticFeedback::onFemaleConnected() {
    if (!_initialized || !_femaleReady) return;
    playTick(_drvFemale);
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
    // Heartbeat disabled to save battery
    return false;
}

/*------------------------------------------------------------------------------
 * Motor Control
 *----------------------------------------------------------------------------*/
void HapticFeedback::playClick(Adafruit_DRV2605& drv) {
    drv.setWaveform(0, HapticEffects::CLICK);
    drv.setWaveform(1, HapticEffects::END);
    drv.go();
}

void HapticFeedback::playSoftClick(Adafruit_DRV2605& drv) {
    drv.setWaveform(0, HapticEffects::SOFT_CLICK);
    drv.setWaveform(1, HapticEffects::END);
    drv.go();
}

void HapticFeedback::playTick(Adafruit_DRV2605& drv) {
    drv.setWaveform(0, HapticEffects::TICK);
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
    // Use waveform mode with soft fuzz - safe and non-blocking
    drv.setWaveform(0, HapticEffects::SOFT_BUZZ);
    drv.setWaveform(1, HapticEffects::END);
    drv.go();
}

void HapticFeedback::stopBuzz(Adafruit_DRV2605& drv) {
    drv.stop();
}

unsigned long HapticFeedback::getGlouGlouInterval(float energy) const {
    // Clamp energy to 0-100
    float e = constrain(energy, 0.0f, 100.0f);
    
    // Normalize to 0-1
    float t = e / 100.0f;
    
    // Ease-in (quadratic): slow acceleration at low energy, fast at high
    // t^2 gives us: 0% -> 0, 50% -> 0.25, 100% -> 1
    float eased = t * t;
    
    // Map: 0 (low energy) -> MIN interval (fast), 1 (high energy) -> MAX interval (slow)
    return GLOU_MIN_INTERVAL_MS + (unsigned long)(eased * (GLOU_MAX_INTERVAL_MS - GLOU_MIN_INTERVAL_MS));
}
