/*==============================================================================
 * LedFeedback.cpp - LED strip feedback implementation
 *
 * Implements all LED behaviors per state with proper hardware configuration.
 * Uses callbacks, no delay(), single FastLED.show() per frame.
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
    , _displayedEnergy(0)
    , _maleConnected(false)
    , _femaleConnected(false)
    , _lastFrameMs(0)
    , _pulseActive(false)
    , _pulseStartMs(0)
    , _pulsePhase(0)
    , _cycleStartMs(0)
    , _flashActive(false)
    , _flashStartMs(0)
    , _flashCycleCount(0)
    , _flowPosition(0)
    , _lastAnimMs(0)
    , _maleAnim(EmboutAnim::OFF)
    , _femaleAnim(EmboutAnim::OFF)
{}

/*------------------------------------------------------------------------------
 * Initialization
 *----------------------------------------------------------------------------*/
void LedFeedback::begin() {
    Serial.println("[LedFeedback] Initializing 3 strips...");
    
    // Male embout: WS2812B on pin 1, 5 LEDs
    FastLED.addLeds<WS2812B, LedConfig::PIN_MALE, GRB>(_ledsMale, LedConfig::EMBOUT_LEDS);
    
    // Female embout: WS2812B on pin 0, 5 LEDs
    FastLED.addLeds<WS2812B, LedConfig::PIN_FEMALE, GRB>(_ledsFemale, LedConfig::EMBOUT_LEDS);
    
    // Tube: APA102 on pins 12/13, 15 LEDs
    FastLED.addLeds<APA102, LedConfig::PIN_TUBE_DATA, LedConfig::PIN_TUBE_CLOCK, BGR, DATA_RATE_MHZ(1)>(
        _ledsTube, LedConfig::TUBE_LEDS);
    
    FastLED.setBrightness(255);
    clearAll();
    FastLED.show();
    
    _cycleStartMs = millis();
    _lastAnimMs = millis();
    
    Serial.println("[LedFeedback] Ready - Male(pin1), Female(pin0), Tube(APA102)");
}

/*------------------------------------------------------------------------------
 * Main Update (frame-rate limited to ~30 FPS)
 *----------------------------------------------------------------------------*/
void LedFeedback::update(RhizomeState state, float energy, bool maleConnected, bool femaleConnected) {
    // Frame rate limiter - only update LEDs every ~33ms (30 FPS)
    unsigned long now = millis();
    if (now - _lastFrameMs < 33) {
        return;  // Skip this frame, don't call FastLED.show()
    }
    _lastFrameMs = now;
    
    _currentState = state;
    _currentEnergy = energy;
    _maleConnected = maleConnected;
    _femaleConnected = femaleConnected;
    
    // Handle DEAD state - all off
    if (state == RhizomeState::DEAD) {
        clearAll();
        FastLED.setBrightness(0);
        FastLED.show();
        return;
    }
    
    // Handle flash event (overrides everything)
    if (_flashActive) {
        renderFlash();
        FastLED.show();
        return;
    }
    
    // Update animations
    updatePulse();
    updateFlow();
    updateGaugeDrain();
    updateAnimationStates();
    
    // Clear and render
    clearAll();
    
    // Set global brightness based on energy
    FastLED.setBrightness(getGlobalBrightness());
    
    // Render embouts
    renderEmbout(_ledsMale, _maleAnim);
    renderEmbout(_ledsFemale, _femaleAnim);
    
    // Render tube based on state
    renderTube();
    
    FastLED.show();
}

/*------------------------------------------------------------------------------
 * Callbacks
 *----------------------------------------------------------------------------*/
void LedFeedback::onHeartbeat() {
    _pulseActive = true;
    _pulseStartMs = millis();
    _pulsePhase = 0.0f;
}

void LedFeedback::onStateChange(RhizomeState oldState, RhizomeState newState) {
    // Reset flow position on state change
    _flowPosition = 0;
    
    // Sync displayed energy to actual on state change
    if (newState != RhizomeState::GIVING) {
        _displayedEnergy = _currentEnergy;
    }
}

void LedFeedback::onAllRhizomesFull() {
    // Trigger the 100% celebration flash
    if (!_flashActive) {
        _flashActive = true;
        _flashStartMs = millis();
        _flashCycleCount = 0;
        Serial.println("[LedFeedback] Flash event triggered!");
    }
}

/*------------------------------------------------------------------------------
 * Animation State Machine
 *----------------------------------------------------------------------------*/
