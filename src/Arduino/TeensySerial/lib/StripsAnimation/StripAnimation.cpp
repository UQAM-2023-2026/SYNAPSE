/*-------------------Libraries----------------------*/
#include <Arduino.h>
#include <FastLED.h>
#include <RhizomeStateAndID.h>

/*------------------LED Configuration--------------------*/
#define LED_PIN     13
#define CLOCK_PIN   12
#define NUM_LEDS    15
#define BRIGHTNESS  100

/* LED strip configuration */
#define NUM_LEDS2    4
#define DATA_PIN2    1
#define LED_TYPE2    WS2812B
#define COLOR_ORDER2 GRB

CRGB leds[NUM_LEDS];
RhizomeStateAndID* pRhizome = nullptr;

/*------------------Setup--------------------*/

void SetupStrips(RhizomeStateAndID& rhizome, uint8_t brightness) {
  pRhizome = &rhizome;
  FastLED.addLeds<APA102, LED_PIN, CLOCK_PIN, BGR>(leds, NUM_LEDS);
  FastLED.setBrightness(brightness);
  FastLED.clear();
  FastLED.show();
}

/*------------------Variables--------------------*/
// idle animation variables
uint8_t idleBrightness = 0;
int8_t idleFadeAmount = 1;
float idleBrightnessScale = 0.05;      // 5% brightness
unsigned long idleLastUpdate = 0;
// SPEED CONTROL - lower number = faster animation
unsigned long idleSpeed = 5;          // Try: 5 (fast), 10 (medium), 20 (slow), 50 (very slow)

// generation_inc animation variables
uint8_t genIncBrightness = 0;
int8_t genIncFadeAmount = 1;
float genIncBrightnessScale = 0.05;    // Starts at 5%
float genIncDimScale = 0.05;           // 5% brightness
float genIncFullScale = 1.0;           // 100% brightness
bool genIncIsFullBrightness = false;   // Tracks which breath we're on
unsigned long genIncLastUpdate = 0;
unsigned long genIncSpeed = 5;        // Milliseconds between updates (lower = faster)

// generation_comp1 animation variables
int genComp1Position = 0; // Current position of the 3-LED group
bool genComp1Direction = true; // true = left to right, false = right to left
unsigned long genComp1LastMove = 0;
const unsigned long genComp1Speed = 50; // milliseconds between moves
unsigned long genComp1LastRestart = 0; // Track when animation last restarted
const unsigned long genComp1RestartDelay = 1000; // milliseconds before restart (2 seconds)
// Trail settings for generation_comp1
int genComp1TrailLength = 2;           // Number of fading LEDs behind
float genComp1TrailFade = 0.3;         // How much each trail LED dims

//generation_comp2 variables
int genComp2Position = 0; // Current position of the 3-LED group
bool genComp2Direction = true; // true = left to right, false = right to left
unsigned long genComp2LastMove = 0;
const unsigned long genComp2Speed = 60; // milliseconds between moves
unsigned long genComp2LastRestart = 0; // Track when animation last restarted
const unsigned long genComp2RestartDelay = 3000; // milliseconds before restart (2 seconds)
// Trail settings for generation_comp2
int genComp2TrailLength = 3;           // Number of fading LEDs behind
float genComp2TrailFade = 0.3;         // How much each trail LED dims

//generation_comp3 variables
int genComp3Position = 0; // Current position of the 3-LED group
bool genComp3Direction = true; // true = left to right, false = right to left
unsigned long genComp3LastMove = 0;
const unsigned long genComp3Speed = 20; // milliseconds between moves
unsigned long genComp3LastRestart = 0; // Track when animation last restarted
const unsigned long genComp3RestartDelay = 1000; // milliseconds before restart (2 seconds)
// Trail settings for generation_comp3
int genComp3TrailLength = 3;           // Number of fading LEDs behind
float genComp3TrailFade = 0.3;         // How much each trail LED dims

//generation_comp4 variables
int genComp4Position = 0; // Current position of the 3-LED group
bool genComp4Direction = true; // true = left to right, false = right to left
unsigned long genComp4LastMove = 0;
const unsigned long genComp4Speed = 100; // milliseconds between moves
unsigned long genComp4LastRestart = 0; // Track when animation last restarted
const unsigned long genComp4RestartDelay = 2000; // milliseconds before restart (2 seconds)
int genComp4NumLeds = 1; // number od orange LEDs (grows each restart)
unsigned long genComp4LastGrow = 0; // Track when we last added an LED
const unsigned long genComp4GrowDelay = 1000; // Add a new LED every 1 second
// Trail settings for generation_comp4
int genComp4TrailLength = 2;           // Number of fading LEDs behind
float genComp4TrailFade = 0.2;         // How much each trail LED dims

