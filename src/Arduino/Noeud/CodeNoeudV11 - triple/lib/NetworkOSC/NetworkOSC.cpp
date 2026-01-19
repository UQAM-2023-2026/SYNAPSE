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
            break;

        case ARDUINO_EVENT_ETH_CONNECTED:
            break;

        case ARDUINO_EVENT_ETH_GOT_IP:
            eth_connected = true;
            Udp.begin(_listenPort);
            break;

        case ARDUINO_EVENT_ETH_DISCONNECTED:
            eth_connected = false;
            break;

        case ARDUINO_EVENT_ETH_STOP:
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
}

void updateNetworkOSC() {
    if (!eth_connected) return;
    
    OSCMessage msgIn;
   
    int packetSize = Udp.parsePacket();
    if (packetSize > 0) {
        while (packetSize--) msgIn.fill(Udp.read());
            
        if (msgIn.size() > 0 && pNode) {
            if (msgIn.isFloat(0)) {
                pNode->setDrainRate(msgIn.getFloat(0));
            } else if (msgIn.isInt(0)) {
                pNode->setDrainRate(static_cast<float>(msgIn.getInt(0)));
            }
        }
    }
}

void sendOSC(int energy1, int energy2, int energy3, 
             int rhizomeId1, int rhizomeId2, int rhizomeId3) {
    int e1 = energy1, e2 = energy2, e3 = energy3;
    int r1 = rhizomeId1, r2 = rhizomeId2, r3 = rhizomeId3;
    
    if (!eth_connected) {
        e1 = e2 = e3 = 0;
        r1 = r2 = r3 = 0;
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

    // /energy3 channel
    OSCMessage msgEnergy3("/energy3");
    msgEnergy3.add(e3);
    Udp.beginPacket(_targetIP, _targetPort);
    msgEnergy3.send(Udp);
    Udp.endPacket();
    msgEnergy3.empty();

    // /conn1 channel
    OSCMessage msgConn1("/conn1");
    msgConn1.add(r1);
    Udp.beginPacket(_targetIP, _targetPort);
    msgConn1.send(Udp);
    Udp.endPacket();
    msgConn1.empty();

    // /conn2 channel
    OSCMessage msgConn2("/conn2");
    msgConn2.add(r2);
    Udp.beginPacket(_targetIP, _targetPort);
    msgConn2.send(Udp);
    Udp.endPacket();
    msgConn2.empty();

    // /conn3 channel
    OSCMessage msgConn3("/conn3");
    msgConn3.add(r3);
    Udp.beginPacket(_targetIP, _targetPort);
    msgConn3.send(Udp);
    Udp.endPacket();
    msgConn3.empty();
}