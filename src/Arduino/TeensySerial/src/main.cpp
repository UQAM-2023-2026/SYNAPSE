#include <Arduino.h>
#include <MicroOscSlip.h> // For communication over Serial with OSC messages

#include <RhizomeStateAndID.h> // Custom Rhizome state and ID management
#include <EnergyManagement.h> // Custom energy management header


/*-----------Rhizome base stats----------------------*/
RhizomeStateAndID rhizome(0); // Initialize Rhizome with ID 0
/*---------------------------------------------------*/





void setup() {
  beginEnergyManagement(rhizome); // Initialize energy management with rhizome reference

  pinMode(9, INPUT_PULLUP); // RX1
  pinMode(13, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(9), []() {
    digitalWrite(13, HIGH);
    oscSlip.sendMessage("/msg", "iif", RECEIVER, 1, 0.0f);
    delay(10);
    digitalWrite(13, LOW);
  }, FALLING);

  Serial.begin(9600);
  Serial2.begin(9600); // Pins 7 (RX1) et 8 (TX1)
}

void loop() {
  oscSlip.onOscMessageReceived(oscMessageReceived); 
}

void oscMessageReceived(MicroOscMessage &msg) {
  // This function is not used in this example
  
    if (msg.checkOscAddress("/msg")) {
      int id = msg.nextAsInt();
      int count = msg.nextAsInt();
      float energy = msg.nextAsFloat();

      Serial.print("Received OSC Message - ID: ");
      Serial.print(id);
      Serial.print(", Count: ");
      Serial.print(count);
      Serial.print(", Energy: ");
      Serial.println(energy);
      if (id != RECEIVER) {
        count ++;
        oscSlip.sendMessage("/msg", "iif", id, count, energy);
      } else {
        Serial.print("we are:");
        Serial.println(count);
      }
    }
}


