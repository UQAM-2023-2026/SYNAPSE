/*==============================================================================
 * EnergyManager.h - Energy management system
 * 
 * Manages energy based on current state:
 * - IDLE: Slow regeneration to threshold (10%)
 * - DISCOVERING: Same as IDLE
 * - GENERATING: Energy generation (rate based on rhizome count)
 * - GIVING: Energy drain at node's rate
 * - MIDDLEMAN: NO energy change (only relay)
 * - DEAD + disconnected: Slow regen to 10%, then transition to IDLE
 * - DEAD + connected: Energy stays at 0
 *============================================================================*/

#ifndef ENERGY_MANAGER_H
#define ENERGY_MANAGER_H

#include <Arduino.h>
#include <RhizomeState.h>

// Forward declarations
class RhizomeData;
class RhizomeStateMachine;

// Callbacks
using EnergyFullCallback = void (*)(void);
using EnergyDepletedCallback = void (*)(void);
using EnergyRestoredCallback = void (*)(void);  // Called when DEAD reaches 10%

class EnergyManager {
public:
    EnergyManager();
    
    // Initialize with dependencies
    void begin(RhizomeData* data, RhizomeStateMachine* stateMachine);
    
    // Call in loop() - updates energy based on state and connection
    void update(bool maleConnected, bool femaleConnected);
    
    // Set drain rate (from node)
    void setDrainRate(float rate) { _drainRate = rate; }
    float getDrainRate() const { return _drainRate; }
    
    // Callbacks
    void onEnergyFull(EnergyFullCallback cb) { _onEnergyFull = cb; }
    void onEnergyDepleted(EnergyDepletedCallback cb) { _onEnergyDepleted = cb; }
    void onEnergyRestored(EnergyRestoredCallback cb) { _onEnergyRestored = cb; }
    
private:
    RhizomeData* _data;
    RhizomeStateMachine* _stateMachine;
    
    // Timing
    unsigned long _lastUpdate;
    static constexpr unsigned long UPDATE_INTERVAL_MS = 100;
    
    // Energy rates
    float _drainRate;                     // Set by node
    static constexpr float BASE_REGEN_RATE = 0.25f;    // Slow regen
    static constexpr float ENERGY_THRESHOLD = 20.0f;   // IDLE/DEAD regen cap
    static constexpr float MAX_ENERGY = 100.0f;
    static constexpr float MIN_ENERGY = 0.0f;
    
    // State tracking for edge detection
    bool _wasAtMax;
    bool _wasAtMin;
    
    // Callbacks
    EnergyFullCallback _onEnergyFull;
    EnergyDepletedCallback _onEnergyDepleted;
    EnergyRestoredCallback _onEnergyRestored;
    
    // Calculate generation rate based on rhizome count
    float getGenerationRate() const;
    
    // Apply energy update based on state
    void updateEnergyForState(RhizomeState state, bool bothDisconnected);
};

// Global instance
extern EnergyManager energyManager;

#endif // ENERGY_MANAGER_H
