#ifndef RHIZOME_STATE_AND_ID_H
#define RHIZOME_STATE_AND_ID_H

#include <Arduino.h>

class RhizomeStateAndID {
public:
  RhizomeStateAndID(uint8_t id = 0);

  // Getters
  uint8_t getID() const;
  uint8_t getCount() const;
  float getEnergy() const;
  uint8_t getState() const;


  // Setters
  void setID(uint8_t newID);
  void setCount(uint8_t newCount);
  void setEnergy(float newEnergy);
  void setState(uint8_t newState);
 

  // Debug
  void printDebug(Stream &output = Serial) const;

private:
  uint8_t id;       // 0–19
  uint8_t count;    // Number of rhizome
  uint8_t state;    // 0–3 (idle, generating, giving, error)
  float energy;   // 0–100

};

#endif