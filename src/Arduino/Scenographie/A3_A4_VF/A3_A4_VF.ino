// Contrôle Actionneur - Version Fusionnée (OSC + Fin de course)
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <OSCMessage.h>

// --- PINS ---
const int buttonPin1 = 9;
const int buttonPin2 = A5;

const int RPWMA1 = 5;   // Retract A3
const int LPWMA1 = 6;   // Extend
const int R_ENA1 = 7;
const int L_ENA1 = 8;

const int RPWMA2 = A1;  // Retract A4
const int LPWMA2 = A2;  // Extend
const int R_ENA2 = A3;
const int L_ENA2 = A4;

// --- ETHERNET / OSC ---
byte mac[] = { 0xA8, 0x61, 0x0A, 0xAE, 0x82, 0x05 };
IPAddress ip(10, 0, 2, 229);
EthernetUDP Udp;
const unsigned int localPort = 1111;

// --- TIMING ---
int pause = 500;
unsigned long lastOSCReception = 0;
const unsigned long timeoutNuit = 10000;
const unsigned long retractFailsafe = 3000;   // max retract time before forced stop
const unsigned long directionChangePause = 100; // pause before each direction change

// --- MODES & STATES ---
bool modeNuit = false;

enum MouvementState { AVANT, PAUSE_BEFORE_RETRACT, PAUSE1, ARRIERE, PAUSE_BEFORE_EXTEND, PAUSE2 };
MouvementState mouvementState = AVANT;
unsigned long mouvementTimer = 0;
unsigned long retractStartTime = 0;

enum NuitState { NUIT_PAUSE_BEFORE_RETRACT, NUIT_SORT, NUIT_PAUSE_BEFORE_EXTEND, NUIT_ENTER };
NuitState nuitState = NUIT_SORT;
unsigned long nuitTimer = 0;
unsigned long nuitRetractStartTime = 0;

void stopAll() {
  digitalWrite(LPWMA1, LOW); digitalWrite(RPWMA1, LOW);
  digitalWrite(LPWMA2, LOW); digitalWrite(RPWMA2, LOW);
}

void setup() {
  Serial.begin(9600);

  pinMode(RPWMA1, OUTPUT); pinMode(LPWMA1, OUTPUT);
  pinMode(R_ENA1, OUTPUT); pinMode(L_ENA1, OUTPUT);
  pinMode(RPWMA2, OUTPUT); pinMode(LPWMA2, OUTPUT);
  pinMode(R_ENA2, OUTPUT); pinMode(L_ENA2, OUTPUT);
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);

  digitalWrite(R_ENA1, HIGH); digitalWrite(L_ENA1, HIGH);
  digitalWrite(R_ENA2, HIGH); digitalWrite(L_ENA2, HIGH);

  // Drive to retracted position on boot, wait for both limit switches
  Serial.println("Homing: retracting...");
  digitalWrite(LPWMA1, LOW); digitalWrite(RPWMA1, HIGH);
  digitalWrite(LPWMA2, LOW); digitalWrite(RPWMA2, HIGH);

  unsigned long homingStart = millis();

  // Wait until both buttons are hit before starting the cycle
  while (digitalRead(buttonPin1) == 1 || digitalRead(buttonPin2) == 1) {
    if (digitalRead(buttonPin1) == 0) {
      digitalWrite(LPWMA1, LOW);
    }
    if (digitalRead(buttonPin2) == HIGH) {
      digitalWrite(LPWMA2, LOW);
    }
    // Failsafe during homing: stop after 2s if switches not detected
    if (millis() - homingStart >= retractFailsafe) {
      Serial.println("Homing failsafe triggered — stopping.");
      stopAll();
      break;
    }
  }

  stopAll();
  Serial.println("Homing done. Starting cycle.");

  Ethernet.begin(mac, ip);
  Udp.begin(localPort);

  mouvementTimer = millis();
  nuitTimer = millis();
}

void loop() {
  checkOSCMessages();

  if (modeNuit) {
    mouvementNuit();
  } else {
    mouvement();
  }
}

// -------------------------------------------------------
// OSC
// -------------------------------------------------------
void checkOSCMessages() {
  OSCMessage msg;
  int size = Udp.parsePacket();

  if (size > 0) {
    lastOSCReception = millis();
    modeNuit = false;

    while (size--) {
      msg.fill(Udp.read());
    }

    if      (msg.getInt(0) == 0) pause = 50;
    else if (msg.getInt(0) == 1) pause = 1000;
    else if (msg.getInt(0) == 2) pause = 2500;

    msg.empty();
  }

  if (millis() - lastOSCReception > timeoutNuit) {
    modeNuit = true;
  }
}

