#include <Arduino.h>
#include "NodeStateAndID.h"

NodeStateAndID::NodeStateAndID(uint8_t id)
  : id(id), drainRateInfra(0), drainRateSupra(0) {}

// Getters
uint8_t NodeStateAndID::getID() const { return id; }
float NodeStateAndID::getDrainRateInfra() const { return drainRateInfra; }
float NodeStateAndID::getDrainRateSupra() const { return drainRateSupra; }
//uint8_t NodeStateAndID::getState() const { return state; }

// Setters
void NodeStateAndID::setID(uint8_t newID) { id = newID % 20; }
void NodeStateAndID::setDrainRateInfra(float newDrainRate) { 
  drainRateInfra = constrain(newDrainRate, 0.0f, 100.0f); 
}
void NodeStateAndID::setDrainRateSupra(float newDrainRate) { 
  drainRateSupra = constrain(newDrainRate, 0.0f, 100.0f); 
}

// Debug
void NodeStateAndID::printDebug(Stream &output) const {
  output.print(F("ID: ")); output.print(id);
  output.print(F(" | Drain Rate Infra: ")); output.print(drainRateInfra);
  output.print(F(" | Drain Rate Supra: ")); output.print(drainRateSupra);
}