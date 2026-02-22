// Contrôle Actionneur - Version Complète et Directe
// Utilisation de digitalWrite pour assurer un signal fort (HIGH)
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <OSCMessage.h>
const int RPWMA1 = 5; // Marche Avant
const int LPWMA1 = 6; // Marche Arrière
const int R_ENA1 = 7; // Enable Droite
const int L_ENA1 = 8; // Enable Gauche 2
const int RPWMA2 = A1; // Marche Avant
const int LPWMA2 = A2; // Marche Arrière
const int R_ENA2 = A3; // Enable Droite
const int L_ENA2 = A4; // Enable Gauche

byte mac[] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
IPAddress ip(10, 0, 2, 228);
const unsigned int localPort = 1111;
EthernetUDP Udp;
int pause = 500;
enum MouvementState {
  AVANT,
  PAUSE1,
  ARRIERE,
  PAUSE2
};

MouvementState mouvementState = AVANT;
unsigned long mouvementTimer = 0;


void setup() {
  Serial.begin(9600); // Pour le débogage sur l'écran PC

  pinMode(53, OUTPUT);   // IMPORTANT sur Mega
  digitalWrite(53, HIGH);
  // CS du W5500
     // force CS sur pin 10
  pinMode(10, OUTPUT);   // CS W5500
  digitalWrite(10, HIGH);
  Ethernet.init(10);

  IPAddress gateway(10, 0, 2, 1);
  IPAddress subnet(255, 255, 255, 0);
  Ethernet.begin(mac, ip, gateway, gateway, subnet);

  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());
  
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable not connected!");
  } else {
    Serial.println("Ethernet cable connected");
  }

    // IP Configuration
  Serial.print("IP Address: ");
  Serial.println(Ethernet.localIP());
  Serial.print("Subnet Mask: ");
  Serial.println(Ethernet.subnetMask());
  Serial.print("Gateway: ");
  Serial.println(Ethernet.gatewayIP());
  Serial.print("DNS: ");
  Serial.println(Ethernet.dnsServerIP());
  
  // Start UDP
  Serial.print("Starting UDP on port ");
  Serial.println(localPort);
  Udp.begin(localPort);
  
  Serial.println("=== DIAGNOSTIC COMPLETE ===");
  Serial.println();


  Serial.println(Ethernet.localIP());


  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Shield non detecte !");
  } else {
    Serial.println("Shield detecte");
  }


  pinMode(RPWMA1, OUTPUT);
  pinMode(LPWMA1, OUTPUT);
  pinMode(R_ENA1, OUTPUT);
  pinMode(L_ENA1, OUTPUT);
  pinMode(RPWMA2, OUTPUT);
  pinMode(LPWMA2, OUTPUT);
  pinMode(R_ENA2, OUTPUT);
  pinMode(L_ENA2, OUTPUT);
 
  // ÉTAPE CRUCIALE : On active le module (Enable)
  // Sans ça, le moteur ne recevra jamais de courant
  digitalWrite(R_ENA1, HIGH);
  digitalWrite(L_ENA1, HIGH);

  digitalWrite(R_ENA2, HIGH);
  digitalWrite(L_ENA2, HIGH);
 
  // On s'assure que tout est à l'arrêt au début
  digitalWrite(RPWMA1, LOW);
  digitalWrite(LPWMA1, LOW);

  digitalWrite(RPWMA2, LOW);
  digitalWrite(LPWMA2, LOW);
  
  //mouvementTimer = millis();
  Serial.println("Mouvement: ENTRER >>>");
}
 
void loop() {
  Ethernet.maintain();
  
  // Check if IP is still valid
  static unsigned long lastIPCheck = 0;
  if (millis() - lastIPCheck > 10000) {  // Every 10 seconds
    if (Ethernet.localIP() == IPAddress(0, 0, 0, 0)) {
      Serial.println("ERROR: Lost IP address! Reinitializing...");
      Ethernet.begin(mac, ip);
      delay(1000);
      Udp.begin(localPort);
      Serial.print("New IP: ");
      Serial.println(Ethernet.localIP());
    }
    lastIPCheck = millis();
  }
  checkOSCMessages();
  mouvement();
}

