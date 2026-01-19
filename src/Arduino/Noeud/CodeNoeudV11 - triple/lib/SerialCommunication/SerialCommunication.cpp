#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h>
#include <NetworkOSC.h>

// Create HardwareSerial instances for custom UART pins
HardwareSerial SerialPogopin1(1);  // UART1
HardwareSerial SerialPogopin2(2);  // UART2
HardwareSerial SerialPogopin3(0);  // UART0 (replaces debug Serial)

// Create three separate OSC instances for each pogopin
MicroOscSlip<32> oscNode1(&SerialPogopin1);
MicroOscSlip<32> oscNode2(&SerialPogopin2);
MicroOscSlip<32> oscNode3(&SerialPogopin3);

static NodeStateAndID *pNode = nullptr;

// Rhizome 1 data
int rhizome1_ID = 0;
int rhizome1_Energy = 0;
unsigned long lastEnergy1_Time = 0;

// Rhizome 2 data
int rhizome2_ID = 0;
int rhizome2_Energy = 0;
unsigned long lastEnergy2_Time = 0;

// Rhizome 3 data
int rhizome3_ID = 0;
int rhizome3_Energy = 0;
unsigned long lastEnergy3_Time = 0;

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

// Pogopin 3 state
static ConnectionState conn3_State = CONN_DISCONNECTED;
static unsigned long debounce3_Start = 0;
static bool isConnected3 = false;
static unsigned long lastNode3_Sent = 0;

// ===== POGOPIN 1 CONNECTION CALLBACKS =====
void onConnected1() {
  oscNode1.sendMessage("/node", "f", pNode->getDrainRate());
  lastNode1_Sent = millis();
}

void onDisconnected1() {
  rhizome1_ID = 0;
  rhizome1_Energy = 0;
}

// ===== POGOPIN 2 CONNECTION CALLBACKS =====
void onConnected2() {
  oscNode2.sendMessage("/node", "f", pNode->getDrainRate());
  lastNode2_Sent = millis();
}

void onDisconnected2() {
  rhizome2_ID = 0;
  rhizome2_Energy = 0;
}

// ===== POGOPIN 3 CONNECTION CALLBACKS =====
void onConnected3() {
  Serial.println("---");
  Serial.println("[CONN] Pogopin 3 connected!");
  
  oscNode3.sendMessage("/node", "f", pNode->getDrainRate());
  lastNode3_Sent = millis();
  
  Serial.print("[SEND] Pogopin 3 - /node drainRate=");
  Serial.println(pNode->getDrainRate());
  Serial.println("---");
}

void onDisconnected3() {
  Serial.println("---");
  Serial.println("[CONN] Pogopin 3 disconnected");
  Serial.println("---");
  
  rhizome3_ID = 0;
  rhizome3_Energy = 0;
}

// ===== POGOPIN 1 CONNECTION STATE MACHINE =====
void updateConnectionState1() {
  unsigned long now = millis();
  bool pinState = digitalRead(POGOPIN1_FLAG_PIN) == LOW;
  
  if (isConnected1 && (now - lastEnergy1_Time > CONNECTION_TIMEOUT)) {
    conn1_State = CONN_DISCONNECTED;
    isConnected1 = false;
    onDisconnected1();
    return;
  }
  
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
      break;
      
    case CONN_DEBOUNCING_DISCONNECT:
      break;
  }
}

// ===== POGOPIN 2 CONNECTION STATE MACHINE =====
void updateConnectionState2() {
  unsigned long now = millis();
  bool pinState = digitalRead(POGOPIN2_FLAG_PIN) == LOW;
  
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
        lastEnergy2_Time = now;
        onConnected2();
      }
      break;
      
    case CONN_CONNECTED:
      break;
      
    case CONN_DEBOUNCING_DISCONNECT:
      break;
  }
}

// ===== POGOPIN 3 CONNECTION STATE MACHINE =====
void updateConnectionState3() {
  unsigned long now = millis();
  bool pinState = digitalRead(POGOPIN3_FLAG_PIN) == LOW;
  
  if (isConnected3 && (now - lastEnergy3_Time > CONNECTION_TIMEOUT)) {
    conn3_State = CONN_DISCONNECTED;
    isConnected3 = false;
    onDisconnected3();
    return;
  }
  
  switch (conn3_State) {
    case CONN_DISCONNECTED:
      if (pinState) {
        conn3_State = CONN_DEBOUNCING_CONNECT;
        debounce3_Start = now;
      }
      break;
      
    case CONN_DEBOUNCING_CONNECT:
      if (!pinState) {
        conn3_State = CONN_DISCONNECTED;
      } else if (now - debounce3_Start >= CONNECT_DEBOUNCE) {
        conn3_State = CONN_CONNECTED;
        isConnected3 = true;
        lastEnergy3_Time = now;
        onConnected3();
      }
      break;
      
    case CONN_CONNECTED:
      break;
      
    case CONN_DEBOUNCING_DISCONNECT:
      break;
  }
}

