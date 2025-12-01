#ifndef CUSTOM_UART_H
#define CUSTOM_UART_H

#include <Arduino.h>
#include <HardwareSerial.h>

class CustomUART {
public:
    // Constructor: specify UART number (0,1,2)
    CustomUART(int uartNum);

    // Initialize UART with baud and RX/TX pins
    void begin(unsigned long baud, int rxPin, int txPin);

    // Send a string
    void write(const char* msg);

    // Send a single byte
    void write(uint8_t byte);

    // Check if data is available
    int available();

    // Read a byte
    int read();

private:
    HardwareSerial* _serial;
    int _uartNum;
};

#endif
