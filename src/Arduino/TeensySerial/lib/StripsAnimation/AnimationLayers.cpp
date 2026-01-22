/*==============================================================================
 * AnimationLayers.cpp - Layer rendering implementation
 * Project: Synapse - Interactive Rhizome Installation
 *============================================================================*/

#include "AnimationLayers.h"
#include <RhizomeStateAndID.h>

/*==============================================================================
 * LayerRenderer Implementation
 *============================================================================*/

LayerRenderer::LayerRenderer(CRGB* ledBuffer, uint8_t numLeds) 
    : _buffer(ledBuffer), _numLeds(numLeds), _flowPosition(0) 
{
    _tempBuffer = new CRGB[numLeds];
}

void LayerRenderer::clearBuffer() {
    fill_solid(_buffer, _numLeds, CRGB::Black);
}

uint8_t LayerRenderer::energyToLedCount(float energy) const {
    // Map 0-100 energy to 0-numLeds
    return (uint8_t)((energy / 100.0f) * _numLeds);
}

/*------------------------------------------------------------------------------
 * Gauge Layer - Base energy visualization
 * Direction: Idle/Generating = droite→gauche (fill from right)
 *            Giving/MiddleMan = gauche→droite (fill from left, drains visually)
 * Remplissage fluide: la LED suivante s'allume progressivement
 *----------------------------------------------------------------------------*/
void LayerRenderer::renderGaugeLayer(const GaugeLayerConfig& config, float energy) {
    if (!config.enabled) return;
    if (energy <= 0) return;
    
    // Calcul précis avec partie fractionnelle
    // Ex: 33% sur 15 LEDs = 4.95 -> 4 LEDs pleines + 1 LED à 95%
    float exactLeds = (energy / 100.0f) * _numLeds;
    uint8_t fullLeds = (uint8_t)exactLeds;  // Partie entière
    uint8_t partialBrightness = (uint8_t)((exactLeds - fullLeds) * 255);  // Partie fractionnelle
    
    // Dessiner les LEDs pleines
    for (uint8_t i = 0; i < fullLeds; i++) {
        uint8_t ledIndex;
        if (config.direction == DIR_RIGHT_TO_LEFT) {
            ledIndex = (_numLeds - 1) - i;
        } else {
            ledIndex = i;
        }
        
        CRGB pixelColor = config.baseColor;
        blendPixel(ledIndex, pixelColor, config.blendMode, config.opacity);
    }
    
    // Dessiner la LED partielle (celle qui se remplit)
    if (fullLeds < _numLeds && partialBrightness > 0) {
        uint8_t partialIndex;
        if (config.direction == DIR_RIGHT_TO_LEFT) {
            partialIndex = (_numLeds - 1) - fullLeds;
        } else {
            partialIndex = fullLeds;
        }
        
        CRGB partialColor = config.baseColor;
        partialColor.nscale8(partialBrightness);  // Intensité proportionnelle
        blendPixel(partialIndex, partialColor, config.blendMode, config.opacity);
    }
}

/*------------------------------------------------------------------------------
 * Pulse Layer - Heartbeat brightness modulation
 * Vraie courbe de battement de cœur:
 * - Montée RAPIDE vers le pic
 * - Descente LENTE vers le repos
 * - Long temps de repos avant le prochain battement
 *----------------------------------------------------------------------------*/
