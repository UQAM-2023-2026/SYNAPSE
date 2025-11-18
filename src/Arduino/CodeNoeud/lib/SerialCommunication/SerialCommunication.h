#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <NodeStateAndID.h>

void beginSerialCommunication(NodeStateAndID &node);
void checkConnectionStatus();
void lookForMessages(); // call from your main loop()

#endif