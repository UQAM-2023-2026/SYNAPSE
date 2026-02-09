/*==============================================================================
 * LedFeedback.cpp - LED strip feedback implementation
 *============================================================================*/

#include "LedFeedback.h"

// Global instance
LedFeedback ledFeedback;

// Callback wrapper for HeartbeatSystem
void ledHeartbeatCallback() {
    ledFeedback.onHeartbeat();
}

/*------------------------------------------------------------------------------
 * Constructor
 *----------------------------------------------------------------------------*/
LedFeedback::LedFeedback()
    : _currentState(RhizomeState::IDLE)
    , _currentEnergy(0)
    , _maleConnected(false)
    , _femaleConnected(false)
    , _pulsePhase(0)
    , _pulseBrightness(LedConfig::PULSE_BASE)
    , _pulseOffset(0.0f)
    , _pulseStartMs(0)
    , _pulseActive(false)
    , _flowType(FlowType::NONE)
    , _lastFlowUpdateMs(0)
    , _lastPacketSpawnMs(0)
    , _lockedLedCount(0)
    , _lastGaugeUpdateMs(0)
{}

/*------------------------------------------------------------------------------
 * Initialization
 *----------------------------------------------------------------------------*/
void LedFeedback::begin() {
    Serial.println("[LedFeedback] Initializing...");
    
    // APA102 on pins 12 (data) and 13 (clock)
    FastLED.addLeds<APA102, 12, 13, BGR, DATA_RATE_MHZ(1)>(_leds, LedConfig::NUM_LEDS);
    FastLED.setBrightness(255);
    
    clearAll();
    FastLED.show();
    
    Serial.println("[LedFeedback] Ready");
}

/*------------------------------------------------------------------------------
 * Main Update
 *----------------------------------------------------------------------------*/
void LedFeedback::update(RhizomeState state, float energy, bool maleConnected, bool femaleConnected) {
    _currentState = state;
    _currentEnergy = energy;
    _maleConnected = maleConnected;
    _femaleConnected = femaleConnected;
    
    // Handle DEAD state - all off
    if (state == RhizomeState::DEAD) {
        clearAll();
        FastLED.show();
        return;
    }
    
    // Start fresh
    clearAll();
    
    // Update flow type based on state
    switch (state) {
        case RhizomeState::IDLE:
            setFlowType(FlowType::NONE);
            break;
        case RhizomeState::DISCOVERING:
            setFlowType(FlowType::SEEKING);
            break;
        case RhizomeState::GENERATING:
            setFlowType(FlowType::INWARD);
            break;
        case RhizomeState::GIVING:
            setFlowType(FlowType::OUTWARD);
            break;
        case RhizomeState::MIDDLEMAN:
            setFlowType(FlowType::PASSTHROUGH);
            break;
        default:
            setFlowType(FlowType::NONE);
            break;
    }
    
    // MIDDLEMAN: Solid orange with passthrough, no pulse
    if (state == RhizomeState::MIDDLEMAN) {
        setAllSolid(255);
        updateFlow();
        FastLED.show();
        return;
    }
    
    // Normal states: Gauge + Flow + Pulse
    updateGauge(energy);
    
    if (state == RhizomeState::GENERATING) {
        updatePackets(energy);
    } else {
        updateFlow();
    }
    
    updatePulse();
    applyPulse();
    
    FastLED.show();
}

/*------------------------------------------------------------------------------
 * Event Handlers
 *----------------------------------------------------------------------------*/
void LedFeedback::onHeartbeat() {
    _pulseActive = true;
    _pulseStartMs = millis();
    // Don't set _pulseBrightness here - updatePulse() calculates it
}

void LedFeedback::onStateChange(RhizomeState oldState, RhizomeState newState) {
    // Reset flow particles on state change
    for (int i = 0; i < MAX_PARTICLES; i++) {
        _particles[i].active = false;
    }
    
    // Reset packets when leaving GENERATING
    if (oldState == RhizomeState::GENERATING) {
        for (int i = 0; i < MAX_PACKETS; i++) {
            _packets[i].active = false;
            _packets[i].locked = false;
        }
        _lockedLedCount = 0;
    }
}

/*------------------------------------------------------------------------------
 * Gauge Layer - Shows energy level MALE→FEMALE direction
 * NOTE: No timing gate - must render every frame after clearAll()
 *----------------------------------------------------------------------------*/
void LedFeedback::updateGauge(float energy) {
    // MALE end is LED 0, FEMALE end is LED NUM_LEDS-1
    // Energy fills from MALE toward FEMALE
    int filledLeds = (int)((energy / 100.0f) * LedConfig::NUM_LEDS);
    
    for (int i = 0; i < filledLeds && i < LedConfig::NUM_LEDS; i++) {
        _leds[i] = getColor(255);
    }
    
    // Partial LED for smooth transition
    if (filledLeds < LedConfig::NUM_LEDS) {
        float fraction = ((energy / 100.0f) * LedConfig::NUM_LEDS) - filledLeds;
        if (fraction > 0) {
            _leds[filledLeds] = getColor((uint8_t)(255 * fraction));
        }
    }
}

