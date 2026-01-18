#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <NodeStateAndID.h>
#include <MicroOsc.h>

// UART Configuration Constants
// UART1: Original working setup (GPIO5=RX, GPIO4=TX)
#define UART1_RX_PIN 5     // GPIO5 (FREE, 10k pull-up) - YOUR ORIGINAL WORKING RX
#define UART1_TX_PIN 4     // GPIO4 (UEXT pin - FREE) - YOUR ORIGINAL WORKING TX
#define UART1_CONNECT_PIN 0  // GPIO13 (UEXT pin - 2.2k pull-up, FREE)

// UART2: Second pogo pin
#define UART2_RX_PIN 5    // GPIO35 (10k pull-up, input-only)
#define UART2_TX_PIN 4    // GPIO33 (FREE)
#define UART2_CONNECT_PIN 13  // GPIO32 (FREE)

#define SERIAL_BAUD 9600

void beginSerialCommunication(NodeStateAndID &node);
void checkConnectionStatus();
void lookForMessages(MicroOscMessage &msg);
float getRhizomeValue();
void loopSendToTouch();
void SerialLoop();


#endif