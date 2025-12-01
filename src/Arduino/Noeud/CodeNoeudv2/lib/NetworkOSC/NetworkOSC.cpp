#include "NetworkOSC.h"

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
            Serial.println("ETH Démarré");
            ETH.setHostname("esp32-osc-module");
            break;

        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("Câble ETH connecté");
            break;

        case ARDUINO_EVENT_ETH_GOT_IP:
            eth_connected = true;
            Udp.begin(_listenPort);
            Serial.print("IP ETH obtenue : ");
            Serial.println(ETH.localIP());
            break;

        case ARDUINO_EVENT_ETH_DISCONNECTED:
        case ARDUINO_EVENT_ETH_STOP:
            eth_connected = false;
            Serial.println("ETH Déconnecté");
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
    int size = Udp.parsePacket();
    if (size > 0) {
        while (size--) msgIn.fill(Udp.read());
        // main fera le dispatch si besoin
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
    msgID.add(idValue);  // now sending int
    Udp.beginPacket(_targetIP, _targetPort);
    msgID.send(Udp);
    Udp.endPacket();
    msgID.empty();

    // /energy channel
    OSCMessage msgEnergy("/energy");
    msgEnergy.add(energyValue); // sending int
    Udp.beginPacket(_targetIP, _targetPort);
    msgEnergy.send(Udp);
    Udp.endPacket();
    msgEnergy.empty();
}








