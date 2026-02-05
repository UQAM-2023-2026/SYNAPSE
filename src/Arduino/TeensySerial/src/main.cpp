/*==============================================================================
 * main.cpp - Rhizome Main Entry Point
 * 
 * Wires together all modules:
 * - Core: RhizomeData, RhizomeState
 * - Connection: PortManager
 * - Protocol: OscRouter, DiscoveryProtocol, NodeProtocol, FullEnergySync
 * - StateMachine: RhizomeStateMachine
 * - Energy: EnergyManager
 * - Feedback: HapticFeedback, LedFeedback
 * - Heartbeat: HeartbeatSystem
 *============================================================================*/

#include <Arduino.h>
#include <FastLED.h>

// Core
#include <RhizomeState.h>
#include <RhizomeData.h>
#include <HardwarePins.h>

// Connection
#include <PortManager.h>

// Protocol
#include <OscRouter.h>
#include <DiscoveryProtocol.h>
#include <NodeProtocol.h>
#include <FullEnergySync.h>

// State Machine
#include <RhizomeStateMachine.h>

// Energy
#include <EnergyManager.h>

// Feedback Systems (new - uses RhizomeState directly)
#include <HeartbeatSystem.h>
#include <HapticFeedback.h>
#include <LedFeedback.h>

/*------------------------------------------------------------------------------
 * Configuration
 * 
 * CRITICAL: Each Rhizome MUST have a UNIQUE ID!
 * If two Rhizomes share the same ID, the discovery protocol will detect
 * a false loop when they're connected in a chain.
 * 
 * Change this value before flashing each Rhizome:
 *   - Rhizome 1: RHIZOME_ID = 1
 *   - Rhizome 2: RHIZOME_ID = 2
 *   - etc.
 *----------------------------------------------------------------------------*/
constexpr uint8_t RHIZOME_ID = 12;  // TODO: Read from EEPROM or set via jumpers

/*------------------------------------------------------------------------------
 * Global Instances
 *----------------------------------------------------------------------------*/
// Core data
RhizomeData rhizomeData(RHIZOME_ID);

/*------------------------------------------------------------------------------
 * Forward Declarations
 *----------------------------------------------------------------------------*/
// (None needed - all in callbacks)

/*------------------------------------------------------------------------------
 * Callback Declarations
 *----------------------------------------------------------------------------*/
// Port events -> State machine
void onMaleConnectCallback();
void onMaleDisconnectCallback();
void onFemaleConnectCallback();
void onFemaleDisconnectCallback();

// Discovery -> State machine
void onLoopDetectedCallback(uint8_t count);

// Node protocol -> State machine & Energy
void onNodeConnectedCallback(float drainRate);

// Full energy sync -> Feedback
void onAllFullCallback();

// Energy -> State machine
void onEnergyDepletedCallback();

// State change -> Feedback systems
void onStateChangeCallback(RhizomeState oldState, RhizomeState newState);

// Energy restored -> State machine
void onEnergyRestoredCallback();

// Heartbeat -> Feedback systems (declared in their headers)
// hapticHeartbeatCallback() - from HapticFeedback.h
// ledHeartbeatCallback() - from LedFeedback.h

/*------------------------------------------------------------------------------
 * Setup
 *----------------------------------------------------------------------------*/
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("=== RHIZOME SYSTEM STARTING ===");
    Serial.print("ID: ");
    Serial.println(RHIZOME_ID);
    
    // Initialize Core
    rhizomeData.setEnergy(10.0f);  // Start with some energy
    Serial.println("[Core] Initialized");
    
    // Initialize PortManager
    portManager.begin();
    portManager.onMaleConnect(onMaleConnectCallback);
    portManager.onMaleDisconnect(onMaleDisconnectCallback);
    portManager.onFemaleConnect(onFemaleConnectCallback);
    portManager.onFemaleDisconnect(onFemaleDisconnectCallback);
    Serial.println("[PortManager] Initialized");
    
    // Initialize OscRouter
    oscRouter.begin();
    oscRouter.setDiscoveryHandler(&discoveryProtocol);
    oscRouter.setNodeHandler(&nodeProtocol);
    oscRouter.setFullEnergyHandler(&fullEnergySync);
    Serial.println("[OscRouter] Initialized");
    
    // Initialize Protocols
    discoveryProtocol.begin(&oscRouter, RHIZOME_ID);
    discoveryProtocol.onLoopDetected(onLoopDetectedCallback);
    
    nodeProtocol.begin(&oscRouter, &rhizomeData);
    nodeProtocol.onNodeConnected(onNodeConnectedCallback);
    
    fullEnergySync.begin(&oscRouter, &rhizomeData);
    fullEnergySync.onAllFull(onAllFullCallback);
    Serial.println("[Protocols] Initialized");
    
    // Initialize State Machine
    stateMachine.begin(&rhizomeData);
    stateMachine.onStateChange(onStateChangeCallback);
    Serial.println("[StateMachine] Initialized");
    
    // Initialize Energy Manager
    energyManager.begin(&rhizomeData, &stateMachine);
    energyManager.onEnergyDepleted(onEnergyDepletedCallback);
    energyManager.onEnergyRestored(onEnergyRestoredCallback);
    Serial.println("[EnergyManager] Initialized");
    
    // Initialize Feedback Systems
    ledFeedback.begin();
    Serial.println("[LedFeedback] Initialized");
    
    hapticFeedback.begin();
    Serial.println("[HapticFeedback] Initialized");
    
    heartbeatSystem.begin();
    heartbeatSystem.setHapticCallback(hapticHeartbeatCallback);
    heartbeatSystem.setLedCallback(ledHeartbeatCallback);
    Serial.println("[Heartbeat] Initialized");
    
    Serial.println("=== RHIZOME SYSTEM READY ===");
    Serial.println("State: IDLE");
}

