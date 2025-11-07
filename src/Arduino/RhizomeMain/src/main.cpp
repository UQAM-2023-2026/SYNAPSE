#include <Arduino.h>

//For Rhizome State & ID
#include <RhizomeStateAndID.h>

//For ir communication
#include <IRCommunication.h>

//For proximity detection
#include <ProximityDetection.h>

//For LEDS Strips
#include <StripsAnimation.h>
void stripsAnimationTask(void *pvParameters); //Task for strips animation

//Communication with Teensy
#include <TeensyCommunication.h>
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
  SetupTeensyCommunication();
  Serial.begin(115200);
  SetupIR();
  SetupProximitySensor();

  attachInterrupt(digitalPinToInterrupt(5), onProximityChange, CHANGE);

  rhizome.setState(2);
  rhizome.setEnergy(50);

  xTaskCreatePinnedToCore(
    stripsAnimationTask,          // Task function
    "Strips Animation Task",      // Name of the task
    8192,                          // Stack size
    NULL,                         // Task input parameter
    1,                            // Priority of the task
    NULL,                         // Task handle
    1                             // Run the task on core 1
  );
}

void stripsAnimationTask(void *pvParameters) {
  //Setup start here
  SetupStrips(255);
  //End of setup

  // Task loop
  for (;;) {
    StripLoop(inProximity);
    vTaskDelay(10 / portTICK_PERIOD_MS); // Small delay to prevent watchdog timer reset
  }
}

void loop() {

  // If in proximity, send IR data every 200 milliseconds
  if (inProximity) {
    unsigned long currentTime = millis();
    if (currentTime - lastIRSendTime >= irInterval) {
      lastIRSendTime = currentTime;
      send_ir_from_rhizome(rhizome);
      receive_ir_data();
      SendBang();
    }
  } else {
    rhizome.setState(0); // Set state to off when not in proximity
    SendStop();
  }

}

