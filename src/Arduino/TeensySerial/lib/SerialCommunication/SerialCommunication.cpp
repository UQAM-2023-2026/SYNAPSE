#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h> // For communication over Serial with OSC messages
#include <EnergyManagement.h>
#include "SharedState.h"

#define CONNECT_PIN 9 // pin used to detect connection
#define SERIAL_PORT Serial2
#define SERIAL_BAUD 9600

MicroOscSlip<32> oscSlip(&Serial2); // Using Serial2 for communication
void oscMessageReceived(MicroOscMessage &msg);

// stored pointer to the external rhizome object
static RhizomeStateAndID *pRhizome = nullptr;

/*--------------------ISR---------------------------*/
// ISR-safe event flag (set by ISR), and persistent connection state
static volatile bool connectedEvent = false;
static bool connectionState = false;

// ISR: only set short flag
void connected() {
  connectedEvent = true;
}
/*--------------------------------------------------*/

//Discovery process variables
static bool discoveryMode = false;
static int discoveryOrigin = -1;
static int discoveredCount = 0;
void sendDiscover();
void updateNumberOfConnectedRhizomes();

void beginSerialCommunication(RhizomeStateAndID &rh) {
  pRhizome = &rh;

  Serial2.begin(9600); // Pins 7 (RX2) et 8 (TX2)
  pinMode(9, INPUT_PULLUP); // grounded when connected
  pinMode(13, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(9), connected, CHANGE); // use CHANGE to detect connect/disconnect
  
  Serial.println("Serial Communication Initialized.");
}

void checkConnectionStatus() {
  if (connectedEvent) {
    connectedEvent = false;
    bool currentlyConnected = (digitalRead(CONNECT_PIN) == LOW); // LOW means connected
    if (currentlyConnected != connectionState) {
      connectionState = currentlyConnected;

      if (connectionState) {
        digitalWrite(13, HIGH); // indicate connection
        sendDiscover();

      } else {
        digitalWrite(13, LOW); // indicate disconnection
        setConnectionToRhizome(false); // we are disconnected from a rhizome
        setConnectionToNode(false); // we are disconnected from a node
        setGeneratingState(false); // we are no longer generating
        discoveryMode = false;
        discoveredCount = 0;
        numberOfConnectedRhizomes(0);
        connectedRhizomesCount = 0;
        // Serial.print("Disconnected. Number of connected rhizomes: ");
        // Serial.println(discoveredCount);

      }
    }
  }
  if (!connectionState) {
    discoveryMode = false;
    discoveredCount = 0;
    numberOfConnectedRhizomes(0);
    connectedRhizomesCount = 0;
    setGeneratingState(false);
    setConnectionToRhizome(false);
    setConnectionToNode(false);
    return;
  }
}

void sendDiscover() {
  if (!pRhizome) return;
  discoveryMode = true;
  discoveryOrigin = pRhizome->getID();
  discoveredCount = 1;

  oscSlip.sendMessage("/discover", "ii", discoveryOrigin, discoveredCount);
  // Serial.print("Sent ID: ");
  // Serial.print(discoveryOrigin);
  // Serial.print(" with count: ");
  // Serial.println(discoveredCount);
}

// call frequently from loop()
void lookForMessages() {
  oscSlip.onOscMessageReceived(oscMessageReceived);
  updateNumberOfConnectedRhizomes();
  //Serial.println(discoveredCount);
}

void oscMessageReceived(MicroOscMessage &msg) {
  if (!pRhizome) return;

  // --- DISCOVERY TOKEN ---
  if (msg.checkOscAddress("/discover")) {
    int origin = msg.nextAsInt();
    discoveredCount = msg.nextAsInt();
    setConnectionToRhizome(true);
    // Serial.print("Received discover message from ID: ");
    // Serial.print(origin);
    // Serial.print(" with count: ");
    // Serial.println(count);

    // Si nous ne sommes PAS l'origine → on s'ajoute au count
    if (origin != pRhizome->getID()) {
        discoveredCount += 1; // nous comptons
        oscSlip.sendMessage("/discover", "ii", origin, discoveredCount);
        // Serial.print("Forwarded discover message. New count: ");
        // Serial.println(count);
    } else {
        // Le token vient de revenir à l'origine → fin de découverte
        oscSlip.sendMessage("/discover_done", "i", discoveredCount);
        discoveryMode = false;
        numberOfConnectedRhizomes(discoveredCount); // optional immediate update
        return;
    }
  } else {
    numberOfConnectedRhizomes(0);
    connectedRhizomesCount = 0;
    setGeneratingState(false);
  }

  if (msg.checkOscAddress("/discover_done")) {
    int total = msg.nextAsInt();
    numberOfConnectedRhizomes(total);
    setConnectionToRhizome(true);
    setGeneratingState(true);
    discoveryMode = false;
    return;
  }

  // Node
  if (msg.checkOscAddress("/node")) {
    setConnectionToNode(true);
    setNodeDrainRate(msg.nextAsFloat());
    return;
  }


  
}

void updateNumberOfConnectedRhizomes() {
  connectedRhizomesCount = discoveredCount; 
}

