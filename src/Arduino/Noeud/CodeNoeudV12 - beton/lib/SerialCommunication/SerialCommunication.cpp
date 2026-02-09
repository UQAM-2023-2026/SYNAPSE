#include <Arduino.h>
#include "SerialCommunication.h"
#include <MicroOscSlip.h>
#include <NetworkOSC.h>

// Create HardwareSerial instances for custom UART pins
HardwareSerial SerialPogopin1(1);  // UART1
HardwareSerial SerialPogopin2(2);  // UART2

// Create two separate OSC instances for each pogopin (256 byte buffer for better reliability)
MicroOscSlip<256> oscNode1(&SerialPogopin1);
MicroOscSlip<256> oscNode2(&SerialPogopin2);

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
  CONN_FLAG_DETECTED,     // FLAG pin went LOW, waiting for OSC
  CONN_CONNECTED          // Received /discover_list and sent /node
};

// Pogopin 1 state
static ConnectionState conn1_State = CONN_DISCONNECTED;
static bool isConnected1 = false;
static unsigned long lastNode1_Sent = 0;
static bool handshake1_Complete = false;  // Track if we've received /energy after /node

// Pogopin 2 state
static ConnectionState conn2_State = CONN_DISCONNECTED;
static bool isConnected2 = false;
static unsigned long lastNode2_Sent = 0;
static bool handshake2_Complete = false;  // Track if we've received /energy after /node

// Track last sent drain rates to detect changes
static float lastSentDrainInfra = -1.0f;
static float lastSentDrainSupra = -1.0f;

// ===== POGOPIN 1 CONNECTION CALLBACKS =====
void onConnected1() {
#if DEBUG_SERIAL_LEVEL >= 1
  Serial.println("---");
  Serial.println("[CONN] Pogopin 1 connected!");
#endif
  
  oscNode1.sendMessage("/node", "f", pNode->getDrainRateInfra());
  lastNode1_Sent = millis();
  handshake1_Complete = false;  // Reset handshake flag
  
#if DEBUG_SERIAL_LEVEL >= 1
  Serial.print("[SEND] Pogopin 1 - /node drain_infra=");
  Serial.println(pNode->getDrainRateInfra());
  Serial.println("---");
#endif
}

void onDisconnected1() {
#if DEBUG_SERIAL_LEVEL >= 1
  Serial.println("---");
  Serial.println("[CONN] Pogopin 1 disconnected");
  Serial.println("---");
#endif
  
  // Reset rhizome 1 data
  rhizome1_ID = 0;
  rhizome1_Energy = 0;
  handshake1_Complete = false;
}

// ===== POGOPIN 2 CONNECTION CALLBACKS =====
void onConnected2() {
#if DEBUG_SERIAL_LEVEL >= 1
  Serial.println("---");
  Serial.println("[CONN] Pogopin 2 connected!");
#endif
  
  oscNode2.sendMessage("/node", "f", pNode->getDrainRateSupra());
  lastNode2_Sent = millis();
  handshake2_Complete = false;  // Reset handshake flag
  
#if DEBUG_SERIAL_LEVEL >= 1
  Serial.print("[SEND] Pogopin 2 - /node drain_supra=");
  Serial.println(pNode->getDrainRateSupra());
  Serial.println("---");
#endif
}

void onDisconnected2() {
#if DEBUG_SERIAL_LEVEL >= 1
  Serial.println("---");
  Serial.println("[CONN] Pogopin 2 disconnected");
  Serial.println("---");
#endif
  
  // Reset rhizome 2 data
  rhizome2_ID = 0;
  rhizome2_Energy = 0;
  handshake2_Complete = false;
}

