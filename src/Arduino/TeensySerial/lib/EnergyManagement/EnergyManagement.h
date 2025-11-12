#ifndef ENERGY_MANAGEMENT_H
#define ENERGY_MANAGEMENT_H

#include <RhizomeStateAndID.h>

void beginEnergyManagement(RhizomeStateAndID &rh); // store non-const pointer so we can call setters
void energyLoop(); // call from main loop()
float getManagedEnergy(); // read current cached energy

#endif