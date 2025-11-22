#include <Arduino.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// Audio objects
AudioPlaySdWav playWav1;
AudioOutputI2S i2s1;
AudioConnection patchCord1(playWav1, 0, i2s1, 0);
AudioConnection patchCord2(playWav1, 1, i2s1, 1);
AudioControlSGTL5000 sgtl5000;

void SetupAudio() {
    AudioMemory(8);

    // Initialize audio shield
    sgtl5000.enable();
    sgtl5000.lineOutLevel(13);
    sgtl5000.volume(1); // Set volume to 50%

    // Initialize SD card
    if (!SD.begin(10)) {
        Serial.println("SD card initialization failed!");
        while (1);
    }
    Serial.println("Carte SD détectée.");

    // Play WAV file
    playWav1.play("RHI_Transfer-Rhizome.wav");
    delay(25); // Laisse le temps au lecteur de démarrer
}

void AudioLoop() {
    if (!playWav1.isPlaying()) {
        Serial.println("Lecture terminée !");
        delay(1000);
        playWav1.play("RHI_Transfer-Rhizome.wav"); // boucle le fichier pour test
        delay(25);
    }
}