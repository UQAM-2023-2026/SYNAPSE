/*==============================================================================
 * LedFeedback.h - LED strip feedback for Rhizome
 *
 * Hardware Configuration:
 * - Male Embout: WS2812B, pin 1, 5 LEDs
 * - Female Embout: WS2812B, pin 0, 5 LEDs  
 * - Tube: APA102, 15 LEDs, Data pin 12, Clock pin 13 (configurable)
 *
 * Color Rules:
 * - ORANGE = energy, WHITE = low energy
 * - CYAN = infra, PURPLE = supra
 * - Global brightness depends on energy level
 * - Lower energy = less saturated color (tends toward white)
 *============================================================================*/

#ifndef LED_FEEDBACK_H
#define LED_FEEDBACK_H

#include <FastLED.h>
#include <RhizomeState.h>

/*------------------------------------------------------------------------------
 * Hardware Configuration
 *----------------------------------------------------------------------------*/
namespace LedConfig {
    // Embout strips (WS2812B)
    constexpr int EMBOUT_LEDS = 5;
    constexpr int PIN_MALE = 1;
    constexpr int PIN_FEMALE = 0;
    
    // Tube strip (APA102) - pins configurable
    constexpr int TUBE_LEDS = 15;
    constexpr int PIN_TUBE_DATA = 12;
    constexpr int PIN_TUBE_CLOCK = 13;
    
    // Colors (CRGB format)
    #define COLOR_ORANGE  CRGB(255, 60, 0)
    #define COLOR_WHITE   CRGB::White
    #define COLOR_CYAN    CRGB(100, 36, 255)   // infra
    #define COLOR_PURPLE  CRGB(99, 200, 142)   // supra
    
    // Timing
    constexpr uint32_t ANIMATION_UPDATE_MS = 30;
    constexpr uint32_t PULSE_DURATION_MS = 800;
    constexpr uint32_t FLASH_DURATION_MS = 200;
    constexpr uint32_t FLASH_CYCLES = 2;
    constexpr uint32_t COLOR_CYCLE_MS = 2000;    // CYAN↔PURPLE cycle period
    
    // Flow animation
    constexpr float FLOW_SPEED = 0.5f;
    constexpr int FLOW_PACKET_SIZE = 3;
    
    // Gauge animation  
    constexpr float GAUGE_DRAIN_SPEED = 0.3f;
}

/*------------------------------------------------------------------------------
 * Animation State for internal tracking
 *----------------------------------------------------------------------------*/
enum class EmboutAnim : uint8_t {
    OFF,
    IDLE_PULSE,         // Pulsation ORANGE→WHITE
    FIXED_ORANGE,       // Orange fixe (no pulse)
    CYCLE_INFRA_SUPRA   // Cycle CYAN↔PURPLE
};

/*------------------------------------------------------------------------------
 * LedFeedback Class
 *----------------------------------------------------------------------------*/
class LedFeedback {
public:
    LedFeedback();
    
    void begin();
    void update(RhizomeState state, float energy, bool maleConnected, bool femaleConnected);
    
    // Callbacks
    void onHeartbeat();
    void onStateChange(RhizomeState oldState, RhizomeState newState);
    void onAllRhizomesFull();  // Event: tous à 100%
    
private:
    // LED arrays
    CRGB _ledsMale[LedConfig::EMBOUT_LEDS];
    CRGB _ledsFemale[LedConfig::EMBOUT_LEDS];
    CRGB _ledsTube[LedConfig::TUBE_LEDS];
    
    // State tracking
    RhizomeState _currentState;
    float _currentEnergy;
    float _displayedEnergy;     // For smooth gauge animation
    bool _maleConnected;
    bool _femaleConnected;    
    // Frame rate limiting
    unsigned long _lastFrameMs;    
    // Heartbeat pulse
    bool _pulseActive;
    uint32_t _pulseStartMs;
    float _pulsePhase;          // 0.0-1.0
    
    // Color cycle (CYAN↔PURPLE)
    uint32_t _cycleStartMs;
    
    // Flash event (100% celebration)
    bool _flashActive;
    uint32_t _flashStartMs;
    uint8_t _flashCycleCount;
    
    // Flow animation
    float _flowPosition;
    uint32_t _lastAnimMs;
    
    // Animation states per embout
    EmboutAnim _maleAnim;
    EmboutAnim _femaleAnim;
    
    // Core rendering
    void renderEmbout(CRGB* leds, EmboutAnim anim);
    void renderTube();
    void renderTubeIdle();
    void renderTubeGenerating();
    void renderTubeGiving();
    void renderTubeMiddleman();
    void renderFlash();
    
    // Animation helpers
    void updateAnimationStates();
    void updatePulse();
    void updateFlow();
    void updateGaugeDrain();
    
    // Color helpers
    CRGB getEnergyColor(float saturation);
    CRGB getCycleColor();
    uint8_t getGlobalBrightness();
    float getEnergySaturation();
    int getGaugeLedCount();
    
    // Utility
    void clearAll();
};

// Global instance
extern LedFeedback ledFeedback;

// Callback for HeartbeatSystem
void ledHeartbeatCallback();

#endif // LED_FEEDBACK_H
