#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <NodeStateAndID.h>
#include <MicroOsc.h>

// ===== POGOPIN 1 (UART1) PIN DEFINITIONS =====
#define POGOPIN1_RX_PIN 5      // GPIO5 (blue wire)
#define POGOPIN1_TX_PIN 4      // GPIO4 (green wire)
#define POGOPIN1_FLAG_PIN 13   // GPIO13 (connection detection flag)
// NOTE: GPIO13 may have special behavior on some ESP32 boards (boot mode pin)
// If FLAG1 detection doesn't work, consider using a different GPIO

// ===== POGOPIN 2 (UART2) PIN DEFINITIONS =====
#define POGOPIN2_RX_PIN 35     // GPIO35 (blue wire)
#define POGOPIN2_TX_PIN 33     // GPIO33 (green wire)
#define POGOPIN2_FLAG_PIN 32   // GPIO32 (connection detection flag)

// Legacy alias for compatibility
#define CONNECT_PIN POGOPIN1_FLAG_PIN

// UART configuration
// ESP32 has 3 hardware UARTs: Serial (UART0), Serial1 (UART1), Serial2 (UART2)
// We'll use Serial1 and Serial2 with custom pins
#define SERIAL_BAUD 9600

// Timing constants
#define CONNECTION_TIMEOUT 500      // 500ms without /energy = disconnected
#define CONNECT_DEBOUNCE 300         // Debounce time for connection
#define DISCONNECT_DEBOUNCE 1500     // Debounce time for disconnection
#define NODE_SEND_INTERVAL 1000     // Resend /node every 1s while connected

// ===== FUNCTION DECLARATIONS =====
void beginSerialCommunication(NodeStateAndID &node);
void checkConnectionStatus();
void lookForMessages(MicroOscMessage &msg);
float getRhizomeValue();     // 1.0 if connected, 0.0 if not
void loopSendToTouch();
void SerialLoop();

#endif