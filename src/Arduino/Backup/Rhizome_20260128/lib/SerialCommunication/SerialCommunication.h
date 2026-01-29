#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <RhizomeStateAndID.h>

void SetupSerialCommunication(RhizomeStateAndID &rh);

// Main loop function - call repeatedly in loop()
void checkConnectionStatus();
void updateConnectionStates();

//State machine
void onLeftConnected();
void onLeftDisconnected();
void onRightConnected();
void onRightDisconnected();
void checkBothDisconnected();
void resetToIdle();

// Utility functions for managing seen IDs
void clearSeenIds();
bool isIdSeen(int id);
void addSeenId(int id);
int getSeenIdsCount();
int getSeenIdAt(int index);

// Status monitoring
//void printRhizomeStatus(); // Print current status on demand
void printStatus();

#endif