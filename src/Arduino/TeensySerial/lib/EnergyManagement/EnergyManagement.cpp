#include <Arduino.h>
#include "EnergyManagement.h"
#include <RhizomeStateAndID.h>
#include "SharedState.h"

// ==========================================================
// Gestion d’énergie du rhizome - Projet Synapse
// ==========================================================

static RhizomeStateAndID *pRhizome = nullptr;
static float energy = 0.0f;

float baseRegenRate = 0.05;      // Recharge lente (quand <10%)
float decayRate = 0.03;         // Perte d’énergie lente (quand >10%)
float nodeDrainRate = 0.0;       // Taux de vidange imposé par le nœud
float generationRate = 0.5;      // Taux de génération quand connecté
float maxEnergy = 100.0;
float minEnergy = 0.0;

bool connectedToRhizome = false;
bool connectedToNode = false;
bool isGenerating = false;

// Intervalle de mise à jour
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100; // ms

void updateEnergy();


void beginEnergyManagement(RhizomeStateAndID &rh) {
  pRhizome = &rh;
  if (pRhizome) energy = pRhizome->getEnergy();
  Serial.print("Energy Management Initialized. Energy: ");
  Serial.println(energy);
}

// Main loop
void energyLoop() {
  unsigned long currentTime = millis();
  if (currentTime - lastUpdate >= updateInterval) {
    lastUpdate = currentTime;
    // update generationRate based on latest discovered count
    numberOfConnectedRhizomes(connectedRhizomesCount);
    updateEnergy();
    Serial.println(energy);
  }
}

float getManagedEnergy() {
  return energy;
}

// ----------------------------------------------------------
// Fonction principale de gestion d’énergie
// ----------------------------------------------------------
void updateEnergy() {
  if (isGenerating && connectedToRhizome) {
    // Génération d’énergie entre rhizomes connectés
    energy += generationRate;
    if (energy > maxEnergy) energy = maxEnergy;

  } else if (connectedToNode && nodeDrainRate > 0) {
    // Si connecté au nœud, celui-ci peut drainer l’énergie sous 10%
    energy -= nodeDrainRate;
    if (energy < minEnergy) energy = minEnergy;

  } else {
    // Gestion autonome du rhizome
    if (energy <= 10) {
      // Recharge lente jusqu’à 10%
      energy += baseRegenRate;
      if (energy > 10) energy = 10;

    } else if (energy > 10 && !isGenerating) {
      // Descente lente vers 10%
      energy -= decayRate;
      if (energy < 10) energy = 10;
    }
  }
  pRhizome->setEnergy(energy);
  //Serial.println(generationRate);
}

void setConnectionToRhizome(bool connected) {
  connectedToRhizome = connected;
}

void setConnectionToNode(bool connected) {
  connectedToNode = connected;
}

void setGeneratingState(bool generating) {
  isGenerating = generating;
}

void setNodeDrainRate(float rate) {
  nodeDrainRate = rate;
}

void setGenerationRate(float rate) {
  generationRate = rate;
}

void numberOfConnectedRhizomes(int count) {
  // This function can be used to adjust energy management based on the number of connected rhizomes
  if( count <= 1) {
    generationRate = 0.5;
  } else if( count == 2) {
    generationRate = 0.75;
  } else if( count == 3) {
    generationRate = 1.0;
  } else if ( count >= 4) {
    generationRate = 1.25;
  }
}