void LayerRenderer::renderPulseLayer(const PulseLayerConfig& config, float energy, uint8_t gaugeLedCount) {
    if (!config.enabled) return;
    
    // Calculate BPM based on energy (higher energy = faster pulse)
    uint8_t currentBPM = map((uint8_t)constrain(energy, 0, 100), 0, 100, config.minBPM, config.maxBPM);
    
    // Période en ms
    uint32_t periodMs = 60000UL / currentBPM;
    
    // Position dans le cycle (0.0 - 1.0)
    uint32_t now = millis();
    float cyclePos = (float)(now % periodMs) / (float)periodMs;
    
    // Courbe de battement de cœur:
    // 0.00 - 0.10 : Montée rapide (systole) - repos vers pic
    // 0.10 - 0.40 : Descente lente (diastole) - pic vers repos
    // 0.40 - 1.00 : Repos (long plateau bas)
    
    float normalizedBrightness;
    
    if (cyclePos < 0.10f) {
        // Montée rapide vers le pic (ease-out pour finir vite au sommet)
        float t = cyclePos / 0.10f;  // 0 -> 1
        // Ease-out quadratic: rapide au début, ralentit vers la fin
        normalizedBrightness = t * (2.0f - t);
    }
    else if (cyclePos < 0.40f) {
        // Descente lente vers le repos (ease-in pour ralentir vers le bas)
        float t = (cyclePos - 0.10f) / 0.30f;  // 0 -> 1
        // Ease-in quadratic: lent au début, accélère vers la fin mais on inverse
        normalizedBrightness = 1.0f - (t * t);
    }
    else {
        // Long repos - plateau bas
        normalizedBrightness = 0.0f;
    }
    
    // Map à la plage de brightness
    uint8_t pulseValue = config.minBrightness + 
                         (uint8_t)(normalizedBrightness * (config.maxBrightness - config.minBrightness));
    
    // Appliquer le pulse à TOUTES les LEDs du buffer qui ont de la couleur
    for (uint8_t i = 0; i < _numLeds; i++) {
        if (_buffer[i].r > 0 || _buffer[i].g > 0 || _buffer[i].b > 0) {
            _buffer[i].nscale8(pulseValue);
        }
    }
}

/*------------------------------------------------------------------------------
 * Flow Layer - Animated energy packets
 * Direction INVARIANTE: toujours gauche → droite (sens horaire)
 * Le flux s'additionne visuellement à la jauge (CRGB +=)
 *----------------------------------------------------------------------------*/
void LayerRenderer::renderFlowLayer(FlowLayerConfig& config, uint32_t currentTime) {
    if (!config.enabled) return;
    
    // Update flow position based on speed
    static uint32_t lastFlowUpdate = 0;
    float deltaTime = (currentTime - lastFlowUpdate) / 1000.0f;
    lastFlowUpdate = currentTime;
    
    // Move position (pixels per second) - TOUJOURS gauche→droite
    float speedFactor = config.speed / 50.0f; // Normalize around 50
    _flowPosition += speedFactor * deltaTime * 30.0f; // ~30 pixels/sec at speed=50
    
    // Wrap position
    float totalCycleLength = config.packetSize + config.packetSpacing;
    if (_flowPosition >= totalCycleLength) {
        _flowPosition -= totalCycleLength;
    }
    
    // Render packets - direction INVARIANTE (gauche→droite)
    for (uint8_t i = 0; i < _numLeds; i++) {
        // LED index = i (pas d'inversion, flux toujours dans le même sens)
        uint8_t ledIndex = i;
        
        // Determine position within packet cycle
        // On soustrait flowPosition pour que les packets "avancent" vers la droite
        float adjustedPos = (float)i - _flowPosition;
        if (adjustedPos < 0) adjustedPos += totalCycleLength * (1 + (int)(-adjustedPos / totalCycleLength));
        float posInCycle = fmod(adjustedPos, totalCycleLength);
        
        // Inside a packet?
        if (posInCycle < config.packetSize) {
            // Calculate brightness based on position in packet (creates soft edges)
            float packetPos = posInCycle / config.packetSize; // 0 to 1
            uint8_t brightness = sin8(packetPos * 128) * config.intensity / 255;
            
            CRGB packetPixel = config.packetColor;
            packetPixel.nscale8(brightness);
            
            // Additive blend sur la jauge
            addPixelClamped(ledIndex, packetPixel);
        }
        // Trail effect (traînée derrière le packet)
        else if (posInCycle < config.packetSize + config.trailLength) {
            float trailPos = (posInCycle - config.packetSize) / config.trailLength;
            uint8_t brightness = (1.0f - trailPos) * config.intensity / 3;
            
            CRGB trailPixel = config.packetColor;
            trailPixel.nscale8(brightness);
            
            addPixelClamped(ledIndex, trailPixel);
        }
    }
}

