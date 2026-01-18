#include "NetworkOSC.h"
#include <OSCMessage.h>
#include "NodeStateAndID.h"

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

    Serial.println("NetworkOSC initialized");
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
            
        // Update drain rate silently (no print)
        if (msgIn.size() > 0 && pNode) {
            if (msgIn.isFloat(0)) {
                pNode->setDrainRate(msgIn.getFloat(0));
            } else if (msgIn.isInt(0)) {
                pNode->setDrainRate(static_cast<float>(msgIn.getInt(0)));
            }
        }
    }
}

// -------------------------------------------------------------------
// Envoi OSC
// -------------------------------------------------------------------
void sendOSC(int energy1, int energy2, bool conn1, bool conn2) {
    int e1 = energy1, e2 = energy2;
    int c1 = conn1 ? 1 : 0;
    int c2 = conn2 ? 1 : 0;
    
    if (!eth_connected) {
        e1 = 0;
        e2 = 0;
        c1 = 0;
        c2 = 0;
    }

    // /energy1 channel
    OSCMessage msgEnergy1("/energy1");
    msgEnergy1.add(e1);
    Udp.beginPacket(_targetIP, _targetPort);
    msgEnergy1.send(Udp);
    Udp.endPacket();
    msgEnergy1.empty();

    // /energy2 channel
    OSCMessage msgEnergy2("/energy2");
    msgEnergy2.add(e2);
    Udp.beginPacket(_targetIP, _targetPort);
    msgEnergy2.send(Udp);
    Udp.endPacket();
    msgEnergy2.empty();

    // /conn1 channel
    OSCMessage msgConn1("/conn1");
    msgConn1.add(c1);
    Udp.beginPacket(_targetIP, _targetPort);
    msgConn1.send(Udp);
    Udp.endPacket();
    msgConn1.empty();

    // /conn2 channel
    OSCMessage msgConn2("/conn2");
    msgConn2.add(c2);
    Udp.beginPacket(_targetIP, _targetPort);
    msgConn2.send(Udp);
    Udp.endPacket();
    msgConn2.empty();
}