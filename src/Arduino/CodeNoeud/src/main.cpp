#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>

#include <SerialCommunication.h>
 
// --- CONFIGURATION RÉSEAU ---
IPAddress local_ip(192, 168, 0, 50);    // IP de cet ESP32
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress dns1(1, 1, 1, 1);
IPAddress dns2(8, 8, 8, 8);
 
// --- CONFIGURATION OSC ---
// CIBLE (Où on envoie)
IPAddress osc_target_ip(192, 168, 0, 35);
const unsigned int osc_target_port = 8000;
 
// LOCAL (Où on écoute)
const unsigned int local_port = 9699;
 
WiFiUDP Udp;
 
// Variables pour le timer (remplacement du delay)
unsigned long previousMillis = 0;
const long interval = 1000; // Envoi toutes les 1000ms (1 seconde)
 
// --- PINS OLIMEX ESP32-POE-ISO ---
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER   12
#define ETH_PHY_MDC     23
#define ETH_PHY_MDIO    18
#define ETH_PHY_TYPE    ETH_PHY_LAN8720
#define ETH_PHY_ADDR    0
 
static bool eth_connected = false;
 
// --- FONCTIONS DE RÉCEPTION OSC ---
// Cette fonction est appelée quand on reçoit un message sur l'adresse "/mon/led"
void actionLed(OSCMessage &msg) {
  // Vérifier si la donnée est un float ou un int
  float valeur = msg.getFloat(0); // On récupère la première valeur
  
  Serial.print("COMMANDE RECUE (/mon/led) : ");
  Serial.println(valeur);
 
  // Ici tu pourrais allumer une LED ou activer un relais
  // ex: if (valeur > 0.5) digitalWrite(LED_BUILTIN, HIGH);
}
 
void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Démarré");
      ETH.setHostname("esp32-osc-bidir");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("Câble connecté");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("IP obtenue : ");
      Serial.println(ETH.localIP());
      
      // IMPORTANT : On commence à écouter sur le port 9699 une fois qu'on a une IP
      Udp.begin(local_port);
      Serial.print("Écoute OSC active sur le port : ");
      Serial.println(local_port);
      
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      eth_connected = false;
      break;
    default:
      break;
  }
}
 
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF);
  WiFi.onEvent(WiFiEvent);
  
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE);
  ETH.config(local_ip, gateway, subnet, dns1, dns2);
}
 
void loop() {
  if (eth_connected) {
    
    // 1. PARTIE RÉCEPTION (Écoute permanente)
    OSCMessage msgIn;
    int size = Udp.parsePacket();
 
    if (size > 0) {
      // Optionnel : Vérifier si ça vient bien de 192.168.0.35
      if (Udp.remoteIP() == osc_target_ip) {
      //   Serial.print("Message reçu de .35 -> ");
          // msgIn.dispatch("/test", actionLed);
      } else {
         Serial.print("Message reçu d'une source inconnue -> ");
      }
 
      while (size--) {
        msgIn.fill(Udp.read());
      }
      
      if (!msgIn.hasError()) {
        // Si le message a l'adresse "/mon/led", on lance la fonction actionLed
        msgIn.dispatch("/test/valeur", actionLed);
        
        // Tu peux ajouter d'autres routes ici :
        // msgIn.dispatch("/mon/fader", actionFader);
      }
      //Serial.println(msgin.);
    }
 
    // 2. PARTIE ENVOI (Toutes les X secondes via millis)
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;
 
      // Création du message
      OSCMessage msgOut("/test/valeur");
      //float randomValue = random(0, 100) / 10.0;
      //msgOut.add(randomValue);
      checkConnectionStatus();
      lookForMessages();
      float val = getRhizomeValue();
      msgOut.add(val);   // <-- now msgOut still works
 
      Udp.beginPacket(osc_target_ip, osc_target_port);
      msgOut.send(Udp);
      Udp.endPacket();
      msgOut.empty();
      
      // Serial.println("Ping envoyé..."); // Décommenter pour debug
    }
  }
}