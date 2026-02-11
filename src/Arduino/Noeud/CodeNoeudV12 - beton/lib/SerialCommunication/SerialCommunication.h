#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

#include <Arduino.h>
#include "NodeStateAndID.h"
#include <MicroOsc.h>

// ===== POGOPIN 1 (UART1) PIN DEFINITIONS =====
#define POGOPIN1_RX_PIN 2      // GPIO2 (was GPIO5 - conflicts with Ethernet on ESP32-PoE-ISO)
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

// ===== MAIN FUNCTIONS =====
void beginSerialCommunication(NodeStateAndID &node);
void SerialLoop();

// ===== DATA ACCESS (for TouchDesigner integration) =====
float getRhizomeValue();           // 1.0 if any connected, 0.0 if not
bool isRhizome1Connected();
bool isRhizome2Connected();
uint8_t getRhizome1ID();
uint8_t getRhizome2ID();
uint8_t getRhizome1Energy();
uint8_t getRhizome2Energy();

// ===== TOUCHDESIGNER INTEGRATION =====
void loopSendToTouch();

// ===== LEGACY COMPATIBILITY =====
void checkConnectionStatus();      // Deprecated - use SerialLoop()
void lookForMessages(MicroOscMessage &msg);  // Deprecated

#endif