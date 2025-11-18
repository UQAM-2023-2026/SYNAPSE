#include <Arduino.h> // Main Arduino library
#include <RhizomeStateAndID.h> // Custom Rhizome state and ID management
#include <EnergyManagement.h> // Custom energy management header
#include <SerialCommunication.h> // Custom serial communication header
#include <StripsAnimation.h> // Custom LED strips animation header


/*-----------Rhizome base stats----------------------*/
RhizomeStateAndID rhizome(0); // Initialize Rhizome with ID 0
/*---------------------------------------------------*/


void setup() {
  Serial.begin(9600);
  Serial.println("Setup complete.");

  SetupSerialCommunication(rhizome); // Initialize serial communication with rhizome reference
  SetupEnergyManagement(rhizome); // Initialize energy management with rhizome reference
  SetupStrips(50); // Setup LED strips with brightness 50

}

void loop() {
  energyLoop(); // Update energy management

  checkConnectionStatus(); // Check for connection changes
  lookForMessages(); // Check for incoming messages

  StripLoop(); // Update LED strips animation
  
}
