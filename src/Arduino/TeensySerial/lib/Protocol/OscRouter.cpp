/*==============================================================================
 * OscRouter.cpp - OSC message routing implementation
 *============================================================================*/

#include "OscRouter.h"
#include "DiscoveryProtocol.h"
#include "NodeProtocol.h"
#include "FullEnergySync.h"
#include <HardwarePins.h>

// Global instance
OscRouter oscRouter;

// Static instance pointer for callbacks
OscRouter* OscRouter::_instance = nullptr;

OscRouter::OscRouter()
    : _oscMale(&MALE_SERIAL)
    , _oscFemale(&FEMALE_SERIAL)
    , _discoveryHandler(nullptr)
    , _nodeHandler(nullptr)
    , _fullEnergyHandler(nullptr)
{
    _instance = this;
}

void OscRouter::begin() {
    MALE_SERIAL.begin(SERIAL_BAUD);
    FEMALE_SERIAL.begin(SERIAL_BAUD);
    
    Serial.println("[OscRouter] Initialized");
}

void OscRouter::update() {
    // Process incoming messages from both ports
    _oscMale.onOscMessageReceived(onMaleMessageStatic);
    _oscFemale.onOscMessageReceived(onFemaleMessageStatic);
}

void OscRouter::onMaleMessageStatic(MicroOscMessage& msg) {
    if (_instance) _instance->handleMaleMessage(msg);
}

void OscRouter::onFemaleMessageStatic(MicroOscMessage& msg) {
    if (_instance) _instance->handleFemaleMessage(msg);
}

void OscRouter::handleMaleMessage(MicroOscMessage& msg) {
    // Messages received on MALE port (from node or middleman in front)
    
    if (msg.checkOscAddress("/node")) {
        if (_nodeHandler) _nodeHandler->handleNodeMessage(msg);
        return;
    }
    
    if (msg.checkOscAddress("/drain")) {
        if (_nodeHandler) _nodeHandler->handleDrainMessage(msg);
        return;
    }
    
    if (msg.checkOscAddress("/full")) {
        if (_fullEnergyHandler) _fullEnergyHandler->handleFullMessage(msg, true);
        return;
    }
    
    // Unknown message
    Serial.print("[OSC MALE] Unknown: ");
    // Note: MicroOsc doesn't expose address after check, so just indicate unknown
    Serial.println("(unknown address)");
}

void OscRouter::handleFemaleMessage(MicroOscMessage& msg) {
    // Messages received on FEMALE port (from rhizome behind)
    
    if (msg.checkOscAddress("/discover_list")) {
        if (_discoveryHandler) _discoveryHandler->handleDiscoverList(msg);
        return;
    }
    
    if (msg.checkOscAddress("/energy")) {
        if (_nodeHandler) _nodeHandler->handleEnergyFromBehind(msg);
        return;
    }
    
    if (msg.checkOscAddress("/full")) {
        if (_fullEnergyHandler) _fullEnergyHandler->handleFullMessage(msg, false);
        return;
    }
    
    // Unknown message
    Serial.print("[OSC FEMALE] Unknown: ");
    // Note: MicroOsc doesn't expose address after check, so just indicate unknown
    Serial.println("(unknown address)");
}

void OscRouter::sendToMale(const char* address, const char* format, ...) {
    // Simple wrapper - protocols use getMaleOsc() for complex sends
    // This is for future extensibility
}

void OscRouter::sendToFemale(const char* address, const char* format, ...) {
    // Simple wrapper - protocols use getFemaleOsc() for complex sends
    // This is for future extensibility
}

// No-op callback for draining pending OSC messages
static void noOpOscCallback(MicroOscMessage&) {}

void OscRouter::flushMaleSerial() {
    // SLIP_END byte resets the remote SLIP parser state
    static const uint8_t SLIP_END = 0xC0;
    MALE_SERIAL.write(SLIP_END);
    
    // Process any pending data through MicroSlip to properly reset its internal state
    // Raw byte drain bypasses MicroSlip's parseIndex, causing message corruption
    int iterations = 0;
    while (MALE_SERIAL.available() && iterations < 16) {
        _oscMale.onOscMessageReceived(noOpOscCallback);
        iterations++;
    }
    if (iterations > 0) {
        Serial.print("[OSC] Flushed MALE: ");
        Serial.print(iterations);
        Serial.println(" iterations");
    }
}

void OscRouter::flushFemaleSerial() {
    // SLIP_END byte resets the remote SLIP parser state
    static const uint8_t SLIP_END = 0xC0;
    FEMALE_SERIAL.write(SLIP_END);
    
    // Process any pending data through MicroSlip to properly reset its internal state
    // Raw byte drain bypasses MicroSlip's parseIndex, causing message corruption
    int iterations = 0;
    while (FEMALE_SERIAL.available() && iterations < 16) {
        _oscFemale.onOscMessageReceived(noOpOscCallback);
        iterations++;
    }
    if (iterations > 0) {
        Serial.print("[OSC] Flushed FEMALE: ");
        Serial.print(iterations);
        Serial.println(" iterations");
    }
}
