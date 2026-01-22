/*==============================================================================
 * AnimationLayers.h - Layer rendering functions
 * Project: Synapse - Interactive Rhizome Installation
 *============================================================================*/

#ifndef ANIMATION_LAYERS_H
#define ANIMATION_LAYERS_H

#include <Arduino.h>
#include <FastLED.h>
#include "LayerTypes.h"

/*------------------------------------------------------------------------------
 * Layer Renderer Class
 * Handles rendering of individual layers and compositing
 *----------------------------------------------------------------------------*/
class LayerRenderer {
public:
    LayerRenderer(CRGB* ledBuffer, uint8_t numLeds);
    
    // Individual layer render functions
    void renderGaugeLayer(const GaugeLayerConfig& config, float energy);
    void renderPulseLayer(const PulseLayerConfig& config, float energy, uint8_t gaugeLedCount);
    void renderFlowLayer(FlowLayerConfig& config, uint32_t currentTime);
    bool renderEventLayer(EventLayerConfig& config, uint32_t currentTime);
    
    // Compositing
    void clearBuffer();
    void applyToOutput(CRGB* outputLeds);
    
    // Utilities
    uint8_t energyToLedCount(float energy) const;
    
private:
    CRGB* _buffer;          // Internal compositing buffer
    CRGB* _tempBuffer;      // Temporary buffer for blending
    uint8_t _numLeds;
    
    // Internal render helpers
    void blendPixel(uint8_t index, CRGB color, BlendMode mode, uint8_t opacity);
    void addPixelClamped(uint8_t index, CRGB color);
    
    // Event animation renders
    void renderFullEnergyEvent(float progress, CRGB color);
    void renderEmptyEnergyEvent(float progress);
    void renderConnectionEvent(float progress, CRGB color);
    void renderDisconnectionEvent(float progress);
    void renderStateChangeEvent(float progress, CRGB color);
    
    // Flow animation state (persistent)
    float _flowPosition;
};

/*------------------------------------------------------------------------------
 * State Controller Class
 * Maps RhizomeState to layer configurations
 *----------------------------------------------------------------------------*/
class StateController {
public:
    StateController();
    
    // Update layer configs based on rhizome state
    void updateFromState(uint8_t rhizomeState, float energy, AnimationState& animState);
    
    // Manual event triggers
    void triggerEvent(EventType event, AnimationState& animState);
    void resetEventTrigger(EventType event, AnimationState& animState);
    
    // Transition helpers
    bool isInTransition() const;
    
private:
    uint8_t _previousState;
    uint32_t _transitionStartTime;
    bool _inTransition;
    
    // State-specific configuration
    void configureIdle(AnimationState& animState);
    void configureGenerating(AnimationState& animState);
    void configureGiving(AnimationState& animState);
    void configureMiddleMan(AnimationState& animState);
    void configureDead(AnimationState& animState);
    
    // Color palettes per state
    CRGB getBaseColorForState(uint8_t state) const;
    CRGB getFlowColorForState(uint8_t state) const;
};

/*------------------------------------------------------------------------------
 * Main Animation Manager
 * Orchestrates the render loop
 *----------------------------------------------------------------------------*/
class AnimationManager {
public:
    AnimationManager();
    
    // Initialization
    void begin(CRGB* leds, uint8_t numLeds, uint8_t brightness);
    
    // Main update - call from loop()
    void update(uint8_t rhizomeState, float energy);
    
    // Event triggers (call from external code)
    void onFullEnergy();
    void onEmptyEnergy();
    void onConnection();
    void onDisconnection();
    
    // MiddleMan sync - receive timing from source rhizome
    void setFlowSync(uint8_t speed);
    void disableFlowSync();
    
    // Configuration access
    AnimationState& getAnimState();
    
    // Debug
    void printDebug(Stream& output = Serial);

private:
    CRGB* _leds;
    CRGB* _compositeBuffer;
    uint8_t _numLeds;
    
    AnimationState _animState;
    LayerRenderer* _renderer;
    StateController _stateController;
    
    // Smooth energy transition
    void updateDisplayedEnergy(float targetEnergy, float deltaTime);
    
    // Render pipeline
    void renderAllLayers();
};

#endif // ANIMATION_LAYERS_H
