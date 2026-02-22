const int RPWM = A1; // Marche Avant
const int LPWM = A2; // Marche Arrière
const int R_EN = A3; // Enable Droite
const int L_EN = A4; // Enable Gauche

void setup() {
  Serial.begin(9600); // Pour le débogage sur l'écran PC
  
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);

  analogWrite(R_EN, 255);
  analogWrite(L_EN, 255);
 
  // On s'assure que tout est à l'arrêt au début
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);

}

void loop() {
  sortir();
delay(5000);

stopMoteur();
delay(1000);

rentrer();
delay(1400);

stopMoteur();
delay(5000);

}

void sortir() {
  analogWrite(RPWM, 255);
  analogWrite(LPWM, 0);
}

void rentrer() {
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 255);
}

void stopMoteur() {
  analogWrite(RPWM, 0);
  analogWrite(LPWM, 0);
}