/*------------------------------------------------------------------------------
 * Event Layer - One-shot priority animations
 *----------------------------------------------------------------------------*/
bool LayerRenderer::renderEventLayer(EventLayerConfig& config, uint32_t currentTime) {
    if (!config.enabled || !config.eventActive) return false;
    
    // Check if event duration has elapsed
    uint32_t elapsed = currentTime - config.eventStartTime;
    if (elapsed >= config.eventDuration) {
        config.eventActive = false;
        config.currentEvent = EVENT_NONE;
        return false;
    }
    
    // Calculate animation progress (0.0 to 1.0)
    float progress = (float)elapsed / config.eventDuration;
    
    // Event-specific animations
    switch (config.currentEvent) {
        case EVENT_FULL_ENERGY:
            renderFullEnergyEvent(progress, config.eventColor);
            break;
            
        case EVENT_EMPTY_ENERGY:
            renderEmptyEnergyEvent(progress);
            break;
            
        case EVENT_CONNECTION:
            renderConnectionEvent(progress, config.eventColor);
            break;
            
        case EVENT_DISCONNECTION:
            renderDisconnectionEvent(progress);
            break;
            
        case EVENT_STATE_CHANGE:
            renderStateChangeEvent(progress, config.eventColor);
            break;
            
        default:
            return false;
    }
    
    return true; // Event is active, may block other renders
}

// Event: Confirmation énergie pleine
// Flash Orange → Blanc → Orange → Blanc (2 cycles complets)
void LayerRenderer::renderFullEnergyEvent(float progress, CRGB color) {
    // 2 cycles complets sur la durée totale
    // Chaque cycle: Orange (25%) → Blanc (25%) → Orange (25%) → Blanc (25%)
    // Soit 4 phases par cycle, 2 cycles = 8 phases
    
    const CRGB orange = CRGB(255, 80, 0);
    const CRGB white = CRGB(255, 255, 255);
    
    // 8 phases au total (2 cycles x 4 transitions)
    float phase = progress * 4.0f; // 0-4 pour 2 cycles (chaque cycle = 2 phases)
    int phaseIndex = (int)phase;
    
    CRGB currentColor;
    
    // Alternance Orange/Blanc sur 2 cycles
    // Phase 0: Orange, Phase 1: Blanc, Phase 2: Orange, Phase 3: Blanc
    if (phaseIndex % 2 == 0) {
        currentColor = orange;
    } else {
        currentColor = white;
    }
    
    // Brightness plein pour un flash franc
    fill_solid(_buffer, _numLeds, currentColor);
}

void LayerRenderer::renderEmptyEnergyEvent(float progress) {
    // Fade to black - extinction progressive
    const CRGB orange = CRGB(255, 80, 0);
    uint8_t brightness = (1.0f - progress) * 255;
    
    // Léger flicker pour effet organique
    if (random8() < 30) {
        brightness = scale8(brightness, random8(120, 200));
    }
    
    fill_solid(_buffer, _numLeds, orange);
    for (uint8_t i = 0; i < _numLeds; i++) {
        _buffer[i].nscale8(brightness);
    }
}

void LayerRenderer::renderConnectionEvent(float progress, CRGB color) {
    // Ripple from center
    uint8_t center = _numLeds / 2;
    float ripplePos = progress * _numLeds;
    
    for (uint8_t i = 0; i < _numLeds; i++) {
        float distFromCenter = abs((int)i - (int)center);
        if (distFromCenter <= ripplePos && distFromCenter >= ripplePos - 3) {
            uint8_t brightness = (1.0f - (ripplePos - distFromCenter) / 3.0f) * 255;
            _buffer[i] = color;
            _buffer[i].nscale8(brightness);
        }
    }
}

void LayerRenderer::renderDisconnectionEvent(float progress) {
    // Scatter/dissolve effect
    for (uint8_t i = 0; i < _numLeds; i++) {
        if (random8() > progress * 255) {
            _buffer[i] = CRGB::Orange;
            _buffer[i].nscale8((1.0f - progress) * 200);
        }
    }
}

