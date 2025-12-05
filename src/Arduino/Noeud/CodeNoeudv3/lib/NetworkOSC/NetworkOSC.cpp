#include "NetworkOSC.h"
#include <OSCMessage.h>

// --- Variables internes
static IPAddress _targetIP;
static unsigned int _targetPort;
static unsigned int _listenPort;

WiFiUDP Udp;
bool eth_connected = false;

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

        // print only first numeric argument
        if (msgIn.size() > 0) {
            if (msgIn.isFloat(0)) {
                Serial.println(msgIn.getFloat(0), 6);  // prints exactly what TD sent
            } else if (msgIn.isInt(0)) {
                Serial.println(msgIn.getInt(0));
            }
        }
    }
}

// -------------------------------------------------------------------
// Envoi OSC
// -------------------------------------------------------------------
void sendOSC(int data0, int data1) {
    OSCMessage msgOut("/data");
    msgOut.add(data0);
    msgOut.add(data1);
    
    Udp.beginPacket(_targetIP, _targetPort);
    msgOut.send(Udp);
    Udp.endPacket();
}








