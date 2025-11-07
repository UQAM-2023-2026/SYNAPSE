#include "TeensyCommunication.h"
#include <Arduino.h>

void SetupTeensyCommunication() {
    Serial2.begin(9600, SERIAL_8N1, 16, 17); // Initialize Serial2 for communication with Teensy RX=16, TX=17
}

void SendBang() {
    Serial2.write("1");
    Serial.println("Sent Bang to Teensy");
}

void SendStop() {
    Serial2.write("0");
    Serial.println("Sent Stop to Teensy");
}
