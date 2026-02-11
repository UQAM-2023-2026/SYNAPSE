#include "NetworkOSC.h"
#include <OSCMessage.h>
#include <OSCBundle.h>
#include "NodeStateAndID.h"

static NodeStateAndID *pNode = nullptr;

// --- Variables internes
static IPAddress _targetIP;
static unsigned int _targetPort;
static unsigned int _listenPort;

WiFiUDP Udp;
bool eth_connected = false;

// Track last values to detect changes
static float lastDrainInfra = -1.0f;
static float lastDrainSupra = -1.0f;

// Rate limiting for serial output (only print every 500ms)
static unsigned long lastPrintTime = 0;
static const unsigned long PRINT_INTERVAL = 500; // ms

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
            delay(100); // Give UDP time to initialize
            Serial.print("UDP listening on port: ");
            Serial.println(_listenPort);
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
// Update Ã  appeler dans loop()
// -------------------------------------------------------------------
void updateNetworkOSC() {
    if (!eth_connected) return;
    
    // Check if UDP is properly initialized before trying to parse
    int packetSize = Udp.parsePacket();
    if (packetSize < 0) {
        // Error in parsePacket, ignore and continue
        return;
    }
    
    if (packetSize > 0) {
        OSCBundle bundleIn;
        
        // Read entire packet into the OSCBundle object
        while (packetSize--) bundleIn.fill(Udp.read());
        
        if (!pNode) return;
        
        // Dispatch /drain_infra messages
        bundleIn.dispatch("/drain_infra", [](OSCMessage &msg) {
            if (!pNode) return;
            
            float newValue = 0.0f;
            if (msg.isFloat(0)) {
                newValue = msg.getFloat(0);
            } else if (msg.isInt(0)) {
                newValue = static_cast<float>(msg.getInt(0));
            }
            
            // Update value even if not printing
            bool valueChanged = (newValue != lastDrainInfra);
            if (valueChanged) {
                pNode->setDrainRateInfra(newValue);
                lastDrainInfra = newValue;
                
                // Only print if enough time has passed
                unsigned long now = millis();
                if (now - lastPrintTime >= PRINT_INTERVAL) {
                    Serial.print("[OSC UPDATE] Pogopin 1 - drain_infra = ");
                    Serial.println(newValue);
                    lastPrintTime = now;
                }
            }
        });
        
        // Dispatch /drain_supra messages
        bundleIn.dispatch("/drain_supra", [](OSCMessage &msg) {
            if (!pNode) return;
            
            float newValue = 0.0f;
            if (msg.isFloat(0)) {
                newValue = msg.getFloat(0);
            } else if (msg.isInt(0)) {
                newValue = static_cast<float>(msg.getInt(0));
            }
            
            // Update value even if not printing
            bool valueChanged = (newValue != lastDrainSupra);
            if (valueChanged) {
                pNode->setDrainRateSupra(newValue);
                lastDrainSupra = newValue;
                
                // Only print if enough time has passed
                unsigned long now = millis();
                if (now - lastPrintTime >= PRINT_INTERVAL) {
                    Serial.print("[OSC UPDATE] Pogopin 2 - drain_supra = ");
                    Serial.println(newValue);
                    lastPrintTime = now;
                }
            }
        });
    }
}

// -------------------------------------------------------------------
// Envoi OSC
// -------------------------------------------------------------------
void sendOSC(int energy1, int energy2, int rhizomeId1, int rhizomeId2) {
    int e1 = energy1, e2 = energy2;
    int r1 = rhizomeId1;  // Rhizome ID (0 if disconnected)
    int r2 = rhizomeId2;  // Rhizome ID (0 if disconnected)
    
    if (!eth_connected) {
        e1 = 0;
        e2 = 0;
        r1 = 0;
        r2 = 0;
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

    // /conn1 channel - now sends Rhizome ID instead of just 1/0
    OSCMessage msgConn1("/conn1");
    msgConn1.add(r1);
    Udp.beginPacket(_targetIP, _targetPort);
    msgConn1.send(Udp);
    Udp.endPacket();
    msgConn1.empty();

    // /conn2 channel - now sends Rhizome ID instead of just 1/0
    OSCMessage msgConn2("/conn2");
    msgConn2.add(r2);
    Udp.beginPacket(_targetIP, _targetPort);
    msgConn2.send(Udp);
    Udp.endPacket();
    msgConn2.empty();
}