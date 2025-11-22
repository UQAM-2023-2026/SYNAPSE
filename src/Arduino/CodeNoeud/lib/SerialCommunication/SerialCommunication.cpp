#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h> // For communication over Serial with OSC messages


#define CONNECT_PIN 9 // pin used to detect connection
#define SERIAL_PORT Serial2
#define SERIAL_BAUD 9600

MicroOscSlip<32> oscSlip(&Serial2); // Using Serial2 for communication
void oscMessageReceived(MicroOscMessage &msg);

// stored pointer to the external node object
static NodeStateAndID *pNode = nullptr;

/*--------------------ISR---------------------------*/
// ISR-safe event flag (set by ISR), and persistent connection state
static volatile bool connectedEvent = false;
static bool connectionState = false;

// ISR: only set short flag
void connected() {
  connectedEvent = true;
}
/*--------------------------------------------------*/

void sendNodeMsg();

void beginSerialCommunication(NodeStateAndID &node) {
  pNode = &node;

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
        sendNodeMsg();

      } else {
        digitalWrite(13, LOW); // indicate disconnection
      }
    }
  }
}

void sendNodeMsg() {
  if (!pNode) return;
  float drainRate = pNode->getDrainRate();

  oscSlip.sendMessage("/node", "f", drainRate);
  // Serial.print("Sent ID: ");
  // Serial.print(discoveryOrigin);
  // Serial.print(" with count: ");
  // Serial.println(discoveredCount);
}

// call frequently from loop()
void lookForMessages() {
  oscSlip.onOscMessageReceived(oscMessageReceived);
  //Serial.println(discoveredCount);
}

void oscMessageReceived(MicroOscMessage &msg) {
  if (!pNode) return;

  // --- DISCOVERY TOKEN ---
  if (msg.checkOscAddress("/energy")) {
    int energy = msg.nextAsInt();
    Serial.println(energy);
    // Serial.print("Received discover message from ID: ");
    // Serial.print(origin);
    // Serial.print(" with count: ");
    // Serial.println(count);

    // Si nous ne sommes PAS l'origine → on s'ajoute au count
  } else {

  }
  
}


