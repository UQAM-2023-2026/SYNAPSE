/*-------------------Libraries----------------------*/
#include "SerialCommunication.h"

#include <Arduino.h>
#include <MicroOscSlip.h>

#include <EnergyManagement.h>
#include <StripsAnimation.h>
#include <HapticFeedback.h>

#include <RhizomeStateAndID.h>
static RhizomeStateAndID *pRhizome = nullptr;

/* Serial configuration */
#define SERIAL_BAUD 9600

//Left handle connection (receiving)
#define CONNECT_PIN1 6
#define SERIAL_PORT_RECEIVE Serial2

//Right handle connection (sending)
#define CONNECT_PIN2 20
#define SERIAL_PORT_SEND Serial3

/*------------------Global Variables-----------------*/
MicroOscSlip<32> oscSlipReceive(&SERIAL_PORT_RECEIVE);
MicroOscSlip<32> oscSlipSend(&SERIAL_PORT_SEND);

void oscMessageReceived(MicroOscMessage &msg);
void isThisANodeMessage(MicroOscMessage &msg);

//Discovery process variables
static bool discoveryMode = false;
static bool listening = false;
static bool waitingForToken = false;
static bool discoveryCompleted = false;

// Status monitoring
static unsigned long lastStatusPrint = 0;
static const unsigned long STATUS_PRINT_INTERVAL = 2000; // Print every 2 seconds

/*--------------------ISR---------------------------*/
// ISR-safe event flag (set by ISR), and persistent connection state
static volatile bool connectedLeftEvent = false;
static bool connectionLeftState = false;
static unsigned long lastLeftChangeTime = 0;

static volatile bool connectedRightEvent = false;
static bool connectionRightState = false;
static unsigned long lastRightChangeTime = 0;

// Debounce settings
static const unsigned long DEBOUNCE_DELAY = 150; // 150ms debounce (increased for stability)

// Circular buffer for seen IDs
static const int MAX_SEEN_IDS = 20;
static int seenIds[MAX_SEEN_IDS];
static int seenStart = 0;
static int seenCount = 0;

void connectedLeft() {
  connectedLeftEvent = true;
}
void connectedRight() {
  connectedRightEvent = true;
}

/*--------------------------------------------------*/

void SetupSerialCommunication(RhizomeStateAndID &rh) {
  pRhizome = &rh;

  Serial2.begin(SERIAL_BAUD);
  Serial3.begin(SERIAL_BAUD);

  pinMode(CONNECT_PIN1, INPUT_PULLUP);
  pinMode(CONNECT_PIN2, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(CONNECT_PIN1), connectedLeft, CHANGE);
  attachInterrupt(digitalPinToInterrupt(CONNECT_PIN2), connectedRight, CHANGE);

  pinMode(22, OUTPUT);
  
  // Initialize seenIds buffer with only self
  clearSeenIds();
  addSeenId(pRhizome->getID());

  // NOTE: We DON'T register callbacks here anymore
  // They will be called in checkConnectionStatus() loop

  ////Serial.println("Serial Communication Initialized.");
  //Serial.print("My ID: "); //Serial.println(pRhizome->getID());
}

/*--------------Discovery Functions-------------------*/

void sendDiscover() {
  if (!pRhizome || !discoveryMode) return;
  
  int myId = pRhizome->getID();
  int count = seenCount;
  
  oscSlipSend.sendMessage("/discover", "ii", myId, count);
  //Serial.print("[SEND] /discover origin="); 
  //Serial.print(myId); 
  //Serial.print(" count="); 
  //Serial.println(count);
  
  waitingForToken = true;
}

void lookForMessages() {
  if (!pRhizome) return;
  // Process incoming messages on receive port
  oscSlipReceive.onOscMessageReceived(oscMessageReceived);
}

void listenToNode() {
  if (!pRhizome) return;
  // Process incoming messages on send port (for node detection)
  oscSlipSend.onOscMessageReceived(isThisANodeMessage);
}

/*--------------------------------------------------*/

// // Debug helper
// static void printSeenIds() {
//   //Serial.print("  Seen IDs (count=");
//   //Serial.print(seenCount);
//   //Serial.print("): ");
//   for (int i = 0; i < seenCount; ++i) {
//     int idx = (seenStart + i) % MAX_SEEN_IDS;
//     //Serial.print(seenIds[idx]);
//     if (i < seenCount - 1) //Serial.print(", ");
//   }
//   //Serial.println();
// }

/*--------------------------------------------------*/

// Helper function to print current rhizome status on demand
// void printRhizomeStatus() {
//   if (!pRhizome) return;
  
//   //Serial.println("┌────────────────────────────────┐");
//   //Serial.print("│ Rhizome ID: ");
//   //Serial.print(pRhizome->getID());
//   //Serial.println("                  │");
  
