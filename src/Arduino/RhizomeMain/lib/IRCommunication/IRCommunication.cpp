#include "IRCommunication.h"
#include <Arduino.h>
#include <IRremote.hpp>
#include <RhizomeStateAndID.h>

#define IR_SEND_PIN 4
#define IR_RECEIVE_PIN 15


//Setup the IR sender and receiver
void SetupIR() {
  IrSender.begin(IR_SEND_PIN, ENABLE_LED_FEEDBACK);
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
}

// Function to reverse bits in a byte (used when receiving IR data)
static uint8_t rev8(uint8_t v) {
  v = (v & 0xF0) >> 4 | (v & 0x0F) << 4;
  v = (v & 0xCC) >> 2 | (v & 0x33) << 2;
  v = (v & 0xAA) >> 1 | (v & 0x55) << 1;
  return v;
}

// Function to receive IR data and print it in LSB-first format
void receive_ir_data() {
    if (!IrReceiver.decode()) return;

    uint32_t raw = IrReceiver.decodedIRData.decodedRawData;

    // MSB-first bytes
    uint8_t msb[4] = {
      (uint8_t)((raw >> 24) & 0xFF),
      (uint8_t)((raw >> 16) & 0xFF),
      (uint8_t)((raw >> 8)  & 0xFF),
      (uint8_t)( raw        & 0xFF)
    };

    // LSB-first (bytes in reverse order)
    uint8_t lsb[4] = { msb[3], msb[2], msb[1], msb[0] };
    Serial.print(F("Received: "));

    for (int i=0;i<4;i++)
    { 
        Serial.print(rev8(lsb[i])); Serial.print(' '); 
    }

    Serial.println();

    IrReceiver.resume();
}


// Function to send IR data from individual values
void send_ir_from_values(uint8_t id, uint8_t state, uint8_t energy, uint8_t side) {
  uint8_t payload[4] = { id, state, energy, side };
  uint32_t code = ((uint32_t)payload[0] << 24) |
                  ((uint32_t)payload[1] << 16) |
                  ((uint32_t)payload[2] << 8)  |
                  ((uint32_t)payload[3]);
  IrSender.sendNECMSB(code, 32);
}

// Function to send IR data from RhizomeStateAndID object
void send_ir_from_rhizome(const RhizomeStateAndID &r) {
  // adjust these calls if your class uses different getter names
  send_ir_from_values(r.getID(), r.getState(), r.getEnergy(), r.getSide());
}
