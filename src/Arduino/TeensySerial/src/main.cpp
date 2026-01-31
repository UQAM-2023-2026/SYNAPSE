  #include <Arduino.h>
  #include <FastLED.h>
  #include <RhizomeStateAndID.h>
  #include <EnergyManagement.h>
  #include <SerialCommunication.h>
  #include <StripsAnimation.h>
  #include <SoundsManagement.h>
  #include "HapticSystem.h"

  /*-----------Status LED Strip (WS2812B on pin 1)--------*/
  #define STATUS_LED_PIN 1
  #define STATUS_NUM_LEDS 5
  CRGB statusLeds[STATUS_NUM_LEDS];

  /*-----------Rhizome base stats----------------------*/
  RhizomeStateAndID rhizome(1);
  /*---------------------------------------------------*/

  void updateStatusLeds() {
    // Red when Serial3 (pins 14-15) not connected, Green when connected
    CRGB color = isRightConnected() ? CRGB::Green : CRGB::Red;
    for (int i = 0; i < STATUS_NUM_LEDS; i++) {
      statusLeds[i] = color;
    }
    FastLED.show();
  }

  void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("Setup starting...");

    // Setup status LED strip
    FastLED.addLeds<WS2812B, STATUS_LED_PIN, GRB>(statusLeds, STATUS_NUM_LEDS);
    FastLED.setBrightness(50);  // Not too bright
    for (int i = 0; i < STATUS_NUM_LEDS; i++) {
      statusLeds[i] = CRGB::Red;  // Start red (disconnected)
    }
    FastLED.show();
    Serial.println("Status LEDs OK");

    SetupEnergyManagement(rhizome);
    Serial.println("Energy OK");
    
    SetupSerialCommunication(rhizome);
    Serial.println("Serial OK");
    
    SetupStrips(rhizome, 255);
    Serial.println("Strips OK");

    // Initialize haptic system
    hapticSystem.begin();
    Serial.println("Haptic OK");

    // Register heartbeat callback - LEDs will call this on each pulse
    StripSetHeartbeatCallback(hapticStripCallbackWrapper);

    Serial.println("System initialized - LED-synced haptics");
  }

  void loop() {
    energyLoop();
    checkConnectionStatus();
    StripLoop();

    // Update status LEDs based on right serial connection
    updateStatusLeds();

    // Update haptic state (energy level for intensity mapping)
    hapticSystem.update(rhizome.getEnergy(), rhizome.getState());
  }