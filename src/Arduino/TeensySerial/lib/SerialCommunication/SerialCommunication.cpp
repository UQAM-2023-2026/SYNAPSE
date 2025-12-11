/*-------------------Libraries----------------------*/
#include "SerialCommunication.h"

#include <Arduino.h>
#include <MicroOscSlip.h>

#include <EnergyManagement.h>
#include <StripsAnimation.h>
//#include <HapticFeedback.h>

#include <RhizomeStateAndID.h>

/*---------------PIN CONFIG-----------------------*/
#define SERIAL_BAUD 9600

//Left handle connection (receiving)
#define CONNECT_PIN1 6
#define SERIAL_PORT_RECEIVE Serial2

//Right handle connection (sending)
#define CONNECT_PIN2 20
#define SERIAL_PORT_SEND Serial3

/*------------------Global Variables-----------------*/
static RhizomeStateAndID *pRhizome = nullptr; // Pointer to RhizomeStateAndID instance

MicroOscSlip<32> oscSlipReceive(&SERIAL_PORT_RECEIVE);
MicroOscSlip<32> oscSlipSend(&SERIAL_PORT_SEND);

void oscMessageReceived(MicroOscMessage &msg);
void isThisANodeMessage(MicroOscMessage &msg);

//Discovery process variables
static bool discoveryMode = false;
static bool listening = false;
static bool waitingForToken = false;
static bool discoveryCompleted = false;

// Node/drain management
static float currentDrainRate = 0.0f;
static bool connectedToNode = false;
static unsigned long lastDrainSent = 0;
static unsigned long lastDrainReceived = 0;
static unsigned long lastEnergySent = 0;
static const unsigned long DRAIN_SEND_INTERVAL = 500;
static const unsigned long ENERGY_SEND_INTERVAL = 100;
static const unsigned long DRAIN_TIMEOUT = 1500;

// Discovery timing (non-blocking)
static unsigned long discoveryInitiatedTime = 0;
static bool discoverySent = false;
static const unsigned long DISCOVERY_DELAY = 350; // Long delay for stable connections

// Status monitoring
static unsigned long lastStatusPrint = 0;
static const unsigned long STATUS_PRINT_INTERVAL = 2000;

// Circular buffer for seen IDs
static const int MAX_SEEN_IDS = 20;
static int seenIds[MAX_SEEN_IDS];
static int seenStart = 0;
static int seenCount = 0;

/*--------------------ISR---------------------------*/
static volatile bool connectedLeftEvent = false;
static bool connectionLeftState = false;
static unsigned long lastLeftChangeTime = 0;

static volatile bool connectedRightEvent = false;
static bool connectionRightState = false;
static unsigned long lastRightChangeTime = 0;

// Debounce settings
static const unsigned long DEBOUNCE_DELAY = 400; // Very long for stability

void connectedLeft() {
  connectedLeftEvent = true;
 // hapticOne();
}
void connectedRight() {
  connectedRightEvent = true;
 // hapticTwo();
}

/*------------------Connection State-----------------*/
void clearSeenIds() {
  seenStart = 0;
  seenCount = 0;
}

bool isIdSeen(int id) {
  for (int i = 0; i < seenCount; ++i) {
    int idx = (seenStart + i) % MAX_SEEN_IDS;
    if (seenIds[idx] == id) return true;
  }
  return false;
}

void addSeenId(int id) {
  if (isIdSeen(id)) return; // Don't add duplicates
  
  if (seenCount < MAX_SEEN_IDS) {
    seenIds[seenCount] = id;
    seenCount++;
  } else {
    // Circular buffer: overwrite oldest
    seenIds[seenStart] = id;
    seenStart = (seenStart + 1) % MAX_SEEN_IDS;
  }
}

int getSeenIdsCount() {
  return seenCount;
}

int getSeenIdAt(int index) {
  if (index < 0 || index >= seenCount) return -1;
  return seenIds[(seenStart + index) % MAX_SEEN_IDS];
}

/*------------------------SETUP--------------------------*/
void SetupSerialCommunication(RhizomeStateAndID &rh) {
  pRhizome = &rh;

  Serial2.begin(SERIAL_BAUD);
  Serial3.begin(SERIAL_BAUD);

  pinMode(CONNECT_PIN1, INPUT_PULLUP);
  pinMode(CONNECT_PIN2, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(CONNECT_PIN1), connectedLeft, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CONNECT_PIN2), connectedRight, CHANGE);
  
  clearSeenIds();
  addSeenId(pRhizome->getID());

  Serial.println("Serial Communication Initialized.");
  Serial.print("My ID: "); Serial.println(pRhizome->getID());
}

