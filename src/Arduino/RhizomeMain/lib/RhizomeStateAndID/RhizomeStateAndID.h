#ifndef RHIZOME_STATE_AND_ID_H
#define RHIZOME_STATE_AND_ID_H

#include <Arduino.h>

class RhizomeStateAndID {
public:
  RhizomeStateAndID(uint8_t id = 0, uint8_t side = 0);

  // Getters
  uint8_t getID() const;
  uint8_t getState() const;
  uint8_t getEnergy() const;
  uint8_t getSide() const;

  // Setters
  void setID(uint8_t newID);
  void setState(uint8_t newState);
  void setEnergy(uint8_t newEnergy);
  void setSide(uint8_t newSide);

  // Debug
  void printDebug(Stream &output = Serial) const;

private:
  uint8_t id;      // 0–19
  uint8_t state;   // 0–3
  uint8_t energy;  // 0–100
  uint8_t side;    // 0 = left, 1 = right
};

#endif