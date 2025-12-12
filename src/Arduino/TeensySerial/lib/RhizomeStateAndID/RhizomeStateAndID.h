#ifndef RHIZOME_STATE_AND_ID_H
#define RHIZOME_STATE_AND_ID_H

#include <Arduino.h>

// États du rhizome
enum RhizomeState {
  IDLE = 0,           // Aucune connexion ou connexion incomplète
  GENERATING = 1,     // Boucle fermée - génère de l'énergie
  GIVING_TO_NODE = 2, // Connecté à un nœud, se vide selon drainRate
  MIDDLEMAN = 3,      // Relais entre rhizome et nœud, ne se vide pas
  DEAD = 4            // Énergie épuisée, LEDs éteintes, pas d'envoi
};

class RhizomeStateAndID {
public:
  RhizomeStateAndID(uint8_t id = 0);

  // Getters
  uint8_t getID() const;
  uint8_t getCount() const;
  float getEnergy() const;
  RhizomeState getState() const;


  // Setters
  void setID(uint8_t newID);
  void setCount(uint8_t newCount);
  void setEnergy(float newEnergy);
  void setState(RhizomeState newState);

  void incrementCount();
 

  // Debug
  void printDebug(Stream &output = Serial) const;

private:
  uint8_t id;          // 0–19
  uint8_t count;       // Number of rhizome
  RhizomeState state;  // IDLE, GENERATING, GIVING_TO_NODE, MIDDLEMAN
  float energy;        // 0–100

};

#endif