// -------------------------------------------------------
// MOUVEMENT NORMAL
// AVANT               → extend for 1500ms
// PAUSE_BEFORE_RETRACT→ 500ms stop before reversing
// PAUSE1              → configurable pause
// ARRIERE             → retract until BOTH limit switches hit (2s failsafe)
// PAUSE_BEFORE_EXTEND → 500ms stop before reversing
// PAUSE2              → configurable pause
// -------------------------------------------------------
void mouvement() {
  unsigned long maintenant = millis();

  int b1 = digitalRead(buttonPin1);
  int b2 = digitalRead(buttonPin2);

  switch (mouvementState) {

    case AVANT:
      digitalWrite(RPWMA1, LOW); digitalWrite(LPWMA1, HIGH);
      digitalWrite(RPWMA2, LOW); digitalWrite(LPWMA2, HIGH);

      if (maintenant - mouvementTimer >= 1500) {
        stopAll();
        mouvementTimer = maintenant;
        mouvementState = PAUSE_BEFORE_RETRACT;
        Serial.println("--- PAUSE before retract (500ms) ---");
      }
      break;

    case PAUSE_BEFORE_RETRACT:
      if (maintenant - mouvementTimer >= directionChangePause) {
        mouvementTimer = maintenant;
        mouvementState = PAUSE1;
        Serial.println("--- PAUSE 1 ---");
      }
      break;

    case PAUSE1:
      if (maintenant - mouvementTimer >= pause) {
        retractStartTime = maintenant;
        mouvementTimer = maintenant;
        mouvementState = ARRIERE;
        Serial.println("Mouvement: RETRACT (until limit switches or 2s failsafe)");
      }
      break;

    case ARRIERE: {
      bool failsafe = (maintenant - retractStartTime >= retractFailsafe);

      if (failsafe) {
        stopAll();
        Serial.println("RETRACT FAILSAFE triggered — forcing PAUSE 2");
        mouvementTimer = maintenant;
        mouvementState = PAUSE_BEFORE_EXTEND;
        break;
      }

      // Stop each motor independently as its switch is hit
      if (b1 == 1) { digitalWrite(LPWMA1, LOW); digitalWrite(RPWMA1, HIGH); }
      else          { digitalWrite(LPWMA1, LOW); digitalWrite(RPWMA1, LOW); }

      if (b2 == 1) { digitalWrite(RPWMA2, HIGH); digitalWrite(LPWMA2, LOW); }
      else          { digitalWrite(RPWMA2, LOW); digitalWrite(LPWMA2, LOW); }

      // Advance when BOTH are fully retracted
      if (b1 == 0 && b2 == 0) {
        stopAll();
        mouvementTimer = maintenant;
        mouvementState = PAUSE_BEFORE_EXTEND;
        Serial.println("--- PAUSE before extend (500ms) ---");
      }
      break;
    }

    case PAUSE_BEFORE_EXTEND:
      if (maintenant - mouvementTimer >= directionChangePause) {
        mouvementTimer = maintenant;
        mouvementState = PAUSE2;
        Serial.println("--- PAUSE 2 ---");
      }
      break;

    case PAUSE2:
      if (maintenant - mouvementTimer >= pause) {
        mouvementTimer = maintenant;
        mouvementState = AVANT;
        Serial.println("Mouvement: EXTEND >>>");
      }
      break;
  }
}

// -------------------------------------------------------
// MODE NUIT
// Same logic — retract until limit switches (with 2s failsafe),
// 500ms pause before each direction change, then extend for 1000ms
// -------------------------------------------------------
void mouvementNuit() {
  unsigned long maintenant = millis();

  int b1 = digitalRead(buttonPin1);
  int b2 = digitalRead(buttonPin2);

  switch (nuitState) {

    case NUIT_PAUSE_BEFORE_RETRACT:
      if (maintenant - nuitTimer >= directionChangePause) {
        nuitRetractStartTime = maintenant;
        nuitTimer = maintenant;
        nuitState = NUIT_SORT;
        Serial.println("Nuit: retracting...");
      }
      break;

    case NUIT_SORT: {
      bool failsafe = (maintenant - nuitRetractStartTime >= retractFailsafe);

      if (failsafe) {
        stopAll();
        Serial.println("Nuit RETRACT FAILSAFE triggered");
        nuitTimer = maintenant;
        nuitState = NUIT_PAUSE_BEFORE_EXTEND;
        break;
      }

      if (b1 == 0) { digitalWrite(RPWMA1, LOW); digitalWrite(LPWMA1, LOW); }
      else          { digitalWrite(LPWMA1, LOW); digitalWrite(RPWMA1, HIGH); }

      if (b2 == 0) { digitalWrite(RPWMA2, LOW); digitalWrite(LPWMA2, LOW); }
      else          { digitalWrite(LPWMA2, LOW); digitalWrite(RPWMA2, HIGH); }

      if (b1 == 0 && b2 == 0) {
        stopAll();
        nuitTimer = maintenant;
        nuitState = NUIT_PAUSE_BEFORE_EXTEND;
        Serial.println("Nuit: both retracted — pause before extending...");
      }
      break;
    }

    case NUIT_PAUSE_BEFORE_EXTEND:
      if (maintenant - nuitTimer >= directionChangePause) {
        nuitTimer = maintenant;
        nuitState = NUIT_ENTER;
        Serial.println("Nuit: extending...");
      }
      break;

    case NUIT_ENTER:
      digitalWrite(LPWMA1, HIGH); digitalWrite(RPWMA1, LOW);
      digitalWrite(LPWMA2, HIGH); digitalWrite(RPWMA2, LOW);

      if (maintenant - nuitTimer >= 1000) {
        stopAll();
        nuitTimer = maintenant;
        nuitState = NUIT_PAUSE_BEFORE_RETRACT;
        Serial.println("Nuit: pause before retracting...");
      }
      break;
  }
}
