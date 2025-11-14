/*
 * ESP32 #2 - NFC P2P Target using Elechouse PN532 Library
 * Corrected: prototype + initAsTarget implementation
 */

#include <Wire.h>
#include "PN532_I2C.h"
#include "PN532.h"

#define SDA_PIN 21
#define SCL_PIN 22
#define I2C_ADDRESS 0x24  // Vérifie avec un scanner I2C si besoin

// Instances globales
PN532_I2C pn532_i2c(Wire);
PN532 nfc(pn532_i2c);

bool targetReady = false;

// Prototype (déclare la fonction avant son utilisation)
bool initAsTarget();

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("\n=== ESP32 #2 - NFC Target (Elechouse) ===");

  // Initialise I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);
  delay(100);

  // Démarre PN532 via la librairie
  nfc.begin();

  // Vérifie la communication
  uint32_t version = nfc.getFirmwareVersion();
  if (!version) {
    Serial.println("❌ PN532 non détecté !");
    Serial.println("→ Vérifie l’adresse I2C et les connexions.");
    while (1) delay(1000);
  }

  Serial.print("✓ PN532 détecté — Firmware: ");
  Serial.print((version >> 24) & 0xFF, HEX);
  Serial.print('.');
  Serial.print((version >> 16) & 0xFF, HEX);
  Serial.println();

  // Configure le module
  if (nfc.SAMConfig() != 0) {
    Serial.println("✓ SAMConfig OK");
  } else {
    Serial.println("⚠️ SAMConfig returned 0 (attention).");
  }

  // Lance le mode Target via la fonction
  if (initAsTarget()) {
    Serial.println("\n🎯 Mode Target prêt — En attente d’un Initiator...");
    Serial.println("Approche les deux modules à 2–4 cm.");
    targetReady = true;
  } else {
    Serial.println("❌ Échec de l'initialisation Target.");
  }
}

void loop() {
  if (!targetReady) {
    delay(200);
    return;
  }

  // Exemple de loop pour lire des données (utilise la signature de ta lib)
  // Certains forks attendent (cmd, response), d'autres juste (response).
  // Ici on tente la version la plus courante : tgGetData(cmd, response)
  uint8_t cmdBuf[1] = {0x00};   // dummy (certaines libs demandent un buffer commande)
  uint8_t rxBuffer[64];
  int8_t len = nfc.tgGetData();// adapte si ta lib a une autre signature

  if (len > 0) {
    Serial.print("\n✅ Données reçues (");
    Serial.print(len);
    Serial.println(" octets):");
    for (int i = 0; i < len; i++) {
      Serial.print(rxBuffer[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    // Réponse simple
    String reply = "ACK from Target " + String(millis());
    nfc.tgSetData((uint8_t*)reply.c_str(), reply.length());
    Serial.println("↩️ Réponse envoyée");
  } else if (len < 0) {
    // Erreur
    Serial.print("❌ tgGetData error: ");
    Serial.println(len);
  }

  delay(200);
}

// -------------------------------------------------------------
// Définition de initAsTarget()
// -------------------------------------------------------------
bool initAsTarget() {
  Serial.println("Initialisation du mode Target...");

  // Appel simple avec timeout optionnel (ici on utilise la valeur par défaut)
  int8_t status = nfc.tgInitAsTarget();

  // La doc Elechouse retourne typiquement 1 pour OK
  if (status == 1) {
    Serial.println("✓ tgInitAsTarget OK");
    return true;
  } else {
    Serial.print("tgInitAsTarget failed (code): ");
    Serial.println(status);
    // si nécessaire, tu peux retenter ou tenter avec timeout : nfc.tgInitAsTarget(1000);
    return false;
  }
}
