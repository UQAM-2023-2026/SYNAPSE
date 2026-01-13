#ifndef NETWORK_OSC_H
#define NETWORK_OSC_H

#include <Arduino.h>
#include <ETH.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include "NodeStateAndID.h"

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

void updateNetworkOSC();

// Send data for multiple rhizomes
void sendOSCMulti(int rhizomeNum, int idValue, int energyValue);

extern WiFiUDP Udp;
extern bool eth_connected;

#endif