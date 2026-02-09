/*==============================================================================
 * FullEnergySync.cpp - Synchronized full energy celebration implementation
 *============================================================================*/

#include "FullEnergySync.h"
#include "OscRouter.h"
#include <RhizomeData.h>

// Global instance
FullEnergySync fullEnergySync;

FullEnergySync::FullEnergySync()
    : _router(nullptr)
    , _data(nullptr)
    , _sentOwnToken(false)
    , _holdingToken(false)
    , _heldTokenId(0)
    , _triggered(false)
    , _onAllFull(nullptr)
{}

void FullEnergySync::begin(OscRouter* router, RhizomeData* data) {
    _router = router;
    _data = data;
    reset();
    
    Serial.println("[FullEnergySync] Initialized");
}

void FullEnergySync::reset() {
    _sentOwnToken = false;
    _holdingToken = false;
    _heldTokenId = 0;
    _triggered = false;
}

void FullEnergySync::update(bool inGeneratingState, bool maleConnected) {
    if (!_router || !_data || !inGeneratingState) return;
    if (_triggered) return;  // Already celebrated
    
    float energy = _data->getEnergy();
    bool isFull = (energy >= 100.0f);
    
    // If we just reached 100% and haven't sent our token yet
    if (isFull && !_sentOwnToken && maleConnected) {
        sendFullToken(_data->getId());
        _sentOwnToken = true;
    }
    
    // If we're holding a token and now at 100%, forward it
    if (_holdingToken && isFull && maleConnected) {
        sendFullToken(_heldTokenId);
        _holdingToken = false;
        Serial.print("[FULL] Forwarding held token ID=");
        Serial.println(_heldTokenId);
    }
}

void FullEnergySync::handleFullMessage(MicroOscMessage& msg, bool fromMale) {
    if (_triggered) return;  // Already celebrated
    
    int tokenId = msg.nextAsInt();
    
    Serial.print("[RECV ");
    Serial.print(fromMale ? "MALE" : "FEMALE");
    Serial.print("] /full ID=");
    Serial.println(tokenId);
    
    // Check if this is our own token coming back
    if ((uint8_t)tokenId == _data->getId()) {
        // Loop complete! All rhizomes are at 100%
        _triggered = true;
        Serial.println("[FULL] Own token received back - ALL RHIZOMES FULL!");
        
        if (_onAllFull) {
            _onAllFull();
        }
        
        // Forward one more time so others also trigger
        if (_router) {
            sendFullToken((uint8_t)tokenId);
        }
        return;
    }
    
    // Not our token - check if we should forward or hold
    float energy = _data->getEnergy();
    
    if (energy >= 100.0f) {
        // We're full, forward immediately
        sendFullToken((uint8_t)tokenId);
    } else {
        // We're not full yet, hold the token
        _holdingToken = true;
        _heldTokenId = (uint8_t)tokenId;
        Serial.print("[FULL] Holding token ID=");
        Serial.print(tokenId);
        Serial.print(" (our energy: ");
        Serial.print(energy);
        Serial.println("%)");
    }
}

void FullEnergySync::sendFullToken(uint8_t id) {
    if (!_router) return;
    
    _router->getMaleOsc().sendMessage("/full", "i", (int)id);
    
    Serial.print("[SEND MALE] /full ID=");
    Serial.println(id);
}
