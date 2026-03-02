/*==============================================================================
 * RhizomeData.h - Rhizome data container
 * 
 * Passive data holder for rhizome identity and metrics.
 * No business logic - just storage and accessors.
 *============================================================================*/

#ifndef RHIZOME_DATA_H
#define RHIZOME_DATA_H

#include <Arduino.h>

class RhizomeData {
public:
    explicit RhizomeData(uint8_t id) : _id(id), _energy(10.0f), _count(1) {}
    
    // Identity
    uint8_t getId() const { return _id; }
    void setId(uint8_t id) { _id = id; }
    
    // Energy (0-100%)
    float getEnergy() const { return _energy; }
    void setEnergy(float energy) { 
        _energy = constrain(energy, 0.0f, 100.0f); 
    }
    
    // Rhizome count in current chain/loop
    uint8_t getCount() const { return _count; }
    void setCount(uint8_t count) { _count = count; }
    
private:
    uint8_t _id;
    float _energy;
    uint8_t _count;
};

#endif // RHIZOME_DATA_H
