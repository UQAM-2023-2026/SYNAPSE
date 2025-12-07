#include "SerialCommunication.h"
#include <Arduino.h>
#include <MicroOscSlip.h>
#include <NetworkOSC.h>

//flaaaag
#define CONNECT_PIN 3
#define SERIAL_PORT Serial2
#define SERIAL_BAUD 9600

#define NODE_DRAIN_RATE 5.0f  // Ã‰nergie/seconde Ã  drainer

MicroOscSlip<32> oscNode(&Serial2);

static NodeStateAndID *pNode = nullptr;

int rhizomeID = 14;          // Teensy's ID
int energyValue = 12;        // Teensy's energy level


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
            
            // Clear any stale data in serial buffer on new connection
            while(Serial2.available()) {
                Serial2.read();
            }
        } else {
            digitalWrite(13, LOW);
            Serial.println("Rhizome Disconnected!");
            rhizomeID = 0;
            energyValue = 0;
        }
    }
    
    // Process incoming OSC messages while connected
    if(connectionState) {
        oscNode.onOscMessageReceived(lookForMessages);
    }
}


// call frequently, placeholder for compatibility
void lookForMessages(MicroOscMessage &msg) {
  if (msg.checkOscAddress("/discover")) {
    int origin = msg.nextAsInt();
    int count = msg.nextAsInt();
    
    Serial.println("---");
    Serial.print("[RECV] /discover from rhizome ");
    Serial.print(origin);
    Serial.print(" (chain of ");
    Serial.print(count);
    Serial.println(" rhizomes)");
    
    // Respond with /node message
    // The rhizome chain will handle routing the drain to the last rhizome
    delay(50); // Small delay to ensure message is processed
    oscNode.sendMessage("/node", "f", NODE_DRAIN_RATE);
    
    Serial.print("[SEND] /node drainRate=");
    Serial.println(NODE_DRAIN_RATE);
    Serial.println("  â†’ Chain updated, last rhizome will drain");
    Serial.println("---");
    
  } else if (msg.checkOscAddress("/energy")) {
    int receivedId = msg.nextAsInt();
    float energy = msg.nextAsFloat();
    
    Serial.print("[ENERGY] Rhizome ");
    Serial.print(receivedId);
    Serial.print(" reporting energy: ");
    Serial.println(energy);

    rhizomeID = receivedId;
    energyValue = static_cast<int>(energy);

    if (energy <= 5) {
      Serial.println("  âš ï¸  WARNING: Rhizome nearly depleted!");
    }
  }
}

// returns 1.0 if connected, 0.0 if not
float getRhizomeValue() {
  return connectionState ? 1.0f : 0.0f;
}

void loopSendToTouch() {
  bool connected = (getRhizomeValue() > 0.0f);

  int idToSend = connected ? rhizomeID : 0;
  int energyToSend = connected ? energyValue : 0;

  sendOSC(idToSend, energyToSend);
}