// Generation complete 5 animation variables
int genComp5Position = 0; // Current position of the LED group
bool genComp5Direction = true; // true = left to right, false = right to left
unsigned long genComp5LastMove = 0;
const unsigned long genComp5Speed = 50; // milliseconds between moves
unsigned long genComp5LastRestart = 0; // Track when animation last restarted
const unsigned long genComp5RestartDelay = 2000; // milliseconds before restart (2 seconds)
// Trail settings for generation_comp5
int genComp5TrailLength = 5;           // Number of fading LEDs behind
float genComp5TrailFade = 0.5;         // How much each trail LED dims
// Breathing animation variables (when energy is full)
uint8_t genComp5BreathPhase = 0;        // Phase for sine wave (0-255)
unsigned long genComp5BreathLastUpdate = 0;
const unsigned long genComp5BreathSpeed = 20;  // ms between updates

// Async breathing animation variables
uint8_t asyncPhase[NUM_LEDS]; // Individual phase for each LED
uint8_t asyncSpeed[NUM_LEDS]; // Individual speed for each LED
float asyncBrightnessScale = 0.10; // 10% brightness
float asyncGlobalSpeed = 0.4; // Global speed multiplier (1 = slow, 2 = medium, 3 = fast)

// generation_trail animation variables
int genTrailPosition = 0;
bool genTrailDirection = true;              // true = left to right, false = right to left
unsigned long genTrailLastMove = 0;
unsigned long genTrailSpeed = 50;           // Milliseconds between moves
unsigned long genTrailLastRestart = 0;
unsigned long genTrailRestartDelay = 1000;  // Milliseconds before restart

// Trail settings
int genTrailHeadSize = 3;                   // Number of full brightness LEDs at front
int genTrailFadeLength = 5;                 // Number of fading LEDs behind
float genTrailFadeAmount = 0.5;             // How much each trail LED dims (0.5 = 50% of previous)


/*------------------Animation Functions--------------------*/
void idle() {
  // Only update if enough time has passed
  if (millis() - idleLastUpdate < idleSpeed) {
    FastLED.show();
    return;
  }
  idleLastUpdate = millis();
  
  // Reverse direction BEFORE we hit the limits
  if (idleBrightness >= 254 && idleFadeAmount > 0) {
    idleFadeAmount = -idleFadeAmount;
  }
  if (idleBrightness <= 1 && idleFadeAmount < 0) {
    idleFadeAmount = -idleFadeAmount;
  }
  
  // Update brightness
  idleBrightness = idleBrightness + idleFadeAmount;
  
  // Set all LEDs to white with breathing brightness (scaled)
  uint8_t scaledBrightness = idleBrightness * idleBrightnessScale;
  fill_solid(leds, NUM_LEDS, CRGB(scaledBrightness, scaledBrightness, scaledBrightness));
  
  FastLED.show();
}

void generation_inc() {
  // Only update if enough time has passed
  if (millis() - genIncLastUpdate < genIncSpeed) {
    FastLED.show();
    return;
  }
  genIncLastUpdate = millis();
  
  // Reverse direction BEFORE we hit the limits
  if (genIncBrightness >= 254 && genIncFadeAmount > 0) {
    genIncFadeAmount = -genIncFadeAmount;
  }
  if (genIncBrightness <= 1 && genIncFadeAmount < 0) {
    genIncFadeAmount = -genIncFadeAmount;
    
    // We hit black - switch to the other brightness for next breath
    genIncIsFullBrightness = !genIncIsFullBrightness;
    
    if (genIncIsFullBrightness) {
      genIncBrightnessScale = genIncFullScale;   // Next breath = 100%
    } else {
      genIncBrightnessScale = genIncDimScale;    // Next breath = 5%
    }
  }
  
  // Update brightness
  genIncBrightness = genIncBrightness + genIncFadeAmount;
  
  // Set all LEDs to white with breathing brightness (scaled)
  uint8_t scaledBrightness = genIncBrightness * genIncBrightnessScale;
  fill_solid(leds, NUM_LEDS, CRGB(scaledBrightness, scaledBrightness, scaledBrightness));
  
  FastLED.show();
}

