/*==============================================================================
 * PortManager.h - Physical connection detection
 * 
 * Handles debounced detection of MALE and FEMALE port connections.
 * Provides callbacks for connection/disconnection events.
 *============================================================================*/

#ifndef PORT_MANAGER_H
#define PORT_MANAGER_H

#include <Arduino.h>

// Connection event callback type
using PortEventCallback = void (*)(void);

class PortManager {
public:
    PortManager();
    
    // Initialize pins and interrupts
    void begin();
    
    // Call in loop() - updates state machines
    void update();
    
    // Connection status
    bool isMaleConnected() const { return _maleConnected; }
    bool isFemaleConnected() const { return _femaleConnected; }
    
    // Event callbacks
    void onMaleConnect(PortEventCallback cb)    { _onMaleConnect = cb; }
    void onMaleDisconnect(PortEventCallback cb) { _onMaleDisconnect = cb; }
    void onFemaleConnect(PortEventCallback cb)    { _onFemaleConnect = cb; }
    void onFemaleDisconnect(PortEventCallback cb) { _onFemaleDisconnect = cb; }
    
private:
    // Connection state machine states
    enum class ConnState {
        DISCONNECTED,
        DEBOUNCING_CONNECT,
        CONNECTED,
        DEBOUNCING_DISCONNECT
    };
    
    // State machines for each port
    ConnState _maleState;
    ConnState _femaleState;
    
    // Debounce timing
    unsigned long _maleDebounceStart;
    unsigned long _femaleDebounceStart;
    
    // Current connection status
    bool _maleConnected;
    bool _femaleConnected;
    
    // Callbacks
    PortEventCallback _onMaleConnect;
    PortEventCallback _onMaleDisconnect;
    PortEventCallback _onFemaleConnect;
    PortEventCallback _onFemaleDisconnect;
    
    // Debounce constants
    static constexpr unsigned long CONNECT_DEBOUNCE_MS = 150;
    static constexpr unsigned long DISCONNECT_DEBOUNCE_MS = 600;
    
    // Internal update functions
    void updateMaleState();
    void updateFemaleState();
};

// Global instance
extern PortManager portManager;

#endif // PORT_MANAGER_H