/*------------------------------------------------------------------------------
 * Main Loop
 *----------------------------------------------------------------------------*/
void loop() {
    // 1. Update connection detection
    portManager.update();
    
    // 2. Process OSC messages
    oscRouter.update();
    
    // 3. Update protocols based on state
    RhizomeState state = stateMachine.getState();
    bool maleConn = portManager.isMaleConnected();
    bool femaleConn = portManager.isFemaleConnected();
    
    if (state == RhizomeState::DISCOVERING) {
        discoveryProtocol.update(maleConn, femaleConn);
    }
    
    nodeProtocol.update(state, maleConn, femaleConn);
    
    if (state == RhizomeState::GENERATING) {
        fullEnergySync.update(true, maleConn);
    }
    
    // 4. Update state machine
    stateMachine.update();
    
    // 5. Update energy (needs connection state for DEAD recovery)
    energyManager.update(maleConn, femaleConn);
    
    // 6. Update feedback systems
    heartbeatSystem.update(rhizomeData.getEnergy());
    heartbeatSystem.setEnabled(state != RhizomeState::DEAD);
    
    // Update haptic and LED feedback with current state
    hapticFeedback.update(state, rhizomeData.getEnergy(), maleConn, femaleConn);
    ledFeedback.update(state, rhizomeData.getEnergy(), maleConn, femaleConn);
}

/*------------------------------------------------------------------------------
 * Port Event Callbacks
 *----------------------------------------------------------------------------*/
void onMaleConnectCallback() {
    stateMachine.onMaleConnected();
}

void onMaleDisconnectCallback() {
    stateMachine.onMaleDisconnected();
}

void onFemaleConnectCallback() {
    stateMachine.onFemaleConnected();
}

void onFemaleDisconnectCallback() {
    stateMachine.onFemaleDisconnected();
}

/*------------------------------------------------------------------------------
 * Protocol Callbacks
 *----------------------------------------------------------------------------*/
void onLoopDetectedCallback(uint8_t count) {
    stateMachine.onLoopDetected(count);
}

void onNodeConnectedCallback(float drainRate) {
    // Only transition to GIVING if FEMALE is disconnected
    if (!portManager.isFemaleConnected()) {
        stateMachine.onNodeConnected(drainRate);
        energyManager.setDrainRate(drainRate);
    } else {
        // FEMALE connected = we become MIDDLEMAN
        stateMachine.onNodeConnected(drainRate);
        // In MIDDLEMAN, we don't drain our own energy
        energyManager.setDrainRate(0.0f);
    }
}

void onAllFullCallback() {
    Serial.println("[EVENT] All rhizomes at 100% - CELEBRATION!");
    hapticFeedback.onAllRhizomesFull();
}

/*------------------------------------------------------------------------------
 * Energy Callbacks
 *----------------------------------------------------------------------------*/
void onEnergyDepletedCallback() {
    stateMachine.onEnergyDepleted();
}

void onEnergyRestoredCallback() {
    Serial.println("[EVENT] Energy restored - returning to IDLE");
    stateMachine.onEnergyRestored();
}

/*------------------------------------------------------------------------------
 * State Change Callback
 *----------------------------------------------------------------------------*/
void onStateChangeCallback(RhizomeState oldState, RhizomeState newState) {
    // Notify feedback systems of state change
    hapticFeedback.onStateChange(oldState, newState);
    ledFeedback.onStateChange(oldState, newState);
    
    // Handle protocol resets
    if (newState == RhizomeState::GENERATING && oldState != RhizomeState::GENERATING) {
        fullEnergySync.reset();  // Ready for new full-energy cycle
    }
    
    if (newState == RhizomeState::MIDDLEMAN) {
        // Entered MIDDLEMAN - ensure we don't drain
        energyManager.setDrainRate(0.0f);
    }
    
    if (newState == RhizomeState::GIVING && oldState == RhizomeState::MIDDLEMAN) {
        // MIDDLEMAN -> GIVING - start draining with stored rate
        energyManager.setDrainRate(nodeProtocol.getDrainRate());
    }
    
    if (newState == RhizomeState::IDLE) {
        // Reset all protocols
        discoveryProtocol.reset();
        nodeProtocol.reset();
        fullEnergySync.reset();
    }
}
