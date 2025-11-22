/*-------------------Libraries----------------------*/
#include "EnergyManagement.h"

#include <Arduino.h>

#include <RhizomeStateAndID.h>
static RhizomeStateAndID *pRhizome = nullptr; // pointer to external rhizome object

// ==========================================================
// Gestion d’énergie du rhizome - Projet Synapse
// ==========================================================

/*----------------Energy variables-------------------- */
static float energy = 0.0f;

float baseRegenRate = 0.05;      // Recharge lente (quand <10%)
float decayRate = 0.05;         // Perte d’énergie lente (quand >10%)
float nodeDrainRate = 0.0;       // Taux de vidange imposé par le nœud
float generationRate = 0.5;      // Taux de génération quand connecté
float maxEnergy = 100.0;
float minEnergy = 0.0;
float energyThreshold = 10.0; // Seuil de 10%

//bool connectedToRhizome = false;
//bool connectedToNode = false;
//bool isGenerating = false;

// Intervalle de mise à jour
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 100; // ms
/*--------------------------------------------------*/



void SetupEnergyManagement(RhizomeStateAndID &rh) {
  pRhizome = &rh;
  
  if (pRhizome) energy = pRhizome->getEnergy();
  Serial.print("Energy Management Initialized. Energy: ");
  Serial.println(energy);
}

/*-------------Setters------------------*/
// void setConnectionToRhizome(bool connected) {
//   connectedToRhizome = connected;
// }

// void setConnectionToNode(bool connected) {
//   connectedToNode = connected;
// }

// void setGeneratingState(bool generating) {
//   isGenerating = generating;
// }

void setNodeDrainRate(float rate) {
  nodeDrainRate = rate;
}

// void setGenerationRate(float rate) {
//   generationRate = rate;
// }

void setGenerationRate(int count) {
  // This function can be used to adjust energy management based on the number of connected rhizomes
  if( count <= 1) {
    generationRate = 0.5;
  } else if( count == 2) {
    generationRate = 0.6;
  } else if( count == 3) {
    generationRate = 0.75;
  } else if ( count >= 4) {
    generationRate = 1.0;
  }
}
/*--------------------------------------------------*/

/*----------------MAIN ENERGY UPDATE------------------*/
void updateEnergy() {
  if (pRhizome->getState() == 2) {
    // Génération d’énergie entre rhizomes connectés
    energy += generationRate;
    if (energy > maxEnergy) energy = maxEnergy;

  } else if (pRhizome->getState() == 3 && nodeDrainRate > 0) {
    // Si connecté au nœud, celui-ci peut drainer l’énergie sous 10%
    energy -= nodeDrainRate;
    if (energy < minEnergy) energy = minEnergy;

  } else {
    // Gestion autonome du rhizome
    if (energy <= energyThreshold) {
      // Recharge lente jusqu’à 10%
      energy += baseRegenRate;
      if (energy > energyThreshold) energy = energyThreshold;

    } else if (energy > energyThreshold && pRhizome->getState() != 2) {
      // Descente lente vers 10%
      energy -= decayRate;
      if (energy < energyThreshold) energy = energyThreshold;
    }
  }
  pRhizome->setEnergy(energy);
  //Serial.println(generationRate);
}
/*--------------------------------------------------*/

/*----------------ENERGY LOOP------------------*/
void energyLoop() {
  unsigned long currentTime = millis();
  if (currentTime - lastUpdate >= updateInterval) {
    lastUpdate = currentTime;

    // update generationRate based on count
    setGenerationRate(pRhizome->getCount());
    updateEnergy();
    //Serial.println(energy);
  }
}
/*--------------------------------------------------*/






