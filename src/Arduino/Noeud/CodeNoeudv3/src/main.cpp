#include <Arduino.h>
#include "NetworkOSC.h"
#include "SerialCommunication.h"


int rhizomeID = 13;         // placeholder ID (int)
int energyValue = 12;        // placeholder energy (int)

// Connector 1
#define CONNECT_PIN_1 32
#define RX1 32
#define TX1 33
Serial2.begin(SERIAL_BAUD, SERIAL_8N1, RX1, TX1);

// Connector 2
#define CONNECT_PIN_2 34
#define RX2 25
#define TX2 26
Serial1.begin(SERIAL_BAUD, SERIAL_8N1, RX2, TX2);

// Connector 3
#define CONNECT_PIN_3 35
#define RX3 27
#define TX3 14
Serial.begin(SERIAL_BAUD, SERIAL_8N1, RX3, TX3); // or leave Serial for debug


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

