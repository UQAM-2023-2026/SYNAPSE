#ifndef SERIALCOMMUNICATION_H
#define SERIALCOMMUNICATION_H

#include <Arduino.h>
#include <NodeStateAndID.h>

// Accept three HardwareSerial references for the three UARTs
void beginSerialCommunication(NodeStateAndID& node, HardwareSerial& uart0, HardwareSerial& uart1, HardwareSerial& uart2);

void checkConnectionStatus();
void lookForMessages();

#endif