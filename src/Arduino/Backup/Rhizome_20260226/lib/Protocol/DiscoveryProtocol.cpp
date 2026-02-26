/*==============================================================================
 * DiscoveryProtocol.cpp - Discovery message handling implementation
 *============================================================================*/

#include "DiscoveryProtocol.h"
#include "OscRouter.h"
#include <string.h>

// Global instance
DiscoveryProtocol discoveryProtocol;

DiscoveryProtocol::DiscoveryProtocol()
    : _router(nullptr)
    , _ownId(0)
    , _seenCount(0)
    , _lastSendTime(0)
    , _loopDetected(false)
    , _onLoopDetected(nullptr)
    , _onLoopBroken(nullptr)
{}

void DiscoveryProtocol::begin(OscRouter* router, uint8_t ownId) {
    _router = router;
    _ownId = ownId;
    reset();
    
    Serial.println("[DiscoveryProtocol] Initialized");
    Serial.print("  Own ID: ");
    Serial.println(_ownId);
}

void DiscoveryProtocol::reset() {
    clearSeenIds();
    addSeenId(_ownId);  // Always include own ID
    _loopDetected = false;
    _lastSendTime = 0;
}

void DiscoveryProtocol::clearSeenIds() {
    _seenCount = 0;
    memset(_seenIds, 0, sizeof(_seenIds));
}

void DiscoveryProtocol::addSeenId(uint8_t id) {
    if (isIdSeen(id)) return;  // No duplicates
    
    if (_seenCount < MAX_SEEN_IDS) {
        _seenIds[_seenCount++] = id;
    }
    // If full, ignore (shouldn't happen with 20 slots)
}

bool DiscoveryProtocol::isIdSeen(uint8_t id) const {
    for (uint8_t i = 0; i < _seenCount; ++i) {
        if (_seenIds[i] == id) return true;
    }
    return false;
}

void DiscoveryProtocol::buildCsvString(char* buffer, size_t bufferSize) const {
    buffer[0] = '\0';
    size_t pos = 0;
    
    for (uint8_t i = 0; i < _seenCount && pos < bufferSize - 4; ++i) {
        if (i > 0) {
            buffer[pos++] = ',';
        }
        int written = snprintf(buffer + pos, bufferSize - pos, "%d", _seenIds[i]);
        if (written > 0) pos += written;
    }
}

void DiscoveryProtocol::update(bool maleConnected, bool femaleConnected) {
    if (!_router) return;
    
    // Keep sending even after loop detected so ALL rhizomes can detect it
    // Only send if MALE is connected
    if (maleConnected) {
        unsigned long now = millis();
        if (now - _lastSendTime >= SEND_INTERVAL_MS) {
            sendDiscoverList();
            _lastSendTime = now;
        }
    }
}

void DiscoveryProtocol::sendDiscoverList() {
    if (!_router) return;
    
    char csv[128];
    buildCsvString(csv, sizeof(csv));
    
    _router->getMaleOsc().sendMessage("/discover_list", "s", csv);
    
    Serial.print("[SEND MALE] /discover_list: ");
    Serial.println(csv);
}

void DiscoveryProtocol::handleDiscoverList(MicroOscMessage& msg) {
    const char* csv = msg.nextAsString();
    
    if (!csv || csv[0] == '\0') {
        Serial.println("[RECV FEMALE] /discover_list: (empty)");
        return;
    }
    
    Serial.print("[RECV FEMALE] /discover_list: ");
    Serial.println(csv);
    
    bool foundOwnId = parseAndMergeIds(csv);
    
    Serial.print("  Seen count: ");
    Serial.println(_seenCount);
    
    if (foundOwnId && !_loopDetected) {
        _loopDetected = true;
        Serial.println("[DISCOVERY] Loop detected! Own ID received back.");
        
        if (_onLoopDetected) {
            _onLoopDetected(_seenCount);
        }
    } else if (!foundOwnId && _loopDetected) {
        // We had a loop, but now our ID is missing - loop broken
        _loopDetected = false;
        Serial.println("[DISCOVERY] Loop broken!");
        
        if (_onLoopBroken) {
            _onLoopBroken();
        }
    }
}

bool DiscoveryProtocol::parseAndMergeIds(const char* csv) {
    bool foundOwnId = false;
    
    // Make a copy for strtok
    char buffer[256];
    strncpy(buffer, csv, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    char* token = strtok(buffer, ",");
    while (token) {
        int id = atoi(token);
        if (id >= 0 && id < 256) {
            if ((uint8_t)id == _ownId) {
                foundOwnId = true;
            }
            addSeenId((uint8_t)id);
        }
        token = strtok(nullptr, ",");
    }
    
    return foundOwnId;
}
