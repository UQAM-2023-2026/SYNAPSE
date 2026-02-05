/*==============================================================================
 * HeartbeatSystem.cpp - Centralized heartbeat management implementation
 * Project: Synapse - Interactive Rhizome Installation
 *============================================================================*/

#include "HeartbeatSystem.h"

// Global singleton instance
HeartbeatSystem heartbeatSystem;

/*------------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
HeartbeatSystem::HeartbeatSystem()
    : _previousPhase(0.0f),
      _lastUpdate(0),
      _enabled(true),
      _initialized(false),
      _hapticCallback(nullptr),
      _ledCallback(nullptr),
      _ledStripCallback(nullptr)
{
    _phase.phase = 0.0f;
    _phase.normalizedBrightness = 0.0f;
    _phase.newBeatStarted = false;
    _phase.currentBPM = HEARTBEAT_MIN_BPM;
}

/*------------------------------------------------------------------------------
 * Initialization
 *----------------------------------------------------------------------------*/
void HeartbeatSystem::begin() {
    // Setup heartbeat indicator LED
    pinMode(HEARTBEAT_LED_PIN, OUTPUT);
    digitalWrite(HEARTBEAT_LED_PIN, LOW);
    
    _lastUpdate = millis();
    _initialized = true;
    
    Serial.println("[Heartbeat] System initialized");
}

/*------------------------------------------------------------------------------
 * Main Update Loop
 * Call this every loop iteration to maintain accurate timing
 *----------------------------------------------------------------------------*/
bool HeartbeatSystem::update(float energy) {
    if (!_initialized) return false;
    
    uint32_t now = millis();
    float deltaTime = (now - _lastUpdate) / 1000.0f;
    _lastUpdate = now;
    
    // Limit deltaTime to avoid large jumps (e.g., after debugging pause)
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    
    // Reset beat flag
    _phase.newBeatStarted = false;
    
    // Calculate BPM based on energy level
    _phase.currentBPM = calculateBPM(energy);
    
    // Advance phase based on BPM
    float beatsPerSecond = _phase.currentBPM / 60.0f;
    _previousPhase = _phase.phase;
    
    if (_enabled) {
        _phase.phase += beatsPerSecond * deltaTime;
        
        // Detect new beat (phase wrap around)
        while (_phase.phase >= 1.0f) {
            _phase.phase -= 1.0f;
            _phase.newBeatStarted = true;
        }
    }
    
    // Calculate heartbeat brightness curve
    _phase.normalizedBrightness = calculateHeartbeatCurve(_phase.phase);
    
    // Handle heartbeat indicator LED
    triggerHeartbeatLed(_phase.newBeatStarted);
    
    // Notify all registered callbacks
    if (_phase.newBeatStarted || _ledStripCallback != nullptr) {
        notifyCallbacks();
    }
    
    return _phase.newBeatStarted;
}

/*------------------------------------------------------------------------------
 * Heartbeat Curve Calculation
 * 
 * Realistic double-peak heartbeat pattern (lub-dub):
 * - Systole (0.00-0.18): Strong, fast peak (the "lub")
 * - Diastole (0.22-0.38): Weaker, shorter peak (the "dub")
 * - Rest (0.38-1.00): Long pause before next beat
 *----------------------------------------------------------------------------*/
