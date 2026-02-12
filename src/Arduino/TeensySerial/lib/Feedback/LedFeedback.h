/*==============================================================================
 * LedFeedback.h - LED strip feedback using Gauge/Flow/Pulse layers
 *
 * LED Behavior Specification:
 * - GAUGE: Shows energy level, always MALE→FEMALE direction
 * - FLOW: State-dependent particle animation
 * - PULSE: HeartbeatSystem modulation (0.5-1.0 brightness)
 * 
 * State behaviors:
 * - IDLE: Gauge + pulse, no flow
 * - DISCOVERING: Gauge + pulse + subtle seeking flow (FEMALE→MALE)
 * - GENERATING: Gauge + pulse + Tetris packets (FEMALE→gauge, lock on arrival)
 * - GIVING: Gauge + pulse + packet detach (gauge→MALE)
 * - MIDDLEMAN: Solid orange + passthrough flow (FEMALE→MALE), no pulse
 * - DEAD: All LEDs off
 *============================================================================*/

#ifndef LED_FEEDBACK_H
#define LED_FEEDBACK_H

#include <FastLED.h>
#include <RhizomeState.h>

/*------------------------------------------------------------------------------
 * Configuration
 *----------------------------------------------------------------------------*/
namespace LedConfig {
    // Strip configuration
    constexpr int NUM_LEDS = 5;
    constexpr int LED_PIN_MALE = 1;
    constexpr int LED_PIN_FEMALE = 0;
    
    // Primary color
    constexpr uint8_t R = 255;
    constexpr uint8_t G = 60;
    constexpr uint8_t B = 0;
    
    // Timing
    constexpr uint32_t FLOW_UPDATE_MS = 30;
    constexpr uint32_t GAUGE_UPDATE_MS = 50;
    
    // Flow speeds (pixels per update)
    constexpr float SEEKING_SPEED = 0.3f;
    constexpr float INWARD_SPEED = 0.8f;
    constexpr float OUTWARD_SPEED = 0.8f;
    constexpr float PASSTHROUGH_SPEED = 1.0f;
    
    // Pulse: lerp toward white on heartbeat (preserves hue)
    // Base brightness applied as multiplier, accent blends toward white
    constexpr float PULSE_BASE = 0.85f;     // Baseline brightness multiplier
    constexpr float PULSE_ACCENT = 0.15f;   // Lerp amount toward white on beat (0.0-1.0)
    
    // Tetris packet
    constexpr int PACKET_SIZE = 3;  // LEDs per packet
    constexpr float PACKET_SPEED = 0.5f;
}

/*------------------------------------------------------------------------------
 * Flow Types
 *----------------------------------------------------------------------------*/
enum class FlowType {
    NONE,           // No flow
    SEEKING,        // Subtle FEMALE→MALE (discovering)
    INWARD,         // FEMALE→gauge (generating)
    OUTWARD,        // Gauge→MALE (giving)
    PASSTHROUGH     // FEMALE→MALE solid (middleman)
};

/*------------------------------------------------------------------------------
 * Particle for Flow Layer
 *----------------------------------------------------------------------------*/
struct FlowParticle {
    float position;     // 0.0 to NUM_LEDS
    float speed;
    uint8_t brightness;
    bool active;
    
    FlowParticle() : position(0), speed(0), brightness(0), active(false) {}
};

/*------------------------------------------------------------------------------
 * Packet for Tetris-style Generation
 *----------------------------------------------------------------------------*/
struct EnergyPacket {
    float position;     // Current position
    int targetLed;      // LED to lock onto (gauge edge)
    bool active;
    bool locked;        // Has reached target
    
    EnergyPacket() : position(0), targetLed(0), active(false), locked(false) {}
};

/*------------------------------------------------------------------------------
 * LedFeedback Class
 *----------------------------------------------------------------------------*/
class LedFeedback {
public:
    LedFeedback();
    
    // Initialization
    void begin();
    
    // Main update - call every loop
    void update(RhizomeState state, float energy, bool maleConnected, bool femaleConnected);
    
    // Heartbeat callback - sets pulse phase
    void onHeartbeat();
    
    // State change notification
    void onStateChange(RhizomeState oldState, RhizomeState newState);
    
private:
    // LED arrays (APA102 on data 12, clock 13)
    CRGB _leds[LedConfig::NUM_LEDS];
    
    // State tracking
    RhizomeState _currentState;
    float _currentEnergy;
    bool _maleConnected;
    bool _femaleConnected;
    
    // Pulse layer
    float _pulsePhase;          // 0.0 to 1.0
    float _pulseBrightness;     // Base brightness multiplier
    float _pulseOffset;         // Lerp amount toward white (0.0 to PULSE_ACCENT)
    uint32_t _pulseStartMs;     // When current pulse started
    bool _pulseActive;          // Is in pulse cycle
    
    // Flow layer
    FlowType _flowType;
    static constexpr int MAX_PARTICLES = 8;
    FlowParticle _particles[MAX_PARTICLES];
    uint32_t _lastFlowUpdateMs;
    
    // Generation packets (Tetris)
    static constexpr int MAX_PACKETS = 4;
    EnergyPacket _packets[MAX_PACKETS];
    uint32_t _lastPacketSpawnMs;
    int _lockedLedCount;        // How many LEDs have locked packets
    
    // Timing
    uint32_t _lastGaugeUpdateMs;
    
    // Layer methods
    void updateGauge(float energy);
    void updateFlow();
    void updatePulse();
    void updatePackets(float energy);
    
    // Flow control
    void setFlowType(FlowType type);
    void spawnParticle();
    void updateParticles();
    void renderParticles();
    
    // Packet control (Tetris generation)
    void spawnPacket(int targetLed);
    void updatePacketPositions();
    void renderPackets();
    int getGaugeEdgeLed(float energy);
    
    // Helpers
    void clearAll();
    void setAllSolid(uint8_t brightness);
    void applyPulse();
    CRGB getColor(uint8_t brightness);
};

// Global instance
extern LedFeedback ledFeedback;

// Callback for HeartbeatSystem
void ledHeartbeatCallback();

#endif // LED_FEEDBACK_H
