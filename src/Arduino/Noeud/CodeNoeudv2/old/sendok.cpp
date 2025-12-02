#include <Arduino.h>
#include <ETH.h>
#include <WiFi.h> // Nécessaire pour accéder à la commande WIFI_OFF
#include <WiFiUdp.h>
#include <OSCMessage.h>

// --- PARAMÈTRES RÉSEAU ---
IPAddress local_ip(192, 168, 0, 50);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

// --- CIBLE OSC ---
IPAddress osc_target_ip(192, 168, 0, 35); // L'adresse de ton ordinateur/serveur
const unsigned int osc_port = 8000;       // Le port d'écoute (à adapter selon ton logiciel : Resolume, MaxMSP, etc.)
const unsigned int osc_port_receive = 8001;

// --- OBJETS ---
WiFiUDP Udp; // Permet d'envoyer les paquets UDP


// --- PINS OLIMEX ESP32-POE-ISO ---
#define ETH_CLK_MODE    ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER   12
#define ETH_PHY_MDC     23
#define ETH_PHY_MDIO    18
#define ETH_PHY_TYPE    ETH_PHY_LAN8720
#define ETH_PHY_ADDR    0

static bool eth_connected = false;

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("Ethernet démarré");
      ETH.setHostname("esp32-poe-iso");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED:
      Serial.println("Câble Ethernet détecté");
      break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.print("IP obtenue : ");
      Serial.println(ETH.localIP());
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("Câble débranché");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("Ethernet arrêté");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void setup() {
  Serial.begin(9600);
  Udp.begin(osc_port_receive); // Démarrer l'écoute des paquets entrants sur le port défini

  // 1. FORCER L'EXTINCTION DU WIFI
  // Cela coupe l'alimentation de la radio Wi-Fi interne
  WiFi.mode(WIFI_OFF); 

  // 2. Gestionnaire d'événements (Le nom "WiFiEvent" vient de la librairie interne, 
  // mais cela gère bien l'Ethernet ici)
  WiFi.onEvent(WiFiEvent);

  // 3. Démarrer l'Ethernet
  ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE);

  // 4. Configurer l'IP Statique
  ETH.config(local_ip, gateway, subnet);
}

void led(OSCMessage &msg) {
  int ledState;
  ledState = msg.getInt(0);
  //digitalWrite(BUILTIN_LED, ledState);
  Serial.print("/led: ");
  Serial.println(ledState);
}

void loop() {
  if (eth_connected) {
    // Création du message OSC
    // Adresse OSC : /test/valeur
    OSCMessage msg("/test/valeur");
    
    // Ajout de données (ex: un nombre aléatoire ou la lecture d'un capteur)
    // Tu peux envoyer des int, float, string...
    float randomValue = random(0, 100) / 10.0;
    msg.add(randomValue);
    
    Serial.print("Envoi OSC vers ");
    Serial.print(osc_target_ip);
    Serial.print(" : ");
    Serial.println(randomValue);
 
    // Envoi du paquet UDP
    Udp.beginPacket(osc_target_ip, osc_port);
    msg.send(Udp);
    Udp.endPacket();
    
    // Libérer la mémoire du message
    msg.empty();

    //recevoir
    OSCMessage msgr;
    int size = Udp.parsePacket();
 
  if (size > 0) {
    while (size--) {
      msgr.fill(Udp.read());
    }
    if (!msgr.hasError()) {
      msgr.dispatch("/led", led);
    } 
    else {
    //  error = msg.getError();
      Serial.println("error: ");
    //  Serial.println(error);
    }
  }
}
  delay(1000); // Envoie toutes les secondes
}