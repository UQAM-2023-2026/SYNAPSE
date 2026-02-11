/*==============================================================================
 * RhizomeConnection.h - Abstraction for a single Rhizome connection
 * 
 * Encapsulates:
 * - OSC communication with one Rhizome
 * - Connection state management
 * - Energy data from the connected Rhizome
 * 
 * Protocol flow:
 * 1. Rhizome sends /discover_list <csv>
 * 2. Node responds with /node <drainRate>
 * 3. Rhizome sends periodic /energy <id> <energy%>
 *============================================================================*/

#ifndef RHIZOME_CONNECTION_H
#define RHIZOME_CONNECTION_H

#include <Arduino.h>
#include <MicroOscSlip.h>

// Timing constants
static constexpr unsigned long CONNECTION_TIMEOUT_MS = 2000;  // No /energy for 2s = disconnected

// Callback types
using ConnectionCallback = void (*)(uint8_t portIndex, bool connected);
using EnergyCallback = void (*)(uint8_t portIndex, uint8_t rhizomeId, uint8_t energy);

class RhizomeConnection {
public:
    RhizomeConnection(uint8_t portIndex, HardwareSerial* serial);
    
    // Initialize the connection (call in setup)
    void begin(int rxPin, int txPin, int flagPin, long baudRate);
    
    // Set drain rate to send on connection
    void setDrainRate(float drainRate);
    float getDrainRate() const { return _drainRate; }
    
    // Update loop (call every loop iteration)
    void update();
    
    // Callbacks
    void onConnectionChange(ConnectionCallback cb) { _connectionCallback = cb; }
    void onEnergyReceived(EnergyCallback cb) { _energyCallback = cb; }
    
    // State queries
    bool isConnected() const { return _connected; }
    uint8_t getRhizomeId() const { return _rhizomeId; }
    uint8_t getEnergy() const { return _energy; }
    uint8_t getPortIndex() const { return _portIndex; }
    
private:
    // Hardware
    uint8_t _portIndex;          // 0 or 1 (for identification)
    HardwareSerial* _serial;
    MicroOscSlip<128> _osc;
    int _flagPin;
    
    // Connection state
    bool _connected;
    unsigned long _lastActivityTime;
    float _drainRate;
    float _lastSentDrainRate;
    
    // Rhizome data
    uint8_t _rhizomeId;
    uint8_t _energy;
    
    // Callbacks
    ConnectionCallback _connectionCallback;
    EnergyCallback _energyCallback;
    
    // Internal handlers
    void handleMessage(MicroOscMessage& msg);
    void onConnected();
    void onDisconnected();
    void sendNodeMessage();
    
    // Static dispatch for MicroOsc callback
    static RhizomeConnection* _instances[2];
    static void messageCallback0(MicroOscMessage& msg);
    static void messageCallback1(MicroOscMessage& msg);
};

#endif // RHIZOME_CONNECTION_H
