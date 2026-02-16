/*==============================================================================
 * RhizomeStateMachine.h - State machine controller
 * 
 * Single authority for rhizome state transitions.
 * All state changes go through this class.
 *============================================================================*/

#ifndef RHIZOME_STATE_MACHINE_H
#define RHIZOME_STATE_MACHINE_H

#include <Arduino.h>
#include <RhizomeState.h>

// Forward declarations
class RhizomeData;
class PortManager;
class DiscoveryProtocol;
class NodeProtocol;
class FullEnergySync;

// State change callback
using StateChangeCallback = void (*)(RhizomeState oldState, RhizomeState newState);

class RhizomeStateMachine {
public:
    RhizomeStateMachine();
    
    // Initialize with dependencies
    void begin(RhizomeData* data);
    
    // Call in loop() - evaluates transitions
    void update();
    
    // Get current state
    RhizomeState getState() const { return _state; }
    
    // State change callback
    void onStateChange(StateChangeCallback cb) { _onStateChange = cb; }
    
    // Event handlers (called by protocol handlers)
    void onLoopDetected(uint8_t count);
    void onLoopBroken();
    void onNodeConnected(float drainRate);
    void onNodeLost();
    void onMaleConnected();
    void onMaleDisconnected();
    void onFemaleConnected();
    void onFemaleDisconnected();
    void onEnergyDepleted();
    void onEnergyRestored();
    
    // Debug
    void printState() const;
    
private:
    RhizomeState _state;
    RhizomeData* _data;
    
    // Cached connection state (updated via events)
    bool _maleConnected;
    bool _femaleConnected;
    
    // Node connection tracking
    bool _nodeConnected;
    float _drainRate;
    
    // Callback
    StateChangeCallback _onStateChange;
    
    // State transition (only way to change state)
    void transitionTo(RhizomeState newState);
    
    // Transition validation
    bool canTransitionTo(RhizomeState newState) const;
};

// Global instance
extern RhizomeStateMachine stateMachine;

#endif // RHIZOME_STATE_MACHINE_H
