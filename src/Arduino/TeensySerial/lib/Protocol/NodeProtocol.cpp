/*==============================================================================
 * NodeProtocol.cpp - Node communication protocol implementation
 *============================================================================*/

#include "NodeProtocol.h"
#include "OscRouter.h"
#include <RhizomeData.h>

// Global instance
NodeProtocol nodeProtocol;

NodeProtocol::NodeProtocol()
    : _router(nullptr)
    , _data(nullptr)
    , _nodeConnected(false)
    , _drainRate(0.0f)
    , _lastEnergySent(0)
    , _lastNodeSentToFemale(0)
    , _lastNodeReceived(0)
    , _onNodeConnected(nullptr)
    , _onNodeLost(nullptr)
    , _onEnergyReceived(nullptr)
{}

void NodeProtocol::begin(OscRouter* router, RhizomeData* data) {
    _router = router;
    _data = data;
    reset();
    
    Serial.println("[NodeProtocol] Initialized");
}

void NodeProtocol::reset() {
    _nodeConnected = false;
    _drainRate = 0.0f;
    _lastEnergySent = 0;
    _lastNodeSentToFemale = 0;
    _lastNodeReceived = 0;
}

void NodeProtocol::update(RhizomeState state, bool maleConnected, bool femaleConnected) {
    if (!_router || !_data) return;
    
    unsigned long now = millis();
    
    // Check for node timeout - applies to GIVING and MIDDLEMAN states
    if ((state == RhizomeState::GIVING || state == RhizomeState::MIDDLEMAN) 
        && _nodeConnected && _lastNodeReceived > 0) {
        unsigned long elapsed = now - _lastNodeReceived;
        
        if (elapsed >= NODE_TIMEOUT_MS) {
            Serial.println("[NODE] Timeout - node connection lost");
            _nodeConnected = false;
            _drainRate = 0.0f;
            _lastNodeReceived = 0;  // Prevent retriggering
            if (_onNodeLost) {
                _onNodeLost();
            }
        }
    }
    
    // GIVING: Periodically send own energy to node
    if (state == RhizomeState::GIVING && _nodeConnected && maleConnected) {
        if (now - _lastEnergySent >= ENERGY_SEND_INTERVAL_MS) {
            sendEnergyToNode();
            _lastEnergySent = now;
        }
    }
    
    // MIDDLEMAN: Periodically send /node to FEMALE (only if we're receiving from upstream)
    if (state == RhizomeState::MIDDLEMAN && femaleConnected && _nodeConnected) {
        if (now - _lastNodeSentToFemale >= NODE_SEND_INTERVAL_MS) {
            sendNodeToFemale();
            _lastNodeSentToFemale = now;
        }
    }
}

void NodeProtocol::handleNodeMessage(MicroOscMessage& msg) {
    float drainRate = msg.nextAsFloat();
    
    Serial.print("[RECV MALE] /node drainRate=");
    Serial.println(drainRate);
    
    _lastNodeReceived = millis();  // Reset timeout
    _nodeConnected = true;
    _drainRate = drainRate;
    
    if (_onNodeConnected) {
        _onNodeConnected(drainRate);
    }
}

void NodeProtocol::handleDrainMessage(MicroOscMessage& msg) {
    float drainRate = msg.nextAsFloat();
    
    Serial.print("[RECV MALE] /drain rate=");
    Serial.println(drainRate);
    
    // /drain comes from a middleman rhizome in front of us
    // Treat it same as /node
    _nodeConnected = true;
    _drainRate = drainRate;
    
    if (_onNodeConnected) {
        _onNodeConnected(drainRate);
    }
}

void NodeProtocol::handleEnergyFromBehind(MicroOscMessage& msg) {
    int id = msg.nextAsInt();
    int energy = msg.nextAsInt();
    
    Serial.print("[RECV FEMALE] /energy ID=");
    Serial.print(id);
    Serial.print(" E=");
    Serial.println(energy);
    
    // Notify callback (for state machine to use)
    if (_onEnergyReceived) {
        _onEnergyReceived((uint8_t)id, (uint8_t)energy);
    }
    
    // Relay to node (MALE port) - MIDDLEMAN behavior
    relayEnergyToNode((uint8_t)id, (uint8_t)energy);
}

void NodeProtocol::sendEnergyToNode() {
    if (!_router || !_data) return;
    
    uint8_t id = _data->getId();
    int energy = (int)_data->getEnergy();
    
    _router->getMaleOsc().sendMessage("/energy", "ii", id, energy);
    
    Serial.print("[SEND MALE] /energy ID=");
    Serial.print(id);
    Serial.print(" E=");
    Serial.println(energy);
}

void NodeProtocol::sendNodeToFemale() {
    if (!_router) return;
    
    // Act as a node to the rhizome behind us
    _router->getFemaleOsc().sendMessage("/node", "f", _drainRate);
    
    Serial.print("[SEND FEMALE] /node drainRate=");
    Serial.println(_drainRate);
}

void NodeProtocol::relayEnergyToNode(uint8_t id, uint8_t energy) {
    if (!_router) return;
    
    _router->getMaleOsc().sendMessage("/energy", "ii", (int)id, (int)energy);
    
    Serial.print("[RELAY MALE] /energy ID=");
    Serial.print(id);
    Serial.print(" E=");
    Serial.println(energy);
}
