# Référence Code K3NG - Easycom et Moteurs TB6600

Ce fichier contient le code de référence fonctionnel et testé pour :
- Parseur Easycom compatible K3NG/PstRotator
- Gestion moteurs pas-à-pas via drivers TB6600
- Lecture encodeurs SSI HH-12

**Code testé OK par ON7KGK - Janvier 2026**

---

## Code Complet Fonctionnel

```cpp
#include <Arduino.h>
#include <EEPROM.h>

// --- CONFIGURATION SENS DE ROTATION ---
const bool REVERSE_AZ = false;
const bool REVERSE_EL = false;

// --- CONFIGURATION MÉCANIQUE ---
const float GEAR_RATIO_AZ = 1.0;
const float GEAR_RATIO_EL = 1.0;
const int   STEPS_PER_REV_MOTOR = 1600;
const int   MANUAL_STEP_SIZE = 20;

// --- RÉGLAGES VITESSE ULTRA-LENTE (Délai µs) ---
int speedMax  = 400;
int speedSlow = 4000;  //8000

// --- PINS MOTEURS ---
#define AZ_STEP 9
#define AZ_DIR  8
#define EL_STEP 13
#define EL_DIR  A2

// --- PINS CAPTEURS (SSI) ---
#define SSI_CLK      10
#define SSI_DATA_AZ  11
#define SSI_DATA_EL  A3
#define SSI_CS_AZ    A0
#define SSI_CS_EL    A1

// --- PINS SÉCURITÉ NC & BOUTONS ---
#define LIMIT_AZ    2
#define LIMIT_EL    3
#define BTN_CW      4
#define BTN_CCW     5
#define BTN_UP      6
#define BTN_DOWN    7
#define BTN_STOP    12

// --- VARIABLES ---
float targetAz = -1.0, targetEl = -1.0;
float currentAz = 0.0, currentEl = 0.0;
long turnsAz = 0, offsetStepsAz = 0, offsetStepsEl = 0;
int rawAz = 0, prevRawAz = 0, rawEl = 0;
unsigned long lastRead = 0;

// --- FONCTION LECTURE SSI ---
int readSSIRaw(int csPin, int dataPin, bool reverse) {
  unsigned long data = 0;
  digitalWrite(csPin, LOW); delayMicroseconds(5);
  for (int i = 0; i < 18; i++) {
    digitalWrite(SSI_CLK, LOW); delayMicroseconds(5);
    digitalWrite(SSI_CLK, HIGH); delayMicroseconds(5);
    if (i < 12 && digitalRead(dataPin)) data |= (1L << (11 - i));
  }
  digitalWrite(csPin, HIGH);
  int val = (int)data;
  return reverse ? (4095 - val) : val;
}

void setup() {
  Serial.begin(9600);
  pinMode(AZ_STEP, OUTPUT); pinMode(AZ_DIR, OUTPUT);
  pinMode(EL_STEP, OUTPUT); pinMode(EL_DIR, OUTPUT);
  pinMode(SSI_CLK, OUTPUT); pinMode(SSI_DATA_AZ, INPUT); pinMode(SSI_DATA_EL, INPUT);
  pinMode(SSI_CS_AZ, OUTPUT); pinMode(SSI_CS_EL, OUTPUT);

  pinMode(LIMIT_AZ, INPUT_PULLUP); pinMode(LIMIT_EL, INPUT_PULLUP);
  pinMode(BTN_CW, INPUT_PULLUP); pinMode(BTN_CCW, INPUT_PULLUP);
  pinMode(BTN_UP, INPUT_PULLUP); pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_STOP, INPUT_PULLUP);

  digitalWrite(SSI_CS_AZ, HIGH); digitalWrite(SSI_CS_EL, HIGH); digitalWrite(SSI_CLK, HIGH);

  EEPROM.get(0, turnsAz);
  EEPROM.get(8, offsetStepsAz);
  EEPROM.get(16, offsetStepsEl);

  if (turnsAz == -1 || (unsigned long)turnsAz == 4294967295) turnsAz = 0;
  if (offsetStepsAz == -1 || (unsigned long)offsetStepsAz == 4294967295) offsetStepsAz = 0;
  if (offsetStepsEl == -1 || (unsigned long)offsetStepsEl == 4294967295) offsetStepsEl = 0;
}

void doStep(int pinStep, int pinDir, int dir, int delayUs, int limitPin) {
  if (digitalRead(limitPin) == HIGH) return;  // SÉCURITÉ: Fin de course NC
  digitalWrite(pinDir, dir);
  digitalWrite(pinStep, HIGH);
  delayMicroseconds(delayUs);
  digitalWrite(pinStep, LOW);
  delayMicroseconds(delayUs);
}

void loop() {
  // 1. LECTURE CAPTEURS (150ms)
  if (millis() - lastRead > 150) {
    lastRead = millis();
    rawAz = readSSIRaw(SSI_CS_AZ, SSI_DATA_AZ, REVERSE_AZ);
    int deltaAz = rawAz - prevRawAz;
    if (deltaAz > 2048) turnsAz--; else if (deltaAz < -2048) turnsAz++;
    prevRawAz = rawAz;
    long currentStepsAz = (turnsAz * 4096L) + rawAz - offsetStepsAz;
    currentAz = (float)currentStepsAz * 360.0 / (4096.0 * GEAR_RATIO_AZ);
    while (currentAz < 0) currentAz += 360.0; while (currentAz >= 360) currentAz -= 360.0;

    rawEl = readSSIRaw(SSI_CS_EL, SSI_DATA_EL, REVERSE_EL);
    long currentStepsEl = (long)rawEl - offsetStepsEl;
    currentEl = (float)currentStepsEl * 90.0 / 4095.0;
  }

  // 2. BOUTONS PHYSIQUES
  if (digitalRead(BTN_STOP) == LOW) { targetAz = -1.0; targetEl = -1.0; }
  if (digitalRead(BTN_CW) == LOW)  { for(int i=0; i<MANUAL_STEP_SIZE; i++) doStep(AZ_STEP, AZ_DIR, HIGH, speedSlow, LIMIT_AZ); }
  if (digitalRead(BTN_CCW) == LOW) { for(int i=0; i<MANUAL_STEP_SIZE; i++) doStep(AZ_STEP, AZ_DIR, LOW,  speedSlow, LIMIT_AZ); }
  if (digitalRead(BTN_UP) == LOW)  { for(int i=0; i<MANUAL_STEP_SIZE; i++) doStep(EL_STEP, EL_DIR, HIGH, speedSlow, LIMIT_EL); }
  if (digitalRead(BTN_DOWN) == LOW){ for(int i=0; i<MANUAL_STEP_SIZE; i++) doStep(EL_STEP, EL_DIR, LOW,  speedSlow, LIMIT_EL); }

  // 3. ASSERVISSEMENT AUTO
  if (targetAz >= 0) {
    float errAz = targetAz - currentAz;
    if (errAz > 180) errAz -= 360; if (errAz < -180) errAz += 360;
    if (abs(errAz) > 0.08) doStep(AZ_STEP, AZ_DIR, errAz > 0 ? HIGH : LOW, (abs(errAz) > 3.0 ? speedMax : speedSlow), LIMIT_AZ);
    else targetAz = -1.0;
  }
  if (targetEl >= 0) {
    float errEl = targetEl - currentEl;
    if (abs(errEl) > 0.08) doStep(EL_STEP, EL_DIR, errEl > 0 ? HIGH : LOW, (abs(errEl) > 3.0 ? speedMax : speedSlow), LIMIT_EL);
    else targetEl = -1.0;
  }

  // 4. COMMUNICATION PSTRotator & STOP LOGICIEL
  if (Serial.available() > 0) {
    String line = Serial.readStringUntil('\r');
    line.toUpperCase(); line.trim();

    // Arrêt d'urgence explicite
    if (line == "S" || line == "SA" || line == "SE" || line == "STOP") {
      targetAz = -1.0; targetEl = -1.0;
    }

    // Calibration
    if (line.startsWith("Z") && line.length() > 1) {
      float cal = line.substring(1).toFloat();
      offsetStepsAz = (turnsAz * 4096L) + rawAz - (long)(cal * 4096.0 * GEAR_RATIO_AZ / 360.0);
      EEPROM.put(8, offsetStepsAz);
    }
    if (line.startsWith("S") && line.length() > 1 && isDigit(line[1])) {
      float cal = line.substring(1).toFloat();
      offsetStepsEl = (long)rawEl - (long)(cal * 4095.0 / 90.0);
      EEPROM.put(16, offsetStepsEl);
    }

    // Gestion intelligente des cibles (Filtre le Stop par position)
    if (line.indexOf("AZ") != -1) {
      int p = line.indexOf("AZ"); String v = "";
      for (int i=p+2; i<line.length() && (isDigit(line[i])||line[i]=='.'||line[i]=='-'); i++) v+=line[i];
      if (v.length() > 0) {
        float nt = v.toFloat();
        targetAz = (abs(nt - currentAz) < 0.15) ? -1.0 : nt;
      }
    }
    if (line.indexOf("EL") != -1) {
      int p = line.indexOf("EL"); String v = "";
      for (int i=p+2; i<line.length() && (isDigit(line[i])||line[i]=='.'||line[i]=='-'); i++) v+=line[i];
      if (v.length() > 0) {
        float nt = v.toFloat();
        targetEl = (abs(nt - currentEl) < 0.15) ? -1.0 : nt;
      }
    }

    // Feedback PSTRotator
    Serial.print("AZ"); Serial.print(currentAz, 1);
    Serial.print(" EL"); Serial.print(currentEl, 1); Serial.print("\r\n");
  }
}
```

