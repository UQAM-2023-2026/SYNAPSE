#include <Arduino.h>
#include <ETH.h>
//#include <AsyncUDP_ESP32_W5500.h>

AsyncUDP udp;

IPAddress tdIP(192,168,0,34);
uint16_t tdPort = 8888;

int update_rate = 16;

size_t createOSCMessage(const char* address, float value, uint8_t* buffer, size_t maxlen) {
    size_t index = 0;
    size_t addrLen = strlen(address);
    memcpy(buffer + index, address, addrLen);
    ixndex += addrLen;
    while (index % 4 != 0) buffer[index++] = 0;

    buffer[index++] = ',';
    buffer[index++] = 'f';
    buffer[index++] = 0;
    buffer[index++] = 0;

    union { float f; uint8_t b[4]; } conv;
    conv.f = value;
    buffer[index++] = conv.b[3];
    buffer[index++] = conv.b[2];
    buffer[index++] = conv.b[1];
    buffer[index++] = conv.b[0];

    return index;
}

void setup() {
    Serial.begin(9600);
    ETH.begin();

    while (!ETH.linkUp()) {
        Serial.println("Waiting for Ethernet link...");
        delay(500);
    }

    if (udp.connect(tdIP, tdPort)) {
        Serial.println("UDP connected to TouchDesigner!");
    } else {
        Serial.println("UDP connection failed!");
    }
}

void loop() {
    static float val = 0.0;
    val += 0.01;
    if (val > 1.0) val = 0.0;

    uint8_t buffer[32];
    size_t len = createOSCMessage("/value", val, buffer, sizeof(buffer));

    // Envoi correct du message
    udp.send(buffer, len);

    Serial.print("Sent OSC: ");
    Serial.println(val);

    delay(update_rate);
}