void LayerRenderer::renderStateChangeEvent(float progress, CRGB color) {
    // Quick color wash
    uint8_t brightness = sin8(progress * 255) ;
    fill_solid(_buffer, _numLeds, color);
    for (uint8_t i = 0; i < _numLeds; i++) {
        _buffer[i].nscale8(brightness);
    }
}

/*------------------------------------------------------------------------------
 * Blending utilities
 *----------------------------------------------------------------------------*/
void LayerRenderer::blendPixel(uint8_t index, CRGB color, BlendMode mode, uint8_t opacity) {
    if (index >= _numLeds) return;
    
    // Apply opacity to incoming color
    CRGB blendedColor = color;
    blendedColor.nscale8(opacity);
    
    switch (mode) {
        case BLEND_REPLACE:
            _buffer[index] = blendedColor;
            break;
            
        case BLEND_ADD:
            addPixelClamped(index, blendedColor);
            break;
            
        case BLEND_MULTIPLY:
            _buffer[index].r = scale8(_buffer[index].r, blendedColor.r);
            _buffer[index].g = scale8(_buffer[index].g, blendedColor.g);
            _buffer[index].b = scale8(_buffer[index].b, blendedColor.b);
            break;
            
        case BLEND_SCREEN:
            _buffer[index].r = 255 - scale8(255 - _buffer[index].r, 255 - blendedColor.r);
            _buffer[index].g = 255 - scale8(255 - _buffer[index].g, 255 - blendedColor.g);
            _buffer[index].b = 255 - scale8(255 - _buffer[index].b, 255 - blendedColor.b);
            break;
    }
}

void LayerRenderer::addPixelClamped(uint8_t index, CRGB color) {
    if (index >= _numLeds) return;
    
    // Additive blend with clamping to prevent overflow
    uint16_t r = _buffer[index].r + color.r;
    uint16_t g = _buffer[index].g + color.g;
    uint16_t b = _buffer[index].b + color.b;
    
    _buffer[index].r = (r > 255) ? 255 : r;
    _buffer[index].g = (g > 255) ? 255 : g;
    _buffer[index].b = (b > 255) ? 255 : b;
}

void LayerRenderer::applyToOutput(CRGB* outputLeds) {
    memcpy(outputLeds, _buffer, sizeof(CRGB) * _numLeds);
}

/*==============================================================================
 * StateController Implementation
 *============================================================================*/

StateController::StateController() 
    : _previousState(255), _transitionStartTime(0), _inTransition(false) 
{}

void StateController::updateFromState(uint8_t rhizomeState, float energy, AnimationState& animState) {
    // Detect state change
    if (rhizomeState != _previousState) {
        _inTransition = true;
        _transitionStartTime = millis();
        // Pas d'event automatique au changement d'état - transitions fluides
        _previousState = rhizomeState;
    }
    
    // Update energy
    animState.currentEnergy = energy;
    
    // Configure layers based on state
    switch (rhizomeState) {
        case IDLE:
            configureIdle(animState);
            break;
        case GENERATING:
            configureGenerating(animState);
            break;
        case GIVING_TO_NODE:
            configureGiving(animState);
            break;
        case MIDDLEMAN:
            configureMiddleMan(animState);
            break;
        case DEAD:
            configureDead(animState);
            break;
    }
    
    // Check for energy threshold events (avec flags anti-retrigger)
    if (energy >= 100.0f && !animState.event.eventTriggered) {
        triggerEvent(EVENT_FULL_ENERGY, animState);
        animState.event.eventTriggered = true;
    } else if (energy < 99.0f) {
        // Reset trigger quand l'énergie redescend significativement
        animState.event.eventTriggered = false;
    }
    
    // Dead event quand énergie = 0
    if (energy <= 0.0f && rhizomeState != DEAD && !animState.event.eventActive) {
        triggerEvent(EVENT_EMPTY_ENERGY, animState);
    }
}

