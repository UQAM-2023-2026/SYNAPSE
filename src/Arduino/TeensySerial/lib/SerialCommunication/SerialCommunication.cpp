#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h> // For communication over Serial with OSC messages

#define CONNECT_PIN 9 // pin used to detect connection
#define SERIAL_PORT Serial2
#define SERIAL_BAUD 9600

MicroOscSlip<32> oscSlip(&Serial2); // Using Serial2 for communication
void oscMessageReceived(MicroOscMessage &msg);

// stored pointer to the external rhizome object
static RhizomeStateAndID *pRhizome = nullptr;

// ISR-safe event flag (set by ISR), and persistent connection state
static volatile bool connectedEvent = false;
static bool connectionState = false;

// ISR: only set short flag
void connected() {
  connectedEvent = true;
}

void beginSerialCommunication(RhizomeStateAndID &rh) {
  pRhizome = &rh;
  pinMode(9, INPUT_PULLUP); // grounded when connected
  pinMode(13, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(9), connected, CHANGE); // use CHANGE to detect connect/disconnect
  Serial2.begin(9600);

}

// call frequently from loop()
void lookForMessages() {

  oscSlip.onOscMessageReceived(oscMessageReceived);

  // handle ISR event: latch connectionState once based on pin level
  if (connectedEvent) {
    connectedEvent = false; // acknowledge the event
    // read pin to determine current connection (LOW = connected with INPUT_PULLUP)
    connectionState = (digitalRead(9) == LOW);
    digitalWrite(13, connectionState ? HIGH : LOW);
    // optional: do one-time actions on connect
    if (connectionState && pRhizome) {
      pRhizome->incrementCount(); // do short, safe action once on connect
    }
  }

  // while connected, send state periodically (throttle with millis)
  static unsigned long lastSend = 0;
  const unsigned long sendInterval = 200; // ms
  if (connectionState && (millis() - lastSend >= sendInterval)) {
    lastSend = millis();
    if (pRhizome) {
      oscSlip.sendMessage("/msg", "iif",
                          pRhizome->getID(),
                          pRhizome->getCount(),
                          pRhizome->getEnergy());
    }
  }
}

void oscMessageReceived(MicroOscMessage &msg) {
  if (!pRhizome) return;

  if (msg.checkOscAddress("/msg")) {
    int id = msg.nextAsInt();
    int count = msg.nextAsInt();
    float energy = msg.nextAsFloat();

    if (id != pRhizome->getID()) {
      pRhizome->incrementCount();
      oscSlip.sendMessage("/msg", "iif",
                          pRhizome->getID(),
                          pRhizome->getCount(),
                          pRhizome->getEnergy());
    } else {
      Serial.print("we are:");
      Serial.println(pRhizome->getCount());
    }
  }
}