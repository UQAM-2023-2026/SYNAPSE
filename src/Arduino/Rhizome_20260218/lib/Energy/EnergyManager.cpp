/*==============================================================================
 * EnergyManager.cpp - Energy management system implementation
 *============================================================================*/

#include "EnergyManager.h"
#include <RhizomeData.h>
#include <RhizomeStateMachine.h>

// Global instance
EnergyManager energyManager;

EnergyManager::EnergyManager()
    : _data(nullptr)
    , _stateMachine(nullptr)
    , _lastUpdate(0)
    , _drainRate(0.0f)
    , _wasAtMax(false)
    , _wasAtMin(false)
    , _onEnergyFull(nullptr)
    , _onEnergyDepleted(nullptr)
    , _onEnergyRestored(nullptr)
{}

void EnergyManager::begin(RhizomeData* data, RhizomeStateMachine* stateMachine) {
    _data = data;
    _stateMachine = stateMachine;
    _lastUpdate = millis();
    
    if (_data) {
        _wasAtMax = (_data->getEnergy() >= MAX_ENERGY);
        _wasAtMin = (_data->getEnergy() <= MIN_ENERGY);
    }
    
    Serial.println("[EnergyManager] Initialized");
}

void EnergyManager::update(bool maleConnected, bool femaleConnected) {
    if (!_data || !_stateMachine) return;
    
    unsigned long now = millis();
    if (now - _lastUpdate < UPDATE_INTERVAL_MS) return;
    _lastUpdate = now;
    
    RhizomeState state = _stateMachine->getState();
    bool bothDisconnected = !maleConnected && !femaleConnected;
    updateEnergyForState(state, bothDisconnected);
}

float EnergyManager::getGenerationRate() const {
    if (!_data) return 0.25f;
    
    uint8_t count = _data->getCount();
    
    if (count <= 1) return 0.25f;
    if (count == 2) return 0.35f;
    if (count == 3) return 0.50f;
    return 0.75f;  // 4 or more
}

void EnergyManager::updateEnergyForState(RhizomeState state, bool bothDisconnected) {
    float energy = _data->getEnergy();
    bool wasDeadAndLow = (state == RhizomeState::DEAD && energy < ENERGY_THRESHOLD);
    
    switch (state) {
        case RhizomeState::DEAD:
            if (bothDisconnected) {
                // DEAD + disconnected: slow regen to 10%
                if (energy < ENERGY_THRESHOLD) {
                    energy += BASE_REGEN_RATE;
                    if (energy > ENERGY_THRESHOLD) energy = ENERGY_THRESHOLD;
                }
            } else {
                // DEAD + connected: stay at 0, no regen
                energy = MIN_ENERGY;
            }
            break;
            
        case RhizomeState::MIDDLEMAN:
            // NO energy change in MIDDLEMAN - we only relay
            break;
            
        case RhizomeState::GENERATING:
            // Generate energy based on rhizome count
            energy += getGenerationRate();
            if (energy > MAX_ENERGY) energy = MAX_ENERGY;
            break;
            
        case RhizomeState::GIVING:
            // Drain energy at node's rate
            if (_drainRate > 0.0f) {
                energy -= _drainRate;
                if (energy < MIN_ENERGY) energy = MIN_ENERGY;
            }
            break;
            
        case RhizomeState::IDLE:
        case RhizomeState::DISCOVERING:
        default:
            // Slow regeneration up to threshold
            if (energy < ENERGY_THRESHOLD) {
                energy += BASE_REGEN_RATE;
                if (energy > ENERGY_THRESHOLD) energy = ENERGY_THRESHOLD;
            }
            break;
    }
    
    _data->setEnergy(energy);
    
    // Edge detection: just reached max
    bool isAtMax = (energy >= MAX_ENERGY);
    if (isAtMax && !_wasAtMax) {
        Serial.println("[ENERGY] Reached 100%");
        if (_onEnergyFull) _onEnergyFull();
    }
    _wasAtMax = isAtMax;
    
    // Edge detection: just reached min (depleted)
    bool isAtMin = (energy <= MIN_ENERGY);
    if (isAtMin && !_wasAtMin) {
        Serial.println("[ENERGY] Depleted to 0%");
        if (_onEnergyDepleted) _onEnergyDepleted();
    }
    _wasAtMin = isAtMin;
    
    // Edge detection: DEAD reached threshold (restored)
    if (state == RhizomeState::DEAD && wasDeadAndLow && energy >= ENERGY_THRESHOLD) {
        Serial.println("[ENERGY] Restored to 10% while DEAD");
        if (_onEnergyRestored) _onEnergyRestored();
    }
}
