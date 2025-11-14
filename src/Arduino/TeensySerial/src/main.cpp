#include <Arduino.h>
#include <MicroOscSlip.h> // For communication over Serial with OSC messages

#include <RhizomeStateAndID.h> // Custom Rhizome state and ID management
#include <EnergyManagement.h> // Custom energy management header
#include <SerialCommunication.h> // Custom serial communication header


/*-----------Rhizome base stats----------------------*/
RhizomeStateAndID rhizome(1); // Initialize Rhizome with ID 0
/*---------------------------------------------------*/





void setup() {
  Serial.begin(9600);
  Serial.println("Setup complete.");

  beginSerialCommunication(rhizome); // Initialize serial communication with rhizome reference
  beginEnergyManagement(rhizome); // Initialize energy management with rhizome reference

}

void loop() {
  checkConnectionStatus(); // Check for connection changes+
  lookForMessages(); // Check for incoming messages

  energyLoop(); // Update energy management
  
}
