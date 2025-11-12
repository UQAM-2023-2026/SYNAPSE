#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <RhizomeStateAndID.h>

void beginSerialCommunication(RhizomeStateAndID &rh);
void lookForMessages(); // call from your main loop()

#endif