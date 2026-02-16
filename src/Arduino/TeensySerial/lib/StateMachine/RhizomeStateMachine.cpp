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
    // GENERATING is exempt - it will generate energy even from 0%
    if (_data && _data->getEnergy() <= 0.0f && 
        _state != RhizomeState::DEAD && 
        _state != RhizomeState::GENERATING) {
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
    
    // Can enter GENERATING from DISCOVERING, DEAD, or MIDDLEMAN if BOTH ports connected
    // MIDDLEMAN can detect a loop if the "node" it was connected to was actually 
    // another rhizome forwarding /node - in a closed loop, there's no real node
    if ((_state == RhizomeState::DISCOVERING || _state == RhizomeState::DEAD || _state == RhizomeState::MIDDLEMAN) 
        && _maleConnected && _femaleConnected) {
        // Clear node connection - we're in a closed loop, not connected to a real node
        _nodeConnected = false;
        _drainRate = 0.0f;
        nodeProtocol.reset();  // Also clear NodeProtocol's state
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
    // GENERATING: Ignore /node - we're in a loop, not connected to a real node
    // This prevents MIDDLEMAN feedback loops when rhizomes send /node to each other
    if (_state == RhizomeState::GENERATING) {
        return;  // Silently ignore - normal in loop mode
    }
    
    // DEAD with no one behind - can't contribute anything
    if (_state == RhizomeState::DEAD && !_femaleConnected) {
        return;  // Silently ignore
    }
    
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
    } else {
        // No rhizome behind, and we have energy - give our own
        transitionTo(RhizomeState::GIVING);
    }
}

void RhizomeStateMachine::onNodeLost() {
    Serial.println("[SM] Node connection lost (timeout)");
    
    _nodeConnected = false;
    _drainRate = 0.0f;
    
    // If we were GIVING or MIDDLEMAN, go back to DISCOVERING
    if (_state == RhizomeState::GIVING || _state == RhizomeState::MIDDLEMAN) {
        if (_maleConnected || _femaleConnected) {
            transitionTo(RhizomeState::DISCOVERING);
            discoveryProtocol.reset();
        } else {
            transitionTo(RhizomeState::IDLE);
        }
        nodeProtocol.reset();
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
