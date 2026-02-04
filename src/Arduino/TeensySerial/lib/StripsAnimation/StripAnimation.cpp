/*-------------------Libraries----------------------*/
#include <Arduino.h>
#include <FastLED.h>
#include <RhizomeStateAndID.h>
#include <AnimationLayers.h>
#include <HeartbeatSystem.h>

/*-----------LED strip Male Side--------*/
#define MALE_LED_PIN 1
#define MALE_NUM_LEDS 5
CRGB maleLeds[MALE_NUM_LEDS];

/*------------LED strip Female Side--------*/
#define FEMALE_LED_PIN 0
#define FEMALE_NUM_LEDS 5
CRGB femaleLeds[FEMALE_NUM_LEDS];

/*------------------LED Tube Configuration--------------------*/
#define LED_PIN     12
#define CLOCK_PIN   13
#define NUM_LEDS    15
#define BRIGHTNESS  100

CRGB leds[NUM_LEDS];
RhizomeStateAndID* pRhizome = nullptr;

// Animation Manager instance
AnimationManager animManager;

// Callback pour haptic feedback (événements ponctuels comme pleine énergie)
static void (*hapticCallbackStrong)(void) = nullptr;

/*------------------Setup--------------------*/

void SetupStrips(RhizomeStateAndID& rhizome, uint8_t brightness) {
  pRhizome = &rhizome;
  FastLED.addLeds<APA102, LED_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  FastLED.clear();
  FastLED.show();
  
  // Initialize animation manager
  animManager.begin(leds, NUM_LEDS, brightness);
}

// Permet d'enregistrer un callback pour le haptic feedback (événements ponctuels)
void StripSetHapticCallback(void (*callback)(void)) {
  hapticCallbackStrong = callback;
}

// Note: Heartbeat callbacks are now managed centrally by HeartbeatSystem
// Use heartbeatSystem.setHapticCallback() instead of StripSetHeartbeatCallback()

/*------------------Main Loop--------------------*/

void StripLoop() {
  if (pRhizome == nullptr) return;
  
  // Update animation system with current state and energy
  animManager.update(
    pRhizome->getState(),
    pRhizome->getEnergy()
  );
}

/*------------------Event Triggers (call from external code)--------------------*/

void StripEventFullEnergy() {
  animManager.onFullEnergy();
  
  // Déclencher vibration haptique forte 2 fois
  if (hapticCallbackStrong) {
    hapticCallbackStrong();
    // Note: le 2ème appel devrait être géré avec un timer
    // ou via le système haptique directement
  }
}

void StripEventEmptyEnergy() {
  animManager.onEmptyEnergy();
}

void StripEventConnection() {
  animManager.onConnection();
}

void StripEventDisconnection() {
  animManager.onDisconnection();
}

/*------------------MiddleMan Sync--------------------*/

void StripSetFlowSync(uint8_t speed) {
  animManager.setFlowSync(speed);
}

void StripDisableFlowSync() {
  animManager.disableFlowSync();
}

/*------------------Debug--------------------*/

void StripPrintDebug() {
  animManager.printDebug(Serial);
}