void StateController::triggerEvent(EventType event, AnimationState& animState) {
    animState.event.currentEvent = event;
    animState.event.eventActive = true;
    animState.event.eventStartTime = millis();
    
    // Event-specific durations and colors
    switch (event) {
        case EVENT_FULL_ENERGY:
            // Flash court et franc: 2 cycles Orange→Blanc
            animState.event.eventDuration = 600; // 600ms pour 2 cycles nets
            animState.event.eventColor = CRGB(255, 80, 0); // Orange de base
            // Après l'event: désactiver le flux
            animState.flow.enabled = false;
            break;
        case EVENT_EMPTY_ENERGY:
            animState.event.eventDuration = 1000; // Fade d'extinction
            animState.event.eventColor = CRGB(255, 80, 0);
            break;
        case EVENT_CONNECTION:
            animState.event.eventDuration = 400;
            animState.event.eventColor = CRGB(255, 120, 20); // Orange clair
            break;
        case EVENT_DISCONNECTION:
            animState.event.eventDuration = 500;
            animState.event.eventColor = CRGB(255, 80, 0);
            break;
        case EVENT_STATE_CHANGE:
            animState.event.eventDuration = 200;
            animState.event.eventColor = CRGB(255, 80, 0);
            break;
        default:
            break;
    }
}

void StateController::resetEventTrigger(EventType event, AnimationState& animState) {
    if (animState.event.currentEvent == event) {
        animState.event.eventTriggered = false;
    }
}

bool StateController::isInTransition() const {
    return _inTransition && (millis() - _transitionStartTime < 500);
}

/*------------------------------------------------------------------------------
 * State-specific configurations
 *----------------------------------------------------------------------------*/

void StateController::configureIdle(AnimationState& animState) {
    // Gauge: orange, direction droite→gauche
    animState.gauge.enabled = true;
    animState.gauge.baseColor = CRGB(255, 80, 0);   // Orange
    animState.gauge.direction = DIR_RIGHT_TO_LEFT;  // Jauge de droite vers gauche
    animState.gauge.opacity = 255;
    
    // Pulse: battement de coeur - lent et organique
    // BPM bas pour un effet calme (comme un coeur au repos)
    animState.pulse.enabled = true;
    animState.pulse.minBPM = 30;          // 30 BPM à 0% énergie (très calme)
    animState.pulse.maxBPM = 60;          // 60 BPM à 100% énergie
    animState.pulse.minBrightness = 100;  // Repos: plus sombre pour contraste
    animState.pulse.maxBrightness = 255;  // Peak: pleine luminosité
    animState.pulse.affectsGaugeOnly = true;
    
    // Flow: désactivé en Idle
    animState.flow.enabled = false;
}

void StateController::configureGenerating(AnimationState& animState) {
    // Gauge: orange, direction droite→gauche (se remplit)
    animState.gauge.enabled = true;
    animState.gauge.baseColor = CRGB(255, 80, 0);   // Orange
    animState.gauge.direction = DIR_RIGHT_TO_LEFT;  // Jauge de droite vers gauche
    animState.gauge.opacity = 255;
    
    // Pulse: mêmes paramètres que Idle
    animState.pulse.enabled = true;
    animState.pulse.minBPM = 30;
    animState.pulse.maxBPM = 60;
    animState.pulse.minBrightness = 100;
    animState.pulse.maxBrightness = 255;
    animState.pulse.affectsGaugeOnly = true;
    
    // Flow: actif, direction gauche→droite (invariante)
    animState.flow.enabled = true;
    animState.flow.packetColor = CRGB(255, 120, 30);  // Orange plus clair
    animState.flow.direction = DIR_LEFT_TO_RIGHT;
    animState.flow.speed = 55;
    animState.flow.packetSize = 3;
    animState.flow.packetSpacing = 10;
    animState.flow.trailLength = 3;
    animState.flow.intensity = 160;
}