void generation_comp1() {
  // 3-LED animation moving across the strip with orange trail
  
  // Check if it's time to restart the animation
  if (millis() - genComp1LastRestart >= genComp1RestartDelay) {
    genComp1Position = 0;
    genComp1LastRestart = millis();
  }
  
  // Clear all LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Add the orange trail FIRST (so head draws on top)
  for (int i = 0; i < genComp1TrailLength; i++) {
    int trailIndex;
    
    if (genComp1Direction) {
      trailIndex = genComp1Position - 1 - i;  // Trail behind (left)
    } else {
      trailIndex = genComp1Position + 3 + i;  // Trail behind (right)
    }
    
    if (trailIndex >= 0 && trailIndex < NUM_LEDS) {
      float fade = pow(genComp1TrailFade, i + 1);
      uint8_t r = 255 * fade;
      uint8_t g = 35 * fade;
      leds[trailIndex] = CRGB(r, g, 0);
    }
  }
  
  // Draw the 3-LED group at current position
  if (genComp1Position >= 0 && genComp1Position < NUM_LEDS) {
    leds[genComp1Position] = CRGB(255, 35, 0);  // First LED - Orange
  }
  if (genComp1Position + 1 >= 0 && genComp1Position + 1 < NUM_LEDS) {
    leds[genComp1Position + 1] = CRGB::White;  // Second LED - White
  }
  if (genComp1Position + 2 >= 0 && genComp1Position + 2 < NUM_LEDS) {
    leds[genComp1Position + 2] = CRGB::White;  // Third LED - White
  }
  
  // Move the position based on timing
  if (millis() - genComp1LastMove >= genComp1Speed) {
    genComp1LastMove = millis();
    
    if (genComp1Direction) {
      genComp1Position++;
    } else {
      genComp1Position--;
    }
  }
  
  FastLED.show();
}

void generation_comp2() {
  // 3-LED animation moving across the strip with orange trail
  
  // Check if it's time to restart the animation
  if (millis() - genComp2LastRestart >= genComp2RestartDelay) {
    genComp2Position = 0;
    genComp2LastRestart = millis();
  }
  
  // Clear all LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Add the orange trail FIRST (so head draws on top)
  for (int i = 0; i < genComp2TrailLength; i++) {
    int trailIndex;
    
    if (genComp2Direction) {
      trailIndex = genComp2Position - 1 - i;  // Trail behind (left)
    } else {
      trailIndex = genComp2Position + 3 + i;  // Trail behind (right)
    }
    
    if (trailIndex >= 0 && trailIndex < NUM_LEDS) {
      float fade = pow(genComp2TrailFade, i + 1);
      uint8_t r = 255 * fade;
      uint8_t g = 35 * fade;
      leds[trailIndex] = CRGB(r, g, 0);
    }
  }
  
  // Draw the 3-LED group at current position
  if (genComp2Position >= 0 && genComp2Position < NUM_LEDS) {
    leds[genComp2Position] = CRGB(255, 35, 0);  // First LED - Orange
  }
  if (genComp2Position + 1 >= 0 && genComp2Position + 1 < NUM_LEDS) {
    leds[genComp2Position + 1] = CRGB(255, 35, 0);  // Second LED - Orange
  }
  if (genComp2Position + 2 >= 0 && genComp2Position + 2 < NUM_LEDS) {
    leds[genComp2Position + 2] = CRGB::White;  // Third LED - White
  }
  
  // Move the position based on timing
  if (millis() - genComp2LastMove >= genComp2Speed) {
    genComp2LastMove = millis();
    
    if (genComp2Direction) {
      genComp2Position++;
    } else {
      genComp2Position--;
    }
  }
  
  FastLED.show();
}

void generation_comp3() {
  // 3-LED animation moving across the strip with orange trail
  
  // Check if it's time to restart the animation
  if (millis() - genComp3LastRestart >= genComp3RestartDelay) {
    genComp3Position = 0;
    genComp3LastRestart = millis();
  }
  
  // Clear all LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Add the orange trail FIRST (so head draws on top)
  for (int i = 0; i < genComp3TrailLength; i++) {
    int trailIndex;
    
    if (genComp3Direction) {
      trailIndex = genComp3Position - 1 - i;  // Trail behind (left)
    } else {
      trailIndex = genComp3Position + 3 + i;  // Trail behind (right)
    }
    
    if (trailIndex >= 0 && trailIndex < NUM_LEDS) {
      float fade = pow(genComp3TrailFade, i + 1);
      uint8_t r = 255 * fade;
      uint8_t g = 35 * fade;
      leds[trailIndex] = CRGB(r, g, 0);
    }
  }
  
  // Draw the 3-LED group at current position
  for (int i = 0; i < 3; i++) {
    int ledIndex = genComp3Position + i;
    if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
      leds[ledIndex] = CRGB(255, 35, 0);
    }
  }
  
  // Move the position based on timing
  if (millis() - genComp3LastMove >= genComp3Speed) {
    genComp3LastMove = millis();
    
    if (genComp3Direction) {
      genComp3Position++;
    } else {
      genComp3Position--;
    }
  }
  
  FastLED.show();
}

