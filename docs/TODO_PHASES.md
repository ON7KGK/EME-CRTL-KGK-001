# PLAN DE DÉVELOPPEMENT PAR PHASES - Station EME Controller

## Vue d'ensemble

Développement progressif en **6 phases** avec validation incrémentale. Chaque phase livre un système fonctionnel testable avant de passer à la suivante.

**Durée totale estimée** : 15-20 jours (développement actif)

---

## MÉTHODOLOGIE

### Approche Itérative

**Principe** : Construire couche par couche, tester à chaque étape
```
Phase 1 (Bases) → Test → ✓
    ↓
Phase 2 (Safety) → Test → ✓
    ↓
Phase 3 (Réseau) → Test → ✓
    ↓
Phase 4 (Custom) → Test → ✓
    ↓
Phase 5 (Python) → Test → ✓
    ↓
Phase 6 (Intégration) → Test → ✓
```

**Avantages** :
- Détection bugs précoce
- Validation fonctionnelle continue
- Pas de "big bang" final
- Rollback facile si problème

---

### Workflow par Bloc

**Pour chaque bloc** :

1. **Squelette code** fourni (structure + TODO)
2. **Implémentation** (remplir TODO)
3. **Compilation** (vérifier syntaxe)
4. **Upload** Arduino
5. **Test unitaire** (Serial Monitor)
6. **Validation** critères définis
7. **Commit Git** (sauvegarde)
8. **Passage bloc suivant**

---

## PHASE 1 : BASES MOTEURS (3-5 jours)

**Objectif** : Système minimal fonctionnel sans réseau

**Livrables** :
- Lecture encodeurs précise
- Contrôle moteurs PWM
- Feedback courant
- Fins de course
- PID position

---

### BLOC 1.1 : Lecture Encodeurs (J0-J1)

**Fonctionnalités** :
- Setup interrupts hardware (INT0-5)
- ISR (Interrupt Service Routine) quadrature
- Compteur global counts
- Conversion counts → degrés
- Direction CW/CCW

**Fichiers** : `encoder.h`, `encoder.cpp`

**Code squelette** :
```cpp
// encoder.h
#ifndef ENCODER_H
#define ENCODER_H

void setupEncoders();
float getAzimuthDegrees();
float getElevationDegrees();
void resetEncoderAz();
void resetEncoderEl();

#endif

// encoder.cpp
#include "encoder.h"

// === CONFIGURATION ===
#define ENC_AZ_A  2   // INT4
#define ENC_AZ_B  3   // INT5
#define ENC_EL_A  20  // INT1
#define ENC_EL_B  21  // INT0

#define COUNTS_PER_REV  6624  // 12 ppr × 552 reduction
#define DEGREES_PER_COUNT (360.0 / COUNTS_PER_REV)

// === VARIABLES GLOBALES ===
volatile long encoderAzCount = 0;
volatile long encoderElCount = 0;

// === ISR AZIMUTH ===
void azEncoderISR() {
  // TODO: Lire état A et B
  // TODO: Déterminer direction
  // TODO: Incrémenter/décrémenter count
}

// === ISR ÉLÉVATION ===
void elEncoderISR() {
  // TODO: Identique Az
}

// === SETUP ===
void setupEncoders() {
  pinMode(ENC_AZ_A, INPUT_PULLUP);
  pinMode(ENC_AZ_B, INPUT_PULLUP);
  pinMode(ENC_EL_A, INPUT_PULLUP);
  pinMode(ENC_EL_B, INPUT_PULLUP);
  
  attachInterrupt(digitalPinToInterrupt(ENC_AZ_A), azEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_AZ_B), azEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_EL_A), elEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC_EL_B), elEncoderISR, CHANGE);
}

// === CONVERSION ===
float getAzimuthDegrees() {
  long count = encoderAzCount;  // Lecture atomique
  float degrees = count * DEGREES_PER_COUNT;
  
  // TODO: Normaliser 0-360°
  return degrees;
}

float getElevationDegrees() {
  // TODO: Identique Az
}
```

**Tests** :
1. Upload code
2. Tourner moteur Az manuellement
3. Serial Monitor affiche counts qui changent
4. Vérifier : 1 tour moteur = 6624 counts
5. Vérifier : Sens rotation correct (CW → count augmente)

