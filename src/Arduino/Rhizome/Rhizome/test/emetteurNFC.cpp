/*
 * ESP32 #1 - NFC P2P Initiator
 * Bibliothèque: Elechouse PN532
 * Communication: I2C
 */

#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

// Configuration I2C
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

// Pins I2C ESP32
#define SDA_PIN 21
#define SCL_PIN 22

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 #1 - NFC P2P Initiator ===");
  
  // Initialiser I2C avec gestion d'erreur améliorée
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000); // 100kHz pour plus de stabilité
  
  // Petite pause pour laisser l'I2C s'initialiser
  delay(1000);
  
  Serial.println("Initialisation du PN532...");
  
  // Initialiser PN532
  nfc.begin();
  
  // Vérifier la version du firmware avec plusieurs tentatives
  uint32_t versiondata = 0;
  for (int i = 0; i < 5; i++) {
    versiondata = nfc.getFirmwareVersion();
    if (versiondata) break;
    Serial.print("Tentative ");
    Serial.print(i + 1);
    Serial.println("/5 pour détecter le PN532...");
    delay(500);
  }
  
  if (!versiondata) {
    Serial.println("ERREUR: PN532 non trouvé après 5 tentatives!");
    Serial.println("Vérifiez les connexions:");
    Serial.println("  ESP32 3.3V -> PN532 VCC");
    Serial.println("  ESP32 GND  -> PN532 GND");
    Serial.println("  ESP32 D21  -> PN532 SDA");
    Serial.println("  ESP32 D22  -> PN532 SCL");
    Serial.println("  PN532 I2C mode: les jumpers doivent être sur I2C");
    while (1) { 
      delay(1000);
    }
  }
  
  Serial.print("✓ PN532 trouvé - Firmware v");
  Serial.print((versiondata >> 16) & 0xFF, DEC);
  Serial.print('.');
  Serial.println((versiondata >> 8) & 0xFF, DEC);
  
  // Configuration SAM (Security Access Module)
  nfc.SAMConfig();
  
  Serial.println("✓ Configuration P2P terminée");
  Serial.println("Placez les deux modules PN532 à 2-4 cm l'un de l'autre");
  Serial.println("Attente de Target...\n");
}

void loop() {
  static unsigned long lastTry = 0;
  if (millis() - lastTry < 2000) {
    delay(100);
    return;
  }
  lastTry = millis();
  
  Serial.println("--- Tentative de connexion P2P ---");
  
  // Rechercher un pair NFC en mode passif
  bool peerFound = nfc.inListPassiveTarget();
  
  if (peerFound) {
    Serial.println("✓ Pair NFC détecté!");
    
    // Message à envoyer
    String message = "State 1";
    uint8_t dataOut[32];
    uint8_t dataIn[32];
    uint8_t dataOutLen = message.length();
    uint8_t dataInLen = sizeof(dataIn);
    
    // Copier le message dans le buffer
    message.getBytes(dataOut, dataOutLen + 1);
    
    // Échanger des données avec le pair
    if (nfc.inDataExchange(dataOut, dataOutLen, dataIn, &dataInLen) == 0) {
      Serial.println("✓ Communication P2P réussie!");
      
      Serial.print("  → Sent: ");
      Serial.println(message);
      
      if (dataInLen > 0) {
        Serial.print("  ← Received: ");
        for (uint8_t i = 0; i < dataInLen; i++) {
          Serial.write(dataIn[i]);
        }
        Serial.println();
      }
    } else {
      Serial.println("✗ Échec de l'échange de données");
    }
    
    Serial.println();
    delay(3000); // Attendre 3 secondes après une communication réussie
    
  } else {
    Serial.println("✗ Aucun pair détecté");
  }
}