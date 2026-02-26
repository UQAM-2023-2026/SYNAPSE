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
    , _heldCount(0)
    , _triggered(false)
    , _syncForwarded(false)
    , _onAllFull(nullptr)
{
    memset(_heldTokenIds, 0, sizeof(_heldTokenIds));
}

void FullEnergySync::begin(OscRouter* router, RhizomeData* data) {
    _router = router;
    _data = data;
    reset();
    
    Serial.println("[FullEnergySync] Initialized");
}

void FullEnergySync::reset() {
    _sentOwnToken = false;
    _heldCount = 0;
    memset(_heldTokenIds, 0, sizeof(_heldTokenIds));
    _triggered = false;
    _syncForwarded = false;
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
    
    // If we're holding tokens and now at 100%, forward them all
    if (_heldCount > 0 && isFull && maleConnected) {
        forwardAllHeldTokens();
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
        // Send /sync to trigger all rhizomes simultaneously
        Serial.println("[FULL] Own token received back - broadcasting /sync");
        sendSyncBroadcast();
        return;
    }
    
    // Not our token - check if we should forward or hold
    float energy = _data->getEnergy();
    
    if (energy >= 100.0f) {
        // We're full, forward immediately
        sendFullToken((uint8_t)tokenId);
    } else {
        // We're not full yet, hold the token
        addHeldToken((uint8_t)tokenId);
        Serial.print("[FULL] Holding token ID=");
        Serial.print(tokenId);
        Serial.print(" (our energy: ");
        Serial.print(energy);
        Serial.print("%, held count: ");
        Serial.print(_heldCount);
        Serial.println(")");
    }
}

void FullEnergySync::handleSyncMessage(MicroOscMessage& msg, bool fromMale) {
    // /sync is a broadcast trigger - all rhizomes trigger immediately
    // ALWAYS forward /sync once to ensure it reaches all rhizomes in the loop
    // (even if we already triggered from our own token returning)
    
    // Forward first (before any processing) to minimize latency
    if (!_syncForwarded && _router) {
        _router->getMaleOsc().sendMessage("/sync", "");
        _syncForwarded = true;
    }
    
    // Now trigger celebration if not already done
    if (!_triggered) {
        Serial.println("[SYNC] Triggering celebration!");
        _triggered = true;
        if (_onAllFull) {
            _onAllFull();
        }
    }
}

void FullEnergySync::sendFullToken(uint8_t id) {
    if (!_router) return;
    
    _router->getMaleOsc().sendMessage("/full", "i", (int)id);
    
    Serial.print("[SEND MALE] /full ID=");
    Serial.println(id);
}

void FullEnergySync::sendSyncBroadcast() {
    if (!_router) return;
    
    _triggered = true;
    _syncForwarded = true;  // We're the originator, don't forward again
    
    // Send /sync first to minimize latency for other rhizomes
    _router->getMaleOsc().sendMessage("/sync", "");
    Serial.println("[SYNC] Broadcasting to all rhizomes");
    
    // Then trigger our own celebration
    if (_onAllFull) {
        _onAllFull();
    }
}

void FullEnergySync::addHeldToken(uint8_t id) {
    // Check if already holding this token
    for (uint8_t i = 0; i < _heldCount; i++) {
        if (_heldTokenIds[i] == id) {
            return;  // Already have this one
        }
    }
    
    // Add if we have space
    if (_heldCount < MAX_HELD_TOKENS) {
        _heldTokenIds[_heldCount++] = id;
    }
}

void FullEnergySync::forwardAllHeldTokens() {
    Serial.print("[FULL] Forwarding all held tokens (count: ");
    Serial.print(_heldCount);
    Serial.println(")");
    
    for (uint8_t i = 0; i < _heldCount; i++) {
        sendFullToken(_heldTokenIds[i]);
    }
    _heldCount = 0;
}