**Critères validation** :
- ✅ Counts incrémentent/décrémentent correctement
- ✅ Conversion degrés exacte (360° = 6624 counts)
- ✅ Pas de "jitter" ou sauts erratiques

---

### BLOC 1.2 : Contrôle Moteurs Basique (J1-J2)

**Fonctionnalités** :
- Setup MC33926 (pins, config)
- Fonctions setMotorAz/El(speed)
- Commandes Serial simples (test)

**Fichiers** : `motor.h`, `motor.cpp`

**Code squelette** :
```cpp
// motor.h
#ifndef MOTOR_H
#define MOTOR_H

void setupMotors();
void setMotorAz(int speed);   // -255 à +255
void setMotorEl(int speed);
void stopAllMotors();

#endif

// motor.cpp
#include "motor.h"

// === PINS (voir PINOUT.md) ===
#define M1_IN1  11
#define M1_IN2  12
#define M2_IN1  6
#define M2_IN2  7
// ... (autres pins)

void setupMotors() {
  // TODO: pinMode pour tous les pins
  // TODO: Configuration initiale MC33926
  // TODO: Enable drivers
}

void setMotorAz(int speed) {
  speed = constrain(speed, -255, 255);
  
  if (speed > 0) {
    // TODO: PWM CW
  } else if (speed < 0) {
    // TODO: PWM CCW
  } else {
    // TODO: Stop
  }
}
```

**Tests** :
1. Upload code
2. Serial : `az 100` → Moteur Az tourne CW lentement
3. Serial : `az -100` → Moteur Az tourne CCW
4. Serial : `az 0` → Moteur Az stop
5. Idem pour El

**Critères validation** :
- ✅ Moteurs tournent dans le bon sens
- ✅ Vitesse proportionnelle au PWM
- ✅ Stop immédiat sur commande 0

---

### BLOC 1.3 : Feedback Courant (J2)

**Fonctionnalités** :
- Lecture ADC (M1FB, M2FB)
- Conversion V → Ampères (0.525 V/A)
- Affichage Serial Monitor

**Fichiers** : `safety.h`, `safety.cpp` (première version)

**Code squelette** :
```cpp
// safety.h
float readCurrentAz();
float readCurrentEl();

// safety.cpp
#define M1_FB  A0
#define M2_FB  A1

float readCurrentAz() {
  int raw = analogRead(M1_FB);
  float voltage = raw * (5.0 / 1023.0);
  return voltage / 0.525;  // MC33926 gain
}

float readCurrentEl() {
  // TODO: Identique
}
```

**Tests** :
1. Moteur arrêté → Courant ~0A
2. Moteur PWM=100 → Courant ~0.5-1.0A
3. Moteur PWM=200 → Courant ~1.5-2.0A
4. Comparer avec pince ampèremétrique (validation)

**Critères validation** :
- ✅ Lecture cohérente (pas de bruit excessif)
- ✅ Valeurs attendues selon charge
- ✅ Pas de dérive dans le temps

---

### BLOC 1.4 : Fins de Course (J2-J3)

**Fonctionnalités** :
- Lecture VMA452 (pins 26-29)
- Détection LOW = fin course atteinte
- Arrêt automatique moteur concerné

**Fichiers** : `safety.cpp` (ajout)

**Code squelette** :
```cpp
// Pins fins de course
#define LIMIT_AZ_CW   26
#define LIMIT_AZ_CCW  27
#define LIMIT_EL_UP   28
#define LIMIT_EL_DOWN 29

void setupLimitSwitches() {
  pinMode(LIMIT_AZ_CW, INPUT_PULLUP);
  pinMode(LIMIT_AZ_CCW, INPUT_PULLUP);
  pinMode(LIMIT_EL_UP, INPUT_PULLUP);
  pinMode(LIMIT_EL_DOWN, INPUT_PULLUP);
}

bool checkLimitSwitches() {
  // TODO: Lire pins
  // TODO: Si Az CW = LOW et moteur tourne CW → Stop
  // TODO: Idem pour autres directions
  // Retourner true si limite atteinte
}
```

**Tests** :
1. Switches ouverts (pas pressés) → Pins = HIGH
2. Fermer manuellement switch Az CW → Pin 26 = LOW
3. Moteur Az tourne CW et atteint fin course → Stop automatique
4. Vérifier impossible de dépasser (protection active)

**Critères validation** :
- ✅ Détection immédiate (<50ms)
- ✅ Arrêt moteur garanti
- ✅ Possibilité repartir direction opposée

