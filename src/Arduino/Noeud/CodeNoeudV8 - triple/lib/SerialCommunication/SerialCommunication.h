#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <NodeStateAndID.h>
#include <MicroOsc.h>

void beginSerialCommunication(NodeStateAndID &node);
void SerialLoop();
void loopSendToTouch();

#endif