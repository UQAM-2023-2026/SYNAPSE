// Contrôle Actionneur - Version Complète et Directe
// Utilisation de digitalWrite pour assurer un signal fort (HIGH)
 
const int RPWM = 5; // Marche Avant
const int LPWM = 6; // Marche Arrière
const int R_EN = 7; // Enable Droite
const int L_EN = 8; // Enable Gauche
const int RPWMA2 = A1; // Marche Avant
const int LPWMA2 = A2; // Marche Arrière
const int R_ENA2 = A3; // Enable Droite
const int L_ENA2 = A4; // Enable Gauche
 
void setup() {
  Serial.begin(9600); // Pour le débogage sur l'écran PC
  
  pinMode(RPWM, OUTPUT);
  pinMode(LPWM, OUTPUT);
  pinMode(R_EN, OUTPUT);
  pinMode(L_EN, OUTPUT);
  pinMode(RPWMA2, OUTPUT);
  pinMode(LPWMA2, OUTPUT);
  pinMode(R_ENA2, OUTPUT);
  pinMode(L_ENA2, OUTPUT);
 
  // ÉTAPE CRUCIALE : On active le module (Enable)
  // Sans ça, le moteur ne recevra jamais de courant
  digitalWrite(R_EN, HIGH);
  digitalWrite(L_EN, HIGH);

  digitalWrite(R_ENA2, HIGH);
  digitalWrite(L_ENA2, HIGH);

  digitalWrite(LPWM, LOW);  // On coupe l'arrière
  digitalWrite(RPWM, HIGH); // On active l'avant (Pleine puissance)
  digitalWrite(LPWMA2, LOW);  // On coupe l'arrière
  digitalWrite(RPWMA2, HIGH);
  delay(5000);
 
  // On s'assure que tout est à l'arrêt au début
  digitalWrite(RPWM, LOW);
  digitalWrite(LPWM, LOW);

  digitalWrite(RPWMA2, LOW);
  digitalWrite(LPWMA2, LOW);
  
  Serial.println("Systeme pret - Demarrage dans 2 secondes");
  delay(2000);
}
 
void loop() {
  // --- SENS 1 ---
  Serial.println("Mouvement: SORTIR >>>");
  digitalWrite(LPWM, LOW);  // On coupe l'arrière
  digitalWrite(RPWM, HIGH); // On active l'avant (Pleine puissance)
  digitalWrite(LPWMA2, LOW);  // On coupe l'arrière
  digitalWrite(RPWMA2, HIGH);
  delay(1400);
 
  // --- STOP ---
  Serial.println("--- PAUSE ---");
  digitalWrite(RPWM, LOW);
  digitalWrite(LPWM, LOW);
  digitalWrite(RPWMA2, LOW);
  digitalWrite(LPWMA2, LOW);
  delay(1000);
 
  // --- SENS 2 ---
  Serial.println("Mouvement: RENTRER <<<");
  digitalWrite(RPWM, LOW);  // On coupe l'avant
  digitalWrite(LPWM, HIGH); // On active l'arrière (Pleine puissance)
  digitalWrite(RPWMA2, LOW);  // On coupe l'avant
  digitalWrite(LPWMA2, HIGH);
  delay(1400);
 
  // --- STOP ---
  Serial.println("--- PAUSE ---");
  digitalWrite(RPWM, LOW);
  digitalWrite(LPWM, LOW);
  digitalWrite(RPWMA2, LOW);
  digitalWrite(LPWMA2, LOW);
  delay(5000);
  }