int LedFeedback::getGaugeEdgeLed(float energy) {
    // Returns the LED index at the edge of the gauge
    return (int)((energy / 100.0f) * LedConfig::NUM_LEDS);
}

/*------------------------------------------------------------------------------
 * Flow Layer - State-dependent particle animation
 *----------------------------------------------------------------------------*/
void LedFeedback::setFlowType(FlowType type) {
    if (_flowType != type) {
        _flowType = type;
        // Clear particles when flow type changes
        for (int i = 0; i < MAX_PARTICLES; i++) {
            _particles[i].active = false;
        }
    }
}

void LedFeedback::updateFlow() {
    if (_flowType == FlowType::NONE) return;
    
    // Gate particle state updates (spawning, movement) but always render
    if (millis() - _lastFlowUpdateMs >= LedConfig::FLOW_UPDATE_MS) {
        _lastFlowUpdateMs = millis();
        updateParticles();
        spawnParticle();
    }
    
    // Always render particles every frame
    renderParticles();
}

void LedFeedback::spawnParticle() {
    // Spawn rate based on flow type
    int spawnChance;
    switch (_flowType) {
        case FlowType::SEEKING: spawnChance = 10; break;  // Rare
        case FlowType::INWARD: spawnChance = 30; break;
        case FlowType::OUTWARD: spawnChance = 30; break;
        case FlowType::PASSTHROUGH: spawnChance = 50; break;
        default: return;
    }
    
    if (random(100) >= spawnChance) return;
    
    // Find inactive particle
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!_particles[i].active) {
            _particles[i].active = true;
            _particles[i].brightness = 200 + random(55);
            
            switch (_flowType) {
                case FlowType::SEEKING:
                    // FEMALE→MALE direction (high→low LED)
                    _particles[i].position = LedConfig::NUM_LEDS - 1;
                    _particles[i].speed = -LedConfig::SEEKING_SPEED;
                    break;
                    
                case FlowType::INWARD:
                    // FEMALE→gauge (high→middle)
                    _particles[i].position = LedConfig::NUM_LEDS - 1;
                    _particles[i].speed = -LedConfig::INWARD_SPEED;
                    break;
                    
                case FlowType::OUTWARD:
                    // Gauge→MALE (middle→low)
                    _particles[i].position = getGaugeEdgeLed(_currentEnergy);
                    _particles[i].speed = -LedConfig::OUTWARD_SPEED;
                    break;
                    
                case FlowType::PASSTHROUGH:
                    // FEMALE→MALE (high→low)
                    _particles[i].position = LedConfig::NUM_LEDS - 1;
                    _particles[i].speed = -LedConfig::PASSTHROUGH_SPEED;
                    break;
                    
                default:
                    break;
            }
            break;
        }
    }
}

void LedFeedback::updateParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!_particles[i].active) continue;
        
        _particles[i].position += _particles[i].speed;
        
        // Check bounds
        if (_particles[i].position < 0 || _particles[i].position >= LedConfig::NUM_LEDS) {
            _particles[i].active = false;
            continue;
        }
        
        // For INWARD flow, stop at gauge edge
        if (_flowType == FlowType::INWARD) {
            int gaugeEdge = getGaugeEdgeLed(_currentEnergy);
            if (_particles[i].position <= gaugeEdge) {
                _particles[i].active = false;
            }
        }
    }
}

void LedFeedback::renderParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!_particles[i].active) continue;
        
        int led = (int)_particles[i].position;
        if (led >= 0 && led < LedConfig::NUM_LEDS) {
            // Add to existing LED (flow on top of gauge)
            CRGB flowColor = getColor(_particles[i].brightness);
            _leds[led] += flowColor;
        }
    }
}

/*------------------------------------------------------------------------------
 * Packet Layer - Tetris-style generation
 *----------------------------------------------------------------------------*/
void LedFeedback::updatePackets(float energy) {
    if (millis() - _lastFlowUpdateMs < LedConfig::FLOW_UPDATE_MS) return;
    _lastFlowUpdateMs = millis();
    
    // Calculate how many packets we should have locked based on energy
    int targetLockedLeds = getGaugeEdgeLed(energy);
    
    // Spawn new packet if needed and not at 100%
    if (energy < 100.0f && millis() - _lastPacketSpawnMs > 500) {
        // Find inactive packet
        for (int i = 0; i < MAX_PACKETS; i++) {
            if (!_packets[i].active) {
                // Target the next unfilled LED
                int targetLed = _lockedLedCount;
                if (targetLed < LedConfig::NUM_LEDS) {
                    _packets[i].active = true;
                    _packets[i].locked = false;
                    _packets[i].position = LedConfig::NUM_LEDS - 1;  // Start from FEMALE end
                    _packets[i].targetLed = targetLed;
                    _lastPacketSpawnMs = millis();
                }
                break;
            }
        }
    }
    
    updatePacketPositions();
    renderPackets();
}

