/*
 * PN532 Émetteur - Envoie des messages
 * Mode: Initiateur P2P
 */

#include <Wire.h>
#include <Adafruit_PN532.h>

// ===== CONFIGURATION DES PINS =====
#define PN532_IRQ 4
#define PN532_RESET 5

// ===== CONSTANTES =====
#define SERIAL_BAUD 115200
#define P2P_TIMEOUT 1000
#define MAX_PAYLOAD 64
#define SEND_INTERVAL 3000  // Intervalle entre les envois (ms)

// ===== OBJETS GLOBAUX =====
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET, &Wire);

// ===== VARIABLES GLOBALES =====
uint16_t messageCounter = 0;
unsigned long lastSendTime = 0;

// ===== PROTOTYPES =====
bool initializeModule();
bool setupP2PInitiator();
bool sendMessage(const char* message);

// ===== SETUP =====
void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║     PN532 Émetteur P2P                 ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  Wire.begin();
  
  if (!initializeModule()) {
    Serial.println("❌ Échec d'initialisation!");
    while (1);
  }
  
  Serial.println("✓ Module prêt!");
  Serial.println("📡 Recherche d'un périphérique NFC...\n");
}

// ===== BOUCLE PRINCIPALE =====
void loop() {
  // Envoyer un message à intervalle régulier
  if (millis() - lastSendTime >= SEND_INTERVAL) {
    
    // Configuration en mode P2P Initiateur
    if (setupP2PInitiator()) {
      Serial.println("✓ Target détecté!");
      
      // Créer et envoyer le message
      char message[50];
      snprintf(message, sizeof(message), "Message #%d", messageCounter++);
      
      if (sendMessage(message)) {
        Serial.println("✓ Message envoyé avec succès\n");
      } else {
        Serial.println("❌ Échec d'envoi\n");
      }
      
    } else {
      Serial.println("⏳ En attente d'un target...");
    }
    
    lastSendTime = millis();
  }
  
  delay(100);
}

// ===== FONCTIONS =====

/**
 * Initialise le module PN532
 */
bool initializeModule() {
  Serial.println("⚙️  Initialisation du PN532...");
  
  nfc.begin();
  uint32_t version = nfc.getFirmwareVersion();
  
  if (!version) {
    Serial.println("❌ Module non détecté!");
    return false;
  }
  
  Serial.print("  ✓ Version firmware: 0x");
  Serial.println(version, HEX);
  
  // Configuration SAM
  nfc.SAMConfig();
  
  return true;
}

/**
 * Configure en mode P2P Initiateur
 * Attend qu'un target soit détecté
 */
bool setupP2PInitiator() {
  Serial.print("🔍 Recherche d'un target P2P... ");
  
  // Configure comme initiateur et attend un target
  bool success = nfc.inListPassiveTarget();
  
  if (success) {
    Serial.println("Trouvé!");
  } else {
    Serial.println("Non trouvé");
  }
  
  return success;
}

/**
 * Envoie un message via P2P
 */
bool sendMessage(const char* message) {
  uint8_t txBuffer[MAX_PAYLOAD];
  uint8_t rxBuffer[MAX_PAYLOAD];
  uint8_t length = strlen(message);
  
  if (length > MAX_PAYLOAD) {
    length = MAX_PAYLOAD;
  }
  
  // Copier le message dans le buffer
  memcpy(txBuffer, message, length);
  
  Serial.println("┌─────────────────────────────────");
  Serial.print("│ 📤 Envoi: ");
  Serial.println(message);
  Serial.print("│ 📊 Taille: ");
  Serial.print(length);
  Serial.println(" bytes");
  
  // Envoi via InDataExchange
  uint8_t responseLength = MAX_PAYLOAD;
  bool success = nfc.inDataExchange(txBuffer, length, rxBuffer, &responseLength);
  
  if (success) {
    Serial.println("│ ✓ Statut: Succès");
    
    // Si on a reçu une réponse
    if (responseLength > 0) {
      Serial.print("│ 📥 Réponse reçue: ");
      Serial.print(responseLength);
      Serial.println(" bytes");
      Serial.print("│ Contenu: ");
      for (uint8_t i = 0; i < responseLength; i++) {
        if (rxBuffer[i] >= 32 && rxBuffer[i] <= 126) {
          Serial.print((char)rxBuffer[i]);
        } else {
          Serial.print(".");
        }
      }
      Serial.println();
    }
  } else {
    Serial.println("│ ❌ Statut: Échec");
  }
  
  Serial.println("└─────────────────────────────────");
  
  return success;
}

/**
 * Fonction optionnelle pour envoyer un message personnalisé
 * Appelable depuis le Serial Monitor
 */
void checkSerialInput() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();
    
    if (input.length() > 0) {
      Serial.println("\n📝 Message personnalisé détecté!");
      
      if (setupP2PInitiator()) {
        sendMessage(input.c_str());
      } else {
        Serial.println("❌ Aucun target disponible");
      }
    }
  }
}