void StateController::configureGiving(AnimationState& animState) {
    // Gauge: orange, direction gauche→droite (se vide)
    animState.gauge.enabled = true;
    animState.gauge.baseColor = CRGB(255, 80, 0);   // Orange
    animState.gauge.direction = DIR_LEFT_TO_RIGHT;  // Jauge de gauche vers droite
    animState.gauge.opacity = 255;
    
    // Pulse: mêmes paramètres que Idle
    animState.pulse.enabled = true;
    animState.pulse.minBPM = 30;
    animState.pulse.maxBPM = 60;
    animState.pulse.minBrightness = 100;
    animState.pulse.maxBrightness = 255;
    animState.pulse.affectsGaugeOnly = true;
    
    // Flow: actif, direction gauche→droite (invariante)
    animState.flow.enabled = true;
    animState.flow.packetColor = CRGB(255, 100, 20);  // Orange
    animState.flow.direction = DIR_LEFT_TO_RIGHT;
    animState.flow.speed = 65;
    animState.flow.packetSize = 4;
    animState.flow.packetSpacing = 8;
    animState.flow.trailLength = 4;
    animState.flow.intensity = 180;
}

void StateController::configureMiddleMan(AnimationState& animState) {
    // MiddleMan: extension du rhizome connecté
    // PAS de battement propre - on relaie simplement l'énergie
    
    // Gauge: orange, direction gauche→droite (relais)
    animState.gauge.enabled = true;
    animState.gauge.baseColor = CRGB(255, 80, 0);   // Orange
    animState.gauge.direction = DIR_LEFT_TO_RIGHT;  // Même sens que Giving
    animState.gauge.opacity = 255;
    
    // Pulse: DÉSACTIVÉ - MiddleMan n'a pas de battement propre
    // Il est une extension du rhizome qui lui donne l'énergie
    animState.pulse.enabled = false;
    
    // Flow: actif, direction gauche→droite (invariante)
    // Paramètres synchronisés avec le rhizome source
    animState.flow.enabled = true;
    animState.flow.packetColor = CRGB(255, 100, 20);  // Orange
    animState.flow.direction = DIR_LEFT_TO_RIGHT;     // Direction invariante
    animState.flow.speed = 55;  // Vitesse synchronisable avec source
    animState.flow.packetSize = 4;
    animState.flow.packetSpacing = 8;
    animState.flow.trailLength = 4;
    animState.flow.intensity = 170;
}

void StateController::configureDead(AnimationState& animState) {
    // DEAD: tout éteint, plus de battement de coeur
    animState.gauge.enabled = false;
    animState.pulse.enabled = false;  // Pas de battement = mort
    animState.flow.enabled = false;
    
    // Keep event layer for revival animations
    animState.event.enabled = true;
}

CRGB StateController::getBaseColorForState(uint8_t state) const {
    // Couleur de base = orange pour tous les états actifs
    switch (state) {
        case IDLE:          return CRGB(255, 80, 0);
        case GENERATING:    return CRGB(255, 80, 0);
        case GIVING_TO_NODE:return CRGB(255, 80, 0);
        case MIDDLEMAN:     return CRGB(255, 80, 0);
        case DEAD:          return CRGB(0, 0, 0);
        default:            return CRGB::Black;
    }
}

CRGB StateController::getFlowColorForState(uint8_t state) const {
    // Couleur du flux = orange clair
    switch (state) {
        case GENERATING:    return CRGB(255, 120, 30);
        case GIVING_TO_NODE:return CRGB(255, 100, 20);
        case MIDDLEMAN:     return CRGB(255, 100, 20);
        default:            return CRGB::Black;
    }
}

/*==============================================================================
 * AnimationManager Implementation
 *============================================================================*/

AnimationManager::AnimationManager() 
    : _leds(nullptr), _compositeBuffer(nullptr), _numLeds(0), _renderer(nullptr)
{}

