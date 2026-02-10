/*==============================================================================
 * RhizomeStateMachine.cpp - State machine controller implementation
 *============================================================================*/

#include "RhizomeStateMachine.h"
#include <RhizomeData.h>
#include <PortManager.h>
#include <DiscoveryProtocol.h>
#include <NodeProtocol.h>
#include <FullEnergySync.h>

// Global instance
RhizomeStateMachine stateMachine;

RhizomeStateMachine::RhizomeStateMachine()
    : _state(RhizomeState::IDLE)
    , _data(nullptr)
    , _maleConnected(false)
    , _femaleConnected(false)
    , _nodeConnected(false)
    , _drainRate(0.0f)
    , _onStateChange(nullptr)
{}

void RhizomeStateMachine::begin(RhizomeData* data) {
    _data = data;
    _state = RhizomeState::IDLE;
    
    Serial.println("[StateMachine] Initialized in IDLE");
}

void RhizomeStateMachine::update() {
    // State machine doesn't poll - it reacts to events
    // This is here for future periodic checks if needed
    
    // Check for DEAD condition based on energy
    if (_data && _data->getEnergy() <= 0.0f && _state != RhizomeState::DEAD) {
        onEnergyDepleted();
    }
}

void RhizomeStateMachine::transitionTo(RhizomeState newState) {
    if (newState == _state) return;
    
    RhizomeState oldState = _state;
    _state = newState;
    
    Serial.print("[STATE] ");
    Serial.print(stateToString(oldState));
    Serial.print(" -> ");
    Serial.println(stateToString(newState));
    
    if (_onStateChange) {
        _onStateChange(oldState, newState);
    }
}

void RhizomeStateMachine::onLoopDetected(uint8_t count) {
    Serial.print("[SM] Loop detected with ");
    Serial.print(count);
    Serial.println(" rhizomes");
    
    if (_data) {
        _data->setCount(count);
    }
    
    // Can only enter GENERATING from DISCOVERING if BOTH ports connected
    // (a loop requires a complete circuit)
    if (_state == RhizomeState::DISCOVERING && _maleConnected && _femaleConnected) {
        transitionTo(RhizomeState::GENERATING);
    }
}

void RhizomeStateMachine::onLoopBroken() {
    Serial.println("[SM] Loop broken");
    
    // Exit GENERATING if loop is broken
    if (_state == RhizomeState::GENERATING) {
        transitionTo(RhizomeState::DISCOVERING);
        discoveryProtocol.reset();
        fullEnergySync.reset();
    }
}

void RhizomeStateMachine::onNodeConnected(float drainRate) {
    Serial.print("[SM] Node connected, drainRate=");
    Serial.println(drainRate);
    
    _nodeConnected = true;
    _drainRate = drainRate;
    
    // Transition based on current connections
    // MIDDLEMAN: MALE connected to node, FEMALE connected to rhizome
    // GIVING: MALE connected to node, FEMALE disconnected, NOT DEAD
    
    if (_femaleConnected) {
        // We have a rhizome behind us - become MIDDLEMAN (route their energy)
        // This works even if we're DEAD - we can still route!
        transitionTo(RhizomeState::MIDDLEMAN);
    } else if (_state != RhizomeState::DEAD) {
        // No rhizome behind, and we have energy - give our own
        transitionTo(RhizomeState::GIVING);
    } else {
        // DEAD with no one behind - we can't contribute anything
        Serial.println("[SM] DEAD with no upstream - staying DEAD");
    }
}

void RhizomeStateMachine::onMaleConnected() {
    _maleConnected = true;
    Serial.println("[SM] MALE connected");
    
    // If we were IDLE, start discovering
    if (_state == RhizomeState::IDLE) {
        transitionTo(RhizomeState::DISCOVERING);
        discoveryProtocol.reset();
    }
}

