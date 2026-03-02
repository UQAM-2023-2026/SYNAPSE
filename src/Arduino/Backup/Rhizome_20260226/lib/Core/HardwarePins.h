/*==============================================================================
 * HardwarePins.h - Hardware pin definitions
 * 
 * Single source of truth for all hardware mappings.
 * NEVER use left/right - only MALE/FEMALE.
 *============================================================================*/

#ifndef HARDWARE_PINS_H
#define HARDWARE_PINS_H

#include <Arduino.h>

namespace Pins {
    // MALE PORT (sends discovery, connects to nodes)
    constexpr uint8_t MALE_FLAG = 20;
    constexpr uint8_t MALE_RX = 15;     // Serial3 RX
    constexpr uint8_t MALE_TX = 14;     // Serial3 TX
    constexpr uint8_t MALE_LED = 1;
    // MALE Haptic: Wire (SDA 18 / SCL 19)
    
    // FEMALE PORT (receives discovery, connects to other rhizomes)
    constexpr uint8_t FEMALE_FLAG = 6;
    constexpr uint8_t FEMALE_RX = 7;    // Serial2 RX
    constexpr uint8_t FEMALE_TX = 8;    // Serial2 TX
    constexpr uint8_t FEMALE_LED = 0;
    // FEMALE Haptic: Wire1 (SDA 17 / SCL 16)
}

// Serial port references
#define MALE_SERIAL   Serial3
#define FEMALE_SERIAL Serial2

// Serial baud rate
constexpr uint32_t SERIAL_BAUD = 9600;

#endif // HARDWARE_PINS_H
