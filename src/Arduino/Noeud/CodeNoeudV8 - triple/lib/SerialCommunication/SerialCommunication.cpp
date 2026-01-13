#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h>
#include <NetworkOSC.h>
#include <SoftwareSerial.h>

#define SERIAL_BAUD 9600

// Pin definitions for 3 rhizomes
#define CONNECT_PIN_1 0   // Rhizome 1 connection detect
#define CONNECT_PIN_2 14  // Rhizome 2 connection detect
#define CONNECT_PIN_3 15  // Rhizome 3 connection detect

// Serial port pins
// Rhizome 1: Serial2 (Hardware UART2) - RX=5, TX=4
// Rhizome 2: Serial1 (Hardware UART1) - RX=9, TX=10
// Rhizome 3: SoftwareSerial - RX=16, TX=17
#define SOFT_RX 16
#define SOFT_TX 17

// Create serial objects
SoftwareSerial softSerial(SOFT_RX, SOFT_TX);

// Create OSC handlers for each rhizome
MicroOscSlip<32> oscRhizome1(&Serial2);
MicroOscSlip<32> oscRhizome2(&Serial1);
MicroOscSlip<32> oscRhizome3(&softSerial);

static NodeStateAndID *pNode = nullptr;

// Data for each rhizome
struct RhizomeData {
  int id;
  int energy;
  bool isConnected;
  ConnectionState connState;
  unsigned long debounceStart;
  unsigned long lastNodeSent;
};

RhizomeData rhizomes[3] = {
  {0, 0, false, CONN_DISCONNECTED, 0, 0},
  {0, 0, false, CONN_DISCONNECTED, 0, 0},
  {0, 0, false, CONN_DISCONNECTED, 0, 0}
};

static const unsigned long CONNECT_DEBOUNCE = 150;
static const unsigned long DISCONNECT_DEBOUNCE = 600;
static const unsigned long NODE_SEND_INTERVAL = 1000;

// Connection state machine enum
enum ConnectionState {
  CONN_DISCONNECTED,
  CONN_DEBOUNCING_CONNECT,
  CONN_CONNECTED,
  CONN_DEBOUNCING_DISCONNECT
};

void onConnected(int rhizomeIndex) {
  Serial.print("--- [CONN] Rhizome ");
  Serial.print(rhizomeIndex + 1);
  Serial.println(" connected! ---");
  
  // Send /node immediately to the specific rhizome
  MicroOscSlip<32>* osc;
  if (rhizomeIndex == 0) osc = &oscRhizome1;
  else if (rhizomeIndex == 1) osc = &oscRhizome2;
  else osc = &oscRhizome3;
  
  osc->sendMessage("/node", "f", pNode->getDrainRate());
  rhizomes[rhizomeIndex].lastNodeSent = millis();
  
  Serial.print("[SEND] Rhizome ");
  Serial.print(rhizomeIndex + 1);
  Serial.print(" /node drainRate=");
  Serial.println(pNode->getDrainRate());
}

void onDisconnected(int rhizomeIndex) {
  Serial.print("--- [CONN] Rhizome ");
  Serial.print(rhizomeIndex + 1);
  Serial.println(" disconnected ---");
  
  // Clear data
  rhizomes[rhizomeIndex].id = 0;
  rhizomes[rhizomeIndex].energy = 0;
}

void updateConnectionState(int rhizomeIndex, int connectPin) {
  unsigned long now = millis();
  bool pinState = digitalRead(connectPin) == LOW;
  RhizomeData &rhi = rhizomes[rhizomeIndex];
  
  switch (rhi.connState) {
    case CONN_DISCONNECTED:
      if (pinState) {
        rhi.connState = CONN_DEBOUNCING_CONNECT;
        rhi.debounceStart = now;
      }
      break;
      
    case CONN_DEBOUNCING_CONNECT:
      if (!pinState) {
        rhi.connState = CONN_DISCONNECTED;
      } else if (now - rhi.debounceStart >= CONNECT_DEBOUNCE) {
        rhi.connState = CONN_CONNECTED;
        rhi.isConnected = true;
        onConnected(rhizomeIndex);
      }
      break;
      
    case CONN_CONNECTED:
      if (!pinState) {
        rhi.connState = CONN_DEBOUNCING_DISCONNECT;
        rhi.debounceStart = now;
      }
      break;
      
    case CONN_DEBOUNCING_DISCONNECT:
      if (pinState) {
        rhi.connState = CONN_CONNECTED;
      } else if (now - rhi.debounceStart >= DISCONNECT_DEBOUNCE) {
        rhi.connState = CONN_DISCONNECTED;
        rhi.isConnected = false;
        onDisconnected(rhizomeIndex);
      }
      break;
  }
}

