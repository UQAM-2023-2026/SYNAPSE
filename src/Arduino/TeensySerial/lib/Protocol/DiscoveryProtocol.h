/*==============================================================================
 * DiscoveryProtocol.h - Discovery message handling
 * 
 * Manages /discover_list protocol for building the seen ID bank.
 * MALE sends, FEMALE receives.
 *============================================================================*/

#ifndef DISCOVERY_PROTOCOL_H
#define DISCOVERY_PROTOCOL_H

#include <Arduino.h>
#include <MicroOscSlip.h>

// Forward declarations
class OscRouter;
class RhizomeStateMachine;

// Callback when loop is detected (own ID received back)
using LoopDetectedCallback = void (*)(uint8_t count);

class DiscoveryProtocol {
public:
    static constexpr uint8_t MAX_SEEN_IDS = 20;
    
    DiscoveryProtocol();
    
    // Initialize with dependencies
    void begin(OscRouter* router, uint8_t ownId);
    
    // Call in loop() when in DISCOVERING state
    void update(bool maleConnected, bool femaleConnected);
    
    // Handle incoming /discover_list on FEMALE port
    void handleDiscoverList(MicroOscMessage& msg);
    
    // Callback for loop detection
    void onLoopDetected(LoopDetectedCallback cb) { _onLoopDetected = cb; }
    
    // ID bank management
    void clearSeenIds();
    void addSeenId(uint8_t id);
    bool isIdSeen(uint8_t id) const;
    uint8_t getSeenCount() const { return _seenCount; }
    
    // Build CSV string of seen IDs
    void buildCsvString(char* buffer, size_t bufferSize) const;
    
    // Reset for new discovery cycle
    void reset();
    
private:
    OscRouter* _router;
    uint8_t _ownId;
    
    // Seen ID bank (circular buffer)
    uint8_t _seenIds[MAX_SEEN_IDS];
    uint8_t _seenCount;
    
    // Periodic send timing
    unsigned long _lastSendTime;
    static constexpr unsigned long SEND_INTERVAL_MS = 350;
    
    // Loop detection
    bool _loopDetected;
    LoopDetectedCallback _onLoopDetected;
    
    // Send /discover_list via MALE port
    void sendDiscoverList();
    
    // Parse incoming CSV and merge IDs
    bool parseAndMergeIds(const char* csv);
};

// Global instance
extern DiscoveryProtocol discoveryProtocol;

#endif // DISCOVERY_PROTOCOL_H
