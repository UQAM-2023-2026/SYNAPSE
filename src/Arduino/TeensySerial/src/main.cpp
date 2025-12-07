#include <Arduino.h> // Main Arduino library
#include <RhizomeStateAndID.h> // Custom Rhizome state and ID management
#include <EnergyManagement.h> // Custom energy management header
#include <SerialCommunication.h> // Custom serial communication header
#include <StripsAnimation.h> // Custom LED strips animation header
#include <SoundsManagement.h> // Custom sounds management header
#include <HapticFeedback.h> // Custom haptic feedback header


/*-----------Rhizome base stats----------------------*/
RhizomeStateAndID rhizome(2); // Initialize Rhizome with ID 0
/*---------------------------------------------------*/


void setup() {
  Serial.begin(115200);
  Serial.println("Setup complete.");

  SetupEnergyManagement(rhizome); // Initialize energy management with rhizome reference
  SetupSerialCommunication(rhizome); // Initialize serial communication with rhizome reference
  //SetupStrips(rhizome,50); // Setup LED strips with brightness 50
  //SetupAudio(); // Initialize audio system

  //SetupHaptic(); // Initialize haptic feedback system

}

void loop() {
  energyLoop(); // Update energy management

  checkConnectionStatus(); // Check for connection changes
  //lookForMessages(); // Check for incoming messages

  //StripLoop(); // Update LED strips animation

  //AudioLoop(); // Update audio playback

  //HapticLoop(); // Update haptic feedback

  // static uint32_t lastPrint = 0;
  // if (millis() - lastPrint > 100) {  // print 20 times/sec max
  //   lastPrint = millis();
  //   Serial.print("Rhizome ID: ");
  //   Serial.print(rhizome.getID());
  //   Serial.print(", Count: ");
  //   Serial.print(rhizome.getCount());
  //   Serial.print(", Energy: ");
  //   Serial.print(rhizome.getEnergy());
  //   Serial.print(", State: ");
  //   Serial.println(rhizome.getState());
  // }


}

// Example: Print status when user sends a command via Serial
void serialEvent() {
  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd == 's' || cmd == 'S') {
      // Press 's' in Serial Monitor to print status
      printRhizomeStatus();
    }
  }
}