---

### BLOC 1.5 : PID Position (J3-J5)

**Fonctionnalités** :
- Implémentation PID classique
- Fonction goto(az, el)
- Asservissement continu
- Anti-windup intégral

**Fichiers** : `pid.h`, `pid.cpp`

**Code squelette** :
```cpp
// pid.h
#ifndef PID_H
#define PID_H

void setupPID();
void setTargetAz(float target);
void setTargetEl(float target);
void updatePID();  // Appeler 20ms

#endif

// pid.cpp
#include "pid.h"

// === PARAMÈTRES (à tuner) ===
float Kp_az = 2.0;
float Ki_az = 0.1;
float Kd_az = 0.5;

float Kp_el = 2.0;
float Ki_el = 0.1;
float Kd_el = 0.5;

// === VARIABLES ===
float targetAz = 0.0;
float targetEl = 0.0;

float integralAz = 0.0;
float integralEl = 0.0;

float lastErrorAz = 0.0;
float lastErrorEl = 0.0;

void setTargetAz(float target) {
  targetAz = target;
  integralAz = 0.0;  // Reset intégrale
}

void updatePID() {
  // === AZIMUTH ===
  float currentAz = getAzimuthDegrees();
  float errorAz = targetAz - currentAz;
  
  // TODO: Normaliser erreur (-180 à +180)
  
  // Proportionnel
  float P_az = Kp_az * errorAz;
  
  // Intégral (avec anti-windup)
  integralAz += errorAz * 0.02;  // dt = 20ms
  integralAz = constrain(integralAz, -50, 50);
  float I_az = Ki_az * integralAz;
  
  // Dérivé
  float D_az = Kd_az * (errorAz - lastErrorAz) / 0.02;
  lastErrorAz = errorAz;
  
  // Commande totale
  float output = P_az + I_az + D_az;
  output = constrain(output, -200, 200);  // Limiter vitesse
  
  // TODO: Fenêtre morte (si erreur < 0.2° → stop)
  
  setMotorAz((int)output);
  
  // === ÉLÉVATION (identique) ===
  // TODO
}
```

**Tests** :
1. Position actuelle = 0°
2. Commande goto Az 90°
3. Observer : Moteur démarre, ralentit en approchant, s'arrête à 90.0°
4. Pas d'overshoot (dépassement)
5. Temps stabilisation <10 secondes

**Critères validation** :
- ✅ Précision finale <0.2°
- ✅ Pas d'oscillations
- ✅ Temps réponse acceptable
- ✅ Stable même avec perturbations (vent simulé)

---

## PHASE 2 : SAFETY & AFFICHAGE (2-3 jours)

**Objectif** : Système sûr avec monitoring complet

**Livrables** :
- Monitoring températures
- Monitoring tensions
- Affichage OLED/Nextion
- Safety complète multi-niveaux

---

### BLOC 2.1 : Monitoring Températures (J5-J6)

**Fonctionnalités** :
- DS18B20 local rack (pin 30)
- Lecture 1-Wire
- Stockage valeurs

**Fichiers** : `sensors.h`, `sensors.cpp`

**Code squelette** :
```cpp
// sensors.h
#include <OneWire.h>
#include <DallasTemperature.h>

void setupSensors();
float getTempAmbient();

// sensors.cpp
#define ONE_WIRE_BUS  30

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setupSensors() {
  sensors.begin();
}

float getTempAmbient() {
  sensors.requestTemperatures();
  return sensors.getTempCByIndex(0);
}
```

**Tests** :
1. Température ambiante affichée (~20-25°C)
2. Souffler air chaud sur capteur → Température augmente
3. Réponse temps ~1 seconde (normal DS18B20)

---

### BLOC 2.2 : Monitoring Tensions (J6)

**Fonctionnalités** :
- Lecture ADC diviseurs (A2-A5)
- Conversion → tensions réelles
- Détection seuils (warning/error)

**Code squelette** :
```cpp
// sensors.cpp (ajout)
float readVoltage24() {
  int raw = analogRead(A2);
  float v = raw * (5.0 / 1023.0) * (59.0 / 12.0);  // Diviseur 47k/12k
  return v;
}

float readVoltage138() {
  // TODO: A3, diviseur 22k/12k
}

float readVoltage12() {
  // TODO: A4, diviseur 18k/12k
}

float readVoltage5() {
  // TODO: A5
}

bool checkVoltages() {
  float v24 = readVoltage24();
  
  if (v24 < 22.0 || v24 > 26.0) {
    Serial.println("WARNING: 24V out of range");
    return false;
  }
  
  // TODO: Idem pour autres tensions
  return true;
}
```