void LedFeedback::updateAnimationStates() {
    switch (_currentState) {
        case RhizomeState::IDLE:
            // Both embouts pulse ORANGE→WHITE
            // If energy > 50%, male cycles CYAN↔PURPLE
            if (_currentEnergy > 50.0f) {
                _maleAnim = EmboutAnim::CYCLE_INFRA_SUPRA;
            } else {
                _maleAnim = EmboutAnim::IDLE_PULSE;
            }
            _femaleAnim = EmboutAnim::IDLE_PULSE;
            break;
            
        case RhizomeState::DISCOVERING:
            // Connected embout = fixed orange, non-connected = continues IDLE animation
            if (_maleConnected) {
                _maleAnim = EmboutAnim::FIXED_ORANGE;
            } else {
                // Continue IDLE behavior (cycle if energy > 50%)
                _maleAnim = (_currentEnergy > 50.0f) ? EmboutAnim::CYCLE_INFRA_SUPRA : EmboutAnim::IDLE_PULSE;
            }
            _femaleAnim = _femaleConnected ? EmboutAnim::FIXED_ORANGE : EmboutAnim::IDLE_PULSE;
            break;
            
        case RhizomeState::GENERATING:
            // Both embouts = fixed orange
            _maleAnim = EmboutAnim::FIXED_ORANGE;
            _femaleAnim = EmboutAnim::FIXED_ORANGE;
            break;
            
        case RhizomeState::GIVING:
            // Male = fixed orange (connected to node)
            // Female = idle (not connected in giving)
            _maleAnim = EmboutAnim::FIXED_ORANGE;
            _femaleAnim = EmboutAnim::IDLE_PULSE;
            break;
            
        case RhizomeState::MIDDLEMAN:
            // Female sends /node to next rhizome = fixed orange
            // Male relays from upstream = cycle CYAN↔PURPLE
            _maleAnim = EmboutAnim::CYCLE_INFRA_SUPRA;
            _femaleAnim = EmboutAnim::FIXED_ORANGE;
            break;
            
        case RhizomeState::DEAD:
            _maleAnim = EmboutAnim::OFF;
            _femaleAnim = EmboutAnim::OFF;
            break;
    }
}

/*------------------------------------------------------------------------------
 * Embout Rendering
 *----------------------------------------------------------------------------*/
void LedFeedback::renderEmbout(CRGB* leds, EmboutAnim anim) {
    switch (anim) {
        case EmboutAnim::OFF:
            fill_solid(leds, LedConfig::EMBOUT_LEDS, CRGB::Black);
            break;
            
        case EmboutAnim::IDLE_PULSE: {
            // Pulsation using SAME energy color as tube
            // Pulse blends toward white during heartbeat
            CRGB baseColor = getEnergyColor(getEnergySaturation());
            CRGB targetColor = blend(baseColor, COLOR_WHITE, (uint8_t)(_pulsePhase * 80));
            fill_solid(leds, LedConfig::EMBOUT_LEDS, targetColor);
            break;
        }
            
        case EmboutAnim::FIXED_ORANGE:
            // Pure ORANGE, only affected by global brightness
            // NOT influenced by energy saturation or white blending
            fill_solid(leds, LedConfig::EMBOUT_LEDS, COLOR_ORANGE);
            break;
            
        case EmboutAnim::CYCLE_INFRA_SUPRA:
            // CYAN↔PURPLE cycle, not affected by energy saturation
            fill_solid(leds, LedConfig::EMBOUT_LEDS, getCycleColor());
            break;
    }
}

/*------------------------------------------------------------------------------
 * Tube Rendering
 *----------------------------------------------------------------------------*/
void LedFeedback::renderTube() {
    switch (_currentState) {
        case RhizomeState::IDLE:
        case RhizomeState::DISCOVERING:
            renderTubeIdle();
            break;
            
        case RhizomeState::GENERATING:
            renderTubeGenerating();
            break;
            
        case RhizomeState::GIVING:
            renderTubeGiving();
            break;
            
        case RhizomeState::MIDDLEMAN:
            renderTubeMiddleman();
            break;
            
        case RhizomeState::DEAD:
            fill_solid(_ledsTube, LedConfig::TUBE_LEDS, CRGB::Black);
            break;
    }
}

void LedFeedback::renderTubeIdle() {
    // Hardware: LED 0 = MALE end, LED 14 = FEMALE end
    // Gauge from male (LED 0) toward female (LED 14)
    // Pulses with heartbeat
    int gaugeLeds = getGaugeLedCount();
    CRGB gaugeColor = getEnergyColor(getEnergySaturation());
    
    // Apply pulse to tube (blend toward WHITE during heartbeat)
    CRGB pulseColor = blend(gaugeColor, COLOR_WHITE, (uint8_t)(_pulsePhase * 80));
    
    for (int i = 0; i < LedConfig::TUBE_LEDS; i++) {
        if (i < gaugeLeds) {
            _ledsTube[i] = pulseColor;
        } else if (i == gaugeLeds) {
            // Partial LED for smooth gauge
            float fraction = ((_currentEnergy / 100.0f) * LedConfig::TUBE_LEDS) - gaugeLeds;
            _ledsTube[i] = pulseColor;
            _ledsTube[i].fadeToBlackBy(255 - (uint8_t)(fraction * 255));
        } else {
            _ledsTube[i] = CRGB::Black;
        }
    }
}

