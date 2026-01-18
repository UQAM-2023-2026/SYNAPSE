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

// --- À appeler dans loop()
void updateNetworkOSC();

// --- Envoi OSC : 4 channels (energy1, energy2, conn1, conn2)
void sendOSC(int energy1, int energy2, bool conn1, bool conn2);

// --- Externs pour main
extern WiFiUDP Udp;
extern bool eth_connected;

#endif
