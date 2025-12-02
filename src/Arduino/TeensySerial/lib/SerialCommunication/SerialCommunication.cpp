/*-------------------Libraries----------------------*/
#include "SerialCommunication.h"

#include <Arduino.h>
#include <MicroOscSlip.h> // For communication over Serial with OSC messages

#include <EnergyManagement.h>
#include <StripsAnimation.h>
#include <HapticFeedback.h>

#include <RhizomeStateAndID.h>
static RhizomeStateAndID *pRhizome = nullptr; // pointer to external rhizome object


// ==========================================================
// Gestion de la communication - Projet Synapse
// ==========================================================

/* Serial configuration */
#define SERIAL_BAUD 9600

//Left handle connection (receiving)
#define CONNECT_PIN1 6
#define SERIAL_PORT_RECEIVE Serial2

//Right handle connection (sending)
#define CONNECT_PIN2 20 // pin used to detect connection
#define SERIAL_PORT_SEND Serial3


/*------------------Global Variables-----------------*/
// Variables for osc
MicroOscSlip<32> oscSlipReceive(&SERIAL_PORT_RECEIVE); // Using Serial3 for communication (receiving)
MicroOscSlip<32> oscSlipSend(&SERIAL_PORT_SEND); // Using Serial2 for communication (sending)
/*--------------------------------------------------*/

void oscMessageReceived(MicroOscMessage &msg);

//Discovery process variables
static bool discoveryMode = false;
static int discoveryOrigin = -1;
static int discoveredCount = 0;

bool listening = false;
/*--------------------------------------------------------*/

/*--------------------ISR---------------------------*/
// ISR-safe event flag (set by ISR), and persistent connection state
static volatile bool connectedLeftEvent = false;
static bool connectionLeftState = false;

static volatile bool connectedRightEvent = false;
static bool connectionRightState = false;

// ISR: only set short flag
void connectedLeft() {
  connectedLeftEvent = true;
}
void connectedRight() {
  connectedRightEvent = true;
}
/*--------------------------------------------------*/
// update the number of connected rhizomes
void updateNumberOfConnectedRhizomes() {
  pRhizome->setCount(discoveredCount);
}


void SetupSerialCommunication(RhizomeStateAndID &rh) {
  pRhizome = &rh;

  Serial2.begin(SERIAL_BAUD); // Pins 7 (RX2) et 8 (TX2)
  Serial3.begin(SERIAL_BAUD); // Pins 15 (RX3) et 14 (TX3)

  pinMode(CONNECT_PIN1, INPUT_PULLUP); // grounded when connected
  pinMode(CONNECT_PIN2, INPUT_PULLUP); // grounded when connected
  
  attachInterrupt(digitalPinToInterrupt(CONNECT_PIN1), connectedLeft, CHANGE); // use CHANGE to detect connect/disconnect
  attachInterrupt(digitalPinToInterrupt(CONNECT_PIN2), connectedRight, CHANGE); // use CHANGE to detect connect/disconnect

  Serial.println("Serial Communication Initialized.");
}

/*--------------Handles first behaviors-------------------*/

void sendDiscover() {
  if (!pRhizome || !discoveryMode) return;
  discoveryOrigin = pRhizome->getID();
  // discoveredCount = 1;
  // pRhizome->setCount(discoveredCount); // reset count before discovery
  oscSlipSend.sendMessage("/discover", "ii", discoveryOrigin, 1); //Hi I'm origin and I'm alone
}

void lookForMessages() {
  if (!listening) return;
  oscSlipReceive.onOscMessageReceived(oscMessageReceived);
}

/*--------------------------------------------------*/
// Main loops to check connection status and handle messages
void checkConnectionStatus() {
  connectionLeftState = digitalRead(CONNECT_PIN1) == LOW; // LOW means connected
  connectionRightState = digitalRead(CONNECT_PIN2) == LOW; // LOW means connected

  if (connectedLeftEvent) {
    connectedLeftEvent = false; // clear the event flag
    listening = true;
    h_connection(0);
  }

  if (connectedRightEvent) {
    connectedRightEvent = false; // clear the event flag
    discoveryMode = true;
    h_connection(1);
  }

  if (connectionLeftState) {
    lookForMessages();
  } else {
      listening = false;
  }

  if (connectionRightState) {
      sendDiscover();
  } else {
      discoveryMode = false;
  }

  if (!connectionLeftState && !connectionRightState) {
    listening = false;
    discoveryMode = false;
    discoveredCount = 0;
    pRhizome->setCount(0);
    pRhizome->setState(0); // set to idle state
  }
  //Serial.println(listening);
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
    // Serial.print("Received discover message from ID: ");
    // Serial.print(origin);
    // Serial.print(" with count: ");
    // Serial.println(count);

    // Si nous ne sommes PAS l'origine → on s'ajoute au count
    if (origin != pRhizome->getID()) {
        discoveredCount += 1; // nous comptons
        pRhizome->setCount(discoveredCount);
        oscSlipSend.sendMessage("/discover", "ii", origin, discoveredCount);
        // Serial.print("Forwarded discover message. New count: ");
        // Serial.println(count);
    } else {
        // Le token vient de revenir à l'origine → fin de découverte
        oscSlipSend.sendMessage("/discover_done", "i", discoveredCount);
        discoveryMode = false;
        pRhizome->setCount(discoveredCount);
        pRhizome->setState(2); // set to generating state
        return;
    }

  } else if (msg.checkOscAddress("/node")) {
    pRhizome->setState(3); // set to giving to node state
    setNodeDrainRate(msg.nextAsFloat());
    oscSlipSend.sendMessage("/energy", "ii", pRhizome->getID(), pRhizome->getEnergy());
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