---

## Points Clés du Code

### 1. Sécurité Fins de Course
```cpp
void doStep(..., int limitPin) {
  if (digitalRead(limitPin) == HIGH) return;  // NC: HIGH = OK, LOW = déclenché
  // ... step
}
```
- Fins de course **NC (Normally Closed)** en série
- HIGH = circuit fermé = OK pour bouger
- LOW = circuit ouvert = FIN DE COURSE → STOP

### 2. Calcul Position Azimuth (multi-tours)
```cpp
int deltaAz = rawAz - prevRawAz;
if (deltaAz > 2048) turnsAz--;      // Passage 4095→0 (sens -)
else if (deltaAz < -2048) turnsAz++; // Passage 0→4095 (sens +)

long currentStepsAz = (turnsAz * 4096L) + rawAz - offsetStepsAz;
currentAz = (float)currentStepsAz * 360.0 / (4096.0 * GEAR_RATIO_AZ);
```

### 3. Calcul Position Élévation (0-90°)
```cpp
long currentStepsEl = (long)rawEl - offsetStepsEl;
currentEl = (float)currentStepsEl * 90.0 / 4095.0;
```
- Mapping direct 0-4095 → 0-90°

### 4. Asservissement Position
```cpp
float errAz = targetAz - currentAz;
if (errAz > 180) errAz -= 360;   // Normalisation chemin court
if (errAz < -180) errAz += 360;

if (abs(errAz) > 0.08)  // Tolérance 0.08°
  doStep(..., abs(errAz) > 3.0 ? speedMax : speedSlow, ...);
else
  targetAz = -1.0;  // Position atteinte
```

