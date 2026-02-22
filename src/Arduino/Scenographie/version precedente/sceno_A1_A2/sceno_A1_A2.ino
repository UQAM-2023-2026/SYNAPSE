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
EthernetUDP Udp;
const unsigned int localPort = 1111;
int pause = 500;
enum MouvementState {
  AVANT,
  PAUSE1,
  ARRIERE,
  PAUSE2
};

// ----- MODE NUIT -----
bool modeNuit = false;
unsigned long lastOSCReception = 0;
const unsigned long timeoutNuit = 10000; // 10 secondes sans message

enum NuitState {
  NUIT_SORT,
  NUIT_ENTER
};

MouvementState mouvementState = AVANT;
unsigned long mouvementTimer = 0;

NuitState nuitState = NUIT_SORT;
unsigned long nuitTimer = 0;


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

  // Remise a la position neutre

  digitalWrite(LPWMA1, LOW);
  digitalWrite(RPWMA1, HIGH);
  digitalWrite(LPWMA2, LOW);
  digitalWrite(RPWMA2, HIGH);
  delay(5000);
  //println("position_neutre");
 
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

  if (modeNuit) {
  mouvementNuit();
  } else {
  mouvement();
  }
}

void checkOSCMessages() {  
    OSCMessage msg;

    int size = Udp.parsePacket();
   // Serial.println(size);

    if (size > 0) { 
      lastOSCReception = millis();   // ← IMPORTANT
      modeNuit = false;              // on quitte le mode nuit
      while (size--) { 
        msg.fill(Udp.read());
      }

      if (msg.getInt(0) == 0) {
        pause = 50;
      }

      else if (msg.getInt(0) == 1) {
        pause = 1000;
      }

      else if (msg.getInt(0) == 2) {
        pause = 2500;
      }

      msg.empty();
      
    }
    // Activation automatique du mode nuit
    if (millis() - lastOSCReception > timeoutNuit) {
     modeNuit = true;
    }
    //Serial.println("out");
  }
  
void mouvementNuit() {

  unsigned long maintenant = millis();

  switch (nuitState) {

    case NUIT_SORT:   // SORTIR 0.5 sec
      digitalWrite(RPWMA1, LOW);
      digitalWrite(LPWMA1, HIGH);

      digitalWrite(RPWMA2, LOW);
      digitalWrite(LPWMA2, HIGH);

      if (maintenant - nuitTimer >= 500) {
        digitalWrite(LPWMA1, LOW);
        digitalWrite(LPWMA2, LOW);
        nuitTimer = maintenant;
        nuitState = NUIT_ENTER;
      }
      break;

    case NUIT_ENTER:  // ENTRER 1 sec
      digitalWrite(LPWMA1, LOW);
      digitalWrite(RPWMA1, HIGH);

      digitalWrite(LPWMA2, LOW);
      digitalWrite(RPWMA2, HIGH);

      if (maintenant - nuitTimer >= 1000) {
        digitalWrite(RPWMA1, LOW);
        digitalWrite(RPWMA2, LOW);
        nuitTimer = maintenant;
        nuitState = NUIT_SORT;
      }
      break;
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

      if (maintenant - mouvementTimer >= 1500) {
        
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

      if (maintenant - mouvementTimer >= 1000) {
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