void RhizomeStateMachine::onMaleDisconnected() {
    _maleConnected = false;
    _nodeConnected = false;
    _drainRate = 0.0f;
    Serial.println("[SM] MALE disconnected");
    
    // Check if both ports disconnected (allow DEAD recovery)
    if (!_maleConnected && !_femaleConnected) {
        // If DEAD, stay DEAD but allow energy restoration
        if (_state == RhizomeState::DEAD) {
            Serial.println("[SM] DEAD and fully disconnected - waiting for energy");
            return;
        }
        transitionTo(RhizomeState::IDLE);
        discoveryProtocol.reset();
        nodeProtocol.reset();
        fullEnergySync.reset();
        return;
    }
    
    // If DEAD but still has FEMALE, stay DEAD (can't route without destination)
    if (_state == RhizomeState::DEAD) {
        Serial.println("[SM] DEAD - lost node, cannot route");
        return;
    }
    
    // GENERATING -> broken loop
    if (_state == RhizomeState::GENERATING) {
        transitionTo(RhizomeState::DISCOVERING);
        discoveryProtocol.reset();  // Clear _loopDetected flag
        fullEnergySync.reset();
        return;
    }
    
    // DISCOVERING -> lost the node in front of us, clear seenIds
    if (_state == RhizomeState::DISCOVERING) {
        discoveryProtocol.reset();
        return;
    }
    
    // GIVING/MIDDLEMAN -> lost node connection
    if (_state == RhizomeState::GIVING || _state == RhizomeState::MIDDLEMAN) {
        if (_femaleConnected) {
            transitionTo(RhizomeState::DISCOVERING);
        } else {
            transitionTo(RhizomeState::IDLE);
        }
        nodeProtocol.reset();
    }
}

void RhizomeStateMachine::onFemaleConnected() {
    _femaleConnected = true;
    Serial.println("[SM] FEMALE connected");
    
    // If we have a node connection (via MALE), we can become MIDDLEMAN
    // This works even if we're DEAD - we can still route!
    if (_nodeConnected) {
        transitionTo(RhizomeState::MIDDLEMAN);
        return;
    }
    
    // If DEAD without node, stay DEAD (wait for node or energy regen)
    if (_state == RhizomeState::DEAD) {
        Serial.println("[SM] DEAD - waiting for node signal to become MIDDLEMAN");
        return;
    }
    
    // If we were IDLE, start discovering
    if (_state == RhizomeState::IDLE) {
        transitionTo(RhizomeState::DISCOVERING);
        discoveryProtocol.reset();
    }
    
    // If GIVING and FEMALE connects -> become MIDDLEMAN
    if (_state == RhizomeState::GIVING && _nodeConnected) {
        transitionTo(RhizomeState::MIDDLEMAN);
    }
}

void RhizomeStateMachine::onFemaleDisconnected() {
    _femaleConnected = false;
    Serial.println("[SM] FEMALE disconnected");
    
    // Check if both ports disconnected (allow DEAD recovery)
    if (!_maleConnected && !_femaleConnected) {
        // If DEAD, stay DEAD but allow energy restoration
        if (_state == RhizomeState::DEAD) {
            Serial.println("[SM] DEAD and fully disconnected - waiting for energy");
            return;
        }
        transitionTo(RhizomeState::IDLE);
        discoveryProtocol.reset();
        nodeProtocol.reset();
        fullEnergySync.reset();
        return;
    }
    
    // MIDDLEMAN -> what happens when upstream disconnects?
    if (_state == RhizomeState::MIDDLEMAN) {
        // Lost upstream rhizome - can we give our own energy?
        if (_data && _data->getEnergy() > 0.0f) {
            transitionTo(RhizomeState::GIVING);
        } else {
            // We're empty too - go back to DEAD
            transitionTo(RhizomeState::DEAD);
        }
        return;
    }
    
    // DISCOVERING -> lost the node behind us, clear seenIds
    if (_state == RhizomeState::DISCOVERING) {
        discoveryProtocol.reset();
        return;
    }
    
    // GENERATING -> broken loop
    if (_state == RhizomeState::GENERATING) {
        transitionTo(RhizomeState::DISCOVERING);
        discoveryProtocol.reset();  // Clear _loopDetected flag
        fullEnergySync.reset();
    }
}

void RhizomeStateMachine::onEnergyDepleted() {
    Serial.println("[SM] Energy depleted");
    transitionTo(RhizomeState::DEAD);
}

void RhizomeStateMachine::onEnergyRestored() {
    // Only exit DEAD if energy > 0 AND both ports disconnected
    // This allows natural reset when picked up
    if (_state == RhizomeState::DEAD && !_maleConnected && !_femaleConnected) {
        Serial.println("[SM] Energy restored while disconnected");
        transitionTo(RhizomeState::IDLE);
    }
}

void RhizomeStateMachine::printState() const {
    Serial.print("State: ");
    Serial.print(stateToString(_state));
    Serial.print(" MALE:");
    Serial.print(_maleConnected ? "1" : "0");
    Serial.print(" FEMALE:");
    Serial.print(_femaleConnected ? "1" : "0");
    Serial.print(" Node:");
    Serial.println(_nodeConnected ? "1" : "0");
}