**Tests** :
1. Tensions affichées correspondent multimètre (±5%)
2. Calibration si nécessaire (facteur correctif)
3. Warning déclenché si déconnecte PSU

---

### BLOC 2.3 : Affichage Nextion (J6-J7)

**Fonctionnalités** :
- Communication UART Serial1
- Fonction nextion(cmd)
- Update Az/El/Status
- Mode veille

**Fichiers** : `display.h`, `display.cpp`

**Code squelette** :
```cpp
// display.h
void setupDisplay();
void updateDisplay();

// display.cpp
void setupDisplay() {
  Serial1.begin(9600);
  delay(500);
  
  nextion("page 0");
  nextion("thsp=15");  // Veille 15s
  nextion("thup=1");   // Wake on touch
  nextion("sleep=1");
}

void nextion(String cmd) {
  Serial1.print(cmd);
  Serial1.write(0xFF);
  Serial1.write(0xFF);
  Serial1.write(0xFF);
}

void updateDisplay() {
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate < 1000) return;
  lastUpdate = millis();
  
  float az = getAzimuthDegrees();
  float el = getElevationDegrees();
  
  nextion("t_az.txt=\"" + String(az, 1) + "°\"");
  nextion("t_el.txt=\"" + String(el, 1) + "°\"");
  
  // TODO: Status
}
```

**Tests** :
1. Écran affiche Az/El
2. Valeurs mettent à jour (1 Hz)
3. Écran s'éteint après 15s
4. Touch réveille écran

---

### BLOC 2.4 : Safety Complète (J7-J8)

**Fonctionnalités** :
- Intégration tous checks
- Seuils progressifs
- Emergency stop global
- Logging événements

**Code squelette** :
```cpp
// safety.cpp (version complète)

#define CURRENT_NORMAL   1.8
#define CURRENT_WARNING  2.2
#define CURRENT_EMERGENCY 2.8

bool systemError = false;

void monitorSafety() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 50) return;  // 20 Hz
  lastCheck = millis();
  
  // === COURANTS ===
  float iAz = readCurrentAz();
  float iEl = readCurrentEl();
  
  if (iAz > CURRENT_EMERGENCY || iEl > CURRENT_EMERGENCY) {
    emergencyStop("Overcurrent");
    return;
  }
  
  if (iAz > CURRENT_WARNING || iEl > CURRENT_WARNING) {
    Serial.println("WARNING: High current");
  }
  
  // === FINS DE COURSE ===
  if (checkLimitSwitches()) {
    // Déjà géré dans fonction
  }
  
  // === TEMPÉRATURES ===
  float temp = getTempAmbient();
  if (temp > 85.0) {
    emergencyStop("High temperature");
  }
  
  // === TENSIONS ===
  if (!checkVoltages()) {
    // Warning seulement (pas emergency)
  }
  
  // === TIMEOUT MOUVEMENT ===
  // TODO: Si PWM actif >5s sans changement encodeur
}

void emergencyStop(String reason) {
  stopAllMotors();
  
  digitalWrite(M1_D1, HIGH);
  digitalWrite(M1_D2, HIGH);
  digitalWrite(M2_D1, HIGH);
  digitalWrite(M2_D2, HIGH);
  digitalWrite(MC_EN, LOW);
  
  systemError = true;
  
  Serial.print("EMERGENCY STOP: ");
  Serial.println(reason);
  
  // TODO: Log EEPROM
  // TODO: Alert réseau
}
```

**Tests** :
1. Simuler surintensité (bloquer moteur) → Emergency stop
2. Simuler fin course → Stop direction concernée
3. Vérifier impossible de redémarrer sans reset

---

## PHASE 3 : RÉSEAU EASYCOM (2-3 jours)

**Objectif** : PstRotator fonctionnel

**Livrables** :
- Ethernet de base
- Serveur TCP 4533
- Protocole Easycom complet
- Tracking lune opérationnel

---

### BLOC 3.1 : Ethernet de Base (J8-J9)

**Fonctionnalités** :
- Init W5500
- Configuration IP
- Ping test

**Fichiers** : `network.h`, `network.cpp`

