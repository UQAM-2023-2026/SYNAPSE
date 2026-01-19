#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h>
#include <NetworkOSC.h>

// Create HardwareSerial instances for custom UART pins
HardwareSerial SerialPogopin1(1);  // UART1
HardwareSerial SerialPogopin2(2);  // UART2

// Create two separate OSC instances for each pogopin
MicroOscSlip<32> oscNode1(&SerialPogopin1);
MicroOscSlip<32> oscNode2(&SerialPogopin2);

static NodeStateAndID *pNode = nullptr;

// Rhizome 1 data
int rhizome1_ID = 0;
int rhizome1_Energy = 0;
unsigned long lastEnergy1_Time = 0;

// Rhizome 2 data
int rhizome2_ID = 0;
int rhizome2_Energy = 0;
unsigned long lastEnergy2_Time = 0;

// Connection state machine
enum ConnectionState {
  CONN_DISCONNECTED,
  CONN_DEBOUNCING_CONNECT,
  CONN_CONNECTED,
  CONN_DEBOUNCING_DISCONNECT
};

// Pogopin 1 state
static ConnectionState conn1_State = CONN_DISCONNECTED;
static unsigned long debounce1_Start = 0;
static bool isConnected1 = false;
static unsigned long lastNode1_Sent = 0;

// Pogopin 2 state
static ConnectionState conn2_State = CONN_DISCONNECTED;
static unsigned long debounce2_Start = 0;
static bool isConnected2 = false;
static unsigned long lastNode2_Sent = 0;

// ===== POGOPIN 1 CONNECTION CALLBACKS =====
void onConnected1() {
  Serial.println("---");
  Serial.println("[CONN] Pogopin 1 connected!");
  
  oscNode1.sendMessage("/node", "f", pNode->getDrainRate());
  lastNode1_Sent = millis();
  
  Serial.print("[SEND] Pogopin 1 - /node drainRate=");
  Serial.println(pNode->getDrainRate());
  Serial.println("---");
}

void onDisconnected1() {
  Serial.println("---");
  Serial.println("[CONN] Pogopin 1 disconnected");
  Serial.println("---");
  
  // Reset rhizome 1 data
  rhizome1_ID = 0;
  rhizome1_Energy = 0;
}

// ===== POGOPIN 2 CONNECTION CALLBACKS =====
void onConnected2() {
  Serial.println("---");
  Serial.println("[CONN] Pogopin 2 connected!");
  
  oscNode2.sendMessage("/node", "f", pNode->getDrainRate());
  lastNode2_Sent = millis();
  
  Serial.print("[SEND] Pogopin 2 - /node drainRate=");
  Serial.println(pNode->getDrainRate());
  Serial.println("---");
}

void onDisconnected2() {
  Serial.println("---");
  Serial.println("[CONN] Pogopin 2 disconnected");
  Serial.println("---");
  
  // Reset rhizome 2 data
  rhizome2_ID = 0;
  rhizome2_Energy = 0;
}

// ===== POGOPIN 1 CONNECTION STATE MACHINE =====
void updateConnectionState1() {
  unsigned long now = millis();
  bool pinState = digitalRead(POGOPIN1_FLAG_PIN) == LOW;
  
  // NOTE: FLAG1 doesn't work reliably, so we rely on OSC message timeout instead
  // If connected but no energy message for CONNECTION_TIMEOUT ms, disconnect
  if (isConnected1 && (now - lastEnergy1_Time > CONNECTION_TIMEOUT)) {
    conn1_State = CONN_DISCONNECTED;
    isConnected1 = false;
    onDisconnected1();
    return;
  }
  
  // Only use FLAG for initial connection if it actually goes LOW
  switch (conn1_State) {
    case CONN_DISCONNECTED:
      if (pinState) {
        conn1_State = CONN_DEBOUNCING_CONNECT;
        debounce1_Start = now;
      }
      break;
      
    case CONN_DEBOUNCING_CONNECT:
      if (!pinState) {
        conn1_State = CONN_DISCONNECTED;
      } else if (now - debounce1_Start >= CONNECT_DEBOUNCE) {
        conn1_State = CONN_CONNECTED;
        isConnected1 = true;
        onConnected1();
      }
      break;
      
    case CONN_CONNECTED:
      // Don't check FLAG for disconnection on Pogopin 1 - use timeout instead
      break;
      
    case CONN_DEBOUNCING_DISCONNECT:
      // Not used for Pogopin 1
      break;
  }
}

