#include <Arduino.h>
#include "NodeStateAndID.h"

NodeStateAndID::NodeStateAndID(uint8_t id)
  : id(id), drainRate(0.5f) {}

  // Getters
uint8_t NodeStateAndID::getID() const { return id; }
float NodeStateAndID::getDrainRate() const { return drainRate; }
//uint8_t NodeStateAndID::getState() const { return state; }

  // Setters
void NodeStateAndID::setID(uint8_t newID) { id = newID % 20; }
void NodeStateAndID::setDrainRate(float newDrainRate) { drainRate = constrain(newDrainRate, 0.0f, 100.0f); }

// Debug
void NodeStateAndID::printDebug(Stream &output) const {
  output.print(F("ID: ")); output.print(id);
  output.print(F(" | Drain Rate: ")); output.print(drainRate);
}