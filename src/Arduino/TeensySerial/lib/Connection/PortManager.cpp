/*==============================================================================
 * PortManager.cpp - Physical connection detection implementation
 *============================================================================*/

#include "PortManager.h"
#include <HardwarePins.h>

// Global instance
PortManager portManager;

PortManager::PortManager()
    : _maleState(ConnState::DISCONNECTED)
    , _femaleState(ConnState::DISCONNECTED)
    , _maleDebounceStart(0)
    , _femaleDebounceStart(0)
    , _maleConnected(false)
    , _femaleConnected(false)
    , _onMaleConnect(nullptr)
    , _onMaleDisconnect(nullptr)
    , _onFemaleConnect(nullptr)
    , _onFemaleDisconnect(nullptr)
{}

void PortManager::begin() {
    pinMode(Pins::MALE_FLAG, INPUT_PULLUP);
    pinMode(Pins::FEMALE_FLAG, INPUT_PULLUP);
    
    Serial.println("[PortManager] Initialized");
    Serial.print("  MALE flag pin: ");
    Serial.println(Pins::MALE_FLAG);
    Serial.print("  FEMALE flag pin: ");
    Serial.println(Pins::FEMALE_FLAG);
}

void PortManager::update() {
    updateMaleState();
    updateFemaleState();
}

void PortManager::updateMaleState() {
    unsigned long now = millis();
    bool pinActive = (digitalRead(Pins::MALE_FLAG) == LOW);
    
    switch (_maleState) {
        case ConnState::DISCONNECTED:
            if (pinActive) {
                _maleState = ConnState::DEBOUNCING_CONNECT;
                _maleDebounceStart = now;
            }
            break;
            
        case ConnState::DEBOUNCING_CONNECT:
            if (!pinActive) {
                _maleState = ConnState::DISCONNECTED;
            } else if (now - _maleDebounceStart >= CONNECT_DEBOUNCE_MS) {
                _maleState = ConnState::CONNECTED;
                _maleConnected = true;
                Serial.println("[PORT] MALE connected");
                if (_onMaleConnect) _onMaleConnect();
            }
            break;
            
        case ConnState::CONNECTED:
            if (!pinActive) {
                _maleState = ConnState::DEBOUNCING_DISCONNECT;
                _maleDebounceStart = now;
            }
            break;
            
        case ConnState::DEBOUNCING_DISCONNECT:
            if (pinActive) {
                _maleState = ConnState::CONNECTED;
            } else if (now - _maleDebounceStart >= DISCONNECT_DEBOUNCE_MS) {
                _maleState = ConnState::DISCONNECTED;
                _maleConnected = false;
                Serial.println("[PORT] MALE disconnected");
                if (_onMaleDisconnect) _onMaleDisconnect();
            }
            break;
    }
}

void PortManager::updateFemaleState() {
    unsigned long now = millis();
    bool pinActive = (digitalRead(Pins::FEMALE_FLAG) == LOW);
    
    switch (_femaleState) {
        case ConnState::DISCONNECTED:
            if (pinActive) {
                _femaleState = ConnState::DEBOUNCING_CONNECT;
                _femaleDebounceStart = now;
            }
            break;
            
        case ConnState::DEBOUNCING_CONNECT:
            if (!pinActive) {
                _femaleState = ConnState::DISCONNECTED;
            } else if (now - _femaleDebounceStart >= CONNECT_DEBOUNCE_MS) {
                _femaleState = ConnState::CONNECTED;
                _femaleConnected = true;
                Serial.println("[PORT] FEMALE connected");
                if (_onFemaleConnect) _onFemaleConnect();
            }
            break;
            
        case ConnState::CONNECTED:
            if (!pinActive) {
                _femaleState = ConnState::DEBOUNCING_DISCONNECT;
                _femaleDebounceStart = now;
            }
            break;
            
        case ConnState::DEBOUNCING_DISCONNECT:
            if (pinActive) {
                _femaleState = ConnState::CONNECTED;
            } else if (now - _femaleDebounceStart >= DISCONNECT_DEBOUNCE_MS) {
                _femaleState = ConnState::DISCONNECTED;
                _femaleConnected = false;
                Serial.println("[PORT] FEMALE disconnected");
                if (_onFemaleDisconnect) _onFemaleDisconnect();
            }
            break;
    }
}
