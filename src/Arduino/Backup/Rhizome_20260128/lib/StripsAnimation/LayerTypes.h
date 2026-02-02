/*==============================================================================
 * LayerTypes.h - Type definitions for layer-based animation system
 * Project: Synapse - Interactive Rhizome Installation
 *============================================================================*/

#ifndef LAYER_TYPES_H
#define LAYER_TYPES_H

#include <Arduino.h>
#include <FastLED.h>

/*------------------------------------------------------------------------------
 * Direction enum - Used for gauge fill and flow direction
 *----------------------------------------------------------------------------*/
enum FlowDirection {
    DIR_LEFT_TO_RIGHT = 0,  // Energy coming IN (Generating, MiddleMan receiving)
    DIR_RIGHT_TO_LEFT = 1   // Energy going OUT (Giving, MiddleMan transmitting)
};

/*------------------------------------------------------------------------------
 * Blend modes for layer compositing
 *----------------------------------------------------------------------------*/
enum BlendMode {
    BLEND_REPLACE = 0,      // Layer replaces underlying pixels
    BLEND_ADD = 1,          // Additive blend (CRGB +=)
    BLEND_MULTIPLY = 2,     // Multiply blend
    BLEND_SCREEN = 3        // Screen blend (inverse multiply)
};

/*------------------------------------------------------------------------------
 * Event types for one-shot animations
 *----------------------------------------------------------------------------*/
enum EventType {
    EVENT_NONE = 0,
    EVENT_FULL_ENERGY,      // 100% energy reached
    EVENT_EMPTY_ENERGY,     // 0% energy (death)
    EVENT_CONNECTION,       // New connection established
    EVENT_DISCONNECTION,    // Connection lost
    EVENT_STATE_CHANGE      // Generic state transition
};

/*------------------------------------------------------------------------------
 * Base Layer Configuration - Common parameters for all layers
 *----------------------------------------------------------------------------*/
struct LayerConfig {
    bool enabled;           // Layer active/inactive
    uint8_t opacity;        // 0-255 global layer opacity
    BlendMode blendMode;    // How this layer composites
    
    LayerConfig() : enabled(false), opacity(255), blendMode(BLEND_ADD) {}
};

/*------------------------------------------------------------------------------
 * Energy Gauge Layer Config
 * - Fills LEDs proportionally to energy level
 * - Direction: Idle/Generating = droite→gauche, Giving/MiddleMan = gauche→droite
 * - Color: Orange plein (CRGB(255, 80, 0))
 *----------------------------------------------------------------------------*/
struct GaugeLayerConfig : LayerConfig {
    CRGB baseColor;         // Main gauge color (orange)
    FlowDirection direction;
    
    GaugeLayerConfig() : LayerConfig(), 
        baseColor(CRGB(255, 80, 0)),  // Orange
        direction(DIR_RIGHT_TO_LEFT)   // Default: Idle/Generating
    {
        enabled = true;     // Gauge is always on by default
        blendMode = BLEND_REPLACE;
    }
};

/*------------------------------------------------------------------------------
 * Pulse Layer Config
 * - Modulates brightness with heartbeat effect
 * - BPM varies with energy level
 *----------------------------------------------------------------------------*/
struct PulseLayerConfig : LayerConfig {
    uint8_t minBPM;         // BPM at 0% energy
    uint8_t maxBPM;         // BPM at 100% energy
    uint8_t minBrightness;  // Pulse low point (0-255)
    uint8_t maxBrightness;  // Pulse high point (0-255)
    bool affectsGaugeOnly;  // Only pulse the filled gauge area
    
    PulseLayerConfig() : LayerConfig(),
        minBPM(30),
        maxBPM(120),
        minBrightness(100),
        maxBrightness(255),
        affectsGaugeOnly(true)
    {
        enabled = true;
        blendMode = BLEND_MULTIPLY;
    }
};

/*------------------------------------------------------------------------------
 * Energy Flow Layer Config
 * - Animated light packets moving along the strip
 * - Direction INVARIANTE: toujours gauche → droite
 * - Additive blend creates "energy transfer" effect (CRGB +=)
 *----------------------------------------------------------------------------*/
struct FlowLayerConfig : LayerConfig {
    CRGB packetColor;       // Color of energy packets (orange variant)
    FlowDirection direction; // Kept for API but always LEFT_TO_RIGHT
    uint8_t packetSize;     // Size of each packet in LEDs
    uint8_t packetSpacing;  // Distance between packets
    uint8_t speed;          // Pixels per second (scaled) - synchronisable
    uint8_t trailLength;    // Fade trail behind packet
    uint8_t intensity;      // Packet brightness multiplier
    
    // Synchronization for MiddleMan relay
    bool syncEnabled;       // If true, use external sync parameters
    uint8_t syncSpeed;      // Speed from source rhizome
    
    FlowLayerConfig() : LayerConfig(),
        packetColor(CRGB(255, 120, 30)),  // Orange clair
        direction(DIR_LEFT_TO_RIGHT),      // INVARIANT
        packetSize(3),
        packetSpacing(10),
        speed(50),
        trailLength(3),
        intensity(160),
        syncEnabled(false),
        syncSpeed(50)
    {
        enabled = false;    // Only active during transfers
        blendMode = BLEND_ADD;
    }
};

/*------------------------------------------------------------------------------
 * Event Layer Config
 * - One-shot priority animations
 * - Blocks other updates during playback
 *----------------------------------------------------------------------------*/
struct EventLayerConfig : LayerConfig {
    EventType currentEvent;
    uint32_t eventStartTime;
    uint16_t eventDuration; // Duration in ms
    bool eventActive;
    bool eventTriggered;    // Prevent retriggering
    CRGB eventColor;
    
    EventLayerConfig() : LayerConfig(),
        currentEvent(EVENT_NONE),
        eventStartTime(0),
        eventDuration(500),
        eventActive(false),
        eventTriggered(false),
        eventColor(CRGB::White)
    {
        enabled = true;     // Always listen for events
        blendMode = BLEND_REPLACE;
        opacity = 255;
    }
};

/*------------------------------------------------------------------------------
 * Complete Animation State - All layers combined
 *----------------------------------------------------------------------------*/
struct AnimationState {
    // Layer configurations
    GaugeLayerConfig gauge;
    PulseLayerConfig pulse;
    FlowLayerConfig flow;
    EventLayerConfig event;
    
    // Global parameters
    uint8_t globalBrightness;
    float currentEnergy;        // Cached 0-100
    float displayedEnergy;      // For smooth transitions
    uint8_t numLeds;
    
    // Gauge transition (sliding effect)
    float gaugeOffset;          // 0.0 = left-aligned, 1.0 = right-aligned
    float targetGaugeOffset;    // Target offset for smooth transition
    bool gaugeTransitioning;    // True during slide animation
    
    // Timing
    uint32_t lastUpdateTime;
    uint16_t frameInterval;     // Target frame time in ms
    
    AnimationState() :
        globalBrightness(255),
        currentEnergy(0),
        displayedEnergy(0),
        numLeds(15),
        gaugeOffset(0.0f),
        targetGaugeOffset(0.0f),
        gaugeTransitioning(false),
        lastUpdateTime(0),
        frameInterval(16)       // ~60 FPS
    {}
};

#endif // LAYER_TYPES_H
