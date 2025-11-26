#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_DRV2605.h"

Adafruit_DRV2605 drv;

unsigned long previousMillis = 0;
unsigned long interval = 1000; 

// --- VARIABLES DE LISSAGE (Pour éviter l'effet "aléatoire") ---
const int numReadings = 20; // On fait la moyenne des 20 dernières lectures
int readings[numReadings];  
int readIndex = 0;
long total = 0;
int averagePot = 0;

// --- DUREE MINIMALE ---
// Estimation : Effet 47 + Pause 66 + Effet 47 ~= 400-500ms
// Si on va plus vite que ça, le moteur bave.
const int MIN_ANIMATION_TIME = 450; 

void SetupHaptic() {
    Serial.begin(115200);
    Wire.begin();

    if (!drv.begin()) {
        Serial.println("DRV2605 not found");
        while (1);
    }

    drv.useERM(); 
    drv.setMode(DRV2605_MODE_INTTRIG); 
    drv.selectLibrary(1); // Banque Strong
    
    // Initialisation du tableau de lissage
    for (int i = 0; i < numReadings; i++) readings[i] = 0;

    Serial.println("Haptic Stabilized Ready");
}

// Fonction pour définir l'animation selon la vitesse
void SetHeartSequence(bool fastMode) {
    // Si le BPM est très rapide, on raccourcit la pause interne
    if (fastMode) {
        drv.setWaveform(0, 47); 
        drv.setWaveform(1, 64); // 64 = Pause très courte (10ms) au lieu de 66
        drv.setWaveform(2, 47);
        drv.setWaveform(3, 0);
    } else {
        // Mode normal
        drv.setWaveform(0, 47); 
        drv.setWaveform(1, 66); // 66 = Pause normale (~30-40ms)
        drv.setWaveform(2, 47);
        drv.setWaveform(3, 0);
    }
}

void HapticLoop() {
    unsigned long currentMillis = millis();

    // --- 1. LISSAGE DU POTENTIOMÈTRE (Anti-Jitter) ---
    total = total - readings[readIndex];       // Enlever la dernière lecture
    readings[readIndex] = analogRead(A0);      // Lire la nouvelle
    total = total + readings[readIndex];       // Ajouter au total
    readIndex = readIndex + 1;                 // Avancer l'index
    if (readIndex >= numReadings) readIndex = 0;
    averagePot = total / numReadings;          // Calculer la moyenne

    // --- 2. CALCUL DU BPM ---
    // On borne entre 40 et 130 BPM (Au-delà de 130, l'ERM ne suit plus physiquement)
    int bpm = map(averagePot, 0, 1023, 40, 130);
    int targetInterval = 60000 / bpm;

    // Sécurité : L'intervalle ne peut pas être plus court que la durée physique du moteur
    if (targetInterval < MIN_ANIMATION_TIME) targetInterval = MIN_ANIMATION_TIME;
    
    interval = targetInterval;

    // --- 3. DÉCLENCHEMENT ---
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis; 
        
        // Petite astuce : Si le BPM est haut (>100), on charge une séquence plus "serrée"
        // pour éviter que le rythme devienne boueux.
        SetHeartSequence(bpm > 100);

        Serial.print("BPM Stable: ");
        Serial.print(bpm);
        Serial.println(" -> BOUM-BOUM");
        
        drv.go(); 
    }
}
