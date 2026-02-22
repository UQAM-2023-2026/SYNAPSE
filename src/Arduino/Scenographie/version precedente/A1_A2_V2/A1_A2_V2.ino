// Contrôle Actionneur - Version Fusionnée (OSC + Fin de course)
#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <OSCMessage.h>

// --- PINS ---
const int buttonPin1 = 9;
const int buttonPin2 = A5;

const int RPWMA1 = 5;   // Extend
const int LPWMA1 = 6;   // Retract
const int R_ENA1 = 7;
const int L_ENA1 = 8;

const int RPWMA2 = A1;  // Extend
const int LPWMA2 = A2;  // Retract
const int R_ENA2 = A3;
const int L_ENA2 = A4;

// --- ETHERNET / OSC ---
byte mac[] = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x01 };
IPAddress ip(10, 0, 2, 228);
EthernetUDP Udp;
const unsigned int localPort = 1111;

// --- TIMING ---
int pause = 500;
unsigned long lastOSCReception = 0;
const unsigned long timeoutNuit = 10000;

// --- MODES & STATES ---
bool modeNuit = false;

// ARRIERE no longer uses a fixed timer — it runs until the limit switch is hit
enum MouvementState { AVANT, PAUSE1, ARRIERE, PAUSE2 };
MouvementState mouvementState = AVANT;
unsigned long mouvementTimer = 0;

enum NuitState { NUIT_SORT, NUIT_ENTER };
NuitState nuitState = NUIT_SORT;
unsigned long nuitTimer = 0;

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
  digitalWrite(RPWMA1, LOW); digitalWrite(LPWMA1, HIGH);
  digitalWrite(RPWMA2, LOW); digitalWrite(LPWMA2, HIGH);

  // Wait until both buttons are hit before starting the cycle
  while (digitalRead(buttonPin1) == 1 || digitalRead(buttonPin2) == 1) {
    // keep retracting — stop whichever motor has already hit its switch
    if (digitalRead(buttonPin1) == 0) {
      digitalWrite(LPWMA1, LOW);
    }
    if (digitalRead(buttonPin2) == HIGH) {
      digitalWrite(LPWMA2, LOW);
    }
  }

  // Both fully retracted
  digitalWrite(LPWMA1, LOW);
  digitalWrite(LPWMA2, LOW);
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
// AVANT    → extend for 1500ms (fixed, no limit switch at the far end)
// PAUSE1   → wait
// ARRIERE  → retract until BOTH limit switches are hit (no timer)
// PAUSE2   → wait
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
        digitalWrite(LPWMA1, LOW);
        digitalWrite(LPWMA2, LOW);
        mouvementTimer = maintenant;
        mouvementState = PAUSE1;
        Serial.println("--- PAUSE 1 ---");
      }
      break;

    case PAUSE1:
      if (maintenant - mouvementTimer >= pause) {
        mouvementTimer = maintenant;
        mouvementState = ARRIERE;
        Serial.println("Mouvement: RETRACT (until limit switches)");
      }
      break;

    case ARRIERE:
      // Retract each motor independently — stop it the moment its switch is hit
      if (b1 == 1) { digitalWrite(RPWMA1, HIGH); digitalWrite(LPWMA1, LOW); }
      else           { digitalWrite(LPWMA1, HIGH); } // motor 1 has hit its switch — stop

      if (b2 == 1) { digitalWrite(RPWMA2, LOW); digitalWrite(LPWMA2, LOW); }
      else           { digitalWrite(LPWMA2, HIGH); } // motor 2 has hit its switch — stop

      // Only advance when BOTH are fully retracted
      if (b1 == 0 && b2 == 0) {
        mouvementTimer = maintenant;
        mouvementState = PAUSE2;
        Serial.println("--- PAUSE 2 (both retracted) ---");
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
// Same logic — retract until limit switches, then extend for 1000ms
// -------------------------------------------------------
void mouvementNuit() {
  unsigned long maintenant = millis();

  int b1 = digitalRead(buttonPin1);
  int b2 = digitalRead(buttonPin2);

  switch (nuitState) {

    case NUIT_SORT: // Retract until both limit switches hit
      if (b1 == 1) { digitalWrite(RPWMA1, HIGH); digitalWrite(LPWMA1, LOW); }
      else           { digitalWrite(LPWMA1, HIGH); }

      if (b2 == 1) { digitalWrite(RPWMA2, HIGH); digitalWrite(LPWMA2, LOW); }
      else           { digitalWrite(LPWMA2, HIGH); }

      if (b1 == 0 && b2 == 0) {
        nuitTimer = maintenant;
        nuitState = NUIT_ENTER;
        Serial.println("Nuit: both retracted, extending...");
      }
      break;

    case NUIT_ENTER: // Extend for 1000ms
      digitalWrite(LPWMA1, HIGH); digitalWrite(RPWMA1, LOW);
      digitalWrite(LPWMA2, HIGH); digitalWrite(RPWMA2, LOW);

      if (maintenant - nuitTimer >= 1000) {
        digitalWrite(LPWMA1, LOW);
        digitalWrite(LPWMA2, LOW);
        nuitTimer = maintenant;
        nuitState = NUIT_SORT;
        Serial.println("Nuit: retracting...");
      }
      break;
  }
}
