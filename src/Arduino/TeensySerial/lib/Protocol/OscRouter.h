/*==============================================================================
 * OscRouter.h - OSC message routing
 * 
 * Routes incoming OSC messages to appropriate protocol handlers.
 * No business logic - just dispatch.
 *============================================================================*/

#ifndef OSC_ROUTER_H
#define OSC_ROUTER_H

#include <Arduino.h>
#include <MicroOscSlip.h>

// Forward declarations for message handlers
class DiscoveryProtocol;
class NodeProtocol;
class FullEnergySync;

class OscRouter {
public:
    OscRouter();
    
    // Initialize serial ports and OSC
    void begin();
    
    // Call in loop() - processes incoming messages
    void update();
    
    // Register protocol handlers
    void setDiscoveryHandler(DiscoveryProtocol* handler) { _discoveryHandler = handler; }
    void setNodeHandler(NodeProtocol* handler) { _nodeHandler = handler; }
    void setFullEnergyHandler(FullEnergySync* handler) { _fullEnergyHandler = handler; }
    
    // Send messages (used by protocol handlers)
    void sendToMale(const char* address, const char* format, ...);
    void sendToFemale(const char* address, const char* format, ...);
    
    // Direct access for protocols that need raw send
    MicroOscSlip<64>& getMaleOsc() { return _oscMale; }
    MicroOscSlip<64>& getFemaleOsc() { return _oscFemale; }
    
    // Clear serial buffers (call on connection to remove garbage)
    void flushMaleSerial();
    void flushFemaleSerial();
    
private:
    MicroOscSlip<64> _oscMale;    // MALE port OSC (Serial3)
    MicroOscSlip<64> _oscFemale;  // FEMALE port OSC (Serial2)
    
    // Protocol handlers
    DiscoveryProtocol* _discoveryHandler;
    NodeProtocol* _nodeHandler;
    FullEnergySync* _fullEnergyHandler;
    
    // Message dispatch
    void handleMaleMessage(MicroOscMessage& msg);
    void handleFemaleMessage(MicroOscMessage& msg);
    
    // Static callbacks for MicroOsc (routes to instance methods)
    static OscRouter* _instance;
    static void onMaleMessageStatic(MicroOscMessage& msg);
    static void onFemaleMessageStatic(MicroOscMessage& msg);
};

// Global instance
extern OscRouter oscRouter;

#endif // OSC_ROUTER_H