void LedFeedback::updatePacketPositions() {
    for (int i = 0; i < MAX_PACKETS; i++) {
        if (!_packets[i].active || _packets[i].locked) continue;
        
        // Move toward target
        _packets[i].position -= LedConfig::PACKET_SPEED;
        
        // Check if reached target (lock in place like Tetris)
        if (_packets[i].position <= _packets[i].targetLed) {
            _packets[i].position = _packets[i].targetLed;
            _packets[i].locked = true;
            _lockedLedCount = max(_lockedLedCount, _packets[i].targetLed + 1);
        }
    }
}

void LedFeedback::renderPackets() {
    for (int i = 0; i < MAX_PACKETS; i++) {
        if (!_packets[i].active) continue;
        
        int startLed = (int)_packets[i].position;
        
        // Render packet (3 LEDs)
        for (int j = 0; j < LedConfig::PACKET_SIZE; j++) {
            int led = startLed + j;
            if (led >= 0 && led < LedConfig::NUM_LEDS) {
                // Locked packets are part of gauge, moving packets are brighter
                uint8_t brightness = _packets[i].locked ? 255 : 200;
                _leds[led] = getColor(brightness);
            }
        }
    }
}

/*------------------------------------------------------------------------------
 * Pulse Layer - Lerp toward white on heartbeat
 * Baseline: LEDs at PULSE_BASE brightness (85%)
 * On heartbeat: blend toward white by pulseOffset amount
 * Preserves hue - no RGB clipping or channel imbalance
 *----------------------------------------------------------------------------*/
void LedFeedback::updatePulse() {
    // Pulse duration ~300ms
    const uint32_t PULSE_DURATION_MS = 300;
    const float ATTACK_FRACTION = 0.1f;  // 10% attack, 90% decay
    
    if (!_pulseActive) {
        _pulseBrightness = LedConfig::PULSE_BASE;  // Baseline brightness
        _pulseOffset = 0.0f;  // No white blend
        return;
    }
    
    uint32_t elapsed = millis() - _pulseStartMs;
    
    if (elapsed >= PULSE_DURATION_MS) {
        _pulseActive = false;
        _pulseBrightness = LedConfig::PULSE_BASE;
        _pulseOffset = 0.0f;
        return;
    }
    
    // Calculate lerp offset toward white (0 → ACCENT → 0)
    float progress = (float)elapsed / PULSE_DURATION_MS;
    
    if (progress < ATTACK_FRACTION) {
        // Fast attack: 0 → ACCENT
        _pulseOffset = LedConfig::PULSE_ACCENT * (progress / ATTACK_FRACTION);
    } else {
        // Slow decay: ACCENT → 0
        float decayProgress = (progress - ATTACK_FRACTION) / (1.0f - ATTACK_FRACTION);
        _pulseOffset = LedConfig::PULSE_ACCENT * (1.0f - decayProgress);
    }
    
    // Brightness stays at base during pulse (white blend handles the accent)
    _pulseBrightness = LedConfig::PULSE_BASE;
}

void LedFeedback::applyPulse() {
    // Step 1: Apply base brightness (multiplicative dim)
    for (int i = 0; i < LedConfig::NUM_LEDS; i++) {
        _leds[i].r = (uint8_t)(_leds[i].r * _pulseBrightness);
        _leds[i].g = (uint8_t)(_leds[i].g * _pulseBrightness);
        _leds[i].b = (uint8_t)(_leds[i].b * _pulseBrightness);
    }
    
    // Step 2: Lerp toward white by pulseOffset (additive bloom without hue shift)
    if (_pulseOffset > 0.001f) {
        uint8_t blendAmount = (uint8_t)(_pulseOffset * 255.0f);
        for (int i = 0; i < LedConfig::NUM_LEDS; i++) {
            // Only blend lit LEDs (skip black pixels)
            if (_leds[i].r > 0 || _leds[i].g > 0 || _leds[i].b > 0) {
                _leds[i] = blend(_leds[i], CRGB::White, blendAmount);
            }
        }
    }
}

/*------------------------------------------------------------------------------
 * Helpers
 *----------------------------------------------------------------------------*/
void LedFeedback::clearAll() {
    fill_solid(_leds, LedConfig::NUM_LEDS, CRGB::Black);
}

void LedFeedback::setAllSolid(uint8_t brightness) {
    fill_solid(_leds, LedConfig::NUM_LEDS, getColor(brightness));
}

CRGB LedFeedback::getColor(uint8_t brightness) {
    float scale = brightness / 255.0f;
    return CRGB(
        (uint8_t)(LedConfig::R * scale),
        (uint8_t)(LedConfig::G * scale),
        (uint8_t)(LedConfig::B * scale)
    );
}