float HeartbeatSystem::calculateHeartbeatCurve(float phase) const {
    if (!_enabled) {
        return 0.5f; // Neutral brightness when disabled
    }
    
    // Systole - Primary peak (LUB)
    if (phase < 0.08f) {
        // Rapid rise
        float t = phase / 0.08f;
        return t * (2.0f - t);  // Ease-out curve, reaches 1.0
    }
    else if (phase < 0.18f) {
        // Descent from systole
        float t = (phase - 0.08f) / 0.10f;
        return 1.0f - t * 0.7f;  // Falls to 0.3
    }
    // Gap between peaks
    else if (phase < 0.22f) {
        return 0.3f;
    }
    // Diastole - Secondary peak (DUB)
    else if (phase < 0.28f) {
        // Rise to secondary peak (weaker than systole)
        float t = (phase - 0.22f) / 0.06f;
        return 0.3f + t * 0.3f;  // Rises to 0.6
    }
    else if (phase < 0.38f) {
        // Descent from diastole
        float t = (phase - 0.28f) / 0.10f;
        return 0.6f - t * 0.45f;  // Falls to 0.15
    }
    // Rest phase
    else {
        // Long rest with slight breathing effect
        float t = (phase - 0.38f) / 0.62f;
        return 0.15f + 0.05f * sin(t * PI);  // Very subtle breathing, 0.15-0.20
    }
}

/*------------------------------------------------------------------------------
 * BPM Calculation based on energy
 *----------------------------------------------------------------------------*/
uint8_t HeartbeatSystem::calculateBPM(float energy) const {
    float clampedEnergy = constrain(energy, 0.0f, 100.0f);
    return map((uint8_t)clampedEnergy, 0, 100, HEARTBEAT_MIN_BPM, HEARTBEAT_MAX_BPM);
}

/*------------------------------------------------------------------------------
 * Heartbeat LED Control (Pin 2)
 * Toggles on each new beat, providing visual sync verification
 *----------------------------------------------------------------------------*/
void HeartbeatSystem::triggerHeartbeatLed(bool newBeat) {
    static bool ledState = false;
    
    if (newBeat && _enabled) {
        ledState = !ledState;
        digitalWrite(HEARTBEAT_LED_PIN, ledState ? HIGH : LOW);
    } else if (!_enabled && ledState) {
        // Turn off LED when disabled
        ledState = false;
        digitalWrite(HEARTBEAT_LED_PIN, LOW);
    }
}

/*------------------------------------------------------------------------------
 * Callback Notifications
 *----------------------------------------------------------------------------*/
void HeartbeatSystem::notifyCallbacks() {
    // Notify haptic system at beat start
    if (_phase.newBeatStarted && _hapticCallback != nullptr) {
        _hapticCallback();
    }
    
    // Notify LED system at beat start (simple callback)
    if (_phase.newBeatStarted && _ledCallback != nullptr) {
        _ledCallback();
    }
    
    // Always notify LED strip with current phase (for smooth animation)
    if (_ledStripCallback != nullptr) {
        _ledStripCallback(_phase);
    }
}

void HeartbeatSystem::setHapticCallback(HeartbeatCallback callback) {
    _hapticCallback = callback;
}

void HeartbeatSystem::setLedCallback(HeartbeatCallback callback) {
    _ledCallback = callback;
}

void HeartbeatSystem::setLedStripCallback(HeartbeatPhaseCallback callback) {
    _ledStripCallback = callback;
}

/*------------------------------------------------------------------------------
 * Control Methods
 *----------------------------------------------------------------------------*/
void HeartbeatSystem::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!enabled) {
        // Reset LED when disabled
        digitalWrite(HEARTBEAT_LED_PIN, LOW);
    }
}

void HeartbeatSystem::forceSync(float phase) {
    _phase.phase = constrain(phase, 0.0f, 0.9999f);
    _previousPhase = _phase.phase;
}

void HeartbeatSystem::setBPM(uint8_t bpm) {
    _phase.currentBPM = constrain(bpm, HEARTBEAT_MIN_BPM, HEARTBEAT_MAX_BPM);
}

/*------------------------------------------------------------------------------
 * Debug Output
 *----------------------------------------------------------------------------*/
void HeartbeatSystem::printDebug(Stream& output) {
    output.print("[Heartbeat] Phase: ");
    output.print(_phase.phase, 3);
    output.print(" BPM: ");
    output.print(_phase.currentBPM);
    output.print(" Brightness: ");
    output.print(_phase.normalizedBrightness, 2);
    output.print(" Enabled: ");
    output.println(_enabled ? "YES" : "NO");
}