void LedFeedback::renderTubeGenerating() {
    // Unidirectional ORANGE flow traversing the tube
    // Hardware: LED 0 = MALE end, LED 14 = FEMALE end
    // Flow direction: male (0) → female (14) - clockwise system direction
    // Gauge grows from male (LED 0) toward female (LED 14)
    
    int gaugeLeds = getGaugeLedCount();
    CRGB baseColor = getEnergyColor(getEnergySaturation());
    
    // Draw base gauge from male end (LED 0) toward female (LED 14)
    for (int i = 0; i < gaugeLeds && i < LedConfig::TUBE_LEDS; i++) {
        _ledsTube[i] = baseColor;
    }
    
    // Draw flowing packet overlay (moving from male toward female: 0→14)
    int flowStart = (int)_flowPosition;
    for (int j = 0; j < LedConfig::FLOW_PACKET_SIZE; j++) {
        int led = flowStart + j;
        if (led >= 0 && led < LedConfig::TUBE_LEDS) {
            // Bright orange flow
            _ledsTube[led] = COLOR_ORANGE;
            _ledsTube[led] += CRGB(40, 20, 0);  // Extra brightness
        }
    }
    
    // At 100%, tube uniformly full orange
    if (_currentEnergy >= 100.0f) {
        fill_solid(_ledsTube, LedConfig::TUBE_LEDS, COLOR_ORANGE);
    }
}

void LedFeedback::renderTubeGiving() {
    // Hardware: LED 0 = MALE end, LED 14 = FEMALE end
    // Energy drains toward male (node connection)
    // Gauge anchored at male (LED 0), shrinks from female side
    
    int gaugeLeds = (int)((_displayedEnergy / 100.0f) * LedConfig::TUBE_LEDS);
    CRGB gaugeColor = getEnergyColor(getEnergySaturation());
    
    // Draw gauge from male end (LED 0) for gaugeLeds count
    for (int i = 0; i < gaugeLeds && i < LedConfig::TUBE_LEDS; i++) {
        _ledsTube[i] = gaugeColor;
    }
    
    // Draw CYAN/PURPLE flux from female toward male (visible only in gauge)
    CRGB fluxColor = getCycleColor();
    int flowStart = (int)_flowPosition;
    for (int j = 0; j < LedConfig::FLOW_PACKET_SIZE; j++) {
        int led = flowStart - j;  // Packet trails behind position
        // Only draw if within gauge AND valid range
        if (led >= 0 && led < gaugeLeds && led < LedConfig::TUBE_LEDS) {
            _ledsTube[led] = fluxColor;
        }
    }
    
    // When energy = 0, all off
    if (_displayedEnergy <= 0.0f) {
        fill_solid(_ledsTube, LedConfig::TUBE_LEDS, CRGB::Black);
    }
}

void LedFeedback::renderTubeMiddleman() {
    // Tube cycles between CYAN and PURPLE
    fill_solid(_ledsTube, LedConfig::TUBE_LEDS, getCycleColor());
}

/*------------------------------------------------------------------------------
 * Flash Event (100% Celebration)
 *----------------------------------------------------------------------------*/
void LedFeedback::renderFlash() {
    uint32_t elapsed = millis() - _flashStartMs;
    uint32_t cycleTime = LedConfig::FLASH_DURATION_MS * 2;  // On + Off
    uint32_t totalTime = cycleTime * LedConfig::FLASH_CYCLES;
    
    if (elapsed >= totalTime) {
        _flashActive = false;
        return;
    }
    
    // Determine if in "on" or "off" phase of flash
    uint32_t posInCycle = elapsed % cycleTime;
    bool flashOn = (posInCycle < LedConfig::FLASH_DURATION_MS);
    
    if (flashOn) {
        // White flash on all LEDs
        FastLED.setBrightness(255);
        fill_solid(_ledsMale, LedConfig::EMBOUT_LEDS, COLOR_WHITE);
        fill_solid(_ledsFemale, LedConfig::EMBOUT_LEDS, COLOR_WHITE);
        fill_solid(_ledsTube, LedConfig::TUBE_LEDS, COLOR_WHITE);
    } else {
        // Off
        clearAll();
    }
}