//   //Serial.print("│ Connected Rhizomes: ");
//   //Serial.print(pRhizome->getCount());
//   //Serial.println("           │");
  
//   //Serial.print("│ Energy: ");
//   //Serial.print(pRhizome->getEnergy(), 2);
//   //Serial.println("                │");
  
//   //Serial.print("│ State: ");
//   // switch(pRhizome->getState()) {
//   //   case 0: //Serial.print("IDLE          "); break;
//   //   case 1: //Serial.print("CONNECTED     "); break;
//   //   case 2: //Serial.print("GENERATING    "); break;
//   //   case 3: //Serial.print("NODE_DRAIN    "); break;
//   //   default: //Serial.print("UNKNOWN       "); break;
//   // }
//   //Serial.println("         │");
  
//   //Serial.print("│ Left: ");
//   //Serial.print(connectionLeftState ? "CONNECTED" : "DISCONN  ");
//   //Serial.println("            │");
  
//   //Serial.print("│ Right: ");
//   //Serial.print(connectionRightState ? "CONNECTED" : "DISCONN  ");
//   //Serial.println("           │");
  
//   //Serial.println("└────────────────────────────────┘");
// }

// Main function to check connection status and process OSC messages
// MUST be called repeatedly in main loop()
void checkConnectionStatus() {
  // Read current physical connection state
  bool leftNow = digitalRead(CONNECT_PIN1) == LOW;
  bool rightNow = digitalRead(CONNECT_PIN2) == LOW;
  
  unsigned long now = millis();

  // Handle LEFT connection event (receiving side) with debouncing
  if (connectedLeftEvent) {
    connectedLeftEvent = false;
    
    // Only process if enough time has passed since last change
    if (now - lastLeftChangeTime >= DEBOUNCE_DELAY) {
      lastLeftChangeTime = now;
      
      if (leftNow && !connectionLeftState) {
        // Just connected on left
        //Serial.println("[EVENT] Left connected - entering listening mode");
        listening = true;
        connectionLeftState = leftNow;
      } else if (!leftNow && connectionLeftState) {
        // Just disconnected on left
        //Serial.println("[EVENT] Left disconnected");
        listening = false;
        waitingForToken = false;
        
        // Reset discovery state to allow fresh discovery on reconnection
        if (discoveryCompleted) {
          //Serial.println("  Resetting discovery state for potential reconnection");
          discoveryCompleted = false;
        }
        
        // If we were generating, stop immediately
        if (pRhizome->getState() == 2) {
          //Serial.println("[STATE] Stopping GENERATING - connection lost");
          pRhizome->setState(0); // back to idle
        }
        
        connectionLeftState = leftNow;
        
        // Only full reset if BOTH sides disconnected
        if (!rightNow) {
          //Serial.println("[RESET] Both sides disconnected - resetting to idle");
          clearSeenIds();
          addSeenId(pRhizome->getID());
          pRhizome->setCount(1);
          pRhizome->setState(0);
        }
      }
    }
  }

  // Handle RIGHT connection event (sending side) with debouncing
  if (connectedRightEvent) {
    connectedRightEvent = false;
    
    // Only process if enough time has passed since last change
    if (now - lastRightChangeTime >= DEBOUNCE_DELAY) {
      lastRightChangeTime = now;
      
      if (rightNow && !connectionRightState) {
        // Just connected on right - START DISCOVERY
        //Serial.println("[EVENT] Right connected - starting discovery");
        //Serial.println("[DISCOVERY] Initiating token passing...");
        
        discoveryMode = true;
        connectionRightState = rightNow;
        
        // DON'T reset seenIds if left is already connected!
        // We might have already seen other rhizomes
        if (!leftNow) {
          // Only reset if this is the first connection
          clearSeenIds();
          addSeenId(pRhizome->getID());
          pRhizome->setCount(1);
        }
        
        //printSeenIds();
        
        // Send initial discover token with MY ID as origin
        // Wait longer for connection to fully stabilize
        delay(200); // Increased delay for more stable connection
        sendDiscover();
        
      } else if (!rightNow && connectionRightState) {
        // Just disconnected on right
        //Serial.println("[EVENT] Right disconnected");
        discoveryMode = false;
        waitingForToken = false;
        
        // Reset discovery state to allow fresh discovery on reconnection
        if (discoveryCompleted) {
          //Serial.println("  Resetting discovery state for potential reconnection");
          discoveryCompleted = false;
        }
        
        // If we were generating, stop immediately
        if (pRhizome->getState() == 2) {
          //Serial.println("[STATE] Stopping GENERATING - connection lost");
          pRhizome->setState(0); // back to idle
        }
        
        connectionRightState = rightNow;
        
        // Only full reset if BOTH sides disconnected
        if (!leftNow) {
          //Serial.println("[RESET] Both sides disconnected - resetting to idle");
          clearSeenIds();
          addSeenId(pRhizome->getID());
          pRhizome->setCount(1);
          pRhizome->setState(0);
        }
      }
    }
  }
  
  // CRITICAL: Update connection states continuously to reflect current physical state
  // This prevents race conditions where state is checked before event handler updates it
  connectionLeftState = leftNow;
  connectionRightState = rightNow;

  // Continuous behavior based on connection state
  // ALWAYS process messages regardless of flags!
  oscSlipReceive.onOscMessageReceived(oscMessageReceived);
  oscSlipSend.onOscMessageReceived(isThisANodeMessage);
  
  // Continuous state validation
  if (leftNow) {
    listening = true;
  }

  if (rightNow) {
    // Right is connected
  }
  
  // Safety check: if we're in GENERATING state but not both sides PHYSICALLY connected, abort
  // Use leftNow/rightNow (physical state) instead of connectionLeftState/connectionRightState
  if (pRhizome->getState() == 2 && (!leftNow || !rightNow)) {
    //Serial.println("[WARNING] In GENERATING state but connection incomplete - reverting to IDLE");
    pRhizome->setState(0);
    discoveryMode = false;
    discoveryCompleted = false;
  }

  // Both disconnected - ensure idle state
  if (!leftNow && !rightNow) {
    if (listening || discoveryMode) {
      //Serial.println("[RESET] Entering idle state (both disconnected)");
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
  
  // Periodic status monitoring (when in GENERATING state)
  if (now - lastStatusPrint >= STATUS_PRINT_INTERVAL) {
    lastStatusPrint = now;
    
    // Only print status if in an interesting state
    if (pRhizome->getState() == 2 || pRhizome->getState() == 3) {
      //Serial.println("====================");
      //Serial.print("Rhizome ID: ");
      //Serial.print(pRhizome->getID());
      //Serial.print(" | Count: ");
      //Serial.print(pRhizome->getCount());
      //Serial.print(" | Energy: ");
      //Serial.print(pRhizome->getEnergy(), 2);
      //Serial.print(" | State: ");
      
      // switch(pRhizome->getState()) {
      //   case 0: //Serial.println("IDLE"); break;
      //   case 1: //Serial.println("CONNECTED"); break;
      //   case 2: //Serial.println("GENERATING"); break;
      //   case 3: //Serial.println("NODE_DRAIN"); break;
      //   default: //Serial.println("UNKNOWN"); break;
      // }
      //Serial.println("====================");
    }
  }
}

/*---------------Handle OSC Messages-------------------*/

void oscMessageReceived(MicroOscMessage &msg) {
  if (!pRhizome) return;
  
  // Debug: log that we received SOMETHING on Serial2 (receive port)
  //Serial.print("[DEBUG] Message received on Serial2 (left/receive port) - ");

  if (msg.checkOscAddress("/discover")) {
    // CRITICAL: Read values BEFORE any other operations
    int origin = msg.nextAsInt();
    int incomingCount = msg.nextAsInt();
    
    //Serial.println("---");
    //Serial.print("[RECV] /discover origin=");
    //Serial.print(origin);
    //Serial.print(" count=");
    //Serial.println(incomingCount);
    
    //printSeenIds();

    // Validate origin ID
    if (origin < 0 || origin > 1000) {
      //Serial.println("[ERROR] Invalid origin ID - ignoring");
      return;
    }

    // Check if this is MY token coming back (loop detected!)
    if (origin == pRhizome->getID()) {
      //Serial.println("[LOOP DETECTED] My token returned!");
      //Serial.print("  Total rhizomes in loop: ");
      //Serial.println(seenCount);
      //Serial.print("  (incomingCount was: ");
      //Serial.print(incomingCount);
      //Serial.println(")");
      
      // Use seenCount (our local count) as the authoritative total
      int finalTotal = seenCount;
      
      // Send discover_done to notify everyone
      oscSlipSend.sendMessage("/discover_done", "i", finalTotal);
      
      pRhizome->setCount(finalTotal);
      pRhizome->setState(2); // generating state
      discoveryMode = false;
      waitingForToken = false;
      discoveryCompleted = true;
      
      //Serial.println("[STATE] Entering GENERATING mode");
      return;
    }

    // Check if we've already seen this origin (duplicate/loop)
    if (isIdSeen(origin)) {
      //Serial.println("[DUPLICATE] Origin already seen - loop detected");
      //Serial.print("  Finishing discovery with count: ");
      //Serial.println(seenCount);
      
      // If both sides are connected, we can complete discovery even if not in discoveryMode
      // This handles reconnection scenarios
      if (connectionLeftState && connectionRightState && !discoveryCompleted) {
        //Serial.println("  Both sides connected - completing discovery");
        oscSlipSend.sendMessage("/discover_done", "i", seenCount);
        pRhizome->setCount(seenCount);
        pRhizome->setState(2); // generating
        discoveryMode = false;
        waitingForToken = false;
        discoveryCompleted = true;
        //Serial.println("  Entered GENERATING mode");
      } else if (discoveryCompleted) {
        //Serial.println("  Discovery already completed - ignoring");
      } else {
        //Serial.println("  Not fully connected - ignoring");
      }
      return;
    }

    // NEW ID - Add to our list
    //Serial.print("[NEW] Adding origin ID ");
    //Serial.print(origin);
    //Serial.println(" to seen list");
    
    addSeenId(origin);
    pRhizome->setCount(seenCount);
    
    //printSeenIds();

    // Forward the token with UPDATED count
    //Serial.print("[FORWARD] Relaying /discover with updated count=");
    //Serial.println(seenCount);
    //Serial.print("  Sending to right (Serial3): origin=");
    //Serial.print(origin);
    //Serial.print(" count=");
    //Serial.println(seenCount);
    
    oscSlipSend.sendMessage("/discover", "ii", origin, seenCount);
    //Serial.println("---");

  } else if (msg.checkOscAddress("/discover_done")) {
    int total = msg.nextAsInt();
    //Serial.println("---");
    //Serial.print("[RECV] /discover_done total=");
    //Serial.println(total);
    
    // Allow reprocessing if we just reset discovery state (reconnection scenario)
    if (discoveryCompleted && pRhizome->getState() == 2) {
      //Serial.println("  Discovery already completed and still GENERATING - ignoring");
      //Serial.println("---");
      return;
    }
    
    // Only enter GENERATING if both sides are connected
    if (!connectionLeftState || !connectionRightState) {
      //Serial.println("  WARNING: Received /discover_done but not fully connected");
      //Serial.println("  Ignoring to prevent incomplete GENERATING state");
      //Serial.println("---");
      return;
    }
    
    // Update our count to match the final total
    if (total > seenCount) {
      //Serial.print("  Updating count from ");
      //Serial.print(seenCount);
      //Serial.print(" to ");
      //Serial.println(total);
    }
    
    pRhizome->setCount(total);
    pRhizome->setState(2); // generating
    discoveryMode = false;
    waitingForToken = false;
    discoveryCompleted = true;
    
    //Serial.println("[STATE] Entering GENERATING mode (discovery complete)");
    
    // Forward to next rhizome ONLY ONCE
    if (connectionRightState) {
      //Serial.println("  Forwarding /discover_done (one time only)");
      oscSlipSend.sendMessage("/discover_done", "i", total);
    }
    //Serial.println("---");

  } else if (msg.checkOscAddress("/node")) {
    //Serial.println("---");
    //Serial.println("[RECV] /node - entering NODE DRAIN mode");
    pRhizome->setState(3);
    
    float drainRate = msg.nextAsFloat();
    setNodeDrainRate(drainRate);
    
    oscSlipSend.sendMessage("/energy", "ii", pRhizome->getID(), pRhizome->getEnergy());
    
    //Serial.print("  Drain rate: ");
    //Serial.print(drainRate);
    //Serial.print(" | Energy: ");
    //Serial.println(pRhizome->getEnergy());
    //Serial.println("---");
  }
}

void isThisANodeMessage(MicroOscMessage &msg) {
  if (!pRhizome) return;
  
  if (msg.checkOscAddress("/node")) {
    //Serial.println("---");
    //Serial.println("[RECV] /node on send port");
    pRhizome->setState(3);
    
    float drainRate = msg.nextAsFloat();
    setNodeDrainRate(drainRate);
    
    oscSlipSend.sendMessage("/energy", "ii", pRhizome->getID(), pRhizome->getEnergy());
    
    //Serial.print("  Drain rate: ");
    //Serial.print(drainRate);
    //Serial.print(" | Energy: ");
    //Serial.println(pRhizome->getEnergy());
    //Serial.println("---");
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
    //Serial.println("[ERROR] Attempted to add negative ID");
    return;
  }
  
  if (isIdSeen(id)) {
    return; // Silently ignore duplicates
  }
  
  noInterrupts();
  if (seenCount < MAX_SEEN_IDS) {
    int idx = (seenStart + seenCount) % MAX_SEEN_IDS;
    seenIds[idx] = id;
    ++seenCount;
  } else {
    // Buffer full - overwrite oldest
    //Serial.println("[WARNING] seenIds buffer full - overwriting oldest");
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