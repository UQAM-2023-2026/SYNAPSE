/*
#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h> // For communication over Serial with OSC messages


#define CONNECT_PIN 3 // pin used to detect connection
#define SERIAL_PORT Serial2
#define SERIAL_BAUD 9600

MicroOscSlip<32> oscSlip(&Serial2); // Using Serial2 for communication
void oscMessageReceived(MicroOscMessage &msg);

// stored pointer to the external node object
static NodeStateAndID *pNode = nullptr;

/*--------------------ISR---------------------------*/
// ISR-safe event flag (set by ISR), and persistent connection state
/*
static volatile bool connectedEvent = false;
static bool connectionState = false;

// ISR: only set short flag
void connected() {
  connectedEvent = true;
}
/*--------------------------------------------------*/
/*
void sendNodeMsg();

void beginSerialCommunication(NodeStateAndID &node) {
  pNode = &node;
  Serial2.begin(9600, SERIAL_8N1, 4, 5); // Pins 4 (TX2) et 5 (RX2)
  pinMode(3, INPUT_PULLUP); // grounded when connected
  pinMode(13, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(CONNECT_PIN), connected, CHANGE); // use CHANGE to detect connect/disconnect
  
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
        sendNodeMsg();
        Serial.println("Connected.");

      } else {
        digitalWrite(13, LOW); // indicate disconnection
      }
    }
  }
}

void sendNodeMsg() {
  if (!pNode) return;
  float drainRate = pNode->getDrainRate();
  Serial.println("Sending node message...");
  oscSlip.sendMessage("/node", "f", drainRate);
//   Serial.print("Sent ID: ");
//  Serial.print(discoveryOrigin);
//   Serial.print(" with count: ");
//  Serial.println(discoveredCount);
}

// call frequently from loop()
void lookForMessages() {
  oscSlip.onOscMessageReceived(oscMessageReceived);
///  Serial.println(discoveredCount);
}

void oscMessageReceived(MicroOscMessage &msg) {
  if (!pNode) return;

  // --- DISCOVERY TOKEN ---
  if (msg.checkOscAddress("/energy")) {
    Serial.println("received");
    int id = msg.nextAsInt();
    int energy = msg.nextAsInt();
    Serial.print("energy: ");
    Serial.println(energy);
    // Serial.print("Received discover message from ID: ");
    // Serial.print(origin);
    // Serial.print(" with count: ");
    // Serial.println(count);

    // Si nous ne sommes PAS l'origine → on s'ajoute au count
  } else {

  }
  
}

*/
#include "SerialCommunication.h"
#include <Arduino.h>

#define CONNECT_PIN 3
#define SERIAL_PORT Serial2
#define SERIAL_BAUD 9600

static NodeStateAndID *pNode = nullptr;

/*--------------------ISR FLAGS---------------------*/
static volatile bool connectedEvent = false;
static bool connectionState = false;

void connected() {
  connectedEvent = true;
}
/*--------------------------------------------------*/

void beginSerialCommunication(NodeStateAndID &node) {
  pNode = &node;

  Serial2.begin(SERIAL_BAUD, SERIAL_8N1, 32, 33);
  pinMode(CONNECT_PIN, INPUT_PULLUP);
  pinMode(13, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(CONNECT_PIN), connected, CHANGE);

  Serial.println("Serial Communication Initialized (connection-only).");
}

void checkConnectionStatus() {
    bool currentlyConnected = (digitalRead(CONNECT_PIN) == LOW);

    if(currentlyConnected != connectionState) {
        connectionState = currentlyConnected;

        if(connectionState) {
            digitalWrite(13, HIGH);
            Serial.println("Rhizome Connected!");
        } else {
            digitalWrite(13, LOW);
            Serial.println("Rhizome Disconnected!");
        }
    }
}


// call frequently, placeholder for compatibility
void lookForMessages() {}

// returns 1.0 if connected, 0.0 if not
float getRhizomeValue() {
  return connectionState ? 1.0f : 0.0f;
}