### 5. Filtre Anti-Vibration PstRotator
```cpp
targetAz = (abs(nt - currentAz) < 0.15) ? -1.0 : nt;
```
- Si nouvelle cible < 0.15° de la position actuelle → ignorer

### 6. Calibration
- **Z123.5** : Calibre azimuth à 123.5°
- **S45.0** : Calibre élévation à 45.0° (S suivi d'un chiffre)

### 7. Réponse Easycom
```cpp
Serial.print("AZ"); Serial.print(currentAz, 1);
Serial.print(" EL"); Serial.print(currentEl, 1); Serial.print("\r\n");
```
- Format: `AZ123.5 EL45.0\r\n`
- Envoyé après CHAQUE commande reçue

---

## Configuration Matérielle

### TB6600 DIP Switches
| SW1 | SW2 | SW3 | Microstepping |
|-----|-----|-----|---------------|
| ON  | ON  | OFF | 1/8 (1600 steps/rev) |

### Câblage Fins de Course NC
```
       Arduino Pin 2 (LIMIT_AZ)
            │
            │
    ┌───────┴───────┐
    │               │
   SW1             SW2
  (CW_NC)        (CCW_NC)
    │               │
    └───────┬───────┘
            │
           GND
```
- En série : si UN switch s'ouvre → circuit ouvert → pin = HIGH
- INPUT_PULLUP : HIGH = limite atteinte

---

*Fichier créé le 2026-01-31 par ON7KGK*
