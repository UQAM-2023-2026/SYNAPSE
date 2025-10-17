#include <Wire.h>
#include <Adafruit_PN532.h>

#define PN532_IRQ   4    
#define PN532_RESET 5    

Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Wire.begin();  // Default: SDA=21, SCL=22
  
  Serial.println("\nScanning I2C bus...");
  for (byte addr = 1; addr < 127; ++addr) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at 0x");
      Serial.println(addr, HEX);
    }
  }
  
  Serial.println("\nInitializing PN532...");
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Aucune réponse du PN532!");
    while (1);
  }
  
  Serial.print("Found PN532 chip version: 0x");
  Serial.println(versiondata, HEX);
  
  nfc.SAMConfig();
  Serial.println("PN532 prêt!");
  Serial.println("En attente d'une carte NFC...\n");
}

void loop() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer pour stocker l'UID
  uint8_t uidLength;                         // Longueur de l'UID
  
  // Cherche une carte (timeout de 100ms)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100);
  
  if (success) {
    Serial.println("Carte détectée!");
    Serial.print("UID Length: ");
    Serial.print(uidLength, DEC);
    Serial.println(" bytes");
    Serial.print("UID Value: ");
    
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(" 0x");
      Serial.print(uid[i], HEX);
    }
    Serial.println("\n");
    
    // Attendre que la carte soit retirée
    delay(1000);
  }
  
  delay(100);  // Petite pause entre chaque scan
}