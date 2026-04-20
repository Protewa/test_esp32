#include <Arduino.h>

// Pins voor ESP32-C6
const int PWM_PIN = 19;
const int STROOM_PIN = 0;   
const int SPANNING_PIN = 1; 
const int KNOP_UP = 5;
const int KNOP_DOWN = 6;

// PWM Instellingen
const int freq = 1000;
const int resolutie = 12;

// Variabelen voor berekening
int setpoint = 0;
const float weerstand = 1.49;

// Variabelen voor Non-blocking Debounce
unsigned long laatsteKnopCheck = 0;
const int debounceTijd = 150; 

void setup() {
  Serial.begin(115200);
  
  while (!Serial && millis() < 3000);

  ledcAttach(PWM_PIN, freq, resolutie);

  pinMode(KNOP_UP, INPUT_PULLUP);
  pinMode(KNOP_DOWN, INPUT_PULLUP);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println("\n--- Systeem Gestart ---");
  Serial.println("Bediening: Knoppen of typ een getal in de Serial Monitor");
}

void loop() {
  // --- 1. Seriële Input Lezen ---
  if (Serial.available() > 0) {
    // Lees het getal dat in de seriële monitor is getypt
    int nieuweSetpoint = Serial.parseInt();
    
    // Alleen aanpassen als er echt iets getypt is (parseInt geeft 0 bij timeout/fout)
    // We checken ook op het newline karakter om 'lege' 0-metingen te filteren
    if (nieuweSetpoint >= 0) {
      setpoint = nieuweSetpoint;
      Serial.print("Setpoint aangepast via Serial naar: ");
      Serial.println(setpoint);
    }
    
    // Buffer legen (verwijdert newline/enter karakters)
    while(Serial.available() > 0) Serial.read();
  }

  // --- 2. Debounce Logica voor Knoppen ---
  if (millis() - laatsteKnopCheck >= debounceTijd) {
    if (digitalRead(KNOP_UP) == LOW) {
      setpoint++;
      laatsteKnopCheck = millis();
    }
    
    if (digitalRead(KNOP_DOWN) == LOW) {
      setpoint--;
      if (setpoint < 0) setpoint = 0;
      laatsteKnopCheck = millis();
    }
  }

  // --- 3. Metingen en Berekeningen ---
  int raw_spanning = analogRead(SPANNING_PIN);
  int pwm_eind = 0;

  if (setpoint > 0 && raw_spanning > 100) {
    // Wet van Ohm berekening: PWM = (Max_PWM * I_gewenst * R) / V_bron
    float pwm_calc = (4095.0f * (float)setpoint * weerstand) / (float)raw_spanning;
    pwm_eind = constrain(static_cast<int>(round(pwm_calc)), 0, 4095);
  }

  // --- 4. Uitvoer ---
  ledcWrite(PWM_PIN, pwm_eind);

  // --- 5. Seriële Monitor Log ---
  static unsigned long laatsteLog = 0;
  if (millis() - laatsteLog > 500) {
    Serial.printf("Setpoint: %d | ADC Spanning: %d | PWM Output: %d\n", setpoint, raw_spanning, pwm_eind);
    laatsteLog = millis();
  }
}