void generation_comp4() {
  // 5-LED animation moving across the strip with orange trail
  
  // Check if it's time to restart the animation
  if (millis() - genComp4LastRestart >= genComp4RestartDelay) {
    genComp4Position = 0;
    genComp4LastRestart = millis();
  }
  
  // Clear all LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Add the orange trail FIRST (so head draws on top)
  for (int i = 0; i < genComp4TrailLength; i++) {
    int trailIndex;
    
    if (genComp4Direction) {
      trailIndex = genComp4Position - 1 - i;  // Trail behind (left)
    } else {
      trailIndex = genComp4Position + 5 + i;  // Trail behind (right)
    }
    
    if (trailIndex >= 0 && trailIndex < NUM_LEDS) {
      float fade = pow(genComp4TrailFade, i + 1);
      uint8_t r = 255 * fade;
      uint8_t g = 35 * fade;
      leds[trailIndex] = CRGB(r, g, 0);
    }
  }
  
  // Draw the 5-LED group at current position
  for (int i = 0; i < 5; i++) {
    int ledIndex = genComp4Position + i;
    if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
      leds[ledIndex] = CRGB(255, 35, 0);
    }
  }
  
  // Move the position based on timing
  if (millis() - genComp4LastMove >= genComp4Speed) {
    genComp4LastMove = millis();
    
    if (genComp4Direction) {
      genComp4Position++;
    } else {
      genComp4Position--;
    }
  }
  
  FastLED.show();
}

void addOrangeTrail(int headPosition, int headSize, bool direction) {
  // Adds a fading orange trail behind any LED group
  
  for (int i = 0; i < genComp5TrailLength; i++) {
    int trailIndex;
    
    if (direction) {
      trailIndex = headPosition - 1 - i;  // Trail behind (left)
    } else {
      trailIndex = headPosition + headSize + i;  // Trail behind (right)
    }
    
    if (trailIndex >= 0 && trailIndex < NUM_LEDS) {
      // Calculate faded brightness
      float fade = pow(genComp5TrailFade, i + 1);
      
      // Orange fading
      uint8_t r = 255 * fade;
      uint8_t g = 35 * fade;
      
      leds[trailIndex] = CRGB(r, g, 0);
    }
  }
}

void generation_comp5() {
  // Animation where trail length is proportional to rhizome energy
  // At 100% energy, strip is full and breathes
  
  // Calculate number of LEDs based on energy (0-100% maps to 1-15 LEDs)
  float energy = pRhizome->getEnergy();
  int numLeds = map(constrain((int)energy, 0, 100), 0, 100, 1, NUM_LEDS);
  
  // If energy is at 100%, do breathing animation
  if (energy >= 100.0f) {
    // Only update phase at specified interval (non-blocking)
    if (millis() - genComp5BreathLastUpdate >= genComp5BreathSpeed) {
      genComp5BreathLastUpdate = millis();
      genComp5BreathPhase += 1;  // Slow increment for smooth breathing
    }
    
    // Use sine wave for smooth breathing (range 50-255 for visible effect)
    uint8_t sineValue = sin8(genComp5BreathPhase);  // 0-255 sine wave
    uint8_t brightness = map(sineValue, 0, 255, 50, 255);  // Map to 50-255 range
    
    // Set all LEDs to orange with breathing brightness
    fill_solid(leds, NUM_LEDS, CRGB(brightness, brightness / 5, 0));
    
    FastLED.show();
    return;
  }
  
  // Check if it's time to restart the animation
  if (millis() - genComp5LastRestart >= genComp5RestartDelay) {
    genComp5Position = 0;
    genComp5LastRestart = millis();
  }
  
  // Clear all LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Add the orange trail FIRST (so head draws on top)
  addOrangeTrail(genComp5Position, numLeds, genComp5Direction);
  
  // Draw the LED group at current position (size based on energy)
  for (int i = 0; i < numLeds; i++) {
    int ledIndex = genComp5Position + i;
    if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
      leds[ledIndex] = CRGB(255, 35, 0);
    }
  }
  
  // Move the position based on timing
  if (millis() - genComp5LastMove >= genComp5Speed) {
    genComp5LastMove = millis();
    
    if (genComp5Direction) {
      genComp5Position++;
    } else {
      genComp5Position--;
    }
  }
  
  FastLED.show();
}

