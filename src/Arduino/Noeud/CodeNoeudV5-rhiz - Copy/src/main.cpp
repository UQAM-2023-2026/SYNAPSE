/*
 * Simulateur de Nœud - Pour tester la communication Rhizome/Nœud
 * 
 * Connecter un Teensy avec ce code via Serial2 au right d'un rhizome
 * Le nœud répondra automatiquement aux messages /discover
 */

#include <Arduino.h>
#include "SerialCommunication.h"

NodeSimulator nodeSimulator(Serial2);

void setup() {
  nodeSimulator.begin();
}

void loop() {
  nodeSimulator.update();
}