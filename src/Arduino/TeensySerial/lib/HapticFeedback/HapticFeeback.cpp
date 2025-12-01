// #include <Arduino.h>
// #include <Wire.h>
// #include <Adafruit_DRV2605.h>

// Adafruit_DRV2605 drv;

// unsigned long previousMillis = 0;
// unsigned long interval = 1000; 

// // --- VARIABLES DE LISSAGE (Pour éviter l'effet "aléatoire") ---
// const int numReadings = 20; // On fait la moyenne des 20 dernières lectures
// int readings[numReadings];  
// int readIndex = 0;
// long total = 0;
// int averagePot = 0;

// // --- DUREE MINIMALE ---
// // Estimation : Effet 47 + Pause 66 + Effet 47 ~= 400-500ms
// // Si on va plus vite que ça, le moteur bave.
// const int MIN_ANIMATION_TIME = 450; 

// void SetupHaptic() {
//     Serial.begin(115200);
//     Wire.begin();

//     if (!drv.begin()) {
//         Serial.println("DRV2605 not found");
//         while (1);
//     }

//     drv.useERM(); 
//     drv.setMode(DRV2605_MODE_INTTRIG); 
//     drv.selectLibrary(1); // Banque Strong
    
//     // Initialisation du tableau de lissage
//     for (int i = 0; i < numReadings; i++) readings[i] = 0;

//     Serial.println("Haptic Stabilized Ready");
// }

// // Fonction pour définir l'animation selon la vitesse
// void SetHeartSequence(bool fastMode) {
//     // Si le BPM est très rapide, on raccourcit la pause interne
//     if (fastMode) {
//         drv.setWaveform(0, 47); 
//         drv.setWaveform(1, 64); // 64 = Pause très courte (10ms) au lieu de 66
//         drv.setWaveform(2, 47);
//         drv.setWaveform(3, 0);
//     } else {
//         // Mode normal
//         drv.setWaveform(0, 47); 
//         drv.setWaveform(1, 66); // 66 = Pause normale (~30-40ms)
//         drv.setWaveform(2, 47);
//         drv.setWaveform(3, 0);
//     }
// }

// void HapticLoop() {
//     unsigned long currentMillis = millis();

//     // --- 1. LISSAGE DU POTENTIOMÈTRE (Anti-Jitter) ---
//     total = total - readings[readIndex];       // Enlever la dernière lecture
//     readings[readIndex] = analogRead(A0);      // Lire la nouvelle
//     total = total + readings[readIndex];       // Ajouter au total
//     readIndex = readIndex + 1;                 // Avancer l'index
//     if (readIndex >= numReadings) readIndex = 0;
//     averagePot = total / numReadings;          // Calculer la moyenne

//     // --- 2. CALCUL DU BPM ---
//     // On borne entre 40 et 130 BPM (Au-delà de 130, l'ERM ne suit plus physiquement)
//     int bpm = map(averagePot, 0, 1023, 40, 130);
//     int targetInterval = 60000 / bpm;

//     // Sécurité : L'intervalle ne peut pas être plus court que la durée physique du moteur
//     if (targetInterval < MIN_ANIMATION_TIME) targetInterval = MIN_ANIMATION_TIME;
    
//     interval = targetInterval;

//     // --- 3. DÉCLENCHEMENT ---
//     if (currentMillis - previousMillis >= interval) {
//         previousMillis = currentMillis; 
        
//         // Petite astuce : Si le BPM est haut (>100), on charge une séquence plus "serrée"
//         // pour éviter que le rythme devienne boueux.
//         SetHeartSequence(bpm > 100);

//         Serial.print("BPM Stable: ");
//         Serial.print(bpm);
//         Serial.println(" -> BOUM-BOUM");
        
//         drv.go(); 
//     }
// }

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_DRV2605.h"

// Création de deux objets distincts
Adafruit_DRV2605 drv1; // Celui sur 18/19
Adafruit_DRV2605 drv2; // Celui sur 16/17

void SetupHaptic() {
  Serial.begin(115200);
  
  // Attente du port série pour voir les messages
  while (!Serial && millis() < 3000); 

  Serial.println("\n--- TEST SYSTÈME SYNAPSE DUAL LRA ---");

  // ------------------------------------------
  // CONFIGURATION DU DRIVER 1 (Pins 18/19 - Wire)
  // ------------------------------------------
  // Note: drv.begin() utilise Wire (18/19) par défaut
  if (!drv1.begin(&Wire)) {
    Serial.println("ERREUR: DRV1 (Pins 18/19) non détecté !");
    Serial.println("Vérifie: VIN=3.3V, GND, SDA=18, SCL=19");
    while (1);
  }
  Serial.println("OK: DRV1 connecté.");

  // Configuration SPÉCIALE LRA pour Driver 1
  drv1.selectLibrary(6); // 6 = Librairie optimisée pour LRA
  drv1.useLRA();         // Indispensable !
  drv1.setMode(DRV2605_MODE_INTTRIG); // Mode déclenchement par commande
  
  // ------------------------------------------
  // CONFIGURATION DU DRIVER 2 (Pins 16/17 - Wire1)
  // ------------------------------------------
  // IMPORTANT: On passe &Wire1 en paramètre pour dire d'utiliser le 2ème port
  if (!drv2.begin(&Wire1)) {
    Serial.println("ERREUR: DRV2 (Pins 16/17) non détecté !");
    Serial.println("Vérifie: VIN=3.3V, GND, SDA=16, SCL=17");
    while (1);
  }
  Serial.println("OK: DRV2 connecté.");

  // Configuration SPÉCIALE LRA pour Driver 2
  drv2.selectLibrary(6); 
  drv2.useLRA();         
  drv2.setMode(DRV2605_MODE_INTTRIG);
  
  Serial.println("Système prêt. Début du cycle...");
}

void HapticLoop() {
  // --- TEST MOTEUR 1 (Pin 18/19) ---
  Serial.print("Moteur 1 (Click)... ");
  
  // Effet 1: Strong Click
  drv1.setWaveform(0, 47); 
  drv1.setWaveform(1, 0); 
  drv1.go();

  // --- TEST MOTEUR 2 (Pin 16/17) ---
  Serial.println("Moteur 2 (Double Click)...");
  
  // Effet 10: Double Click
  drv2.setWaveform(0, 47); 
  drv2.setWaveform(1, 0); 
  drv2.go();

  delay(1000);
}

void hapticOne() {
  drv1.setWaveform(0, 47); 
  drv1.setWaveform(1, 0); 
  drv1.go();
  //Serial.println("Haptic One Triggered");
}

void hapticTwo() {
  drv2.setWaveform(0, 47); 
  drv2.setWaveform(1, 0); 
  drv2.go();
  //Serial.println("Haptic Two Triggered");
}

void h_connection(bool which) {
  if (which) {
    // Trigger motor 2
    drv2.setWaveform(0, 10); 
    drv2.setWaveform(1, 0); 
    drv2.go();
    //Serial.println("Haptic Connection Two Triggered");
  } else {
    // Trigger motor 1
    drv1.setWaveform(0, 10); 
    drv1.setWaveform(1, 0); 
    drv1.go();
    //Serial.println("Haptic Connection One Triggered");
  }
}

