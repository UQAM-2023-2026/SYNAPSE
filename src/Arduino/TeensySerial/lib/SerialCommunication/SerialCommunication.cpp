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
#define CONNECT_PIN 20 // pin used to detect connection
#define CONNECT_LED1 23
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
  pinMode(CONNECT_LED1, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(CONNECT_PIN), connected, CHANGE); // use CHANGE to detect connect/disconnect

  Serial.println("Serial Communication Initialized.");
}

// update the number of connected rhizomes
void updateNumberOfConnectedRhizomes() {
  pRhizome->setCount(discoveredCount);
}


/*--------------Send Messages-------------------*/

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

/*--------------------------------------------------*/

/*--------------------------------------------------*/
// Main loops to check connection status and handle messages
void checkConnectionStatus() {
  if (connectedEvent) {
    connectedEvent = false;
    bool currentlyConnected = (digitalRead(CONNECT_PIN) == LOW); // LOW means connected
    if (currentlyConnected != connectionState) {
      connectionState = currentlyConnected;

      if (connectionState) {
        digitalWrite(CONNECT_LED1, HIGH); // indicate connection
        sendDiscover();
        //Serial.println("Connected to another rhizome.");

      } else {
        digitalWrite(CONNECT_LED1, LOW); // indicate disconnection
        discoveredCount = 0;
        pRhizome->setCount(0);
        pRhizome->setState(0); // set to idle state
        discoveryMode = false;
        //Serial.print("Disconnected. Number of connected rhizomes: ");
        // Serial.println(discoveredCount);

      }
    }
  }
  // if (!connectionState) {
  //   discoveryMode = false;
  //   discoveredCount = 0;
  //   numberOfConnectedRhizomes(0);
  //   connectedRhizomesCount = 0;
  //   setGeneratingState(false);
  //   setConnectionToRhizome(false);
  //   setConnectionToNode(false);
  //   StateLoop(0);
  //   return;
  // }
}



// call frequently from loop()
void lookForMessages() {
  oscSlip.onOscMessageReceived(oscMessageReceived);
  updateNumberOfConnectedRhizomes();
  //Serial.println(discoveredCount);
}
/*--------------------------------------------------*/

/*---------------Handle OSC Messages-------------------*/
void oscMessageReceived(MicroOscMessage &msg) {
  if (!pRhizome) return;


  // --- DISCOVERY TOKEN ---
  if (msg.checkOscAddress("/discover")) {
    int origin = msg.nextAsInt();
    discoveredCount = msg.nextAsInt();
    pRhizome->setCount(discoveredCount);
    pRhizome->setState(1); // set to connection state
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
        pRhizome->setCount(discoveredCount);
        pRhizome->setState(2); // set to generating state
        return;
    }

  } else if (msg.checkOscAddress("/node")) {
    pRhizome->setState(3); // set to giving to node state
    setNodeDrainRate(msg.nextAsFloat());
    oscSlip.sendMessage("/energy", "ii", pRhizome->getID(), pRhizome->getEnergy());
    Serial.print("Node connected. Drain rate set to: ");
    Serial.print(msg.nextAsFloat());
    Serial.print(". Current energy: ");
    Serial.println(pRhizome->getEnergy());

    return;

  } else if (msg.checkOscAddress("/discover_done")) {
    int total = msg.nextAsInt();
    pRhizome->setCount(total);
    pRhizome->setState(2); // set to generating state
    discoveryMode = false;
    return;

  } else {
    pRhizome->setState(0); // set to idle state
    pRhizome->setCount(0);
    return;
  }
}



