/*==============================================================================
 * FullEnergySync.h - Synchronized full energy celebration
 * 
 * Implements token-based synchronization so all rhizomes in a loop
 * trigger haptic + LED celebration simultaneously when ALL reach 100%.
 * 
 * Protocol:
 * 1. When reaching 100%: send /full <own_id> via MALE
 * 2. When receiving /full <id>:
 *    - If own energy < 100%: hold token (don't forward)
 *    - If own energy >= 100%: forward via MALE
 * 3. When receiving /full <own_id> back: all confirmed, trigger celebration
 *============================================================================*/

#ifndef FULL_ENERGY_SYNC_H
#define FULL_ENERGY_SYNC_H

#include <Arduino.h>
#include <MicroOscSlip.h>

// Forward declarations
class OscRouter;
class RhizomeData;

// Callback when all rhizomes confirmed at 100%
using AllFullCallback = void (*)(void);

class FullEnergySync {
public:
    FullEnergySync();
    
    // Initialize with dependencies
    void begin(OscRouter* router, RhizomeData* data);
    
    // Call in loop() during GENERATING state
    void update(bool inGeneratingState, bool maleConnected);
    
    // Handle incoming /full message
    // fromMale: true if received on MALE port, false if on FEMALE
    void handleFullMessage(MicroOscMessage& msg, bool fromMale);
    
    // Callback registration
    void onAllFull(AllFullCallback cb) { _onAllFull = cb; }
    
    // Reset state (call when exiting GENERATING)
    void reset();
    
    // Check if celebration was triggered
    bool hasTriggered() const { return _triggered; }
    
private:
    OscRouter* _router;
    RhizomeData* _data;
    
    // Token state
    bool _sentOwnToken;         // Have we sent our /full token?
    bool _holdingToken;         // Are we holding a token waiting for 100%?
    uint8_t _heldTokenId;       // ID of held token
    bool _triggered;            // Has celebration been triggered?
    
    // Callback
    AllFullCallback _onAllFull;
    
    // Send /full via MALE port
    void sendFullToken(uint8_t id);
};

// Global instance
extern FullEnergySync fullEnergySync;

#endif // FULL_ENERGY_SYNC_H
