/*==============================================================================
 * RhizomeConnection.cpp - Single Rhizome connection implementation
 *============================================================================*/

#include "RhizomeConnection.h"

// Static instance pointers for callback dispatch (max 2 connections)
RhizomeConnection* RhizomeConnection::_instances[2] = {nullptr, nullptr};

// Static callback dispatchers (MicroOsc requires static function pointers)
void RhizomeConnection::messageCallback0(MicroOscMessage& msg) {
    if (_instances[0]) _instances[0]->handleMessage(msg);
}

void RhizomeConnection::messageCallback1(MicroOscMessage& msg) {
    if (_instances[1]) _instances[1]->handleMessage(msg);
}

RhizomeConnection::RhizomeConnection(uint8_t portIndex, HardwareSerial* serial)
    : _portIndex(portIndex)
    , _serial(serial)
    , _osc(serial)
    , _flagPin(-1)
    , _connected(false)
    , _lastActivityTime(0)
    , _drainRate(5.0f)
    , _lastSentDrainRate(-1.0f)
    , _rhizomeId(0)
    , _energy(0)
    , _connectionCallback(nullptr)
    , _energyCallback(nullptr)
{
    // Register this instance for static callback dispatch
    if (portIndex < 2) {
        _instances[portIndex] = this;
    }
}

void RhizomeConnection::begin(int rxPin, int txPin, int flagPin, long baudRate) {
    _flagPin = flagPin;
    
    // Initialize serial
    _serial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
    
    // Initialize flag pin (optional hardware detection)
    if (_flagPin >= 0) {
        pinMode(_flagPin, INPUT_PULLUP);
    }
    
    _lastActivityTime = millis();
    
    Serial.print("[RhizomeConnection ");
    Serial.print(_portIndex);
    Serial.print("] Initialized - RX:");
    Serial.print(rxPin);
    Serial.print(" TX:");
    Serial.print(txPin);
    Serial.print(" FLAG:");
    Serial.println(flagPin);
}

void RhizomeConnection::setDrainRate(float drainRate) {
    _drainRate = drainRate;
}

void RhizomeConnection::update() {
    // Process incoming OSC messages
    if (_portIndex == 0) {
        _osc.onOscMessageReceived(messageCallback0);
    } else {
        _osc.onOscMessageReceived(messageCallback1);
    }
    
    // Check FLAG pin for immediate physical disconnection detection
    if (_connected && _flagPin >= 0) {
        bool physicallyConnected = (digitalRead(_flagPin) == LOW);  // LOW = connected
        if (!physicallyConnected) {
            onDisconnected();
            return;
        }
    }
    
    // If connected, check for timeout (backup detection)
    if (_connected) {
        unsigned long now = millis();
        
        // Check for connection timeout
        if (now - _lastActivityTime >= CONNECTION_TIMEOUT_MS) {
            onDisconnected();
        }
        
        // Send drain rate update if changed
        if (_drainRate != _lastSentDrainRate) {
            sendNodeMessage();
            _lastSentDrainRate = _drainRate;
        }
    } else {
        // Reset drain rate tracking when disconnected
        _lastSentDrainRate = -1.0f;
    }
}

void RhizomeConnection::handleMessage(MicroOscMessage& msg) {
    // Ignore loopback of our own /node message
    if (msg.checkOscAddress("/node")) {
        return;
    }
    
    // Handle /discover_list - Rhizome announces presence
    if (msg.checkOscAddress("/discover_list")) {
        const char* csv = msg.nextAsString();
        
        Serial.print("[Port ");
        Serial.print(_portIndex);
        Serial.print("] /discover_list: ");
        Serial.println(csv);
        
        // Mark activity
        _lastActivityTime = millis();
        
        // Trigger connection if not already connected
        if (!_connected) {
            onConnected();
        } else {
            // Already connected but Rhizome is still discovering
            // This means our /node message was lost - resend it
            sendNodeMessage();
        }
        return;
    }
    
    // Handle /energy - Rhizome reports its energy level
    if (msg.checkOscAddress("/energy")) {
        int id = msg.nextAsInt();
        int energy = msg.nextAsInt();
        
        // Update state
        _rhizomeId = (uint8_t)id;
        _energy = (uint8_t)energy;
        _lastActivityTime = millis();
        
        Serial.print("[Port ");
        Serial.print(_portIndex);
        Serial.print("] /energy ID=");
        Serial.print(id);
        Serial.print(" E=");
        Serial.print(energy);
        Serial.println("%");
        
        // Notify callback
        if (_energyCallback) {
            _energyCallback(_portIndex, _rhizomeId, _energy);
        }
        
        // Fallback: if we receive energy but weren't connected, connect now
        if (!_connected) {
            Serial.print("[Port ");
            Serial.print(_portIndex);
            Serial.println("] Connected via /energy fallback");
            onConnected();
        }
        
        // Warning for low energy
        if (energy <= 5) {
            Serial.print("[Port ");
            Serial.print(_portIndex);
            Serial.println("] WARNING: Rhizome nearly depleted!");
        }
        return;
    }
}

void RhizomeConnection::onConnected() {
    _connected = true;
    _lastActivityTime = millis();
    
    Serial.println("---");
    Serial.print("[Port ");
    Serial.print(_portIndex);
    Serial.println("] CONNECTED");
    
    // Send /node response to Rhizome
    sendNodeMessage();
    
    Serial.println("---");
    
    // Notify callback
    if (_connectionCallback) {
        _connectionCallback(_portIndex, true);
    }
}

void RhizomeConnection::onDisconnected() {
    _connected = false;
    
    Serial.println("---");
    Serial.print("[Port ");
    Serial.print(_portIndex);
    Serial.println("] DISCONNECTED");
    Serial.println("---");
    
    // Reset rhizome data
    _rhizomeId = 0;
    _energy = 0;
    
    // Notify callback
    if (_connectionCallback) {
        _connectionCallback(_portIndex, false);
    }
}

void RhizomeConnection::sendNodeMessage() {
    _osc.sendMessage("/node", "f", _drainRate);
    _lastSentDrainRate = _drainRate;
    
    Serial.print("[Port ");
    Serial.print(_portIndex);
    Serial.print("] Sent /node drainRate=");
    Serial.println(_drainRate);
}
