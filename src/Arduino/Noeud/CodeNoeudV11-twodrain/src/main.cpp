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
        //IPAddress(10,0,2,225),
        // ESP 
        IPAddress(10,0,2,170),

        //gateway
        //IPAddress(10,0,1,1),
        IPAddress(10,0,2,1),

        //subnet
        IPAddress(255,255,0,0),

        //dns1
        IPAddress(1,1,1,1),

        //dns2
        IPAddress(8,8,8,8),

        //osc_listen_port
        //osc Out
        9602,

        //target_IP
        //IPAddress(10,0,2,222),
        // adresse ordi lah lah 
        IPAddress(10,0,2,222),
        
        //target_port
        //osc In
        8002
    );
}

void loop() {
    updateNetworkOSC();
    SerialLoop();
    loopSendToTouch();
    //delay(20);  // 20ms = good balance between responsiveness and stability
}