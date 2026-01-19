#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <NodeStateAndID.h>
#include <MicroOsc.h>

// ===== POGOPIN 1 (UART1) PIN DEFINITIONS =====
#define POGOPIN1_RX_PIN 5      // GPIO5 (blue wire)
#define POGOPIN1_TX_PIN 4      // GPIO4 (green wire)
#define POGOPIN1_FLAG_PIN 13   // GPIO13 (connection detection flag)

// ===== POGOPIN 2 (UART2) PIN DEFINITIONS =====
#define POGOPIN2_RX_PIN 35     // GPIO35 (blue wire)
#define POGOPIN2_TX_PIN 33     // GPIO33 (green wire)
#define POGOPIN2_FLAG_PIN 32   // GPIO32 (connection detection flag)

// ===== POGOPIN 3 PIN DEFINITIONS =====
#define POGOPIN3_RX_PIN 16     // GPIO16 (blue wire) - Available on header
#define POGOPIN3_TX_PIN 15     // GPIO15 (green wire) - Available on header
#define POGOPIN3_FLAG_PIN 14   // GPIO14 (connection detection flag) - Available on header

// Legacy alias for compatibility
#define CONNECT_PIN POGOPIN1_FLAG_PIN

// UART configuration
#define SERIAL_BAUD 9600

// Timing constants - BALANCED FOR SPEED AND STABILITY
#define CONNECTION_TIMEOUT 250       // 250ms without /energy = disconnected (reduced from 500ms)
#define CONNECT_DEBOUNCE 100         // Debounce time for connection (balanced)
#define DISCONNECT_DEBOUNCE 100      // Debounce time for disconnection (reduced from 1500ms)
#define NODE_SEND_INTERVAL 1000      // Resend /node every 1s while connected

// ===== FUNCTION DECLARATIONS =====
void beginSerialCommunication(NodeStateAndID &node);
void checkConnectionStatus();
void lookForMessages(MicroOscMessage &msg);
float getRhizomeValue();     // 1.0 if connected, 0.0 if not
void loopSendToTouch();
void SerialLoop();

#endif