#ifndef ENERGY_MANAGEMENT_H
#define ENERGY_MANAGEMENT_H

#include <RhizomeStateAndID.h>

void SetupEnergyManagement(RhizomeStateAndID &rh); // store non-const pointer so we can call setters
void energyLoop(); // call from main loop()

void setNodeDrainRate(float rate);
// void setConnectionToRhizome(bool connected);
// void setConnectionToNode(bool connected);
// void setGeneratingState(bool generating);
// void setNodeDrainRate(float rate);
// void setGenerationRate(float rate);
// void numberOfConnectedRhizomes(int count);

#endif