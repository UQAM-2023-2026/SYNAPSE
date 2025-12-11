#include <Arduino.h>
#include "NetworkOSC.h"
#include "SerialCommunication.h"
#include "NodeStateAndID.h"

// Make node global so it doesn't get destroyed after setup()
NodeStateAndID node(1);

void setup() {
    Serial.begin(115200);

    beginSerialCommunication(node);
    beginNetworkOSC(node);

    initNetworkOSC(
        //local_IP
        //IPAddress(192,168,0,50),
        // ESP 
        IPAddress(10,0,2,180),
        //IPAddress(10,0,2,250),

        //gateway
        IPAddress(10,0,2,1),
        //IPAddress(10,0,1,1),

        //subnet
        IPAddress(255,255,255,0),

        //dns1
        IPAddress(1,1,1,1),

        //dns2
        IPAddress(8,8,8,8),

        //osc_listen_port
        9699,

        //target_IP
        //IPAddress(192,168,0,35),
        // adresse ordi lah lah 
        IPAddress(10,0,2,247),

        //target_port
        8000
    );
}

void loop() {
    updateNetworkOSC();
    checkConnectionStatus();
    loopSendToTouch();
    delay(100);
}