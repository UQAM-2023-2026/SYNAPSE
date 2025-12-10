/*-------------------Libraries----------------------*/
#include "SerialCommunication.h"

#include <Arduino.h>
#include <MicroOscSlip.h>

#include <EnergyManagement.h>
#include <StripsAnimation.h>
#include <HapticFeedback.h>

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
}
void connectedRight() {
  connectedRightEvent = true;
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
void sendDiscover() {
  if (!pRhizome || !discoveryMode) return;

//   int myId = pRhizome->getID();
//   int count = seenCount;

//   // original token (keeps compatibility)
//   oscSlipSend.sendMessage("/discover", "ii", myId, count);
//   Serial.print("[SEND] /discover origin=");
//   Serial.print(myId);
//   Serial.print(" count=");
//   Serial.println(count);

  // build CSV of seen IDs and send as a second message
  String csv;
  for (int i = 0; i < seenCount; ++i) {
    int id = getSeenIdAt(i);
    if (i) csv += ','; 
    csv += String(id);
  }
  // send list as a single string argument
  oscSlipSend.sendMessage("/discover_list", "s", csv.c_str());
  Serial.print("[SEND] /discover_list ");
  Serial.println(csv);

  waitingForToken = true;
  discoverySent = true;
}

/*---------------Handle OSC Messages-------------------*/
void oscMessageReceived(MicroOscMessage &msg) {
  if (!pRhizome) return;

//   if (msg.checkOscAddress("/discover")) {
//     int origin = msg.nextAsInt();
//     int incomingCount = msg.nextAsInt();
    
//     Serial.println("---");
//     Serial.print("[RECV] /discover origin=");
//     Serial.print(origin);
//     Serial.print(" count=");
//     Serial.println(incomingCount);
    
//     //printSeenIds();

//     if (origin < 0 || origin > 1000) {
//       Serial.println("[ERROR] Invalid origin ID - ignoring");
//       return;
//     }

//     if (origin == pRhizome->getID()) {
//       Serial.println("[LOOP DETECTED] My token returned!");
//       Serial.print("  Total rhizomes in loop: ");
//       Serial.println(seenCount);
//       Serial.print("  (incomingCount was: ");
//       Serial.print(incomingCount);
//       Serial.println(")");
      
//       int finalTotal = max(seenCount, incomingCount);
      
//       Serial.print("  Final total (using max): ");
//       Serial.println(finalTotal);
      
//       oscSlipSend.sendMessage("/discover_done", "i", finalTotal);
      
//       pRhizome->setCount(finalTotal);
//       pRhizome->setState(2);
//       discoveryMode = false;
//       waitingForToken = false;
//       discoveryCompleted = true;
      
//       Serial.println("[STATE] Entering GENERATING mode");
//       return;
//     }

//     if (isIdSeen(origin)) {
//       Serial.println("[DUPLICATE] Origin already seen - loop detected");
//       Serial.print("  Finishing discovery with count: ");
//       Serial.println(seenCount);
      
//       if (connectionLeftState && connectionRightState && !discoveryCompleted) {
//         Serial.println("  Both sides connected - completing discovery");
//         oscSlipSend.sendMessage("/discover_done", "i", seenCount);
//         pRhizome->setCount(seenCount);
//         pRhizome->setState(2);
//         discoveryMode = false;
//         waitingForToken = false;
//         discoveryCompleted = true;
//         Serial.println("  Entered GENERATING mode");
//       } else if (discoveryCompleted) {
//         Serial.println("  Discovery already completed - ignoring");
//       } else {
//         Serial.println("  Not fully connected - ignoring");
//       }
//       return;
//     }

//     Serial.print("[NEW] Adding origin ID ");
//     Serial.print(origin);
//     Serial.println(" to seen list");
    
//     addSeenId(origin);
//     pRhizome->setCount(seenCount);
    
//     //printSeenIds();

//     Serial.print("[FORWARD] Relaying /discover with updated count=");
//     Serial.println(seenCount);
//     Serial.print("  Sending to right (Serial3): origin=");
//     Serial.print(origin);
//     Serial.print(" count=");
//     Serial.println(seenCount);
    
//     oscSlipSend.sendMessage("/discover", "ii", origin, seenCount);
//     Serial.println("---");

//   } else if (msg.checkOscAddress("/discover_done")) {
//     int total = msg.nextAsInt();
//     Serial.println("---");
//     Serial.print("[RECV] /discover_done total=");
//     Serial.println(total);
    
//     if (discoveryCompleted && pRhizome->getState() == 2) {
//       Serial.println("  Discovery already completed and still GENERATING - ignoring");
//       Serial.println("---");
//       return;
//     }
    
//     if (!connectionLeftState || !connectionRightState) {
//       Serial.println("  WARNING: Received /discover_done but not fully connected");
//       Serial.println("  Ignoring to prevent incomplete GENERATING state");
//       Serial.println("---");
//       return;
//     }
    
//     if (total > seenCount) {
//       Serial.print("  Updating count from ");
//       Serial.print(seenCount);
//       Serial.print(" to ");
//       Serial.println(total);
//     }
    
//     pRhizome->setCount(total);
//     pRhizome->setState(2);
//     discoveryMode = false;
//     waitingForToken = false;
//     discoveryCompleted = true;
    
//     Serial.println("[STATE] Entering GENERATING mode (discovery complete)");
    
//     if (connectionRightState) {
//       Serial.println("  Forwarding /discover_done (one time only)");
//       oscSlipSend.sendMessage("/discover_done", "i", total);
//     }
//     Serial.println("---");

//   } else if (msg.checkOscAddress("/node")) {
//     float drainRate = msg.nextAsFloat();
    
//     Serial.println("---");
//     Serial.print("[RECV] /node drainRate=");
//     Serial.println(drainRate);
    
//     currentDrainRate = drainRate;
//     connectedToNode = true;
    
//     // KEY FIX: Check if rhizome is behind us NOW
//     if (connectionLeftState) {
//       Serial.println("[STATE] MIDDLEMAN - rhizome behind us");
//       pRhizome->setState(4);
//       setNodeDrainRate(0.0f);
//       lastDrainSent = millis();
//       oscSlipReceive.sendMessage("/drain", "f", drainRate);
//     } else {
//       Serial.println("[STATE] GIVING_TO_NODE - no rhizome behind");
//       pRhizome->setState(3);
//       setNodeDrainRate(drainRate);
//       float energy = pRhizome->getEnergy();
//       int energyInt = (int)energy;
//       oscSlipSend.sendMessage("/energy", "ii", pRhizome->getID(), energyInt);
//       lastEnergySent = millis();
//     }
//     Serial.println("---");
    
//   } else if (msg.checkOscAddress("/drain")) {
//     float drainRate = msg.nextAsFloat();
    
//     Serial.println("---");
//     Serial.print("[RECV] /drain rate=");
//     Serial.println(drainRate);
    
//     currentDrainRate = drainRate;
//     lastDrainReceived = millis();
    
//     // ALWAYS accept drain and become GIVING (even if was in GENERATING)
//     Serial.println("[STATE] GIVING_TO_NODE (from /drain)");
//     pRhizome->setState(3);
//     setNodeDrainRate(drainRate);
    
//     float energy = pRhizome->getEnergy();
//     int energyInt = (int)energy;
//     oscSlipReceive.sendMessage("/energy", "ii", pRhizome->getID(), energyInt);
//     Serial.println("---");
//   }
  if (msg.checkOscAddress("/discover_list")) {
    // receive CSV of ids from other node, merge into seenIds
    const char *csv = msg.nextAsString(); // MicroOscMessage string accessor
    Serial.print("[RECV] /discover_list csv=");
    Serial.println(csv ? csv : "(null)");

    if (csv && csv[0] != '\0') {
      // parse CSV
      char buf[256];
      strncpy(buf, csv, sizeof(buf) - 1);
      buf[sizeof(buf) - 1] = '\0';
      char *tok = strtok(buf, ",");
      while (tok) {
        int id = atoi(tok);
        if (id >= 0) {
          if (!isIdSeen(id)) {
            Serial.print("  [MERGE] adding id ");
            Serial.println(id);
            addSeenId(id);
          } else {
            Serial.print("  [MERGE] already have id ");
            Serial.println(id);
          }
        }
        tok = strtok(NULL, ",");
      }
      // Update rhizome count and report
      pRhizome->setCount(getSeenIdsCount());
      Serial.print("[MERGE DONE] seenCount=");
      Serial.println(getSeenIdsCount());
    } else {
      Serial.println("[RECV] /discover_list empty");
    }
    return;
  }
}

void isThisANodeMessage(MicroOscMessage &msg) {
  if (!pRhizome) return;
  
  if (msg.checkOscAddress("/node")) {
    float drainRate = msg.nextAsFloat();
    
    Serial.println("---");
    Serial.print("[RECV] /node drainRate=");
    Serial.println(drainRate);
    
    currentDrainRate = drainRate;
    connectedToNode = true;
    
    // Check if rhizome behind us
    if (connectionLeftState) {
      Serial.println("[STATE] MIDDLEMAN - rhizome behind us");
      pRhizome->setState(4);
      setNodeDrainRate(0.0f);
      lastDrainSent = millis();
      oscSlipReceive.sendMessage("/drain", "f", drainRate);
    } else {
      Serial.println("[STATE] GIVING_TO_NODE - no rhizome behind");
      pRhizome->setState(3);
      setNodeDrainRate(drainRate);
      float energy = pRhizome->getEnergy();
      int energyInt = (int)energy;
      oscSlipSend.sendMessage("/energy", "ii", pRhizome->getID(), energyInt);
      lastEnergySent = millis();
    }
    Serial.println("---");
  }
}

/*-------------------CHECK CONNECTIONS-------------------------------*/
void checkConnectionStatus() {
  bool connectionLeftState = digitalRead(CONNECT_PIN1) == LOW;
  bool connectionRightState = digitalRead(CONNECT_PIN2) == LOW;

  unsigned long now = millis();

  // Handle LEFT connection event
  if (connectedLeftEvent) {
    connectedLeftEvent = false;
    
    if (now - lastLeftChangeTime >= DEBOUNCE_DELAY) {
      lastLeftChangeTime = now;
      
      if (connectionLeftState) {
        Serial.println("[EVENT] Left connected - entering listening mode");
        listening = true;
        
      } else if (!connectionLeftState) {
        Serial.println("[EVENT] Left disconnected");
        listening = false;
        waitingForToken = false;

        if(pRhizome->getState() !=2){
            pRhizome->setState(0);
            Serial.println("[STATE] Entering IDLE mode");
        }
        
        
        if (discoveryCompleted) {
          Serial.println("  Resetting discovery state for potential reconnection");
          discoveryCompleted = false;
        }
        
        // DON'T stop GENERATING - might be transitioning to node mode
        // Only stop if BOTH sides disconnect
        
        //connectionLeftState = leftNow;
        
        if (!connectionRightState) {
          Serial.println("[RESET] Both sides disconnected - resetting to idle");
          clearSeenIds();
          addSeenId(pRhizome->getID());
          pRhizome->setCount(1);
          pRhizome->setState(0);
        }
      }
    }
  }

  // Handle RIGHT connection event
  if (connectedRightEvent) {
    connectedRightEvent = false;
    
    if (now - lastRightChangeTime >= DEBOUNCE_DELAY) {
      lastRightChangeTime = now;
      
      if (connectionRightState) {
        Serial.println("[EVENT] Right connected - starting discovery");
        Serial.println("[DISCOVERY] Initiating token passing...");
        
        discoveryMode = true;
        //connectionRightState = rightNow;
        
        if (!connectionLeftState) {
          clearSeenIds();
          addSeenId(pRhizome->getID());
          pRhizome->setCount(1);
        }
        
        //printSeenIds();
        discoveryInitiatedTime = millis();
        discoverySent = false;
        Serial.println("  Waiting for connection to stabilize...");
        
      } else if (!connectionRightState) {
        Serial.println("[EVENT] Right disconnected");
        discoveryMode = false;
        waitingForToken = false;
        
        if (discoveryCompleted) {
          Serial.println("  Resetting discovery state for potential reconnection");
          discoveryCompleted = false;
        }
        
        if (connectedToNode || pRhizome->getState() == 2 || pRhizome->getState() == 3) {
          Serial.println("[NODE] Disconnected from node");
          connectedToNode = false;
          currentDrainRate = 0.0f;
          setNodeDrainRate(0.0f);
        }
        
        // DON'T stop GENERATING - might be transitioning
        
        connectionRightState = rightNow;
        
        if (!leftNow) {
          Serial.println("[RESET] Both sides disconnected - resetting to idle");
          clearSeenIds();
          addSeenId(pRhizome->getID());
          pRhizome->setCount(1);
          pRhizome->setState(0);
        }
      }
    }
  }
  
  connectionLeftState = leftNow;
  connectionRightState = rightNow;
  
  if (discoveryMode && !discoverySent && (now - discoveryInitiatedTime >= DISCOVERY_DELAY)) {
    sendDiscover();
  }

  oscSlipReceive.onOscMessageReceived(oscMessageReceived);
  oscSlipSend.onOscMessageReceived(isThisANodeMessage);
  
  if (leftNow) {
    listening = true;
  }
  
  // Safety check: ONLY if BOTH sides disconnected
  if (pRhizome->getState() == 2 && !leftNow && !rightNow) {
    Serial.println("[WARNING] Both sides disconnected - reverting to IDLE");
    pRhizome->setState(0);
    discoveryMode = false;
    discoveryCompleted = false;
  }
  
  // MIDDLEMAN: send drain periodically
  if (pRhizome->getState() == 4 && connectedToNode) {
    if (now - lastDrainSent >= DRAIN_SEND_INTERVAL) {
      lastDrainSent = now;
      if (connectionLeftState) {
        oscSlipReceive.sendMessage("/drain", "f", currentDrainRate);
      } else {
        Serial.println("[MIDDLEMAN] No rhizome behind - becoming GIVING");
        pRhizome->setState(3);
        setNodeDrainRate(currentDrainRate);
      }
    }
    
    if (now - lastDrainReceived > DRAIN_TIMEOUT) {
      Serial.println("[MIDDLEMAN] Timeout - becoming GIVING");
      pRhizome->setState(3);
      setNodeDrainRate(currentDrainRate);
    }
  }
  
  // GIVING_TO_NODE: send energy continuously
  if (pRhizome->getState() == 3 && connectedToNode) {
    if (now - lastEnergySent >= ENERGY_SEND_INTERVAL) {
      lastEnergySent = now;
      float energy = pRhizome->getEnergy();
      int energyInt = (int)energy;
      oscSlipSend.sendMessage("/energy", "ii", pRhizome->getID(), energyInt);
    }
    
    if (pRhizome->getEnergy() <= 0.1f) {
      Serial.println("[GIVING] Energy depleted!");
      pRhizome->setState(0);
      connectedToNode = false;
      setNodeDrainRate(0.0f);
    }
  }

  if (!leftNow && !rightNow) {
    if (listening || discoveryMode) {
      Serial.println("[RESET] Entering idle state (both disconnected)");
      listening = false;
      discoveryMode = false;
      waitingForToken = false;
      discoveryCompleted = false;
      clearSeenIds();
      addSeenId(pRhizome->getID());
      pRhizome->setCount(1);
      pRhizome->setState(0);
    }
  }
  
  if (now - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
    lastStatusPrint = now;
    
    if (pRhizome->getState() >= 2) {
      Serial.println("====================");
      Serial.print("Rhizome ID: ");
      Serial.print(pRhizome->getID());
      Serial.print(" | Count: ");
      Serial.print(pRhizome->getCount());
      Serial.print(" | Energy: ");
      Serial.print(pRhizome->getEnergy(), 2);
      Serial.print(" | State: ");
      
      switch(pRhizome->getState()) {
        case 0: Serial.print("IDLE"); break;
        case 1: Serial.print("CONNECTED"); break;
        case 2: Serial.print("GENERATING"); break;
        case 3: Serial.print("GIVING_TO_NODE"); break;
        case 4: Serial.print("MIDDLEMAN"); break;
        default: Serial.print("UNKNOWN"); break;
      }
      
      if (pRhizome->getState() == 3 || pRhizome->getState() == 4) {
        Serial.print(" | DrainRate: ");
        Serial.print(currentDrainRate, 2);
      }
      
      Serial.println();
      Serial.println("====================");
    }
  }
}

/*---------------Seen IDs Management-------------------*/
void clearSeenIds() {
  noInterrupts();
  seenStart = 0;
  seenCount = 0;
  for (int i = 0; i < MAX_SEEN_IDS; ++i) {
    seenIds[i] = -1;
  }
  interrupts();
}

bool isIdSeen(int id) {
  if (seenCount == 0) return false;
  for (int i = 0; i < seenCount; ++i) {
    int idx = (seenStart + i) % MAX_SEEN_IDS;
    if (seenIds[idx] == id) return true;
  }
  return false;
}

void addSeenId(int id) {
  if (id < 0) {
    Serial.println("[ERROR] Attempted to add negative ID");
    return;
  }
  
  if (isIdSeen(id)) {
    return;
  }
  
  noInterrupts();
  if (seenCount < MAX_SEEN_IDS) {
    int idx = (seenStart + seenCount) % MAX_SEEN_IDS;
    seenIds[idx] = id;
    ++seenCount;
  } else {
    Serial.println("[WARNING] seenIds buffer full - overwriting oldest");
    seenIds[seenStart] = id;
    seenStart = (seenStart + 1) % MAX_SEEN_IDS;
  }
  interrupts();
}

int getSeenIdsCount() {
  return seenCount;
}

int getSeenIdAt(int index) {
  if (index < 0 || index >= seenCount) return -1;
  int idx = (seenStart + index) % MAX_SEEN_IDS;
  return seenIds[idx];
}