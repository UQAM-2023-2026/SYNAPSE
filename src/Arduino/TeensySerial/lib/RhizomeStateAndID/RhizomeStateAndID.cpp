#include <Arduino.h>
#include "RhizomeStateAndID.h"

RhizomeStateAndID::RhizomeStateAndID(uint8_t id)
  : id(id), count(0), energy(0), state(0) {}

  // Getters
uint8_t RhizomeStateAndID::getID() const { return id; }
uint8_t RhizomeStateAndID::getCount() const { return count; }
float RhizomeStateAndID::getEnergy() const { return energy; }
uint8_t RhizomeStateAndID::getState() const { return state; }

  // Setters
void RhizomeStateAndID::setID(uint8_t newID) { id = newID % 20; }
void RhizomeStateAndID::setCount(uint8_t newCount) { count = newCount; }
void RhizomeStateAndID::setState(uint8_t newState) { state = newState % 4; }
void RhizomeStateAndID::setEnergy(float newEnergy) { energy = constrain(newEnergy, 0.0f, 100.0f); }

void RhizomeStateAndID::incrementCount() {
  count = (count + 1);
}

// Debug
void RhizomeStateAndID::printDebug(Stream &output) const {
  output.print(F("ID: ")); output.print(id);
  output.print(F("% | Count: ")); output.println(count);
  output.print(F(" | Energy: ")); output.print(energy);
  output.print(F(" | State: ")); output.print(state);
}