**Code squelette** :
```cpp
// network.h
#include <Ethernet.h>

void setupNetwork();
void handleNetwork();

// network.cpp
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 177);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

void setupNetwork() {
  pinMode(53, OUTPUT);  // SS doit être OUTPUT
  
  Ethernet.begin(mac, ip, gateway, subnet);
  
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());
}
```

**Tests** :
1. Upload code
2. Connecter câble Ethernet
3. LED link W5500 allumée
4. Depuis PC : `ping 192.168.1.177`
5. Réponse OK

---

### BLOC 3.2 : Serveur TCP 4533 (J9-J10)

**Fonctionnalités** :
- Écoute port 4533
- Acceptation client
- Lecture commandes

**Code squelette** :
```cpp
// network.cpp (ajout)
EthernetServer server(4533);
EthernetClient client;

void setupNetwork() {
  // ... (init existante)
  server.begin();
  Serial.println("Server started on port 4533");
}

void handleNetwork() {
  // Accepter nouveau client
  EthernetClient newClient = server.available();
  if (newClient) {
    if (!client || !client.connected()) {
      client = newClient;
      Serial.println("Client connected");
    }
  }
  
  // Lire données client
  if (client && client.connected()) {
    if (client.available()) {
      String cmd = client.readStringUntil('\n');
      cmd.trim();
      processEasycomCommand(cmd);
    }
  }
}
```

**Tests** :
1. Depuis PC : `telnet 192.168.1.177 4533`
2. Connexion établie
3. Taper commande → Mega reçoit (Serial Monitor)

---

### BLOC 3.3 : Protocole Easycom (J10-J11)

