#ifndef NETWORK_OSC_H
#define NETWORK_OSC_H

#include <Arduino.h>
#include <ETH.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

// --- INITIALISATION DU NETWORK
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

// --- Envoi OSC : adresse "/ID", nom + valeur
//void sendOSC(const char* name, float value);
void sendOSC(int idValue, int energyValue);

// --- Externs pour main
extern WiFiUDP Udp;
extern bool eth_connected;

#endif