void async_breath() {
  // Asynchronous breathing - each LED breathes at its own speed and phase
  
  // Initialize arrays on first run if needed
  static bool initialized = false;
  if (!initialized) {
    for (int i = 0; i < NUM_LEDS; i++) {
      asyncPhase[i] = random8();
      asyncSpeed[i] = random8(3, 5); // Random speed between 3-6 (guaranteed minimum)
    }
    initialized = true;
  }
  
  for (int i = 0; i < NUM_LEDS; i++) {
    // Update phase for each LED at its own speed multiplied by global speed
    asyncPhase[i] += (asyncSpeed[i] * asyncGlobalSpeed);
    
    // Calculate brightness using sine wave (convert float phase to uint8_t)
    uint8_t sineValue = sin8((uint8_t)asyncPhase[i]);
    
    // Scale brightness to 10%
    uint8_t scaledBrightness = sineValue * asyncBrightnessScale;
    
    // Set LED to white
    leds[i] = CRGB(scaledBrightness, scaledBrightness, scaledBrightness);
  }
  
  FastLED.show();
}

void generation_trail() {
  // 3-LED animation with fading trail
  
  // Check if it's time to restart the animation
  if (millis() - genTrailLastRestart >= genTrailRestartDelay) {
    genTrailPosition = 0;
    genTrailLastRestart = millis();
  }
  
  // Clear all LEDs
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  
  // Draw the fading trail behind the head
  for (int i = 0; i < genTrailFadeLength; i++) {
    int trailIndex;
    
    if (genTrailDirection) {
      trailIndex = genTrailPosition - 1 - i;  // Trail behind (left)
    } else {
      trailIndex = genTrailPosition + genTrailHeadSize + i;  // Trail behind (right)
    }
    
    if (trailIndex >= 0 && trailIndex < NUM_LEDS) {
      // Calculate faded brightness for this trail position
      float fade = pow(genTrailFadeAmount, i + 1);  // Each LED is dimmer
      uint8_t r = 255 * fade;
      uint8_t g = 35 * fade;
      leds[trailIndex] = CRGB(r, g, 0);
    }
  }
  
  // Draw the 3 main LEDs at full brightness
  for (int i = 0; i < genTrailHeadSize; i++) {
    int ledIndex = genTrailPosition + i;
    if (ledIndex >= 0 && ledIndex < NUM_LEDS) {
      leds[ledIndex] = CRGB(255, 35, 0);  // Orange
    }
  }
  
  // Move the position based on timing
  if (millis() - genTrailLastMove >= genTrailSpeed) {
    genTrailLastMove = millis();
    
    if (genTrailDirection) {
      genTrailPosition++;
      // Stop at end, wait for restart
    } else {
      genTrailPosition--;
      // Stop at start, wait for restart
    }
  }
  
  FastLED.show();
}

/*------------------Main Loop--------------------*/
void StripLoop() {
  RhizomeState state = pRhizome->getState();
  
  if(state == IDLE) {
    generation_inc(); // Call the idle animation
  } else if(state == GENERATING) {
    generation_comp5(); // Call the generation animation
  } else if(state == GIVING_TO_NODE) {
    generation_trail(); // Call the animation for giving to node
  } else if(state == MIDDLEMAN) {
    fill_solid(leds, NUM_LEDS, CRGB(255, 35, 0)); // Solid orange for middleman
    FastLED.show();
  } else if(state == DEAD) {
    // Turn off all LEDs when dead
    FastLED.clear();
    FastLED.show();
  } else {
    generation_inc(); // Default to idle if state is unknown
  }
  //idle(); // Call the idle animation
  //generation_inc(); // Call the generation incomplete animation
  //generation_comp1(); // Call the generation complete 1 animation
  //generation_comp2(); // Call the generation complete 2 animation
  //generation_comp3(); // Call the generation complete 3 animation
  //generation_comp4(); // Call the generation complete 4 animation
  //generation_comp5(); // Call the generation complete 5 animation
  //async_breath(); // Call the async breathing animation
  //generation_trail(); // call the generation trail animation
}
