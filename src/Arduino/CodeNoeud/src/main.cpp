#include <Arduino.h>

#include <NodeStateAndID.h> // Custom Node state and ID management
#include <SerialCommunication.h> // Custom serial communication header

/*-----------Rhizome base stats----------------------*/
NodeStateAndID node(0); // Initialize Node with ID 1
/*---------------------------------------------------*/

void setup() {
  Serial.begin(9600);
  Serial.println("Setup complete.");

  beginSerialCommunication(node); // Initialize serial communication with node reference
}

void loop() {
  checkConnectionStatus();
  lookForMessages();
}