// ===== POGOPIN 2 CONNECTION STATE MACHINE =====
void updateConnectionState2() {
  unsigned long now = millis();
  bool pinState = digitalRead(POGOPIN2_FLAG_PIN) == LOW;
  
  // Timeout-based disconnection if no energy messages received
  if (isConnected2 && (now - lastEnergy2_Time > CONNECTION_TIMEOUT)) {
    conn2_State = CONN_DISCONNECTED;
    isConnected2 = false;
    onDisconnected2();
    return;
  }
  
  switch (conn2_State) {
    case CONN_DISCONNECTED:
      if (pinState) {
        conn2_State = CONN_DEBOUNCING_CONNECT;
        debounce2_Start = now;
      }
      break;
      
    case CONN_DEBOUNCING_CONNECT:
      if (!pinState) {
        conn2_State = CONN_DISCONNECTED;
      } else if (now - debounce2_Start >= CONNECT_DEBOUNCE) {
        conn2_State = CONN_CONNECTED;
        isConnected2 = true;
        lastEnergy2_Time = now; // Initialize timeout counter
        onConnected2();
      }
      break;
      
    case CONN_CONNECTED:
      // Don't check FLAG for disconnection on Pogopin 2 - use timeout instead (FLAG pins are unreliable)
      break;
      
    case CONN_DEBOUNCING_DISCONNECT:
      // Not used for Pogopin 2
      break;
  }
}

// ===== MESSAGE HANDLERS =====
void handleNode1Message(MicroOscMessage &msg) {
  // Check for discover_list
  if (msg.checkOscAddress("/discover_list")) {
    const char* csv = msg.nextAsString();
    Serial.print("[RECV-1] /discover_list: ");
    Serial.println(csv);
    
    // FALLBACK: If we receive message and not connected, force connection
    if (!isConnected1) {
      Serial.println("[FALLBACK] Pogopin 1 - Forcing connection (FLAG not working, but receiving OSC)");
      isConnected1 = true;
      conn1_State = CONN_CONNECTED;
      onConnected1();
    }
    return;
  }
  
  if (msg.checkOscAddress("/energy")) {
    int receivedId = msg.nextAsInt();
    int energy = msg.nextAsInt();

    rhizome1_ID = receivedId;
    rhizome1_Energy = static_cast<int>(energy);
    lastEnergy1_Time = millis(); // Update timeout counter
    
    Serial.print("[ENERGY] Pogopin 1 - Rhizome ID: ");
    Serial.print(receivedId);
    Serial.print(" Energy: ");
    Serial.print(energy);
    Serial.println("%");
    
    if (energy <= 5) {
      Serial.println("  ⚠️  WARNING: Rhizome 1 nearly depleted!");
    }
    
    // FALLBACK: If we receive energy and not connected, force connection
    if (!isConnected1) {
      Serial.println("[FALLBACK] Pogopin 1 - Forcing connection (FLAG not working, but receiving OSC)");
      isConnected1 = true;
      conn1_State = CONN_CONNECTED;
      lastEnergy1_Time = millis();
      onConnected1();
    }
  }
}

void handleNode2Message(MicroOscMessage &msg) {
  // Check for discover_list
  if (msg.checkOscAddress("/discover_list")) {
    const char* csv = msg.nextAsString();
    Serial.print("[RECV-2] /discover_list: ");
    Serial.println(csv);
    
    // FALLBACK: If we receive message and not connected, force connection
    if (!isConnected2) {
      isConnected2 = true;
      conn2_State = CONN_CONNECTED;
      onConnected2();
    }
    return;
  }
  
  if (msg.checkOscAddress("/energy")) {
    int receivedId = msg.nextAsInt();
    int energy = msg.nextAsInt();

    rhizome2_ID = receivedId;
    rhizome2_Energy = static_cast<int>(energy);
    lastEnergy2_Time = millis(); // Update timeout counter
    
    Serial.print("[ENERGY] Pogopin 2 - Rhizome ID: ");
    Serial.print(receivedId);
    Serial.print(" Energy: ");
    Serial.print(energy);
    Serial.println("%");
    
    if (energy <= 5) {
      Serial.println("  ⚠️  WARNING: Rhizome 2 nearly depleted!");
    }
    
    // FALLBACK: If we receive energy and not connected, force connection
    if (!isConnected2) {
      isConnected2 = true;
      conn2_State = CONN_CONNECTED;
      lastEnergy2_Time = millis();
      onConnected2();
    }
  }
}

