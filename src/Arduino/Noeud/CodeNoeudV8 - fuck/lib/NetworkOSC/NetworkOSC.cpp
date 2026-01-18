#include "NetworkOSC.h"
#include <OSCMessage.h>
#include "NodeStateAndID.h"

// Forward declaration instead of include
extern void sendDrainRateToAllUARTs(float drainRate);

static NodeStateAndID *pNode = nullptr;

// --- Variables internes
static IPAddress _targetIP;
static unsigned int _targetPort;
static unsigned int _listenPort;

WiFiUDP Udp;
bool eth_connected = false;

void beginNetworkOSC(NodeStateAndID &node) {
    pNode = &node;
}

// -------------------------------------------------------------------
// WiFi / ETH events
// -------------------------------------------------------------------
void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("📡 ETH Started");
            break;

        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("📡 ETH Connected");
            break;

        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.print("📡 ETH IP: ");
            Serial.println(ETH.localIP());
            eth_connected = true;
            Udp.begin(_listenPort);
            break;

        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("❌ ETH Disconnected");
            eth_connected = false;
            break;

        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("❌ ETH Stopped");
            eth_connected = false;
            break;

        default:
            break;
    }
}

// -------------------------------------------------------------------
// Initialisation
// -------------------------------------------------------------------
void initNetworkOSC(
    IPAddress localIP,
    IPAddress gateway,
    IPAddress subnet,
    IPAddress dns1,
    IPAddress dns2,
    unsigned int listenPort,
    IPAddress targetIP,
    unsigned int targetPort
) {
    _listenPort = listenPort;
    _targetIP = targetIP;
    _targetPort = targetPort;

    WiFi.mode(WIFI_OFF);
    WiFi.onEvent(WiFiEvent);

    ETH.begin(
        ETH_PHY_ADDR,
        ETH_PHY_POWER,
        ETH_PHY_MDC,
        ETH_PHY_MDIO,
        ETH_PHY_TYPE,
        ETH_CLK_MODE
    );

    ETH.config(localIP, gateway, subnet, dns1, dns2);

    Serial.println("=================================");
    Serial.println("NETWORK READY");
    Serial.print("Listen Port: ");
    Serial.println(listenPort);
    Serial.print("Target: ");
    Serial.print(targetIP);
    Serial.print(":");
    Serial.println(targetPort);
    Serial.println("=================================");
}

// -------------------------------------------------------------------
// Update à appeler dans loop()
void updateNetworkOSC() {
    if (!eth_connected) return;
    
    OSCMessage msgIn;
   
    int packetSize = Udp.parsePacket();
    if (packetSize > 0) {
        // read entire packet into the OSCMessage object
        while (packetSize--) msgIn.fill(Udp.read());
        
        // Update drain rate
        if (msgIn.size() > 0 && pNode) {
            float newDrainRate = 0.0f;
            bool updated = false;
            
            if (msgIn.isFloat(0)) {
                newDrainRate = msgIn.getFloat(0);
                updated = true;
            } else if (msgIn.isInt(0)) {
                newDrainRate = static_cast<float>(msgIn.getInt(0));
                updated = true;
            }
            
            if (updated) {
                float oldDrainRate = pNode->getDrainRate();
                
                // Only update and print if the value actually changed
                if (oldDrainRate != newDrainRate) {
                    pNode->setDrainRate(newDrainRate);
                    
                    Serial.print("📥 DrainRate: ");
                    Serial.print(oldDrainRate);
                    Serial.print(" → ");
                    Serial.println(newDrainRate);
                    
                    // Send the new drain rate to ALL connected UARTs
                    sendDrainRateToAllUARTs(newDrainRate);
                }
            }
        }
    }
}

// -------------------------------------------------------------------
// Envoi OSC
// -------------------------------------------------------------------
void sendOSC(int idValue, int energyValue) {
    if (!eth_connected) {
        idValue = 0;
        energyValue = 0;
    }

    // /ID channel
    OSCMessage msgID("/ID");
    msgID.add(idValue);
    Udp.beginPacket(_targetIP, _targetPort);
    msgID.send(Udp);
    Udp.endPacket();
    msgID.empty();

    // /energy channel
    OSCMessage msgEnergy("/energy");
    msgEnergy.add(energyValue);
    Udp.beginPacket(_targetIP, _targetPort);
    msgEnergy.send(Udp);
    Udp.endPacket();
    msgEnergy.empty();
}