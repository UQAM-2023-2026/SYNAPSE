/*==============================================================================
 * NodeProtocol.h - Node communication protocol
 * 
 * Handles:
 * - /node messages (from real node on MALE port)
 * - /drain messages (from middleman rhizome on MALE port)  
 * - /energy messages (from rhizome behind on FEMALE port, relayed to node)
 * 
 * In MIDDLEMAN state:
 * - Sends /node to FEMALE (act as node to rhizome behind)
 * - Relays /energy from FEMALE to MALE
 * - Does NOT send own energy
 *============================================================================*/

#ifndef NODE_PROTOCOL_H
#define NODE_PROTOCOL_H

#include <Arduino.h>
#include <MicroOscSlip.h>
#include <RhizomeState.h>

// Forward declarations
class OscRouter;
class RhizomeData;

// Callback when node connection is established
using NodeConnectedCallback = void (*)(float drainRate);
// Callback when node connection is lost (timeout)
using NodeLostCallback = void (*)(void);
// Callback when energy is received from rhizome behind (for relay)
using EnergyReceivedCallback = void (*)(uint8_t id, uint8_t energy);

class NodeProtocol {
public:
    NodeProtocol();
    
    // Initialize with dependencies
    void begin(OscRouter* router, RhizomeData* data);
    
    // Call in loop() - handles periodic sends based on state
    void update(RhizomeState state, bool maleConnected, bool femaleConnected);
    
    // Message handlers (called by OscRouter)
    void handleNodeMessage(MicroOscMessage& msg);
    void handleDrainMessage(MicroOscMessage& msg);
    void handleEnergyFromBehind(MicroOscMessage& msg);
    
    // Callbacks
    void onNodeConnected(NodeConnectedCallback cb) { _onNodeConnected = cb; }
    void onNodeLost(NodeLostCallback cb) { _onNodeLost = cb; }
    void onEnergyReceived(EnergyReceivedCallback cb) { _onEnergyReceived = cb; }
    
    // State queries
    bool isNodeConnected() const { return _nodeConnected; }
    float getDrainRate() const { return _drainRate; }
    
    // Reset on disconnection
    void reset();
    
private:
    OscRouter* _router;
    RhizomeData* _data;
    
    // Node connection state
    bool _nodeConnected;
    float _drainRate;
    
    // Periodic timing
    unsigned long _lastEnergySent;
    unsigned long _lastNodeSentToFemale;
    unsigned long _lastNodeReceived;  // For timeout detection
    static constexpr unsigned long ENERGY_SEND_INTERVAL_MS = 100;
    static constexpr unsigned long NODE_SEND_INTERVAL_MS = 500;
    static constexpr unsigned long NODE_TIMEOUT_MS = 1500;  // 1.5 seconds - Node sends every ~1s
    
    // Callbacks
    NodeConnectedCallback _onNodeConnected;
    NodeLostCallback _onNodeLost;
    EnergyReceivedCallback _onEnergyReceived;
    
    // Send functions
    void sendEnergyToNode();
    void sendNodeToFemale();
    void relayEnergyToNode(uint8_t id, uint8_t energy);
};

// Global instance
extern NodeProtocol nodeProtocol;

#endif // NODE_PROTOCOL_H
