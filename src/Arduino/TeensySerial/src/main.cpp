  #include <Arduino.h>
  #include <FastLED.h>
  #include <RhizomeStateAndID.h>
  #include <EnergyManagement.h>
  #include <SerialCommunication.h>
  #include <StripsAnimation.h>
  #include <HapticSystem.h>
  #include <HeartbeatSystem.h>

  // /*-----------Status LED Strip (WS2812B on pin 1)--------*/
  // #define STATUS_LED_PIN 1
  // #define STATUS_NUM_LEDS 5
  // CRGB statusLeds[STATUS_NUM_LEDS];

  /*-----------Rhizome base stats----------------------*/
  RhizomeStateAndID rhizome(1);
  /*---------------------------------------------------*/

  // void updateStatusLeds() {
  //   // Red when Serial3 (pins 14-15) not connected, Green when connected
  //   CRGB color = isRightConnected() ? CRGB::Green : CRGB::Red;
  //   for (int i = 0; i < STATUS_NUM_LEDS; i++) {
  //     statusLeds[i] = color;
  //   }
  //   FastLED.show();
  // }

  void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("Setup starting...");

    SetupEnergyManagement(rhizome);
    Serial.println("Energy OK");
    
    SetupSerialCommunication(rhizome);
    Serial.println("Serial OK");
    
    SetupStrips(rhizome, 255);
    Serial.println("Strips OK");

    // Initialize haptic system
    hapticSystem.begin();
    Serial.println("Haptic OK");

    // Initialize heartbeat system (centralizes LED strip pulse, haptics, and indicator LED)
    heartbeatSystem.begin();
    heartbeatSystem.setHapticCallback(hapticHeartbeatCallback);
    Serial.println("Heartbeat OK");
    
    Serial.println("System initialized - All heartbeats synchronized");
  }

  void loop() {
    energyLoop();
    checkConnectionStatus();
    StripLoop();

    // Update heartbeat timing (drives LED pulse, haptics, and indicator LED)
    heartbeatSystem.update(rhizome.getEnergy());
    heartbeatSystem.setEnabled(rhizome.getState() != 4);  // Disable when dead
    
    // Update haptic system with current state and connection info
    hapticSystem.update(
      rhizome.getEnergy(),
      rhizome.getState(),
      isLeftConnected(),
      isRightConnected()
    );
  }