/*------------------------------------------------------------------------------
 * Animation Updates (non-blocking)
 *----------------------------------------------------------------------------*/
void LedFeedback::updatePulse() {
    if (!_pulseActive) {
        _pulsePhase = 0.0f;
        return;
    }
    
    uint32_t elapsed = millis() - _pulseStartMs;
    
    if (elapsed >= LedConfig::PULSE_DURATION_MS) {
        _pulseActive = false;
        _pulsePhase = 0.0f;
        return;
    }
    
    // Smooth sine-like pulse curve (0 → 1 → 0)
    float progress = (float)elapsed / LedConfig::PULSE_DURATION_MS;
    _pulsePhase = sin(progress * PI);
}

void LedFeedback::updateFlow() {
    if (millis() - _lastAnimMs < LedConfig::ANIMATION_UPDATE_MS) return;
    _lastAnimMs = millis();
    
    // Flow position for generating animation (male -> female: 0 -> 14)
    if (_currentState == RhizomeState::GENERATING) {
        _flowPosition += LedConfig::FLOW_SPEED;
        if (_flowPosition >= LedConfig::TUBE_LEDS) {
            _flowPosition = -LedConfig::FLOW_PACKET_SIZE;  // Wrap around
        }
    }
    
    // Flow position for giving animation (female -> male: 14 -> 0)
    if (_currentState == RhizomeState::GIVING) {
        _flowPosition -= LedConfig::FLOW_SPEED;
        if (_flowPosition < -LedConfig::FLOW_PACKET_SIZE) {
            _flowPosition = LedConfig::TUBE_LEDS;  // Wrap around from female end
        }
    }
}

void LedFeedback::updateGaugeDrain() {
    // Smooth gauge drain for GIVING state
    if (_currentState == RhizomeState::GIVING) {
        if (_displayedEnergy > _currentEnergy) {
            _displayedEnergy -= LedConfig::GAUGE_DRAIN_SPEED;
            if (_displayedEnergy < _currentEnergy) {
                _displayedEnergy = _currentEnergy;
            }
        }
    } else {
        // Keep displayed in sync with actual when not giving
        _displayedEnergy = _currentEnergy;
    }
}

/*------------------------------------------------------------------------------
 * Color Helpers
 *----------------------------------------------------------------------------*/
CRGB LedFeedback::getEnergyColor(float saturation) {
    // Base color is ORANGE
    // Lower saturation blends toward WHITE
    // saturation: 1.0 = full orange, 0.0 = white
    return blend(COLOR_WHITE, COLOR_ORANGE, (uint8_t)(saturation * 255));
}

CRGB LedFeedback::getCycleColor() {
    // Smooth cycle between CYAN and PURPLE
    uint32_t elapsed = millis() - _cycleStartMs;
    float progress = (float)(elapsed % LedConfig::COLOR_CYCLE_MS) / LedConfig::COLOR_CYCLE_MS;
    
    // Sine wave for smooth transition: 0→1→0
    float blend_amount = (sin(progress * TWO_PI) + 1.0f) / 2.0f;
    
    return blend(COLOR_CYAN, COLOR_PURPLE, (uint8_t)(blend_amount * 255));
}

uint8_t LedFeedback::getGlobalBrightness() {
    // 0% energy = 0% brightness (min 10 for visibility)
    // 100% energy = 100% brightness
    uint8_t minBrightness = 10;
    uint8_t maxBrightness = 255;
    
    return minBrightness + (uint8_t)((_currentEnergy / 100.0f) * (maxBrightness - minBrightness));
}

float LedFeedback::getEnergySaturation() {
    // Non-linear mapping: orange visible early, white only at very low energy
    // 0% → 0.30 (white tint)
    // 10% → 0.70 (orange dominant)
    // 20% → 0.82 (mostly orange)
    // 50% → 0.95 (nearly full orange)
    // 100% → 1.0 (full orange)
    float normalized = _currentEnergy / 100.0f;
    // Stronger curve: pow(x, 0.4) for faster orange appearance
    float curved = pow(normalized, 0.4f);
    return 0.30f + curved * 0.70f;
}

int LedFeedback::getGaugeLedCount() {
    return (int)((_currentEnergy / 100.0f) * LedConfig::TUBE_LEDS);
}

/*------------------------------------------------------------------------------
 * Utility
 *----------------------------------------------------------------------------*/
void LedFeedback::clearAll() {
    fill_solid(_ledsMale, LedConfig::EMBOUT_LEDS, CRGB::Black);
    fill_solid(_ledsFemale, LedConfig::EMBOUT_LEDS, CRGB::Black);
    fill_solid(_ledsTube, LedConfig::TUBE_LEDS, CRGB::Black);
}
