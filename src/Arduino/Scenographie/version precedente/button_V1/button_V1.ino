const int buttonPin1 = 9;  // the number of the pushbutton pin
const int buttonPin2 = A5;  // the number of the pushbutton pin
const int RPWM1 = 5; // Marche Avant
const int LPWM1 = 6; // Marche Arrière
const int R_EN1 = 7; // Enable Droite
const int L_EN1 = 8; // Enable Gauche
const int RPWM2 = A1; // Marche Avant
const int LPWM2 = A2; // Marche Arrière
const int R_EN2 = A3; // Enable Droite
const int L_EN2 = A4; // Enable Gauche


// variables will change:
int buttonState1 = 0;  // variable for reading the pushbutton status
int buttonState2 = 0;

void setup() {

  Serial.begin(9600); // Pour le débogage sur l'écran PC
  
  pinMode(RPWM1, OUTPUT);
  pinMode(LPWM1, OUTPUT);
  pinMode(R_EN1, OUTPUT);
  pinMode(L_EN1, OUTPUT);
  pinMode(RPWM2, OUTPUT);
  pinMode(LPWM2, OUTPUT);
  pinMode(R_EN2, OUTPUT);
  pinMode(L_EN2, OUTPUT);
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);

  digitalWrite(R_EN1, HIGH);
  digitalWrite(L_EN1, HIGH);
  digitalWrite(R_EN2, HIGH);
  digitalWrite(L_EN2, HIGH);
 
  // On s'assure que tout est à l'arrêt au début
  digitalWrite(RPWM1, LOW);
  digitalWrite(LPWM1, LOW);
  digitalWrite(RPWM2, LOW);
  digitalWrite(LPWM2, LOW);
  
}

void loop() {
  // read the state of the pushbutton value:
  
  buttonState1 = digitalRead(buttonPin1);
  buttonState2 = digitalRead(buttonPin2);
  Serial.println(buttonState1, buttonState2);

  // check if the pushbutton is pressed. If it is, the buttonState is 0:
  if (buttonState1 == 0) {
  
  digitalWrite(RPWM1, LOW);
  digitalWrite(LPWM1, HIGH);
  delay (1400);
  }  

  else if (buttonState1 == 1) {
  digitalWrite(RPWM1, HIGH);
  digitalWrite(LPWM1, LOW);
  }

  if (buttonState2 == 0) {
  
  digitalWrite(RPWM2, LOW);
  digitalWrite(LPWM2, HIGH);
  delay (1400);
  }  
  
  else if (buttonState2 == 1) {
  digitalWrite(RPWM2, HIGH);
  digitalWrite(LPWM2, LOW);
  }
}