void handleNodeMessage(MicroOscMessage &msg, int rhizomeIndex) {
  if (msg.checkOscAddress("/discover_list")) {
    const char* csv = msg.nextAsString();
    Serial.print("[RECV] Rhizome ");
    Serial.print(rhizomeIndex + 1);
    Serial.print(" /discover_list: ");
    Serial.println(csv);
    return;
  }
  
  if (msg.checkOscAddress("/energy")) {
    int receivedId = msg.nextAsInt();
    int energy = msg.nextAsInt();

    rhizomes[rhizomeIndex].id = receivedId;
    rhizomes[rhizomeIndex].energy = energy;
    
    Serial.print("[ENERGY] Rhizome ");
    Serial.print(rhizomeIndex + 1);
    Serial.print(" (ID ");
    Serial.print(receivedId);
    Serial.print("): ");
    Serial.print(energy);
    Serial.println("%");
    
    if (energy <= 5) {
      Serial.print("  ⚠️  WARNING: Rhizome ");
      Serial.print(rhizomeIndex + 1);
      Serial.println(" nearly depleted!");
    }
  }
}

void beginSerialCommunication(NodeStateAndID &node) {
  pNode = &node;

  // Initialize all serial ports
  Serial2.begin(SERIAL_BAUD, SERIAL_8N1, 5, 4);      // Rhizome 1
  Serial1.begin(SERIAL_BAUD, SERIAL_8N1, 9, 10);     // Rhizome 2
  softSerial.begin(SERIAL_BAUD);                      // Rhizome 3

  // Setup connection detection pins
  pinMode(CONNECT_PIN_1, INPUT_PULLUP);
  pinMode(CONNECT_PIN_2, INPUT_PULLUP);
  pinMode(CONNECT_PIN_3, INPUT_PULLUP);
  
  pNode->setDrainRate(5.0f);
 
  Serial.println("=== Serial Communication Initialized ===");
  Serial.println("=== NODE WITH 3 RHIZOMES STARTED ===");
  Serial.print("Drain rate: ");
  Serial.println(5.0f);
  Serial.println("Waiting for rhizomes on 3 ports...");
  Serial.println("  Port 1: Serial2 (GPIO 5/4)");
  Serial.println("  Port 2: Serial1 (GPIO 9/10)");
  Serial.println("  Port 3: SoftSerial (GPIO 16/17)");
}

void loopSendToTouch() {
  // Send all 6 values: ID1, energy1, ID2, energy2, ID3, energy3
  // We'll call sendOSC 3 times with different channels
  
  for (int i = 0; i < 3; i++) {
    int idToSend = rhizomes[i].isConnected ? rhizomes[i].id : 0;
    int energyToSend = rhizomes[i].isConnected ? rhizomes[i].energy : 0;
    
    // You'll need to modify NetworkOSC to handle multiple rhizomes
    // For now, sending on separate channels
    sendOSCMulti(i + 1, idToSend, energyToSend);
  }
}

void SerialLoop() {
  // Update connection states for all 3 rhizomes
  updateConnectionState(0, CONNECT_PIN_1);
  updateConnectionState(1, CONNECT_PIN_2);
  updateConnectionState(2, CONNECT_PIN_3);
  
  // Listen for messages from all rhizomes
  oscRhizome1.onOscMessageReceived([](MicroOscMessage &msg) {
    handleNodeMessage(msg, 0);
  });
  
  oscRhizome2.onOscMessageReceived([](MicroOscMessage &msg) {
    handleNodeMessage(msg, 1);
  });
  
  oscRhizome3.onOscMessageReceived([](MicroOscMessage &msg) {
    handleNodeMessage(msg, 2);
  });
  
  // Periodically resend /node to all connected rhizomes
  unsigned long now = millis();
  for (int i = 0; i < 3; i++) {
    if (rhizomes[i].isConnected && (now - rhizomes[i].lastNodeSent >= NODE_SEND_INTERVAL)) {
      MicroOscSlip<32>* osc;
      if (i == 0) osc = &oscRhizome1;
      else if (i == 1) osc = &oscRhizome2;
      else osc = &oscRhizome3;
      
      osc->sendMessage("/node", "f", pNode->getDrainRate());
      rhizomes[i].lastNodeSent = now;
    }
  }
}