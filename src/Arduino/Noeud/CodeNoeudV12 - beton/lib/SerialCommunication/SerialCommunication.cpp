/*==============================================================================
 * SerialCommunication.cpp - Refactored dual Rhizome connection manager
 * 
 * Uses RhizomeConnection class to eliminate code duplication.
 * Manages 2 independent connections (Pogopin 1 & 2).
 * 
 * LEGACY CODE: See SerialCommunication.cpp.legacy for original implementation
 *============================================================================*/

#include <Arduino.h>
#include "SerialCommunication.h"
#include <RhizomeConnection.h>
#include <NetworkOSC.h>

// Hardware serial instances
static HardwareSerial SerialPogopin1(1);  // UART1
static HardwareSerial SerialPogopin2(2);  // UART2

// Connection instances (replaces all duplicated state variables)
static RhizomeConnection connection1(0, &SerialPogopin1);
static RhizomeConnection connection2(1, &SerialPogopin2);

// Node configuration reference
static NodeStateAndID* pNode = nullptr;

// ===== CALLBACKS =====
static void onConnectionChanged(uint8_t portIndex, bool connected) {
    Serial.print("[SERIAL] Port ");
    Serial.print(portIndex);
    Serial.println(connected ? " connected" : " disconnected");
}

static void onEnergyReceived(uint8_t portIndex, uint8_t rhizomeId, uint8_t energy) {
    // Energy is automatically stored in RhizomeConnection
    // This callback can be used for additional processing if needed
}

// ===== PUBLIC API =====
void beginSerialCommunication(NodeStateAndID &node) {
    pNode = &node;
    
    // Initialize connection 1 (Pogopin 1 / UART1 / drain_infra)
    connection1.begin(POGOPIN1_RX_PIN, POGOPIN1_TX_PIN, POGOPIN1_FLAG_PIN, SERIAL_BAUD);
    connection1.setDrainRate(5.0f);  // Default
    connection1.onConnectionChange(onConnectionChanged);
    connection1.onEnergyReceived(onEnergyReceived);
    
    // Initialize connection 2 (Pogopin 2 / UART2 / drain_supra)
    connection2.begin(POGOPIN2_RX_PIN, POGOPIN2_TX_PIN, POGOPIN2_FLAG_PIN, SERIAL_BAUD);
    connection2.setDrainRate(5.0f);  // Default
    connection2.onConnectionChange(onConnectionChanged);
    connection2.onEnergyReceived(onEnergyReceived);
    
    // Set default drain rates in NodeStateAndID
    pNode->setDrainRateInfra(5.0f);
    pNode->setDrainRateSupra(5.0f);
    
    // Startup log
    Serial.println("===========================================");
    Serial.println("===  SERIAL COMMUNICATION INITIALIZED  ===");
    Serial.println("===========================================");
    Serial.print("Port 0: RX="); Serial.print(POGOPIN1_RX_PIN);
    Serial.print(" TX="); Serial.print(POGOPIN1_TX_PIN);
    Serial.print(" FLAG="); Serial.println(POGOPIN1_FLAG_PIN);
    Serial.print("Port 1: RX="); Serial.print(POGOPIN2_RX_PIN);
    Serial.print(" TX="); Serial.print(POGOPIN2_TX_PIN);
    Serial.print(" FLAG="); Serial.println(POGOPIN2_FLAG_PIN);
    Serial.print("Baud Rate: "); Serial.println(SERIAL_BAUD);
    Serial.println("===========================================");
    Serial.println("Waiting for Rhizome connections...");
    Serial.println();
}

void SerialLoop() {
    // Sync drain rates from NodeStateAndID to connections
    connection1.setDrainRate(pNode->getDrainRateInfra());
    connection2.setDrainRate(pNode->getDrainRateSupra());
    
    // Update both connections (handles OSC, timeouts, etc.)
    connection1.update();
    connection2.update();
}

// ===== DATA ACCESS =====
float getRhizomeValue() {
    // Return 1.0 if at least one rhizome is connected
    return (connection1.isConnected() || connection2.isConnected()) ? 1.0f : 0.0f;
}

bool isRhizome1Connected() {
    return connection1.isConnected();
}

bool isRhizome2Connected() {
    return connection2.isConnected();
}

uint8_t getRhizome1ID() {
    return connection1.getRhizomeId();
}

uint8_t getRhizome2ID() {
    return connection2.getRhizomeId();
}

uint8_t getRhizome1Energy() {
    return connection1.getEnergy();
}

uint8_t getRhizome2Energy() {
    return connection2.getEnergy();
}

// ===== TOUCHDESIGNER INTEGRATION =====
void loopSendToTouch() {
    int energy1 = connection1.isConnected() ? connection1.getEnergy() : 0;
    int energy2 = connection2.isConnected() ? connection2.getEnergy() : 0;
    int rhizId1 = connection1.isConnected() ? connection1.getRhizomeId() : 0;
    int rhizId2 = connection2.isConnected() ? connection2.getRhizomeId() : 0;
    
    sendOSC(energy1, energy2, rhizId1, rhizId2);
}

// ===== LEGACY COMPATIBILITY =====
void checkConnectionStatus() {
    // Legacy function - now handled by SerialLoop()
}

void lookForMessages(MicroOscMessage &msg) {
    // Legacy function - messages are now handled internally by RhizomeConnection
}
