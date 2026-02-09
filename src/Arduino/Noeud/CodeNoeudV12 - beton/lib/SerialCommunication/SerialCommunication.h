#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <Arduino.h>
#include "NodeStateAndID.h"
#include <MicroOsc.h>

// ===== POGOPIN 1 (UART1) PIN DEFINITIONS =====
#define POGOPIN1_RX_PIN 5      // GPIO5 (blue wire)
#define POGOPIN1_TX_PIN 4      // GPIO4 (green wire)
#define POGOPIN1_FLAG_PIN 13   // GPIO13 (connection detection flag)

// ===== POGOPIN 2 (UART2) PIN DEFINITIONS =====
#define POGOPIN2_RX_PIN 35     // GPIO35 (blue wire)
#define POGOPIN2_TX_PIN 33     // GPIO33 (green wire)
#define POGOPIN2_FLAG_PIN 32   // GPIO32 (connection detection flag)

// Legacy alias for compatibility
#define CONNECT_PIN POGOPIN1_FLAG_PIN

// UART configuration
#define SERIAL_BAUD 9600

// Timing constants
#define CONNECTION_TIMEOUT 10000     // 10s without /energy = consider disconnected
                                      // With 10ms loop delay and ~1Hz /energy messages, this should never trigger
                                      // unless communication actually stops
#define HANDSHAKE_TIMEOUT 3000       // 3s after sending /node without /energy = retry
#define NODE_RESEND_INTERVAL 1000    // Minimum 1s between /node sends
#define FLAG_DISCONNECT_DEBOUNCE 100 // 100ms debounce for FLAG-based disconnect
                                      // Prevents momentary bounces from killing connection
                                      // While still allowing fast disconnect on real removal

// Debug verbosity (set to 0 for production, 1 for normal debug, 2 for verbose)
#define DEBUG_SERIAL_LEVEL 1

// ===== FUNCTION DECLARATIONS =====
void beginSerialCommunication(NodeStateAndID &node);
void checkConnectionStatus();
void lookForMessages(MicroOscMessage &msg);
float getRhizomeValue();     // 1.0 if connected, 0.0 if not
void loopSendToTouch();
void SerialLoop();

#endif