// ===== POGOPIN 1 CONNECTION STATE MACHINE (TWO-LAYER DETECTION) =====
void updateConnectionState1() {
  static unsigned long flagHighStartTime1 = 0;
  
  bool flagConnected = (digitalRead(POGOPIN1_FLAG_PIN) == LOW);  // FLAG LOW = physical contact
  
  if (flagConnected && conn1_State == CONN_DISCONNECTED) {
    // FLAG detected - start listening for OSC
    conn1_State = CONN_FLAG_DETECTED;
    lastEnergy1_Time = millis();  // Reset timeout
    flagHighStartTime1 = 0;  // Reset debounce timer
    
    Serial.println("[FLAG] Pogopin 1 - Physical contact detected (FLAG went LOW)");
  } 
  else if (flagConnected && (conn1_State == CONN_FLAG_DETECTED || conn1_State == CONN_CONNECTED)) {
    // FLAG is LOW (connected) - reset debounce timer
    flagHighStartTime1 = 0;
  }
  else if (!flagConnected && (conn1_State == CONN_FLAG_DETECTED || conn1_State == CONN_CONNECTED)) {
    // FLAG is HIGH (disconnected) - start/continue debounce timer
    unsigned long now = millis();
    
    if (flagHighStartTime1 == 0) {
      // First time seeing FLAG HIGH - start debounce timer
      flagHighStartTime1 = now;
      Serial.println("[FLAG] Pogopin 1 - FLAG went HIGH, debouncing...");
    } 
    else if (now - flagHighStartTime1 >= FLAG_DISCONNECT_DEBOUNCE) {
      // FLAG has been HIGH for debounce period - real disconnect
      Serial.print("[FLAG] Pogopin 1 - Confirmed disconnect after ");
      Serial.print(now - flagHighStartTime1);
      Serial.println("ms");
      
      conn1_State = CONN_DISCONNECTED;
      isConnected1 = false;
      flagHighStartTime1 = 0;
      onDisconnected1();
    }
  }
}




// ===== POGOPIN 2 CONNECTION STATE MACHINE (TWO-LAYER DETECTION) =====
void updateConnectionState2() {
  static unsigned long flagHighStartTime2 = 0;
  
  bool flagConnected = (digitalRead(POGOPIN2_FLAG_PIN) == LOW);  // FLAG LOW = physical contact
  
  if (flagConnected && conn2_State == CONN_DISCONNECTED) {
    // FLAG detected - start listening for OSC
    conn2_State = CONN_FLAG_DETECTED;
    lastEnergy2_Time = millis();  // Reset timeout
    flagHighStartTime2 = 0;  // Reset debounce timer
    
    Serial.println("[FLAG] Pogopin 2 - Physical contact detected (FLAG went LOW)");
  } 
  else if (flagConnected && (conn2_State == CONN_FLAG_DETECTED || conn2_State == CONN_CONNECTED)) {
    // FLAG is LOW (connected) - reset debounce timer
    flagHighStartTime2 = 0;
  }
  else if (!flagConnected && (conn2_State == CONN_FLAG_DETECTED || conn2_State == CONN_CONNECTED)) {
    // FLAG is HIGH (disconnected) - start/continue debounce timer
    unsigned long now = millis();
    
    if (flagHighStartTime2 == 0) {
      // First time seeing FLAG HIGH - start debounce timer
      flagHighStartTime2 = now;
      Serial.println("[FLAG] Pogopin 2 - FLAG went HIGH, debouncing...");
    } 
    else if (now - flagHighStartTime2 >= FLAG_DISCONNECT_DEBOUNCE) {
      // FLAG has been HIGH for debounce period - real disconnect
      Serial.print("[FLAG] Pogopin 2 - Confirmed disconnect after ");
      Serial.print(now - flagHighStartTime2);
      Serial.println("ms");
      
      conn2_State = CONN_DISCONNECTED;
      isConnected2 = false;
      flagHighStartTime2 = 0;
      onDisconnected2();
    }
  }
}



