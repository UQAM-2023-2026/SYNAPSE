#include <Arduino.h>
#include "SerialCommunication.h"
#include <MicroOscSlip.h>
#include <NetworkOSC.h>

// Create HardwareSerial instances for custom UART pins
HardwareSerial SerialPogopin1(1);  // UART1
HardwareSerial SerialPogopin2(2);  // UART2

// Create two separate OSC instances for each pogopin (128 byte buffer for larger messages)
MicroOscSlip<128> oscNode1(&SerialPogopin1);
MicroOscSlip<128> oscNode2(&SerialPogopin2);

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

// Track last sent drain rates to detect changes
static float lastSentDrainInfra = -1.0f;
static float lastSentDrainSupra = -1.0f;

// ===== POGOPIN 1 CONNECTION CALLBACKS =====
void onConnected1() {
  Serial.println("---");
  Serial.println("[CONN] Pogopin 1 connected!");
  
  float drainInfra = pNode->getDrainRateInfra();
  oscNode1.sendMessage("/node", "f", drainInfra);
  lastNode1_Sent = millis();
  lastSentDrainInfra = drainInfra;
  
  Serial.print("[SEND] Pogopin 1 - /node drain_infra=");
  Serial.println(drainInfra);
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
  
  float drainSupra = pNode->getDrainRateSupra();
  oscNode2.sendMessage("/node", "f", drainSupra);
  lastNode2_Sent = millis();
  lastSentDrainSupra = drainSupra;
  
  Serial.print("[SEND] Pogopin 2 - /node drain_supra=");
  Serial.println(drainSupra);
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

// ===== POGOPIN 1 CONNECTION STATE MACHINE (NO DEBOUNCING) =====
void updateConnectionState1() {
  bool flagConnected = (digitalRead(POGOPIN1_FLAG_PIN) == LOW);  // FLAG LOW = connected
  
  if (flagConnected && !isConnected1) {
    // Just connected
    isConnected1 = true;
    conn1_State = CONN_CONNECTED;
    lastEnergy1_Time = millis();
    onConnected1();
  } else if (!flagConnected && isConnected1) {
    // Just disconnected
    isConnected1 = false;
    conn1_State = CONN_DISCONNECTED;
    onDisconnected1();
  }
}

// ===== POGOPIN 2 CONNECTION STATE MACHINE (NO DEBOUNCING) =====
void updateConnectionState2() {
  bool flagConnected = (digitalRead(POGOPIN2_FLAG_PIN) == LOW);  // FLAG LOW = connected
  
  if (flagConnected && !isConnected2) {
    // Just connected
    isConnected2 = true;
    conn2_State = CONN_CONNECTED;
    lastEnergy2_Time = millis();
    onConnected2();
  } else if (!flagConnected && isConnected2) {
    // Just disconnected
    isConnected2 = false;
    conn2_State = CONN_DISCONNECTED;
    onDisconnected2();
  }
}

// ===== MESSAGE HANDLERS =====
void handleNode1Message(MicroOscMessage &msg) {
  // Ignore /node - it's loopback of our own message
  if (msg.checkOscAddress("/node")) {
    return;
  }
  
  // Check for discover_list
  if (msg.checkOscAddress("/discover_list")) {
    const char* csv = msg.nextAsString();
    Serial.print("[RECV-1] /discover_list: ");
    Serial.println(csv);
    
    // Connection is detected when we receive OSC messages (FLAG pins unreliable)
    if (!isConnected1) {
      Serial.println("[CONN] Pogopin 1 - Connected via OSC");
      isConnected1 = true;
      conn1_State = CONN_CONNECTED;
      lastEnergy1_Time = millis();
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
    
    // FALLBACK: If we receive energy and not connected, force connection (silently)
    if (!isConnected1) {
      Serial.println("[FALLBACK] Pogopin 1 - Connected via OSC");
      isConnected1 = true;
      conn1_State = CONN_CONNECTED;
      lastEnergy1_Time = millis();
      // Don't call onConnected1() here - just update state silently to avoid spam
    }
  }
}

void handleNode2Message(MicroOscMessage &msg) {
  // Handle /node message from rhizome
  if (msg.checkOscAddress("/node")) {
    float value = msg.nextAsFloat();
    lastEnergy2_Time = millis();
    
    Serial.print("[RECV-2] /node value=");
    Serial.println(value);
    
    rhizome2_Energy = (int)(value * 100);
    if (rhizome2_ID == 0) rhizome2_ID = 2;
    return;
  }
  
  // Check for discover_list
  if (msg.checkOscAddress("/discover_list")) {
    const char* csv = msg.nextAsString();
    Serial.print("[RECV-2] /discover_list: ");
    Serial.println(csv);
    
    // Connection is detected when we receive OSC messages (FLAG pins unreliable)
    if (!isConnected2) {
      Serial.println("[CONN] Pogopin 2 - Connected via OSC");
      isConnected2 = true;
      conn2_State = CONN_CONNECTED;
      lastEnergy2_Time = millis();
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
    
    // FALLBACK: If we receive energy and not connected, force connection (silently)
    if (!isConnected2) {
      Serial.println("[FALLBACK] Pogopin 2 - Connected via OSC");
      isConnected2 = true;
      conn2_State = CONN_CONNECTED;
      lastEnergy2_Time = millis();
      // Don't call onConnected2() here - just update state silently to avoid spam
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
  
  // Set default drain rates
  pNode->setDrainRateInfra(5.0f);   // Default for Pogopin 1
  pNode->setDrainRateSupra(5.0f);   // Default for Pogopin 2
 
  // Startup messages
  Serial.println("===========================================");
  Serial.println("===  DUAL POGOPIN SERIAL COMM INIT  ===");
  Serial.println("===========================================");
  Serial.println("POGOPIN 1 (UART1) - drain_infra:");
  Serial.print("  RX Pin (GPIO): "); Serial.println(POGOPIN1_RX_PIN);
  Serial.print("  TX Pin (GPIO): "); Serial.println(POGOPIN1_TX_PIN);
  Serial.print("  FLAG Pin (GPIO): "); Serial.println(POGOPIN1_FLAG_PIN);
  Serial.println("---");
  Serial.println("POGOPIN 2 (UART2) - drain_supra:");
  Serial.print("  RX Pin (GPIO): "); Serial.println(POGOPIN2_RX_PIN);
  Serial.print("  TX Pin (GPIO): "); Serial.println(POGOPIN2_TX_PIN);
  Serial.print("  FLAG Pin (GPIO): "); Serial.println(POGOPIN2_FLAG_PIN);
  Serial.println("---");
  Serial.print("Baud Rate: "); Serial.println(SERIAL_BAUD);
  Serial.print("Default Drain Rate Infra (Pogopin 1): "); Serial.println(pNode->getDrainRateInfra());
  Serial.print("Default Drain Rate Supra (Pogopin 2): "); Serial.println(pNode->getDrainRateSupra());
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
  float currentDrainInfra = pNode->getDrainRateInfra();
  float currentDrainSupra = pNode->getDrainRateSupra();
  
  // Update both connection state machines
  updateConnectionState1();
  updateConnectionState2();
  
  // Listen for messages on both UARTs
  oscNode1.onOscMessageReceived(handleNode1Message);
  oscNode2.onOscMessageReceived(handleNode2Message);
  
  // Handle Pogopin 1 drain rate updates
  if (isConnected1) {
    // Check if drain_infra has changed
    if (currentDrainInfra != lastSentDrainInfra) {
      oscNode1.sendMessage("/node", "f", currentDrainInfra);
      lastSentDrainInfra = currentDrainInfra;
      Serial.print("[UPDATE] Pogopin 1 - Sending drain_infra=");
      Serial.println(currentDrainInfra);
      lastNode1_Sent = now;
    }
  } else {
    // Reset tracking when disconnected
    lastSentDrainInfra = -1.0f;
  }
  
  // Handle Pogopin 2 drain rate updates
  if (isConnected2) {
    // Check if drain_supra has changed
    if (currentDrainSupra != lastSentDrainSupra) {
      oscNode2.sendMessage("/node", "f", currentDrainSupra);
      lastSentDrainSupra = currentDrainSupra;
      Serial.print("[UPDATE] Pogopin 2 - Sending drain_supra=");
      Serial.println(currentDrainSupra);
      lastNode2_Sent = now;
    }
  } else {
    // Reset tracking when disconnected
    lastSentDrainSupra = -1.0f;
  }
}