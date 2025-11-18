/*-------------------Libraries----------------------*/
#include "SerialCommunication.h"

#include <Arduino.h>
#include <MicroOscSlip.h> // For communication over Serial with OSC messages

#include <EnergyManagement.h>
#include <StripsAnimation.h>

#include <RhizomeStateAndID.h>
static RhizomeStateAndID *pRhizome = nullptr; // pointer to external rhizome object


// ==========================================================
// Gestion de la communication - Projet Synapse
// ==========================================================

/* Serial configuration */
#define CONNECT_PIN 9 // pin used to detect connection
#define SERIAL_PORT Serial2
#define SERIAL_BAUD 9600
/*--------------------------------------------------------*/

/*------------------Global Variables-----------------*/
// Variables for osc
MicroOscSlip<32> oscSlip(&SERIAL_PORT); // Using Serial2 for communication
void oscMessageReceived(MicroOscMessage &msg);

//Discovery process variables
static bool discoveryMode = false;
static int discoveryOrigin = -1;
static int discoveredCount = 0;
/*--------------------------------------------------------*/

/*--------------------ISR---------------------------*/
// ISR-safe event flag (set by ISR), and persistent connection state
static volatile bool connectedEvent = false;
static bool connectionState = false;

// ISR: only set short flag
void connected() {
  connectedEvent = true;
}
/*--------------------------------------------------*/



void SetupSerialCommunication(RhizomeStateAndID &rh) {
  pRhizome = &rh;

  Serial2.begin(SERIAL_BAUD); // Pins 7 (RX2) et 8 (TX2)
  pinMode(CONNECT_PIN, INPUT_PULLUP); // grounded when connected
  pinMode(13, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(CONNECT_PIN), connected, CHANGE); // use CHANGE to detect connect/disconnect

  Serial.println("Serial Communication Initialized.");
}

void sendDiscover();
void updateNumberOfConnectedRhizomes();

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
        StateLoop(0);
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
    StateLoop(0);
    return;
  }
}

void sendDiscover() {
  if (!pRhizome) return;
  discoveryMode = true;
  discoveryOrigin = pRhizome->getID();
  discoveredCount = 1;
  pRhizome->setCount(discoveredCount); // reset count before discovery

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
    pRhizome->setCount(discoveredCount);
    setConnectionToRhizome(true);
    // Serial.print("Received discover message from ID: ");
    // Serial.print(origin);
    // Serial.print(" with count: ");
    // Serial.println(count);

    // Si nous ne sommes PAS l'origine → on s'ajoute au count
    if (origin != pRhizome->getID()) {
        discoveredCount += 1; // nous comptons
        pRhizome->setCount(discoveredCount);
        oscSlip.sendMessage("/discover", "ii", origin, discoveredCount);
        // Serial.print("Forwarded discover message. New count: ");
        // Serial.println(count);
    } else {
        // Le token vient de revenir à l'origine → fin de découverte
        oscSlip.sendMessage("/discover_done", "i", discoveredCount);
        discoveryMode = false;
        numberOfConnectedRhizomes(discoveredCount); // optional immediate update
        pRhizome->setCount(discoveredCount); // reset count before discovery
        StateLoop(2);
        return;
    }
  } else {
    numberOfConnectedRhizomes(0);
    connectedRhizomesCount = 0;
    setGeneratingState(false);
  }
  
  // Node
  if (msg.checkOscAddress("/node")) {
    setConnectionToNode(true);
    setNodeDrainRate(msg.nextAsFloat());
    StateLoop(3); // Update LED strips animation
    return;
  }

  if (msg.checkOscAddress("/discover_done")) {
    int total = msg.nextAsInt();
    numberOfConnectedRhizomes(total);
    setConnectionToRhizome(true);
    setGeneratingState(true);
    discoveryMode = false;
    StateLoop(2);
    return;
  }
}

void updateNumberOfConnectedRhizomes() {
  connectedRhizomesCount = discoveredCount; 
  pRhizome->setCount(discoveredCount); // reset count before discovery
}

