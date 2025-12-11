#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h>
#include <NetworkOSC.h>

#define SERIAL_PORT Serial2
#define SERIAL_BAUD 9600
#define CONNECTION_TIMEOUT 500  // 500ms without /energy = disconnected
#define CONNECT_PIN 0          // Pin de détection de connexion


MicroOscSlip<32> oscNode(&Serial2);

static NodeStateAndID *pNode = nullptr;

int rhizomeID = 0;
int energyValue = 0;
unsigned long lastEnergyMessageTime = 0;

// Connection state machine (same as rhizome)
enum ConnectionState {
  CONN_DISCONNECTED,
  CONN_DEBOUNCING_CONNECT,
  CONN_CONNECTED,
  CONN_DEBOUNCING_DISCONNECT
};

static ConnectionState connState = CONN_DISCONNECTED;
static unsigned long debounceStart = 0;
static bool isConnected = false;

static const unsigned long CONNECT_DEBOUNCE = 150;
static const unsigned long DISCONNECT_DEBOUNCE = 600;

// Periodic /node resend
static unsigned long lastNodeSent = 0;
static const unsigned long NODE_SEND_INTERVAL = 1000; // Resend every 1s while connected


void onConnected() {
  Serial.println("---");
  Serial.println("[CONN] Rhizome connected!");
  
  // Send /node immediately
  oscNode.sendMessage("/node", "f", pNode->getDrainRate());
  lastNodeSent = millis();
  
  Serial.print("[SEND] /node drainRate=");
  Serial.println(pNode->getDrainRate());
  Serial.println("---");
}

void onDisconnected() {
  Serial.println("---");
  Serial.println("[CONN] Rhizome disconnected");
  Serial.println("---");
}

void updateConnectionState() {
  unsigned long now = millis();
  bool pinState = digitalRead(CONNECT_PIN) == LOW;
  
  switch (connState) {
    case CONN_DISCONNECTED:
      if (pinState) {
        connState = CONN_DEBOUNCING_CONNECT;
        debounceStart = now;
      }
      break;
      
    case CONN_DEBOUNCING_CONNECT:
      if (!pinState) {
        connState = CONN_DISCONNECTED;
      } else if (now - debounceStart >= CONNECT_DEBOUNCE) {
        connState = CONN_CONNECTED;
        isConnected = true;
        onConnected();
      }
      break;
      
    case CONN_CONNECTED:
      if (!pinState) {
        connState = CONN_DEBOUNCING_DISCONNECT;
        debounceStart = now;
      }
      break;
      
    case CONN_DEBOUNCING_DISCONNECT:
      if (pinState) {
        connState = CONN_CONNECTED;
      } else if (now - debounceStart >= DISCONNECT_DEBOUNCE) {
        connState = CONN_DISCONNECTED;
        isConnected = false;
        onDisconnected();
      }
      break;
  }
}

void handleNodeMessage(MicroOscMessage &msg) {
  // Ignore /discover_list - we respond via pin detection, not message
  if (msg.checkOscAddress("/discover_list")) {
    const char* csv = msg.nextAsString();
    Serial.print("[RECV] /discover_list: ");
    Serial.println(csv);
    Serial.println("  (Ignored - using pin detection)");
    return;
  }
  
  if (msg.checkOscAddress("/energy")) {
    int receivedId = msg.nextAsInt();
    int energy = msg.nextAsInt();

    rhizomeID = receivedId;
    energyValue = static_cast<int>(energy);
    
    Serial.print("[ENERGY] Rhizome ");
    Serial.print(receivedId);
    Serial.print(": ");
    Serial.print(energy);
    Serial.println("%");
    
    if (energy <= 5) {
      Serial.println("  ⚠️  WARNING: Rhizome nearly depleted!");
    }
  }
}

void beginSerialCommunication(NodeStateAndID &node) {
  pNode = &node;

  Serial2.begin(SERIAL_BAUD, SERIAL_8N1, 5, 4);

   pinMode(CONNECT_PIN, INPUT_PULLUP);
  pNode->setDrainRate(5.0f);  // Default drain rate
 
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

// void lookForMessages(MicroOscMessage &msg) {
//   if (msg.checkOscAddress("/discover")) {
//     int origin = msg.nextAsInt();
//     int count = msg.nextAsInt();
    
//     float currentDrainRate = 0.0f;
//     if (pNode) {
//       currentDrainRate = pNode->getDrainRate();
//     }
    
//     Serial.print("[RECV] /discover from rhizome ");
//     Serial.print(origin);
//     // Serial.print(" (chain of ");
//     // Serial.print(count);
//     // Serial.println(" rhizomes)");
//     Serial.println();
    
//     oscNode.sendMessage("/node", "f", currentDrainRate);
    
//     Serial.print("[SEND] /node drainRate=");
//     Serial.println(currentDrainRate, 2);
    
//   } else if (msg.checkOscAddress("/energy")) {
//     int receivedId = msg.nextAsInt();
//     float energy = msg.nextAsInt();
    
//     Serial.print("[RECV] /energy from rhizome ");
//     Serial.print(receivedId);
//     Serial.print(" energy=");
//     Serial.println(energy, 2);

//     rhizomeID = receivedId;
//     energyValue = static_cast<int>(energy);
//     lastEnergyMessageTime = millis();

//     // Send drain rate every time we receive energy
//     float currentDrainRate = 5.0f;
//     if (pNode) {
//       currentDrainRate = pNode->getDrainRate();
//     }
//     oscNode.sendMessage("/node", "f", currentDrainRate);
//     Serial.print("[SEND] /node drainRate=");
//     Serial.println(currentDrainRate, 2);
//   }
// }

float getRhizomeValue() {
  return (rhizomeID > 0) ? 1.0f : 0.0f;
}

void loopSendToTouch() {
  int idToSend = isConnected ? rhizomeID : 0;
  int energyToSend = isConnected ? energyValue : 0;

  sendOSC(idToSend, energyToSend);
}

void SerialLoop() {
  // Update connection state machine
  updateConnectionState();
  
  // Listen for messages from rhizomes
  oscNode.onOscMessageReceived(handleNodeMessage);
  
  // Periodically resend /node while connected (in case rhizome missed it)
  if (isConnected) {
    unsigned long now = millis();
    if (now - lastNodeSent >= NODE_SEND_INTERVAL) {
      oscNode.sendMessage("/node", "f", pNode->getDrainRate());
      lastNodeSent = now;
      Serial.print("[SEND] /node drainRate=");
      Serial.println(pNode->getDrainRate());
    }
  }
}