/*--------------Discovery Functions-------------------*/
void completeDiscovery() {
  if (discoveryCompleted) return;
  
  Serial.println("[DISCOVERY] Loop complete!");
  Serial.print("  Total rhizomes: ");
  Serial.println(seenCount);
  
  discoveryCompleted = true;
  discoveryMode = false;
  pRhizome->setState(2); // GENERATING
  
  // Send discover_done to RIGHT (propagate around the loop)
  if (connectionRightState) {
    oscSlipSend.sendMessage("/discover_done", "i", seenCount);
    Serial.println("[SEND] /discover_done");
  }
}

void sendDiscover() {
  if (!pRhizome || !discoveryMode || discoverySent) return;

  // Build CSV of all seen IDs
  String csv;
  for (int i = 0; i < seenCount; ++i) {
    if (i > 0) csv += ',';
    csv += String(getSeenIdAt(i));
  }
  
  oscSlipSend.sendMessage("/discover_list", "s", csv.c_str());
  Serial.print("[SEND] /discover_list: ");
  Serial.println(csv);

  discoverySent = true;
}

void handleDiscoverList(const char* csv) {
  if (!csv || csv[0] == '\0') {
    Serial.println("[RECV] /discover_list empty");
    return;
  }
  
  Serial.print("[RECV] /discover_list: ");
  Serial.println(csv);
  
  bool foundMyId = false;
  int newCount = 0;
  
  // Parse and merge IDs
  char buf[256];
  strncpy(buf, csv, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  
  char* tok = strtok(buf, ",");
  while (tok) {
    int id = atoi(tok);
    if (id >= 0) {
      if (id == pRhizome->getID()) {
        foundMyId = true;
        Serial.println("[LOOP] Found my ID - loop complete!");
      }
      if (!isIdSeen(id)) {
        addSeenId(id);
        newCount++;
      }
    }
    tok = strtok(NULL, ",");
  }
  
  pRhizome->setCount(seenCount);
  Serial.print("[MERGE] seenCount now: ");
  Serial.println(seenCount);
  
  // If we found our ID, loop is complete
  if (foundMyId) {
    completeDiscovery();
  } else if (connectionRightState) {
    // Forward updated list to right (add our ID if not already there)
    discoverySent = false;
    sendDiscover();
  }
}

void oscMessageReceived(MicroOscMessage &msg) {
  if (!pRhizome) return;
  Serial.println("[OSC MSG] Received: ");
  

  if (msg.checkOscAddress("/discover_list")) {
    handleDiscoverList(msg.nextAsString());
    return;
  }
  
  if (msg.checkOscAddress("/discover_done")) {
    int total = msg.nextAsInt();
    Serial.print("[RECV] /discover_done total=");
    Serial.println(total);
    
    // Only process and forward if we haven't completed yet
    if (!discoveryCompleted) {
      pRhizome->setCount(total);
      pRhizome->setState(2);
      discoveryCompleted = true;
      discoveryMode = false;
      
      // Forward to right ONLY if we just completed (one-time propagation)
      if (connectionRightState) {
        oscSlipSend.sendMessage("/discover_done", "i", total);
        Serial.println("[SEND] /discover_done (forwarded)");
      }
    }
    // If already completed, just ignore - don't forward again
    return;
  }
  
  // CRITICAL: Always accept /drain messages regardless of current state
  // This fixes the R2 not responding issue
  if (msg.checkOscAddress("/drain")) {
    float drainRate = msg.nextAsFloat();
    Serial.print("[RECV] /drain rate=");
    Serial.println(drainRate);
    
    currentDrainRate = drainRate;
    lastDrainReceived = millis();
    connectedToNode = true; // Treat sender as node
    
    // Become GIVING_TO_NODE
    pRhizome->setState(3);
    setNodeDrainRate(drainRate);
    
    // Send energy back via LEFT (where drain came from)
    float energy = pRhizome->getEnergy();
    oscSlipReceive.sendMessage("/energy", "ii", pRhizome->getID(), (int)energy);
    lastEnergySent = millis();
    return;
  }
}

// Handle /node from right side (actual node)
void isThisANodeMessage(MicroOscMessage &msg) {
  if (!pRhizome) return;
  Serial.println("[OSC MSG] isThisANodeMessage Received: ");
  
  if (msg.checkOscAddress("/node")) {
    float drainRate = msg.nextAsFloat();
    Serial.print("[RECV] /node drainRate=");
    Serial.println(drainRate);
    
    currentDrainRate = drainRate;
    connectedToNode = true;
    lastDrainReceived = millis();
    
    // Check if there's a rhizome behind us (left connected)
    if (connectionLeftState) {
      Serial.println("[STATE] -> MIDDLEMAN");
      pRhizome->setState(4);
      setNodeDrainRate(0.0f); // Don't drain ourselves
      
      // Forward drain to left
      oscSlipReceive.sendMessage("/drain", "f", drainRate);
      lastDrainSent = millis();
    } else {
      Serial.println("[STATE] -> GIVING_TO_NODE");
      pRhizome->setState(3);
      setNodeDrainRate(drainRate);
    }
  }
  
  // Relay energy messages from left to node
  if (msg.checkOscAddress("/energy")) {
    // This shouldn't happen on right port, but handle it
  }
}

void LookForSerialMessages() {
  oscSlipReceive.onOscMessageReceived(oscMessageReceived);
  oscSlipSend.onOscMessageReceived(isThisANodeMessage);
}

/*-------------------CHECK CONNECTIONS-------------------------------*/
void checkConnectionStatus() {
  
  unsigned long now = millis();
  
  // Update connection state machines (handles debouncing)
  updateConnectionStates();
  
  // Process incoming OSC messages
  oscSlipReceive.onOscMessageReceived(oscMessageReceived);
  oscSlipSend.onOscMessageReceived(isThisANodeMessage);
  
  // MIDDLEMAN: periodically send drain and relay energy
  if (pRhizome->getState() == 4 && connectedToNode) {
    // Resend drain periodically
    if (now - lastDrainSent >= DRAIN_SEND_INTERVAL) {
      if (connectionLeftState) {
        oscSlipReceive.sendMessage("/drain", "f", currentDrainRate);
        lastDrainSent = now;
      } else {
        // No one behind us anymore
        Serial.println("[MIDDLEMAN] Left gone -> GIVING_TO_NODE");
        pRhizome->setState(3);
        setNodeDrainRate(currentDrainRate);
      }
    }
  }
  
  // GIVING_TO_NODE: send energy to whoever is draining us
  if (pRhizome->getState() == 3) {
    if (now - lastEnergySent >= ENERGY_SEND_INTERVAL) {
      float energy = pRhizome->getEnergy();
      
      // Send energy back the way drain came from
      if (connectedToNode && connectionRightState) {
        // Direct to node
        oscSlipSend.sendMessage("/energy", "ii", pRhizome->getID(), (int)energy);
      } else if (connectionLeftState) {
        // To rhizome acting as middleman (drain came from left)
        oscSlipReceive.sendMessage("/energy", "ii", pRhizome->getID(), (int)energy);
      }
      lastEnergySent = now;
    }
  }
  
  // Status print (only when active)
  if (now - lastStatusPrint >= STATUS_PRINT_INTERVAL && pRhizome->getState() >= 2) {
    printStatus();
    lastStatusPrint = now;
  }
}

void printStatus() {
  const char* stateNames[] = {"IDLE", "CONNECTED", "GENERATING", "GIVING_TO_NODE", "MIDDLEMAN"};
  int state = pRhizome->getState();
  
  Serial.print("ID:");
  Serial.print(pRhizome->getID());
  Serial.print(" Count:");
  Serial.print(pRhizome->getCount());
  Serial.print(" E:");
  Serial.print(pRhizome->getEnergy(), 1);
  Serial.print(" S:");
  Serial.print(state < 5 ? stateNames[state] : "?");
  Serial.print(" L:");
  Serial.print(connectionLeftState ? "1" : "0");
  Serial.print(" R:");
  Serial.println(connectionRightState ? "1" : "0");
}

/*--------------------Connection State Machine---------------------------*/
// Replace the simple boolean states with a state machine
enum ConnectionState {
  CONN_DISCONNECTED,
  CONN_DEBOUNCING_CONNECT,
  CONN_CONNECTED,
  CONN_DEBOUNCING_DISCONNECT
};

static ConnectionState leftConnState = CONN_DISCONNECTED;
static ConnectionState rightConnState = CONN_DISCONNECTED;
static unsigned long leftDebounceStart = 0;
static unsigned long rightDebounceStart = 0;

static const unsigned long CONNECT_DEBOUNCE = 150;    // Fast connect detection
static const unsigned long DISCONNECT_DEBOUNCE = 600; // Slow disconnect (pogo bounce)

// Call this instead of checking events directly
void updateConnectionStates() {
  unsigned long now = millis();
  bool leftPin = digitalRead(CONNECT_PIN1) == LOW;
  bool rightPin = digitalRead(CONNECT_PIN2) == LOW;

  
  // LEFT connection state machine
  switch (leftConnState) {
    case CONN_DISCONNECTED:
      if (leftPin) {
        leftConnState = CONN_DEBOUNCING_CONNECT;
        leftDebounceStart = now;
      }
      break;
      
    case CONN_DEBOUNCING_CONNECT:
      if (!leftPin) {
        leftConnState = CONN_DISCONNECTED; // False trigger
        Serial.println("[DEBOUNCE] Left connect bounce ignored");
      } else if (now - leftDebounceStart >= CONNECT_DEBOUNCE) {
        leftConnState = CONN_CONNECTED;
        connectionLeftState = true;
        onLeftConnected(); // Callback
      }
      break;
      
    case CONN_CONNECTED:
      if (!leftPin) {
        leftConnState = CONN_DEBOUNCING_DISCONNECT;
        leftDebounceStart = now;
      }
      break;
      
    case CONN_DEBOUNCING_DISCONNECT:
      if (leftPin) {
        leftConnState = CONN_CONNECTED; // Was just a bounce
      } else if (now - leftDebounceStart >= DISCONNECT_DEBOUNCE) {
        leftConnState = CONN_DISCONNECTED;
        connectionLeftState = false;
        onLeftDisconnected(); // Callback
      }
      break;
  }
  
  // RIGHT connection state machine (same logic)
  switch (rightConnState) {
    case CONN_DISCONNECTED:
      if (rightPin) {
        rightConnState = CONN_DEBOUNCING_CONNECT;
        rightDebounceStart = now;
      }
      break;
      
    case CONN_DEBOUNCING_CONNECT:
      if (!rightPin) {
        rightConnState = CONN_DISCONNECTED;
      } else if (now - rightDebounceStart >= CONNECT_DEBOUNCE) {
        rightConnState = CONN_CONNECTED;
        connectionRightState = true;
        onRightConnected();
      }
      break;
      
    case CONN_CONNECTED:
      if (!rightPin) {
        rightConnState = CONN_DEBOUNCING_DISCONNECT;
        rightDebounceStart = now;
      }
      break;
      
    case CONN_DEBOUNCING_DISCONNECT:
      if (rightPin) {
        rightConnState = CONN_CONNECTED;
      } else if (now - rightDebounceStart >= DISCONNECT_DEBOUNCE) {
        rightConnState = CONN_DISCONNECTED;
        connectionRightState = false;
        onRightDisconnected();
      }
      break;
  }
}

/*-------------------Simplified State Machine---------------------------*/

// Clean callbacks for connection events
void onLeftConnected() {
  Serial.println("[CONN] Left connected");
  listening = true;
  
  // If we're already connected right and generating, stay generating
  // Just start listening for messages from left
}

void onLeftDisconnected() {
  Serial.println("[CONN] Left disconnected");
  listening = false;
  
  // If we were MIDDLEMAN, become GIVING_TO_NODE
  if (pRhizome->getState() == 4 && connectedToNode) {
    Serial.println("[STATE] MIDDLEMAN -> GIVING_TO_NODE (left disconnected)");
    pRhizome->setState(3);
    setNodeDrainRate(currentDrainRate);
  }
  
  // If we were GENERATING, the loop is broken - go back to IDLE
  if (pRhizome->getState() == 2) {
    Serial.println("[STATE] GENERATING -> IDLE (loop broken)");
    discoveryCompleted = false;
    pRhizome->setState(0);
  }
  
  checkBothDisconnected();
}

void onRightConnected() {
  Serial.println("[CONN] Right connected");
  
  // Always send discover_list when right connects
  discoveryMode = true;
  discoverySent = false;
  
  // If left not connected, we're starting fresh
  if (!connectionLeftState) {
    clearSeenIds();
    addSeenId(pRhizome->getID());
  }
  
  sendDiscover();
}

void onRightDisconnected() {
  Serial.println("[CONN] Right disconnected");
  discoveryMode = false;
  discoverySent = false;
  
  // Lost connection to node
  if (connectedToNode) {
    Serial.println("[NODE] Connection lost");
    connectedToNode = false;
    currentDrainRate = 0.0f;
    setNodeDrainRate(0.0f);
  }
  
  // If was GENERATING, the loop is broken - go back to IDLE
  if (pRhizome->getState() == 2) {
    Serial.println("[STATE] GENERATING -> IDLE (loop broken)");
    discoveryCompleted = false;
    pRhizome->setState(0);
  }
  
  checkBothDisconnected();
}

void checkBothDisconnected() {
  if (!connectionLeftState && !connectionRightState) {
    Serial.println("[RESET] Both disconnected -> IDLE");
    resetToIdle();
  }
}

void resetToIdle() {
  listening = false;
  discoveryMode = false;
  waitingForToken = false;
  discoveryCompleted = false;
  connectedToNode = false;
  currentDrainRate = 0.0f;
  setNodeDrainRate(0.0f);
  clearSeenIds();
  addSeenId(pRhizome->getID());
  pRhizome->setCount(1);
  pRhizome->setState(0);
}