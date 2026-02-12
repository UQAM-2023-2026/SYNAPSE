// Contrôle Actionneur - Version Complète et Directe
// Utilisation de digitalWrite pour assurer un signal fort (HIGH)
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <OSCMessage.h>
const int RPWMA1 = 5; // Marche Avant
const int LPWMA1 = 6; // Marche Arrière
const int R_ENA1 = 7; // Enable Droite
const int L_ENA1 = 8; // Enable Gauche
const int RPWMA2 = 2; // Marche Avant
const int LPWMA2 = 3; // Marche Arrière
const int R_ENA2 = 4; // Enable Droite
const int L_ENA2 = 9; // Enable Gauche

byte mac[] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
IPAddress ip(10, 0, 2, 228);
EthernetUDP Udp;
const unsigned int localPort = 1111;
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

  Ethernet.begin(mac, ip);
  Udp.begin(localPort);
  
  //mouvementTimer = millis();
  Serial.println("Mouvement: ENTRER >>>");
}
 
void loop() {

  checkOSCMessages();
  mouvement();
}

void checkOSCMessages() {  
    OSCMessage msg;

    int size = Udp.parsePacket();
    Serial.println(size);

     if (size > 0) { 
      while (size--) { 
        msg.fill(Udp.read());
      }

      if (msg.getInt(0) == 0) {
        pause = 1000;
      }

      else if (msg.getInt(0) == 1) {
        pause = 2500;
      }

      else if (msg.getInt(0) == 2) {
        pause = 4000;
      }

      msg.empty();
      
    }
    Serial.println("out");
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
