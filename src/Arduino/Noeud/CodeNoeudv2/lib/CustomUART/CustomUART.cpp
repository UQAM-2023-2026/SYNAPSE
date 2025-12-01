#include "CustomUART.h"

CustomUART::CustomUART(int uartNum) : _uartNum(uartNum) {
    // Choose HardwareSerial instance based on UART number
    switch (_uartNum) {
        case 0:
            _serial = &Serial;    // UART0
            break;
        case 1:
            _serial = &Serial1;   // UART1
            break;
        case 2:
            _serial = &Serial2;   // UART2
            break;
        default:
            _serial = &Serial;    // fallback
            break;
    }
}

void CustomUART::begin(unsigned long baud, int rxPin, int txPin) {
    _serial->begin(baud, SERIAL_8N1, rxPin, txPin);
}

void CustomUART::write(const char* msg) {
    _serial->print(msg);
}

void CustomUART::write(uint8_t byte) {
    _serial->write(byte);
}

int CustomUART::available() {
    return _serial->available();
}

int CustomUART::read() {
    return _serial->read();
}