void AnimationManager::begin(CRGB* leds, uint8_t numLeds, uint8_t brightness) {
    _leds = leds;
    _numLeds = numLeds;
    _animState.numLeds = numLeds;
    _animState.globalBrightness = brightness;
    
    // Allocate composite buffer
    _compositeBuffer = new CRGB[numLeds];
    
    // Create renderer
    _renderer = new LayerRenderer(_compositeBuffer, numLeds);
    
    // Initialize state
    _animState.lastUpdateTime = millis();
}

void AnimationManager::update(uint8_t rhizomeState, float energy) {
    uint32_t currentTime = millis();
    float deltaTime = (currentTime - _animState.lastUpdateTime) / 1000.0f;
    _animState.lastUpdateTime = currentTime;
    
    // Update state controller (configures layers)
    _stateController.updateFromState(rhizomeState, energy, _animState);
    
    // Smooth energy transition for display
    updateDisplayedEnergy(energy, deltaTime);
    
    // Render all layers
    renderAllLayers();
    
    // Copy to output and show
    _renderer->applyToOutput(_leds);
    FastLED.show();
}

void AnimationManager::updateDisplayedEnergy(float targetEnergy, float deltaTime) {
    // Smooth transition for visual continuity
    float diff = targetEnergy - _animState.displayedEnergy;
    float maxChange = 50.0f * deltaTime; // Max 50% per second
    
    if (abs(diff) < maxChange) {
        _animState.displayedEnergy = targetEnergy;
    } else {
        _animState.displayedEnergy += (diff > 0 ? maxChange : -maxChange);
    }
}

void AnimationManager::renderAllLayers() {
    uint32_t currentTime = millis();
    
    // Clear composite buffer
    _renderer->clearBuffer();
    
    // Check if event layer takes priority
    bool eventActive = _renderer->renderEventLayer(_animState.event, currentTime);
    
    if (!eventActive) {
        // Layer 0: Energy Gauge (base)
        _renderer->renderGaugeLayer(_animState.gauge, _animState.displayedEnergy);
        
        // Calculate gauge LED count for pulse layer
        uint8_t gaugeLedCount = _renderer->energyToLedCount(_animState.displayedEnergy);
        
        // Layer 1: Pulse (brightness modulation)
        _renderer->renderPulseLayer(_animState.pulse, _animState.displayedEnergy, gaugeLedCount);
        
        // Layer 2: Energy Flow (additive)
        _renderer->renderFlowLayer(_animState.flow, currentTime);
    }
}

// Event trigger methods
void AnimationManager::onFullEnergy() {
    _stateController.triggerEvent(EVENT_FULL_ENERGY, _animState);
}

void AnimationManager::onEmptyEnergy() {
    _stateController.triggerEvent(EVENT_EMPTY_ENERGY, _animState);
}

void AnimationManager::onConnection() {
    _stateController.triggerEvent(EVENT_CONNECTION, _animState);
}

void AnimationManager::onDisconnection() {
    _stateController.triggerEvent(EVENT_DISCONNECTION, _animState);
}

AnimationState& AnimationManager::getAnimState() {
    return _animState;
}

void AnimationManager::printDebug(Stream& output) {
    output.print("Energy: ");
    output.print(_animState.currentEnergy);
    output.print(" (displayed: ");
    output.print(_animState.displayedEnergy);
    output.print(") | Gauge: ");
    output.print(_animState.gauge.enabled ? "ON" : "OFF");
    output.print(" | Pulse: ");
    output.print(_animState.pulse.enabled ? "ON" : "OFF");
    output.print(" | Flow: ");
    output.print(_animState.flow.enabled ? "ON" : "OFF");
    output.print(" | Event: ");
    output.println(_animState.event.eventActive ? "ACTIVE" : "none");
}

/*------------------------------------------------------------------------------
 * MiddleMan Synchronization
 * Allows external code to sync flow parameters with source rhizome
 *----------------------------------------------------------------------------*/
void AnimationManager::setFlowSync(uint8_t speed) {
    _animState.flow.syncEnabled = true;
    _animState.flow.syncSpeed = speed;
    _animState.flow.speed = speed;  // Apply immediately
}

void AnimationManager::disableFlowSync() {
    _animState.flow.syncEnabled = false;
}
