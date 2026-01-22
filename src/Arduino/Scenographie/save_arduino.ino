#include <Adafruit_NeoPixel.h>

// Configuration des LED strips
const int nbofledsStrip1 = 80, nbofledsStrip2 = 80, nbofledsStrip3 = 120;
const int pinStrip1 = 2, pinStrip2 = 3, pinStrip3 = 4;
Adafruit_NeoPixel strip1(nbofledsStrip1, pinStrip1, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2(nbofledsStrip2, pinStrip2, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip3(nbofledsStrip3, pinStrip3, NEO_GRB + NEO_KHZ800);

// Variables pour contrôler les LEDs
uint8_t brightness = 255; // Luminosité des LEDs
int selectedSeq = 0;      // Gamme de couleurs selon l'UID RFID
float hue = 0;            // Teinte actuelle des LEDs
float defaultHue = 0;     // Couleur par défaut selon le vinyle

// Plages pour les sonars
const float minDistance = 0.0;  // Distance minimale (en cm)
const float maxDistance = 91.0; // Distance maximale (3 pieds en cm)

// Gestion du temps pour `millis()`
unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 50; // Intervalle de mise à jour en ms

void setup() {
  Serial1.begin(57600); // Communication série avec l'Arduino des servos et sonars

  // Initialisation des LED strips
  strip1.begin();
  strip2.begin();
  strip3.begin();
  applyColorToAllStrips(strip1.Color(0, 0, 0)); // Initialisation à noir

  // Debug sur Serial pour surveiller le comportement
  Serial.begin(9600);
  Serial.println("LEDs prêtes avec gestion à 3 pieds !");
}

void loop() {
  // Lire les données envoyées par l'Arduino des servos
  if (Serial1.available()) {
    String data = Serial1.readStringUntil('\n');
    if (data.length() > 0) { // Vérification si la chaîne n'est pas vide
      parseData(data);
    }
  }

  // Mettre à jour les LEDs à intervalles réguliers
  unsigned long currentTime = millis();
  if (currentTime - lastUpdateTime >= updateInterval) {
    lastUpdateTime = currentTime;
    updateLEDs();
  }
}

// Fonction pour interpréter les données série
void parseData(const String& data) {
  if (data.startsWith("vinyl:")) {
    int vinylIndex = data.indexOf(",sonar1:");
    int sonar1Index = data.indexOf(",sonar2:");

    String uid = data.substring(6, vinylIndex);                  // UID RFID
    float sonar1 = data.substring(vinylIndex + 8, sonar1Index).toFloat(); // Distance sonar 1
    float sonar2 = data.substring(sonar1Index + 8).toFloat();    // Distance sonar 2

    // Debug : Vérifiez les valeurs reçues
    Serial.print("UID: ");
    Serial.println(uid);
    Serial.print("Sonar 1: ");
    Serial.println(sonar1);
    Serial.print("Sonar 2: ");
    Serial.println(sonar2);

    // Assigner les plages HSV par défaut selon l'UID
    if (uid == "DC3F1237") {           // VINYLE ORANGE
      selectedSeq = 1;
      defaultHue = 12;
      hue = (sonar2 <= maxDistance) ? map(sonar2, minDistance, maxDistance, 30, 12) : defaultHue;
    } else if (uid == "FF23AA26") {   //VINYLE ROUGE
      selectedSeq = 2;
      defaultHue = 255; // Couleur par défaut : rouge
      hue = (sonar2 <= maxDistance) ? map(sonar2, minDistance, maxDistance, 225, 255) : defaultHue;
    } else if (uid == "934983E") {     // VINYLE VERT
      selectedSeq = 3;
      defaultHue = 100; 
      hue = (sonar2 <= maxDistance) ? map(sonar2, minDistance, maxDistance, 135, 100) : defaultHue;
    } else {
      hue = hue; // Conserver la couleur précédente si aucun vinyle détecté
    }

    // Ajuster la luminosité
    if (sonar1 > maxDistance) {
      brightness = 255; // Luminosité maximale au-delà de 3 pieds
    } else {
      brightness = constrain(map(sonar1, maxDistance, minDistance, 255, 50), 50, 255);
    }
  }
}

// Fonction pour mettre à jour les LEDs
void updateLEDs() {
  uint32_t color = determineColor();
  applyColorToAllStrips(color);
}

// Fonction pour calculer la couleur des LEDs
uint32_t determineColor() {
  return strip1.ColorHSV(hue * 256, 255, brightness);
}

// Fonction pour appliquer une couleur à tous les strips
void applyColorToAllStrips(uint32_t color) {
  strip1.fill(color);
  strip2.fill(color);
  strip3.fill(color);
  strip1.show();
  strip2.show();
  strip3.show();
}
