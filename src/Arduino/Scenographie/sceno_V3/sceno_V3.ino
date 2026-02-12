#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <OSCMessage.h>
const int RPWM = 5; // Marche Avant
const int LPWM = 6; // Marche Arrière
const int R_EN = 7; // Enable Droite
const int L_EN = 8; // Enable Gauche

byte mac[] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
IPAddress ip(10, 0, 1, 228);
EthernetUDP Udp;
const unsigned int localPort = 1111;
int mode = -1;
bool nouvelleCommande = false;

enum GlobalState {
  ETAT_RENTREE,
  ETAT_ANIMATION
};

GlobalState globalState = ETAT_RENTREE;

unsigned long timer = 0;

// ---- animation ----
enum AnimState {
  ANIM_IDLE,
  SORTIE,
  PAUSE,
  RENTREE,
  MICRO_SORTIE,
  MICRO_PAUSE
};

AnimState animState = ANIM_IDLE;
int microCount = 0;
const int microMax = 3;

void checkOSCMessages() {
  OSCMessage msg;
  int size = Udp.parsePacket();

  if (size > 0) {
    while (size--) msg.fill(Udp.read());

    if (msg.isInt(0)) {
      mode = msg.getInt(0);
      Serial.print("OSC recu -> MODE ");
      Serial.println(mode);

      // RESET GLOBAL
      nouvelleCommande = true;
      globalState = ETAT_RENTREE;
      animState = ANIM_IDLE;
      microCount = 0;
      timer = millis();
    }
    msg.empty();
  }
}

void mouvement() {
  unsigned long maintenant = millis();

  // ================== ETAPE 1 : RENTREE COMPLETE ==================
  if (globalState == ETAT_RENTREE) {
    digitalWrite(RPWM, LOW);
    digitalWrite(LPWM, HIGH);   // rentrer

    if (maintenant - timer >= 5000) { // ajuster à ton actuateur
      digitalWrite(LPWM, LOW);
      timer = maintenant;
      globalState = ETAT_ANIMATION;
      animState = ANIM_IDLE;
      Serial.println("Rentrée complète OK -> Animation");
    }
    return;
  }

  // ================== ETAPE 2 : ANIMATION ==================
  if (globalState == ETAT_ANIMATION) {

    // -------- MODE 0 --------
    if (mode == 0) {
      switch (animState) {

        case ANIM_IDLE:
          animState = SORTIE;
          timer = maintenant;
          break;

        case SORTIE:
          digitalWrite(RPWM, HIGH);
          if (maintenant - timer >= 2000) {
            digitalWrite(RPWM, LOW);
            animState = MICRO_SORTIE;
            timer = maintenant;
          }
          break;

        case MICRO_SORTIE:
          digitalWrite(RPWM, HIGH);
          if (maintenant - timer >= 1000) {
            digitalWrite(RPWM, LOW);
            animState = MICRO_PAUSE;
            timer = maintenant;
          }
          break;

        case MICRO_PAUSE:
          if (maintenant - timer >= 500) {
            microCount++;
            timer = maintenant;
            if (microCount >= microMax) {
              animState = RENTREE;
            } else {
              animState = MICRO_SORTIE;
            }
          }
          break;

        case RENTREE:
          digitalWrite(LPWM, HIGH);
          if (maintenant - timer >= 2000) {
            digitalWrite(LPWM, LOW);
            animState = ANIM_IDLE;
          }
          break;
      }
    }

    // -------- MODE 1 --------
    else if (mode == 1) {
      switch (animState) {

        case ANIM_IDLE:
          digitalWrite(RPWM, HIGH);
          timer = maintenant;
          animState = SORTIE;
          break;

        case SORTIE:
          if (maintenant - timer >= 3000) {
            digitalWrite(RPWM, LOW);
            timer = maintenant;
            animState = PAUSE;
          }
          break;

        case PAUSE:
          if (maintenant - timer >= 1000) {
            animState = RENTREE;
            timer = maintenant;
          }
          break;

        case RENTREE:
          digitalWrite(LPWM, HIGH);
          if (maintenant - timer >= 3000) {
            digitalWrite(LPWM, LOW);
            animState = ANIM_IDLE;
          }
          break;
      }
    }

    // -------- MODE 2 --------
    else if (mode == 2) {
      switch (animState) {

        case ANIM_IDLE:
          digitalWrite(RPWM, HIGH);
          timer = maintenant;
          animState = SORTIE;
          break;

        case SORTIE:
          if (maintenant - timer >= 3000) {
            digitalWrite(RPWM, LOW);
            timer = maintenant;
            animState = PAUSE;
          }
          break;

        case PAUSE:
          if (maintenant - timer >= 3000) {
            animState = RENTREE;
            timer = maintenant;
          }
          break;

        case RENTREE:
          digitalWrite(LPWM, HIGH);
          if (maintenant - timer >= 3000) {
            digitalWrite(LPWM, LOW);
            animState = ANIM_IDLE;
          }
          break;
      }
    }
  }
}
void loop() {
  checkOSCMessages();
  mouvement();
}


