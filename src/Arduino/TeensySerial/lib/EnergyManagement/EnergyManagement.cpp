#include <Arduino.h>
#include "EnergyManagement.h"
#include <RhizomeStateAndID.h>

// ==========================================================
// Gestion d’énergie du rhizome - Projet Synapse
// ==========================================================

static RhizomeStateAndID *pRhizome = nullptr;
static float energy = 0.0f;

float baseRegenRate = 0.02;      // Recharge lente (quand <10%)
float decayRate = 0.015;         // Perte d’énergie lente (quand >10%)
float nodeDrainRate = 0.0;       // Taux de vidange imposé par le nœud
float generationRate = 0.5;      // Taux de génération quand connecté
float maxEnergy = 1.0;
float minEnergy = 0.0;

bool connectedToRhizome = false;
bool connectedToNode = false;
bool isGenerating = false;

// Intervalle de mise à jour
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100; // ms


void beginEnergyManagement(RhizomeStateAndID &rh) {
  pRhizome = &rh;
  if (pRhizome) energy = pRhizome->getEnergy();
}

// renamed to avoid Arduino symbol collision
void energyLoop() {
  unsigned long currentTime = millis();
  if (currentTime - lastUpdate >= updateInterval) {
    lastUpdate = currentTime;
    updateEnergy();
  }

  // DEBUG - affichage de l'énergie
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 1000) {
    lastPrint = millis();
    Serial.print("Energie: ");
    Serial.println(energy);
  }

  // Exemple de changements d’état pour tester
  simulateConnections();
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
    if (energy <= 0) {
      // Recharge lente jusqu’à 10%
      energy += baseRegenRate;
      if (energy > 10) energy = 10;

    } else if (energy > 10 && !isGenerating) {
      // Descente lente vers 10%
      energy -= decayRate;
      if (energy < 10) energy = 10;
    }
  }
}