void checkOSCMessages() {  
  int size = Udp.parsePacket();

  
  static unsigned long lastStatus = 0;
  if (millis() - lastStatus > 5000) {
    Serial.print("UDP listening on ");
    Serial.print(Ethernet.localIP());
    Serial.print(":");
    Serial.print(localPort);
    Serial.println(" - Waiting for packets...");
    lastStatus = millis();
  }
  
  if (size > 0) {
    Serial.println("\n*** PACKET RECEIVED! ***");
    Serial.print("Size: ");
    Serial.println(size);
    Serial.print("From: ");
    Serial.print(Udp.remoteIP());
    Serial.print(":");
    Serial.println(Udp.remotePort());
    
    // Read and display raw bytes
    Serial.print("Data (hex): ");
    while (size > 0) {
      byte b = Udp.read();
      if (b < 16) Serial.print("0");
      Serial.print(b, HEX);
      Serial.print(" ");
      size--;
    }
    Serial.println("\n*** END PACKET ***\n");
  }
    
    if (size > 0) {
      Serial.print("Received packet of size ");
      Serial.println(size);
      Serial.print("From ");
      IPAddress remote = Udp.remoteIP();
      Serial.print(remote);
      Serial.print(", port ");
      Serial.println(Udp.remotePort());
      
      OSCMessage msg;
      while (size--) { 
        msg.fill(Udp.read());
      }

      if (!msg.hasError()) {
        int value = msg.getInt(0);
        Serial.print("OSC value: ");
        Serial.println(value);
        
        if (value == 0) {
          pause = 1000;
          Serial.println("Pause = 1000ms");
        }
        else if (value == 1) {
          pause = 2500;
          Serial.println("Pause = 2500ms");
        }
        else if (value == 2) {
          pause = 4000;
          Serial.println("Pause = 4000ms");
        }
      } else {
        Serial.println("OSC Error!");
      }
      
      msg.empty();
    }
}
void mouvement() {
  
  unsigned long maintenant = millis();

  switch (mouvementState) {
    case AVANT:

      digitalWrite(LPWMA1, LOW);
      digitalWrite(RPWMA1, HIGH);
      digitalWrite(LPWMA2, LOW);
      digitalWrite(RPWMA2, HIGH);

      if (maintenant - mouvementTimer >= 5000) {
        
        digitalWrite(RPWMA1, LOW);   // stop
        digitalWrite(RPWMA2, LOW);
        mouvementTimer = maintenant;
        mouvementState = PAUSE1;
        Serial.println("--- PAUSE ---");

      }
      break;

    case PAUSE1:
      if (maintenant - mouvementTimer >= pause) {
        mouvementTimer = maintenant;
        mouvementState = ARRIERE;
        Serial.println("Mouvement: SORTIR <<<");
      }
      break;

    case ARRIERE:
      digitalWrite(RPWMA1, LOW);
      digitalWrite(LPWMA1, HIGH);

      digitalWrite(RPWMA2, LOW);
      digitalWrite(LPWMA2, HIGH);

      if (maintenant - mouvementTimer >= 4000) {
       digitalWrite(LPWMA1, LOW);   // stop
        digitalWrite(LPWMA2, LOW);
        mouvementTimer = maintenant;
        mouvementState = PAUSE2;
        Serial.println("--- PAUSE ---");
      }
      break;

    case PAUSE2:
    //Serial.println("PAUSE2");
      if (maintenant - mouvementTimer >= pause) {
        mouvementTimer = maintenant;
        mouvementState = AVANT;
        Serial.println("Mouvement: ENTRER >>>");
      }
      break;
  }
}