// ===== MESSAGE HANDLERS =====
void handleNode1Message(MicroOscMessage &msg) {
  // Ignore /node - it's loopback of our own message
  if (msg.checkOscAddress("/node")) {
    return;
  }
  
  // CRITICAL: Always respond to /discover_list regardless of state
  if (msg.checkOscAddress("/discover_list")) {
    const char* csv = msg.nextAsString();
    
#if DEBUG_SERIAL_LEVEL >= 2
    Serial.print("[RECV-1] /discover_list: ");
    Serial.println(csv);
#endif
    
    // ALWAYS send /node in response (this fixes the 5% failure case)
    oscNode1.sendMessage("/node", "f", pNode->getDrainRateInfra());
    lastNode1_Sent = millis();
    handshake1_Complete = false;  // Reset handshake - waiting for /energy
    
#if DEBUG_SERIAL_LEVEL >= 1
    Serial.print("[SEND] Pogopin 1 - /node drain_infra=");
    Serial.println(pNode->getDrainRateInfra());
#endif
    
    // Update connection state if not already connected
    if (conn1_State != CONN_CONNECTED) {
      conn1_State = CONN_CONNECTED;
      isConnected1 = true;
      lastEnergy1_Time = millis();
      
#if DEBUG_SERIAL_LEVEL >= 1
      Serial.println("[CONN] Pogopin 1 - OSC handshake initiated");
#endif
    }
    
    return;
  }
  
  if (msg.checkOscAddress("/energy")) {
    int receivedId = msg.nextAsInt();
    int energy = msg.nextAsInt();

    rhizome1_ID = receivedId;
    rhizome1_Energy = static_cast<int>(energy);
    
    unsigned long now = millis();
    unsigned long timeSinceLastEnergy = now - lastEnergy1_Time;
    lastEnergy1_Time = now; // Update timeout counter
    
    // Mark handshake as complete
    if (!handshake1_Complete) {
      handshake1_Complete = true;
#if DEBUG_SERIAL_LEVEL >= 1
      Serial.println("[HANDSHAKE] Pogopin 1 - Complete (/energy received)");
#endif
    }
    
#if DEBUG_SERIAL_LEVEL >= 1
    Serial.print("[ENERGY] Pogopin 1 - ID:");
    Serial.print(receivedId);
    Serial.print(" Energy:");
    Serial.print(energy);
    Serial.print("% (interval:");
    Serial.print(timeSinceLastEnergy);
    Serial.println("ms)");
    
    if (energy <= 5) {
      Serial.println("  ⚠️  WARNING: Rhizome 1 nearly depleted!");
    }
#endif
    
    // FALLBACK: If we receive energy but aren't connected, accept it
    // This handles the case where connection is unstable but data is still flowing
    if (!isConnected1) {
#if DEBUG_SERIAL_LEVEL >= 1
      Serial.println("[FALLBACK] Pogopin 1 - Accepting /energy despite connection state mismatch");
#endif
      isConnected1 = true;
      conn1_State = CONN_CONNECTED;
      handshake1_Complete = true;
    }
  }
}

