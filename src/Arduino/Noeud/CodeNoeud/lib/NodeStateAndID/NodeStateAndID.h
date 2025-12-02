#ifndef NODE_STATE_AND_ID_H
#define NODE_STATE_AND_ID_H

#include <Arduino.h>

class NodeStateAndID {
public:
  NodeStateAndID(uint8_t id = 0);

  // Getters
  uint8_t getID() const;
  float getDrainRate() const;
  //uint8_t getState() const;


  // Setters
  void setID(uint8_t newID);
  void setDrainRate(float newDrainRate);
  //void setState(uint8_t newState);

  // Debug
  void printDebug(Stream &output = Serial) const;

private:
  uint8_t id;       // 0–19
  float drainRate;  // 0–100
  //uint8_t state;    // 0–3 (idle, generating, giving, error)


};

#endif