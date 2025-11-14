#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

#define SDA_PIN 21
#define SCL_PIN 22

PN532_I2C pn532_i2c(Wire);
PN532 nfc(pn532_i2c);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  Serial.println(F("PN532 I2C Card Detection (ESP32 - SDA=21, SCL=22)"));

  // Initialize I2C with explicit ESP32 pins
  Wire.begin(SDA_PIN, SCL_PIN);

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println(F("Didn't find PN532 (check wiring/power)."));
    while (1) { delay(1000); }
  }

  Serial.print(F("Found PN532 with firmware version: 0x"));
  Serial.println(versiondata, HEX);

  // Configure board to read RFID tags
  nfc.SAMConfig();

  Serial.println(F("Waiting for an ISO14443A (MIFARE) card..."));
}

void loop() {
  uint8_t uid[7];     // buffer to store the returned UID
  uint8_t uidLength;  // length of the UID (4 or 7 bytes)
  
  // Check for a card
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    Serial.print(F("Card detected! UID: "));
    for (uint8_t i = 0; i < uidLength; i++) {
      if (uid[i] < 0x10) Serial.print('0');
      Serial.print(uid[i], HEX);
      if (i + 1 < uidLength) Serial.print(':');
    }
    Serial.println();
    delay(1000); // small delay to avoid multiple reads
  }

  delay(200);
}