void handleNode2Message(MicroOscMessage &msg) {
  // Ignore /node - it's loopback of our own message
  if (msg.checkOscAddress("/node")) {
    return;
  }
  
  // CRITICAL: Always respond to /discover_list regardless of state
  if (msg.checkOscAddress("/discover_list")) {
    const char* csv = msg.nextAsString();
    
#if DEBUG_SERIAL_LEVEL >= 2
    Serial.print("[RECV-2] /discover_list: ");
    Serial.println(csv);
#endif
    
    // ALWAYS send /node in response (this fixes the 5% failure case)
    oscNode2.sendMessage("/node", "f", pNode->getDrainRateSupra());
    lastNode2_Sent = millis();
    handshake2_Complete = false;  // Reset handshake - waiting for /energy
    
#if DEBUG_SERIAL_LEVEL >= 1
    Serial.print("[SEND] Pogopin 2 - /node drain_supra=");
    Serial.println(pNode->getDrainRateSupra());
#endif
    
    // Update connection state if not already connected
    if (conn2_State != CONN_CONNECTED) {
      conn2_State = CONN_CONNECTED;
      isConnected2 = true;
      lastEnergy2_Time = millis();
      
#if DEBUG_SERIAL_LEVEL >= 1
      Serial.println("[CONN] Pogopin 2 - OSC handshake initiated");
#endif
    }
    
    return;
  }
  
  if (msg.checkOscAddress("/energy")) {
    int receivedId = msg.nextAsInt();
    int energy = msg.nextAsInt();

    rhizome2_ID = receivedId;
    rhizome2_Energy = static_cast<int>(energy);
    
    unsigned long now = millis();
    unsigned long timeSinceLastEnergy = now - lastEnergy2_Time;
    lastEnergy2_Time = now; // Update timeout counter
    
    // Mark handshake as complete
    if (!handshake2_Complete) {
      handshake2_Complete = true;
#if DEBUG_SERIAL_LEVEL >= 1
      Serial.println("[HANDSHAKE] Pogopin 2 - Complete (/energy received)");
#endif
    }
    
#if DEBUG_SERIAL_LEVEL >= 1
    Serial.print("[ENERGY] Pogopin 2 - ID:");
    Serial.print(receivedId);
    Serial.print(" Energy:");
    Serial.print(energy);
    Serial.print("% (interval:");
    Serial.print(timeSinceLastEnergy);
    Serial.println("ms)");
    
    if (energy <= 5) {
      Serial.println("  ⚠️  WARNING: Rhizome 2 nearly depleted!");
    }
#endif
    
    // FALLBACK: If we receive energy but aren't connected, accept it
    // This handles the case where connection is unstable but data is still flowing
    if (!isConnected2) {
#if DEBUG_SERIAL_LEVEL >= 1
      Serial.println("[FALLBACK] Pogopin 2 - Accepting /energy despite connection state mismatch");
#endif
      isConnected2 = true;
      conn2_State = CONN_CONNECTED;
      handshake2_Complete = true;
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
  Serial.print("Buffer Size: 256 bytes per pogopin\n");
  Serial.print("Debug Level: "); Serial.println(DEBUG_SERIAL_LEVEL);
  Serial.print("Default Drain Rate Infra (Pogopin 1): "); Serial.println(pNode->getDrainRateInfra());
  Serial.print("Default Drain Rate Supra (Pogopin 2): "); Serial.println(pNode->getDrainRateSupra());
  Serial.println("===========================================");
  Serial.println("=== NODE SIMULATOR STARTED ===");
  Serial.println("Waiting for rhizome connections...");
  Serial.println();
  
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
  
  // DIAGNOSTIC: Print connection state every 2 seconds
  static unsigned long lastDiagnostic = 0;
  if (now - lastDiagnostic > 2000) {
    Serial.println("=== CONNECTION STATUS ===");
    Serial.print("Pogopin 1: ");
    Serial.print(isConnected1 ? "CONNECTED" : "DISCONNECTED");
    Serial.print(" | State: ");
    Serial.print(conn1_State);
    Serial.print(" | FLAG: ");
    Serial.print(digitalRead(POGOPIN1_FLAG_PIN) == LOW ? "LOW(connected)" : "HIGH(disconnected)");
    Serial.print(" | Time since last /energy: ");
    Serial.print((now - lastEnergy1_Time) / 1000.0);
    Serial.println("s");
    
    Serial.print("Pogopin 2: ");
    Serial.print(isConnected2 ? "CONNECTED" : "DISCONNECTED");
    Serial.print(" | State: ");
    Serial.print(conn2_State);
    Serial.print(" | FLAG: ");
    Serial.print(digitalRead(POGOPIN2_FLAG_PIN) == LOW ? "LOW(connected)" : "HIGH(disconnected)");
    Serial.print(" | Time since last /energy: ");
    Serial.print((now - lastEnergy2_Time) / 1000.0);
    Serial.println("s");
    Serial.println("========================");
    lastDiagnostic = now;
  }
  
  // Update both connection state machines (FLAG detection)
  updateConnectionState1();
  updateConnectionState2();
  
  // Listen for messages on both UARTs
  oscNode1.onOscMessageReceived(handleNode1Message);
  oscNode2.onOscMessageReceived(handleNode2Message);
  
  // ===== POGOPIN 1 HANDSHAKE TIMEOUT & DRAIN RATE UPDATES =====
  if (isConnected1 && conn1_State == CONN_CONNECTED) {
    unsigned long timeSinceLastEnergy = now - lastEnergy1_Time;
    unsigned long timeSinceLastNode = now - lastNode1_Sent;
    
    // SAFETY CHECK: Detect integer underflow (lastEnergy1_Time was set in the future)
    if (timeSinceLastEnergy > 2147483647UL) {  // More than ~24 days = clearly wrong
      Serial.println("[ERROR] Pogopin 1 - Time calculation overflow detected, resetting");
      lastEnergy1_Time = now;
      timeSinceLastEnergy = 0;
    }
    
    // HANDSHAKE TIMEOUT: If we sent /node but haven't received /energy
    if (!handshake1_Complete && timeSinceLastNode > HANDSHAKE_TIMEOUT) {
#if DEBUG_SERIAL_LEVEL >= 1
      Serial.println("[TIMEOUT] Pogopin 1 - No /energy received after /node, resending /node");
#endif
      oscNode1.sendMessage("/node", "f", currentDrainInfra);
      lastNode1_Sent = now;
    }
    
    // DRAIN RATE UPDATE: Only if handshake is complete and value changed
    if (handshake1_Complete && currentDrainInfra != lastSentDrainInfra) {
      // Respect minimum send interval
      if (timeSinceLastNode >= NODE_RESEND_INTERVAL) {
        oscNode1.sendMessage("/node", "f", currentDrainInfra);
        lastSentDrainInfra = currentDrainInfra;
        lastNode1_Sent = now;
        
#if DEBUG_SERIAL_LEVEL >= 1
        Serial.print("[UPDATE] Pogopin 1 - Sending drain_infra=");
        Serial.println(currentDrainInfra);
#endif
      }
    }
    
    // CONNECTION TIMEOUT: No /energy for extended period
    // IMPORTANT: Only check timeout if enough time has actually passed
    if (timeSinceLastEnergy > CONNECTION_TIMEOUT && timeSinceLastEnergy < 2147483647UL) {
#if DEBUG_SERIAL_LEVEL >= 1
      Serial.print("[TIMEOUT] Pogopin 1 - No /energy for ");
      Serial.print(timeSinceLastEnergy);
      Serial.println("ms, disconnecting");
      Serial.print("  Last /energy was ");
      Serial.print(timeSinceLastEnergy / 1000.0);
      Serial.println(" seconds ago");
#endif
      isConnected1 = false;
      conn1_State = CONN_DISCONNECTED;
      onDisconnected1();
    }
  } else {
    // Reset tracking when disconnected
    lastSentDrainInfra = -1.0f;
  }
  
  // ===== POGOPIN 2 HANDSHAKE TIMEOUT & DRAIN RATE UPDATES =====
  if (isConnected2 && conn2_State == CONN_CONNECTED) {
    unsigned long timeSinceLastEnergy = now - lastEnergy2_Time;
    unsigned long timeSinceLastNode = now - lastNode2_Sent;
    
    // SAFETY CHECK: Detect integer underflow (lastEnergy2_Time was set in the future)
    if (timeSinceLastEnergy > 2147483647UL) {  // More than ~24 days = clearly wrong
      Serial.println("[ERROR] Pogopin 2 - Time calculation overflow detected, resetting");
      lastEnergy2_Time = now;
      timeSinceLastEnergy = 0;
    }
    
    // HANDSHAKE TIMEOUT: If we sent /node but haven't received /energy
    if (!handshake2_Complete && timeSinceLastNode > HANDSHAKE_TIMEOUT) {
#if DEBUG_SERIAL_LEVEL >= 1
      Serial.println("[TIMEOUT] Pogopin 2 - No /energy received after /node, resending /node");
#endif
      oscNode2.sendMessage("/node", "f", currentDrainSupra);
      lastNode2_Sent = now;
    }
    
    // DRAIN RATE UPDATE: Only if handshake is complete and value changed
    if (handshake2_Complete && currentDrainSupra != lastSentDrainSupra) {
      // Respect minimum send interval
      if (timeSinceLastNode >= NODE_RESEND_INTERVAL) {
        oscNode2.sendMessage("/node", "f", currentDrainSupra);
        lastSentDrainSupra = currentDrainSupra;
        lastNode2_Sent = now;
        
#if DEBUG_SERIAL_LEVEL >= 1
        Serial.print("[UPDATE] Pogopin 2 - Sending drain_supra=");
        Serial.println(currentDrainSupra);
#endif
      }
    }
    
    // CONNECTION TIMEOUT: No /energy for extended period
    // IMPORTANT: Only check timeout if enough time has actually passed
    if (timeSinceLastEnergy > CONNECTION_TIMEOUT && timeSinceLastEnergy < 2147483647UL) {
#if DEBUG_SERIAL_LEVEL >= 1
      Serial.print("[TIMEOUT] Pogopin 2 - No /energy for ");
      Serial.print(timeSinceLastEnergy);
      Serial.println("ms, disconnecting");
      Serial.print("  Last /energy was ");
      Serial.print(timeSinceLastEnergy / 1000.0);
      Serial.println(" seconds ago");
#endif
      isConnected2 = false;
      conn2_State = CONN_DISCONNECTED;
      onDisconnected2();
    }
  } else {
    // Reset tracking when disconnected
    lastSentDrainSupra = -1.0f;
  }
}