// ===== INITIALIZATION =====
void beginSerialCommunication(NodeStateAndID &node) {
  pNode = &node;

  // ===== INITIALIZE POGOPIN 1 (UART1) =====
  SerialPogopin1.begin(SERIAL_BAUD, SERIAL_8N1, POGOPIN1_RX_PIN, POGOPIN1_TX_PIN);
  pinMode(POGOPIN1_FLAG_PIN, INPUT_PULLUP);
  
  // ===== INITIALIZE POGOPIN 2 (UART2) =====
  SerialPogopin2.begin(SERIAL_BAUD, SERIAL_8N1, POGOPIN2_RX_PIN, POGOPIN2_TX_PIN);
  pinMode(POGOPIN2_FLAG_PIN, INPUT_PULLUP);
  
  // Set default drain rate
  pNode->setDrainRate(5.0f);
 
  // Startup messages
  Serial.println("===========================================");
  Serial.println("===  DUAL POGOPIN SERIAL COMM INIT  ===");
  Serial.println("===========================================");
  Serial.println("POGOPIN 1 (UART1):");
  Serial.print("  RX Pin (GPIO): "); Serial.println(POGOPIN1_RX_PIN);
  Serial.print("  TX Pin (GPIO): "); Serial.println(POGOPIN1_TX_PIN);
  Serial.print("  FLAG Pin (GPIO): "); Serial.println(POGOPIN1_FLAG_PIN);
  Serial.println("---");
  Serial.println("POGOPIN 2 (UART2):");
  Serial.print("  RX Pin (GPIO): "); Serial.println(POGOPIN2_RX_PIN);
  Serial.print("  TX Pin (GPIO): "); Serial.println(POGOPIN2_TX_PIN);
  Serial.print("  FLAG Pin (GPIO): "); Serial.println(POGOPIN2_FLAG_PIN);
  Serial.println("---");
  Serial.print("Baud Rate: "); Serial.println(SERIAL_BAUD);
  Serial.print("Default Drain Rate: "); Serial.println(pNode->getDrainRate());
  Serial.println("===========================================");
  Serial.println("=== NODE SIMULATOR STARTED ===");
  Serial.println("Waiting for rhizome connections...");
  Serial.println();
  Serial.println("DEBUG: Monitoring both FLAG pins...");
  Serial.print("FLAG 1 (GPIO13): "); Serial.println(digitalRead(POGOPIN1_FLAG_PIN) ? "HIGH" : "LOW");
  Serial.print("FLAG 2 (GPIO32): "); Serial.println(digitalRead(POGOPIN2_FLAG_PIN) ? "HIGH" : "LOW");
  
  lastEnergy1_Time = millis();
  lastEnergy2_Time = millis();
}

void checkConnectionStatus() {
  oscNode1.onOscMessageReceived(lookForMessages);
  oscNode2.onOscMessageReceived(lookForMessages);
}

void lookForMessages(MicroOscMessage &msg) {
  // This is kept for compatibility but not used
}

float getRhizomeValue() {
  // Return 1.0 if at least one rhizome is connected
  return (rhizome1_ID > 0 || rhizome2_ID > 0) ? 1.0f : 0.0f;
}

void loopSendToTouch() {
  // Send all 4 channels with rhizome IDs and energy values
  // If disconnected, rhizome ID will be 0
  int energy1 = isConnected1 ? rhizome1_Energy : 0;
  int energy2 = isConnected2 ? rhizome2_Energy : 0;
  int rhizId1 = isConnected1 ? rhizome1_ID : 0;
  int rhizId2 = isConnected2 ? rhizome2_ID : 0;

  sendOSC(energy1, energy2, rhizId1, rhizId2);
}

void SerialLoop() {
  unsigned long now = millis();
  static float lastDrainRate1 = -1.0;
  static float lastDrainRate2 = -1.0;
  float currentDrainRate = pNode->getDrainRate();
  
  // Update both connection state machines
  updateConnectionState1();
  updateConnectionState2();
  
  // Listen for messages from both rhizomes
  oscNode1.onOscMessageReceived(handleNode1Message);
  oscNode2.onOscMessageReceived(handleNode2Message);
  
  // Periodically resend /node to Pogopin 1 while connected
  if (isConnected1) {
    if (now - lastNode1_Sent >= NODE_SEND_INTERVAL) {
      oscNode1.sendMessage("/node", "f", currentDrainRate);
      lastNode1_Sent = now;
      if (currentDrainRate != lastDrainRate1) {
        Serial.print("[SEND] Pogopin 1 - /node drainRate=");
        Serial.println(currentDrainRate);
        lastDrainRate1 = currentDrainRate;
      }
    }
  }
  
  // Periodically resend /node to Pogopin 2 while connected
  if (isConnected2) {
    if (now - lastNode2_Sent >= NODE_SEND_INTERVAL) {
      oscNode2.sendMessage("/node", "f", currentDrainRate);
      lastNode2_Sent = now;
      if (currentDrainRate != lastDrainRate2) {
        Serial.print("[SEND] Pogopin 2 - /node drainRate=");
        Serial.println(currentDrainRate);
        lastDrainRate2 = currentDrainRate;
      }
    }
  }
}