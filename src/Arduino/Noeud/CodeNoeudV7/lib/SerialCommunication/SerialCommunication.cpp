#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h>
#include <NetworkOSC.h>

#define SERIAL_PORT Serial2
#define SERIAL_BAUD 9600
#define CONNECTION_TIMEOUT 500  // 500ms without /energy = disconnected

MicroOscSlip<32> oscNode(&Serial2);

static NodeStateAndID *pNode = nullptr;

int rhizomeID = 0;
int energyValue = 0;
unsigned long lastEnergyMessageTime = 0;

void beginSerialCommunication(NodeStateAndID &node) {
  pNode = &node;

  Serial2.begin(SERIAL_BAUD, SERIAL_8N1, 5, 4);
  pinMode(13, OUTPUT);

  Serial.println("=== Serial Communication Initialized ===");
  Serial.println("=== NODE SIMULATOR STARTED ===");
  Serial.print("Drain rate: ");
  Serial.println(5.0f);
  Serial.println("Waiting for rhizomes...");
  
  lastEnergyMessageTime = millis();
}

void checkConnectionStatus() {
  oscNode.onOscMessageReceived(lookForMessages);
}

void lookForMessages(MicroOscMessage &msg) {
  if (msg.checkOscAddress("/discover")) {
    int origin = msg.nextAsInt();
    int count = msg.nextAsInt();
    
    float currentDrainRate = 0.0f;
    if (pNode) {
      currentDrainRate = pNode->getDrainRate();
    }
    
    Serial.print("[RECV] /discover from rhizome ");
    Serial.print(origin);
    // Serial.print(" (chain of ");
    // Serial.print(count);
    // Serial.println(" rhizomes)");
    Serial.println();
    
    oscNode.sendMessage("/node", "f", currentDrainRate);
    
    Serial.print("[SEND] /node drainRate=");
    Serial.println(currentDrainRate, 2);
    
  } else if (msg.checkOscAddress("/energy")) {
    int receivedId = msg.nextAsInt();
    float energy = msg.nextAsInt();
    
    Serial.print("[RECV] /energy from rhizome ");
    Serial.print(receivedId);
    Serial.print(" energy=");
    Serial.println(energy, 2);

    rhizomeID = receivedId;
    energyValue = static_cast<int>(energy);
    lastEnergyMessageTime = millis();

    // Send drain rate every time we receive energy
    float currentDrainRate = 5.0f;
    if (pNode) {
      currentDrainRate = pNode->getDrainRate();
    }
    oscNode.sendMessage("/node", "f", currentDrainRate);
    Serial.print("[SEND] /node drainRate=");
    Serial.println(currentDrainRate, 2);
  }
}

float getRhizomeValue() {
  return (rhizomeID > 0) ? 1.0f : 0.0f;
}

void loopSendToTouch() {
  bool connected = (millis() - lastEnergyMessageTime) < CONNECTION_TIMEOUT;
  
  int idToSend = connected ? rhizomeID : 0;
  int energyToSend = connected ? energyValue : 0;

  sendOSC(idToSend, energyToSend);
}