**Fonctionnalités** :
- Parser commandes (AZ, EL, SA, SE...)
- Générer réponses (+0###.#)
- Format correct

**Fichiers** : `easycom.h`, `easycom.cpp`

**Code squelette** :
```cpp
// easycom.cpp
void processEasycomCommand(String cmd) {
  if (cmd.startsWith("AZ") && cmd.length() > 2) {
    // Set azimuth
    float target = cmd.substring(2).toFloat();
    setTargetAz(target);
    
  } else if (cmd == "AZ") {
    // Query azimuth
    float az = getAzimuthDegrees();
    char response[16];
    snprintf(response, sizeof(response), "+0%06.1f\r\n", az);
    client.print(response);
    
  } else if (cmd.startsWith("EL")) {
    // TODO: Identique Az
    
  } else if (cmd == "SA") {
    // Stop azimuth
    setTargetAz(getAzimuthDegrees());  // Target = position actuelle
    
  } else if (cmd == "SE") {
    // Stop élévation
    // TODO
  }
}
```

**Tests** :
1. Telnet : `AZ180.5` → Moteur va à 180.5°
2. Telnet : `AZ` → Réponse `+0180.5`
3. Telnet : `EL45.0` puis `EL` → `+045.0`
4. Telnet : `SA` → Moteur Az s'arrête

---

### BLOC 3.4 : Intégration PstRotator (J11-J12)

**Fonctionnalités** :
- PstRotator se connecte
- Tracking lune fonctionne
- Commandes fluides

**Tests** :
1. Lancer PstRotator
2. Configurer : IP 192.168.1.177, Port 4533, Protocol Easycom
3. Connect → Status OK
4. Target Moon → Tracking actif
5. Observer Az/El suivent lune

**Critères validation** :
- ✅ Connexion stable >10 minutes
- ✅ Tracking précis (<0.5° erreur)
- ✅ Pas de déconnexions
- ✅ Réponse fluide aux commandes

---

## PHASE 4 : RÉSEAU CUSTOM (2-3 jours)

**Objectif** : Python app contrôle manuel

**Livrables** :
- Serveur TCP 5000
- Protocole JSON
- Télémétrie 10 Hz
- Lock arbitrage

---

### BLOC 4.1 : Serveur TCP 5000 (J12-J13)

**Fonctionnalités** :
- Multi-sockets W5500
- Port 5000 indépendant
- Coexistence avec 4533

**Code squelette** :
```cpp
// network.cpp (ajout)
EthernetServer serverEasycom(4533);
EthernetServer serverCustom(5000);

EthernetClient clientEasycom;
EthernetClient clientCustom;

void handleNetwork() {
  // Socket 0 : Easycom
  handleEasycomSocket();
  
  // Socket 1 : Custom
  handleCustomSocket();
}

void handleCustomSocket() {
  EthernetClient newClient = serverCustom.available();
  if (newClient) {
    if (!clientCustom || !clientCustom.connected()) {
      clientCustom = newClient;
      Serial.println("Custom client connected");
    }
  }
  
  if (clientCustom && clientCustom.connected()) {
    if (clientCustom.available()) {
      String cmd = clientCustom.readStringUntil('\n');
      processCustomCommand(cmd);
    }
  }
}
```

**Tests** :
1. Telnet 192.168.1.177 4533 → Easycom OK
2. Telnet 192.168.1.177 5000 → Custom OK
3. Les 2 simultanément → Pas d'interférence

---

### BLOC 4.2 : Protocole Custom JSON (J13-J14)

**Fonctionnalités** :
- Parser JSON (ArduinoJson library)
- Commandes lock/unlock/jog/goto/status
- Réponses JSON

**Code squelette** :
```cpp
#include <ArduinoJson.h>

void processCustomCommand(String cmd) {
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, cmd);
  
  if (error) {
    Serial.println("JSON parse error");
    return;
  }
  
  const char* command = doc["cmd"];
  
  if (strcmp(command, "lock") == 0) {
    if (requestLock("python")) {
      sendJsonResponse("{\"status\":\"locked\"}");
    } else {
      sendJsonResponse("{\"status\":\"denied\"}");
    }
    
  } else if (strcmp(command, "jog") == 0) {
    if (controlOwner == "python") {
      const char* axis = doc["axis"];
      const char* dir = doc["dir"];
      // TODO: Jog moteur
    }
    
  } else if (strcmp(command, "goto") == 0) {
    // TODO
  }
}

void sendJsonResponse(String json) {
  clientCustom.println(json);
}
```

**Tests** :
1. `{"cmd":"lock"}` → Réponse `{"status":"locked"}`
2. `{"cmd":"jog","axis":"az","dir":"cw"}` → Moteur tourne
3. `{"cmd":"unlock"}` → Contrôle libéré

---

### BLOC 4.3 : Télémétrie Broadcast (J14)

**Fonctionnalités** :
- Envoi automatique 10 Hz
- Format JSON complet
- Tous clients reçoivent

**Code squelette** :
```cpp
void sendTelemetry() {
  static unsigned long lastSend = 0;
  if (millis() - lastSend < 100) return;  // 10 Hz
  lastSend = millis();
  
  StaticJsonDocument<512> doc;
  
  doc["az"] = getAzimuthDegrees();
  doc["el"] = getElevationDegrees();
  doc["target_az"] = targetAz;
  doc["target_el"] = targetEl;
  doc["i_az"] = readCurrentAz();
  doc["i_el"] = readCurrentEl();
  doc["t_amb"] = getTempAmbient();
  doc["v_24"] = readVoltage24();
  doc["control"] = controlOwner;
  doc["state"] = getStateString();
  
  JsonArray alerts = doc.createNestedArray("alerts");
  // TODO: Ajouter alertes actives
  
  String output;
  serializeJson(doc, output);
  
  if (clientCustom && clientCustom.connected()) {
    clientCustom.println(output);
  }
}
```

---

### BLOC 4.4 : Arbitrage Lock (J14-J15)

**Fonctionnalités** :
- États système (IDLE, PSTROTATOR, MANUAL, ERROR)
- Lock exclusif Python
- Timeout auto-release

**Code squelette** :
```cpp
enum SystemState {
  IDLE,
  PSTROTATOR_ACTIVE,
  MANUAL_ACTIVE,
  ERROR_STATE
};

SystemState systemState = IDLE;
String controlOwner = "";
unsigned long lockTimestamp = 0;

bool requestLock(String requester) {
  if (systemState == IDLE || systemState == PSTROTATOR_ACTIVE) {
    systemState = MANUAL_ACTIVE;
    controlOwner = requester;
    lockTimestamp = millis();
    return true;
  }
  return false;
}

void releaseLock() {
  systemState = PSTROTATOR_ACTIVE;  // Ou IDLE si pas de client
  controlOwner = "";
}

void checkLockTimeout() {
  if (systemState == MANUAL_ACTIVE) {
    if (millis() - lockTimestamp > 60000) {  // 60s timeout
      Serial.println("Lock timeout - auto release");
      releaseLock();
    }
  }
}
```

---

## PHASE 5 : APPLICATION PYTHON PYQT5 (3-5 jours)

**Objectif** : Interface complète monitoring/contrôle

**Livrables** :
- Fenêtre PyQt5 fonctionnelle
- Connexion TCP custom
- Affichage télémétrie temps réel
- Contrôle manuel (boutons)
- Graphes (optionnel phase 5)

---

### BLOC 5.1 : Fenêtre Basique (J15-J16)

**Fonctionnalités** :
- Connexion TCP socket
- Affichage Az/El
- Déconnexion propre

**Fichier** : `tracker_app.py`

**Code squelette** :
```python
import sys
import socket
import json
from PyQt5.QtWidgets import *
from PyQt5.QtCore import *

class TrackerApp(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("EME Tracker Controller")
        self.setGeometry(100, 100, 800, 600)
        
        self.socket = None
        self.connected = False
        
        self.initUI()
        
    def initUI(self):
        central = QWidget()
        self.setCentralWidget(central)
        layout = QVBoxLayout(central)
        
        # Labels Az/El
        self.labelAz = QLabel("Azimuth: ---")
        self.labelEl = QLabel("Elevation: ---")
        layout.addWidget(self.labelAz)
        layout.addWidget(self.labelEl)
        
        # Bouton connexion
        self.btnConnect = QPushButton("Connect")
        self.btnConnect.clicked.connect(self.connect)
        layout.addWidget(self.btnConnect)
        
    def connect(self):
        try:
            self.socket = socket.socket()
            self.socket.connect(("192.168.1.177", 5000))
            self.connected = True
            self.btnConnect.setText("Connected")
            
            # Thread réception
            self.thread = ReceiverThread(self.socket)
            self.thread.dataReceived.connect(self.updateData)
            self.thread.start()
            
        except Exception as e:
            QMessageBox.critical(self, "Error", str(e))
    
    def updateData(self, data):
        self.labelAz.setText(f"Azimuth: {data['az']:.1f}°")
        self.labelEl.setText(f"Elevation: {data['el']:.1f}°")

class ReceiverThread(QThread):
    dataReceived = pyqtSignal(dict)
    
    def __init__(self, sock):
        super().__init__()
        self.sock = sock
        
    def run(self):
        while True:
            try:
                line = self.sock.recv(1024).decode()
                data = json.loads(line)
                self.dataReceived.emit(data)
            except:
                break

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = TrackerApp()
    window.show()
    sys.exit(app.exec_())
```

**Tests** :
1. Lancer app
2. Cliquer "Connect"
3. Az/El affichés et mettent à jour

---

### BLOC 5.2 : Boutons Contrôle (J16-J17)

**Fonctionnalités** :
- Boutons CW/CCW/UP/DOWN/STOP
- Lock/Unlock
- Envoi commandes JSON

**Code ajout** :
```python
# Dans initUI()
btnLayout = QHBoxLayout()

self.btnCW = QPushButton("◄ CW")
self.btnCW.clicked.connect(lambda: self.jog("az", "cw"))
btnLayout.addWidget(self.btnCW)

self.btnCCW = QPushButton("CCW ►")
self.btnCCW.clicked.connect(lambda: self.jog("az", "ccw"))
btnLayout.addWidget(self.btnCCW)

self.btnStop = QPushButton("STOP")
self.btnStop.clicked.connect(self.stop)
btnLayout.addWidget(self.btnStop)

layout.addLayout(btnLayout)

def jog(self, axis, direction):
    cmd = {"cmd": "jog", "axis": axis, "dir": direction}
    self.socket.send((json.dumps(cmd) + "\n").encode())

def stop(self):
    cmd = {"cmd": "stop", "axis": "all"}
    self.socket.send((json.dumps(cmd) + "\n").encode())
```

**Tests** :
1. Cliquer "CW" → Moteur tourne
2. Cliquer "STOP" → Moteur s'arrête
3. Toutes directions fonctionnent

---

### BLOC 5.3 : Bargraphs & Monitoring (J17-J18)

**Fonctionnalités** :
- Bargraphs courants (PyQt5 QProgressBar)
- Affichage températures
- Affichage tensions
- Indicateurs visuels (vert/rouge)

**Code ajout** :
```python
# Bargraph courant Az
self.barCurrentAz = QProgressBar()
self.barCurrentAz.setRange(0, 280)  # 2.8A = 100%
layout.addWidget(QLabel("Current Az:"))
layout.addWidget(self.barCurrentAz)

# Dans updateData()
current_az_percent = int(data['i_az'] * 100)
self.barCurrentAz.setValue(current_az_percent)

if data['i_az'] > 2.2:
    self.barCurrentAz.setStyleSheet("QProgressBar::chunk { background-color: red; }")
else:
    self.barCurrentAz.setStyleSheet("QProgressBar::chunk { background-color: green; }")
```

---

### BLOC 5.4 : Mode Auto/Manuel (J18-J19)

**Fonctionnalités** :
- Toggle tracking ON/OFF
- Lock quand manuel
- Unlock quand auto

**Tests** :
1. Mode Auto (défaut) → PstRotator contrôle
2. Cliquer "Manual" → Lock envoyé, boutons actifs
3. Cliquer "Auto" → Unlock, PstRotator reprend

---

### BLOC 5.5 : Polish Interface (J19-J20)

**Fonctionnalités** :
- Alertes visuelles/sonores
- Logs console (QTextEdit)
- Graphes historiques (matplotlib)
- Sauvegarde logs CSV

---

## PHASE 6 : INTÉGRATION COMPLÈTE (2-3 jours)

**Objectif** : Système final opérationnel

**Livrables** :
- Communication Nano R4
- Tests longue durée
- Calibration finale
- Documentation utilisateur

---

### BLOC 6.1 : MCU Secondaire Nano R4 (J20-J21)

**Fonctionnalités** :
- Firmware Nano complet
- Communication UART Mega
- Télémétrie tête RF

**Tests** :
1. Nano envoie données températures PA
2. Mega reçoit et intègre dans télémétrie
3. Python app affiche températures tête RF

---

### BLOC 6.2 : NTP Client (J21)

**Fonctionnalités** :
- Sync heure UTC via Internet
- Affichage heure Nextion

---

### BLOC 6.3 : Tests Longue Durée (J21-J22)

**Tests** :
1. Tracking lune 4h continu
2. Pas de crashes, pas de drifts
3. Monitoring stable
4. Mémoire stable (pas de leaks)

---

### BLOC 6.4 : Calibration & Tuning (J22-J23)

**Tâches** :
1. Tuning PID final (minimiser overshoot)
2. Calibration encodeurs (offsets)
3. Calibration ADC tensions
4. Ajustement seuils safety
5. Backup EEPROM config

---

## OUTILS & ENVIRONNEMENT

### VS Code + PlatformIO

**Configuration** :
```ini
[env:megaatmega2560]
platform = atmegatmega2560
board = megaatmega2560
framework = arduino
monitor_speed = 115200
lib_deps = 
    adafruit/Adafruit SSD1306
    milesburton/DallasTemperature
    bblanchon/ArduinoJson
```

---

### Librairies Arduino

| Librairie | Usage | Installation |
|-----------|-------|--------------|
| Ethernet | W5500 | Built-in Arduino |
| OneWire | DS18B20 | Library Manager |
| DallasTemperature | DS18B20 | Library Manager |
| ArduinoJson | Parsing JSON | Library Manager |

---

### Python Environnement
```bash
pip install PyQt5 pyserial matplotlib numpy
```

---

## STRATÉGIE GIT

### Branches
```
main          → Code stable validé
dev           → Développement actif
feature/pid   → Feature spécifique
bugfix/xxx    → Corrections
```

### Commits

**Format** :
```
[Phase X.Y] Description courte

- Changement 1
- Changement 2

Closes #issue
```

**Fréquence** : Après chaque bloc validé

---

## CHECKLIST FINALE

### Avant Production

- [ ] Tous blocs validés
- [ ] Tests longue durée OK (>24h)
- [ ] Calibration complète
- [ ] Documentation à jour
- [ ] Backup firmware (.hex)
- [ ] Backup config EEPROM
- [ ] Plan de rollback si problème

### Mise en Service

- [ ] Installation mécanique sécurisée
- [ ] Câblage vérifié (pas de court-circuits)
- [ ] Alimentations testées
- [ ] Encodeurs fonctionnels
- [ ] Fins de course testées
- [ ] Premier tracking sous surveillance

---

**Version** : 1.0
**Date** : 2026-01-30
**Auteur** : ON7KGK - JO20BM85DP
**Projet** : EME 10 GHz Station Controller
```
