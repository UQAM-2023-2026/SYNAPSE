#ifndef NETWORK_OSC_H
#define NETWORK_OSC_H

#include <Arduino.h>
#include <ETH.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include "NodeStateAndID.h"


// --- INITIALISATION DU NETWORK
void beginNetworkOSC(NodeStateAndID &node);

void initNetworkOSC(
    IPAddress localIP,
    IPAddress gateway,
    IPAddress subnet,
    IPAddress dns1,
    IPAddress dns2,
    unsigned int listenPort,
    IPAddress targetIP,
    unsigned int targetPort
);

// --- Ã€ appeler dans loop()
void updateNetworkOSC();

// --- Envoi OSC : 4 channels (energy1, energy2, conn1, conn2)
// conn1 and conn2 now send the Rhizome ID (0 if disconnected)
void sendOSC(int energy1, int energy2, int rhizomeId1, int rhizomeId2);

// --- Externs pour main
extern WiFiUDP Udp;
extern bool eth_connected;

#endif