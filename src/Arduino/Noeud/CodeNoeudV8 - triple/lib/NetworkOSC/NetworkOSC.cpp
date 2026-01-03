#include "NetworkOSC.h"
#include <OSCMessage.h>
#include "NodeStateAndID.h"

static NodeStateAndID *pNode = nullptr;

static IPAddress _targetIP;
static unsigned int _targetPort;
static unsigned int _listenPort;

WiFiUDP Udp;
bool eth_connected = false;

void beginNetworkOSC(NodeStateAndID &node) {
    pNode = &node;
}

void WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            Serial.println("ETH Started");
            break;

        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("ETH Connected");
            break;

        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.print("ETH IP: ");
            Serial.println(ETH.localIP());
            eth_connected = true;
            Udp.begin(_listenPort);
            break;

        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("ETH Disconnected");
            eth_connected = false;
            break;

        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("ETH Stopped");
            eth_connected = false;
            break;

        default:
            break;
    }
}

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

    Serial.println("NetworkOSC initialized for 3 rhizomes");
}

void updateNetworkOSC() {
    if (!eth_connected) return;
    
    OSCMessage msgIn;
   
    int packetSize = Udp.parsePacket();
    if (packetSize > 0) {
        while (packetSize--) msgIn.fill(Udp.read());
            
        // Update drain rate (sent to all 3 rhizomes)
        if (msgIn.size() > 0 && pNode) {
            if (msgIn.isFloat(0)) {
                pNode->setDrainRate(msgIn.getFloat(0));
            } else if (msgIn.isInt(0)) {
                pNode->setDrainRate(static_cast<float>(msgIn.getInt(0)));
            }
        }
        Serial.print("[OSC IN] Drain rate updated to: ");
        Serial.println(pNode->getDrainRate());
    }
}

// Send OSC for specific rhizome
void sendOSCMulti(int rhizomeNum, int idValue, int energyValue) {
    if (!eth_connected) {
        idValue = 0;
        energyValue = 0;
    }

    // Create unique OSC addresses for each rhizome
    char idAddr[16], energyAddr[16];
    sprintf(idAddr, "/ID%d", rhizomeNum);
    sprintf(energyAddr, "/energy%d", rhizomeNum);

    // Send /ID1, /ID2, or /ID3
    OSCMessage msgID(idAddr);
    msgID.add(idValue);
    Udp.beginPacket(_targetIP, _targetPort);
    msgID.send(Udp);
    Udp.endPacket();
    msgID.empty();

    // Send /energy1, /energy2, or /energy3
    OSCMessage msgEnergy(energyAddr);
    msgEnergy.add(energyValue);
    Udp.beginPacket(_targetIP, _targetPort);
    msgEnergy.send(Udp);
    Udp.endPacket();
    msgEnergy.empty();
}