#include <Arduino.h>
#include "RhizomeStateAndID.h"

RhizomeStateAndID::RhizomeStateAndID(uint8_t id, uint8_t side)
  : id(id), side(side), state(0), energy(0) {}

uint8_t RhizomeStateAndID::getID() const { return id; }
uint8_t RhizomeStateAndID::getState() const { return state; }
uint8_t RhizomeStateAndID::getEnergy() const { return energy; }
uint8_t RhizomeStateAndID::getSide() const { return side; }

void RhizomeStateAndID::setID(uint8_t newID) { id = newID % 20; }
void RhizomeStateAndID::setState(uint8_t newState) { state = newState % 4; }
void RhizomeStateAndID::setEnergy(uint8_t newEnergy) { energy = constrain(newEnergy, 0, 100); }
void RhizomeStateAndID::setSide(uint8_t newSide) { side = newSide > 1 ? 0 : newSide; }

void RhizomeStateAndID::printDebug(Stream &output) const {
  output.print(F("ID: ")); output.print(id);
  output.print(F(" | State: ")); output.print(state);
  output.print(F(" | Energy: ")); output.print(energy);
  output.print(F("% | Side: ")); output.println(side == 0 ? F("Left") : F("Right"));
}