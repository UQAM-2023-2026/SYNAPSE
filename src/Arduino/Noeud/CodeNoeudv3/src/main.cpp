#include <Arduino.h>
#include "NetworkOSC.h"
#include "SerialCommunication.h"


int rhizomeID = 13;         // placeholder ID (int)
int energyValue = 12;        // placeholder energy (int)



void setup() {
    Serial.begin(115200);

    NodeStateAndID node;
    beginSerialCommunication(node);

    initNetworkOSC(
        //local_IP
        IPAddress(192,168,0,50),
        //gateway
        IPAddress(192,168,0,1),
        //subnet
        IPAddress(255,255,255,0),
        //dns1
        IPAddress(1,1,1,1),
        //dns2
        IPAddress(8,8,8,8),
        //osc_listen_port
        9699,
        //target_IP
        IPAddress(192,168,0,35),
        //target_port
        8000
    );
}


void loop() {
    updateNetworkOSC();

    checkConnectionStatus();
    bool connected = (getRhizomeValue() > 0.0f);

    int idToSend = connected ? rhizomeID : 0;
    int energyToSend = connected ? energyValue : 0;

    sendOSC(idToSend, energyToSend);  // both ints

    delay(100);
}

