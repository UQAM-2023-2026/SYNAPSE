#include <Arduino.h>

//For Rhizome State & ID
#include <RhizomeStateAndID.h>

//For ir communication
#include <IRCommunication.h>

//For proximity detection
#include <ProximityDetection.h>

//For LEDS Strips
#include <StripsAnimations.h>
/*-------------------------------Objects----------------------------------------------*/

// Rhizome state object
RhizomeStateAndID rhizome(2, 1); // ID=0, Left side
/*-----------------------------Variables------------------------------------------------*/

// Proximity state
volatile bool inProximity = false;

void IRAM_ATTR onProximityChange() { // Interrupt handler for proximity sensor state change
  int state = readProximitySensor();
  inProximity = (state == HIGH);
}

//Timer for IR Sending
unsigned long lastIRSendTime = 0;
const unsigned long irInterval = 200; // Send IR data every 200 milliseconds
/*------------------------------code-----------------------------------------------*/

void setup() {
  Serial.begin(115200);
  SetupIR();
  SetupProximitySensor();

  attachInterrupt(digitalPinToInterrupt(5), onProximityChange, CHANGE);

  rhizome.setState(2);
  rhizome.setEnergy(50);
}

void loop() {

  // If in proximity, send IR data every 200 milliseconds
  if (inProximity) {
    unsigned long currentTime = millis();
    if (currentTime - lastIRSendTime >= irInterval) {
      lastIRSendTime = currentTime;
      send_ir_from_rhizome(rhizome);
      receive_ir_data();
    }
  } else {
    rhizome.setState(0); // Set state to off when not in proximity
  }

}

