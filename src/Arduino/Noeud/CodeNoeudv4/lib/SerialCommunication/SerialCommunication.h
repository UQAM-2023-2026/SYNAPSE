/*
#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <NodeStateAndID.h>

void beginSerialCommunication(NodeStateAndID &node);
void checkConnectionStatus();
void lookForMessages(); // call from your main loop()


#endif
*/
#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <NodeStateAndID.h>

void beginSerialCommunication(NodeStateAndID &node);
void checkConnectionStatus();
void lookForMessages();      // call frequently from loop()
float getRhizomeValue();     // 1.0 if connected, 0.0 if not

#endif
