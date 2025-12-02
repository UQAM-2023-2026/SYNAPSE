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
        IPAddress(192,168,18,50),
        IPAddress(192,168,0,1),
        IPAddress(255,255,255,0),
        IPAddress(1,1,1,1),
        IPAddress(8,8,8,8),
        9699,
        IPAddress(192,168,18,5),
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