// ===== MESSAGE HANDLERS =====
void handleNode1Message(MicroOscMessage &msg) {
  if (msg.checkOscAddress("/discover_list")) {
    if (!isConnected1) {
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
    lastEnergy1_Time = millis();
    
    if (!isConnected1) {
      isConnected1 = true;
      conn1_State = CONN_CONNECTED;
      lastEnergy1_Time = millis();
      onConnected1();
    }
  }
}

void handleNode2Message(MicroOscMessage &msg) {
  if (msg.checkOscAddress("/discover_list")) {
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
    lastEnergy2_Time = millis();
    
    if (!isConnected2) {
      isConnected2 = true;
      conn2_State = CONN_CONNECTED;
      lastEnergy2_Time = millis();
      onConnected2();
    }
  }
}

void handleNode3Message(MicroOscMessage &msg) {
  if (msg.checkOscAddress("/discover_list")) {
    const char* csv = msg.nextAsString();
    Serial.print("[RECV-3] /discover_list: ");
    Serial.println(csv);
    
    if (!isConnected3) {
      Serial.println("[FALLBACK] Pogopin 3 - Forcing connection");
      isConnected3 = true;
      conn3_State = CONN_CONNECTED;
      onConnected3();
    }
    return;
  }
  
  if (msg.checkOscAddress("/energy")) {
    int receivedId = msg.nextAsInt();
    int energy = msg.nextAsInt();

    rhizome3_ID = receivedId;
    rhizome3_Energy = static_cast<int>(energy);
    lastEnergy3_Time = millis();
    
    Serial.print("[ENERGY] Pogopin 3 - Rhizome ID: ");
    Serial.print(receivedId);
    Serial.print(" Energy: ");
    Serial.print(energy);
    Serial.println("%");
    
    if (energy <= 5) {
      Serial.println("  ⚠️  WARNING: Rhizome 3 nearly depleted!");
    }
    
    if (!isConnected3) {
      Serial.println("[FALLBACK] Pogopin 3 - Forcing connection");
      isConnected3 = true;
      conn3_State = CONN_CONNECTED;
      lastEnergy3_Time = millis();
      onConnected3();
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
  
  // ===== INITIALIZE POGOPIN 3 (Custom UART pins) =====
  SerialPogopin3.begin(SERIAL_BAUD, SERIAL_8N1, POGOPIN3_RX_PIN, POGOPIN3_TX_PIN);
  pinMode(POGOPIN3_FLAG_PIN, INPUT_PULLUP);
  
  // Set default drain rate
  pNode->setDrainRate(5.0f);
  
  // Startup messages
  Serial.println("===========================================");
  Serial.println("===  TRIPLE POGOPIN SERIAL COMM INIT  ===");
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
  Serial.println("POGOPIN 3 (Custom UART):");
  Serial.print("  RX Pin (GPIO): "); Serial.println(POGOPIN3_RX_PIN);
  Serial.print("  TX Pin (GPIO): "); Serial.println(POGOPIN3_TX_PIN);
  Serial.print("  FLAG Pin (GPIO): "); Serial.println(POGOPIN3_FLAG_PIN);
  Serial.println("---");
  Serial.print("Baud Rate: "); Serial.println(SERIAL_BAUD);
  Serial.print("Default Drain Rate: "); Serial.println(pNode->getDrainRate());
  Serial.println("===========================================");
  Serial.println("=== NODE SIMULATOR STARTED ===");
  Serial.println("Waiting for rhizome connections...");
  Serial.println();
  
  // Test Pogopin 3 immediately
  Serial.println("Testing Pogopin 3 transmission...");
  delay(100);
  oscNode3.sendMessage("/test", "i", 123);
  Serial.println("Test message sent on Pogopin 3");
  
  lastEnergy1_Time = millis();
  lastEnergy2_Time = millis();
  lastEnergy3_Time = millis();
}

void checkConnectionStatus() {
  oscNode1.onOscMessageReceived(lookForMessages);
  oscNode2.onOscMessageReceived(lookForMessages);
  oscNode3.onOscMessageReceived(lookForMessages);
}

void lookForMessages(MicroOscMessage &msg) {
  // Kept for compatibility
}

float getRhizomeValue() {
  return (rhizome1_ID > 0 || rhizome2_ID > 0 || rhizome3_ID > 0) ? 1.0f : 0.0f;
}

void loopSendToTouch() {
  int energy1 = isConnected1 ? rhizome1_Energy : 0;
  int energy2 = isConnected2 ? rhizome2_Energy : 0;
  int energy3 = isConnected3 ? rhizome3_Energy : 0;
  int rhizId1 = isConnected1 ? rhizome1_ID : 0;
  int rhizId2 = isConnected2 ? rhizome2_ID : 0;
  int rhizId3 = isConnected3 ? rhizome3_ID : 0;

  sendOSC(energy1, energy2, energy3, rhizId1, rhizId2, rhizId3);
}

void SerialLoop() {
  unsigned long now = millis();
  static float lastDrainRate1 = -1.0;
  static float lastDrainRate2 = -1.0;
  static float lastDrainRate3 = -1.0;
  float currentDrainRate = pNode->getDrainRate();
  
  // Update all three connection state machines
  updateConnectionState1();
  updateConnectionState2();
  updateConnectionState3();
  
  // Listen for messages from all three rhizomes
  oscNode1.onOscMessageReceived(handleNode1Message);
  oscNode2.onOscMessageReceived(handleNode2Message);
  oscNode3.onOscMessageReceived(handleNode3Message);
  
  // Periodically resend /node to Pogopin 1
  if (isConnected1 && (now - lastNode1_Sent >= NODE_SEND_INTERVAL)) {
    oscNode1.sendMessage("/node", "f", currentDrainRate);
    lastNode1_Sent = now;
    lastDrainRate1 = currentDrainRate;
  }
  
  // Periodically resend /node to Pogopin 2
  if (isConnected2 && (now - lastNode2_Sent >= NODE_SEND_INTERVAL)) {
    oscNode2.sendMessage("/node", "f", currentDrainRate);
    lastNode2_Sent = now;
    lastDrainRate2 = currentDrainRate;
  }
  
  // Periodically resend /node to Pogopin 3
  if (isConnected3 && (now - lastNode3_Sent >= NODE_SEND_INTERVAL)) {
    oscNode3.sendMessage("/node", "f", currentDrainRate);
    lastNode3_Sent = now;
    lastDrainRate3 = currentDrainRate;
  }
}