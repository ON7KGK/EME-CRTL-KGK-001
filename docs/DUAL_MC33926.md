# POLOLU DUAL MC33926 MOTOR DRIVER - Guide Complet

## Vue d'ensemble

Le Pololu Dual MC33926 est un driver de moteurs DC double pont H basé sur le chip Freescale MC33926. Un seul module peut contrôler 2 moteurs DC indépendants avec feedback courant intégré et protections multiples.

**URL Produit** : https://www.pololu.com/product/1213

---

## SPÉCIFICATIONS TECHNIQUES

### Électriques

| Paramètre | Valeur | Notes |
|-----------|--------|-------|
| **Tension moteurs (VIN)** | 5.5-28V DC | Alimentation puissance |
| **Courant continu** | 3A par canal | Fonctionnement normal |
| **Courant pic** | 5A par canal | Quelques secondes max |
| **Tension logique (VDD)** | 5V | Depuis MCU ou régulateur interne |
| **Courant logique** | ~30mA | Consommation contrôle |
| **Fréquence PWM max** | 20 kHz | Recommandé : 0.5-10 kHz |
| **Résistance ON** | 0.3Ω typique | Rdson pont H |
| **Feedback courant** | **0.525 V/A** | Gain précis |

### Protections Intégrées

| Protection | Seuil | Comportement |
|------------|-------|--------------|
| **Thermique** | 150°C jonction | Shutdown automatique |
| **Court-circuit** | Détection rapide | Limitation courant + flag |
| **Sous-tension** | <5.5V | Désactivation sorties |
| **Surtension** | >28V | Risque destruction (externe requis) |

### Physiques

| Paramètre | Valeur |
|-----------|--------|
| **Dimensions** | 51 × 38 mm |
| **Montage** | 4 trous M3 |
| **Connecteurs** | Headers + borniers vis |
| **Poids** | ~15g |
| **Température fonctionnement** | -40°C à +85°C |

---

## ARCHITECTURE INTERNE

### Schéma Bloc
```
                  ┌─────────────────────────────────┐
                  │   POLOLU DUAL MC33926          │
                  │                                 │
VIN (24V) ────────┤ Power Input                     │
GND ──────────────┤                                 │
                  │                                 │
VDD (5V) ─────────┤ Logic Power (optionnel)        │
                  │                                 │
                  │  ┌──────────┐   ┌──────────┐   │
M1IN1 ────────────┼─→│ MC33926  │   │ MC33926  │   │
M1IN2 ────────────┼─→│  Moteur 1│   │  Moteur 2│   │
M1D1 ─────────────┼─→│  (Az)    │   │  (El)    │   │
M1D2 ─────────────┼─→│          │   │          │   │
M1SF ←────────────┼──│          │   │          │   │
M1FB ←────────────┼──│          │   │          │   │
                  │  └─────┬────┘   └────┬─────┘   │
M2IN1 ────────────┼────────┼─────────────┼─────→   │
M2IN2 ────────────┼────────┼─────────────┼─────→   │
M2D1 ─────────────┼────────┼─────────────┼─────→   │
M2D2 ─────────────┼────────┼─────────────┼─────→   │
M2SF ←────────────┼────────┼─────────────┼────────┤
M2FB ←────────────┼────────┼─────────────┼────────┤
                  │        │             │         │
EN ───────────────┼────────┴─────────────┴─────→   │
SLEW ─────────────┼──→ (commun 2 drivers)          │
INV ──────────────┼──→ (commun 2 drivers)          │
                  │                                 │
                  │     M1OUT1  M1OUT2              │
                  │        │      │                 │
                  └────────┼──────┼─────────────────┘
                           │      │
                           └──┬───┘
                              │
                          Moteur Az

                     M2OUT1  M2OUT2
                        │      │
                        └──┬───┘
                           │
                       Moteur El
```

---

## PINOUT DÉTAILLÉ

### Connecteur Logique (Header 1×19)

| Pin | Signal | Direction | Type | Description |
|-----|--------|-----------|------|-------------|
| 1 | GND | - | Ground | Masse logique |
| 2 | **M1IN1** | IN | PWM | Az PWM direction 1 |
| 3 | **M1IN2** | IN | PWM | Az PWM direction 2 |
| 4 | **M1D1** | IN | Digital | Az Disable 1 (LOW=actif) |
| 5 | **M1D2** | IN | Digital | Az Disable 2 (LOW=actif) |
| 6 | **M1SF** | OUT | Digital | Az Status Flag (LOW=défaut) |
| 7 | **M1FB** | OUT | Analog | Az Feedback courant (0.525V/A) |
| 8 | **M2IN1** | IN | PWM | El PWM direction 1 |
| 9 | **M2IN2** | IN | PWM | El PWM direction 2 |
| 10 | **M2D1** | IN | Digital | El Disable 1 (LOW=actif) |
| 11 | **M2D2** | IN | Digital | El Disable 2 (LOW=actif) |
| 12 | **M2SF** | OUT | Digital | El Status Flag (LOW=défaut) |
| 13 | **M2FB** | OUT | Analog | El Feedback courant (0.525V/A) |
| 14 | **EN** | IN | Digital | Enable global (HIGH=ON) |
| 15 | **SLEW** | IN | Digital | Slew rate (HIGH=rapide) |
| 16 | **INV** | IN | Digital | Inversion (LOW=normal) |
| 17 | VDD | IN/OUT | 5V | Logic power (optionnel) |
| 18 | GND | - | Ground | Masse logique |
| 19 | (NC) | - | - | Non connecté (spare) |

### Connecteur Puissance

**Bornier vis 2P (VIN)** :
- Pin 1 : VIN (24V DC)
- Pin 2 : GND (masse puissance)

**Borniers vis 2P (Moteurs)** :
- **M1OUT** : Moteur 1 (Azimuth)
  - M1OUT1 : Sortie +
  - M1OUT2 : Sortie -
- **M2OUT** : Moteur 2 (Élévation)
  - M2OUT1 : Sortie +
  - M2OUT2 : Sortie -

---

## MODE DE CONTRÔLE : SIGN-MAGNITUDE

### Principe

Le MC33926 utilise le mode **sign-magnitude** (signe-magnitude) :
- **2 PWM par moteur** (IN1, IN2)
- **Direction** : Lequel des 2 est actif
- **Vitesse** : Duty cycle du PWM actif

### Exemples

**Moteur tourne sens 1 (CW)** :
```
M1IN1 = PWM (0-255)
M1IN2 = 0
→ Moteur Az tourne CW à vitesse proportionnelle à PWM
```

**Moteur tourne sens 2 (CCW)** :
```
M1IN1 = 0
M1IN2 = PWM (0-255)
→ Moteur Az tourne CCW à vitesse proportionnelle à PWM
```

**Moteur arrêté (freinage actif)** :
```
M1IN1 = 0
M1IN2 = 0
→ Moteur Az en court-circuit = freinage électrique
```

**Moteur roue libre (coast)** :
```
M1D1 = HIGH (disable)
M1D2 = HIGH (disable)
→ Moteur Az déconnecté = roue libre
```

---

## CODE ARDUINO

### Setup
```cpp
// === PINS DÉFINITION ===

// PWM Moteurs (Timers séparés)
#define M1_IN1  11   // Az PWM dir 1 (Timer 1)
#define M1_IN2  12   // Az PWM dir 2 (Timer 1)
#define M2_IN1  6    // El PWM dir 1 (Timer 4)
#define M2_IN2  7    // El PWM dir 2 (Timer 4)

// Disable
#define M1_D1   22   // Az Disable 1
#define M1_D2   4    // Az Disable 2
#define M2_D1   24   // El Disable 1
#define M2_D2   5    // El Disable 2

// Status
#define M1_SF   23   // Az Status Flag
#define M2_SF   25   // El Status Flag

// Feedback Courant
#define M1_FB   A0   // Az Current (0.525V/A)
#define M2_FB   A1   // El Current (0.525V/A)

// Contrôle Global
#define MC_EN   8    // Enable global
#define MC_SLEW 9    // Slew rate
#define MC_INV  10   // Inversion

// === SETUP ===
void setupMotorDriver() {
  // PWM pins
  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);
  
  // Disable pins
  pinMode(M1_D1, OUTPUT);
  pinMode(M1_D2, OUTPUT);
  pinMode(M2_D1, OUTPUT);
  pinMode(M2_D2, OUTPUT);
  
  // Contrôle global
  pinMode(MC_EN, OUTPUT);
  pinMode(MC_SLEW, OUTPUT);
  pinMode(MC_INV, OUTPUT);
  
  // Status inputs
  pinMode(M1_SF, INPUT_PULLUP);
  pinMode(M2_SF, INPUT_PULLUP);
  
  // Configuration initiale
  digitalWrite(MC_EN, HIGH);      // Enable drivers
  digitalWrite(MC_SLEW, HIGH);    // Mode rapide (tracking)
  digitalWrite(MC_INV, LOW);      // Pas d'inversion
  
  // Enable tous les moteurs
  digitalWrite(M1_D1, LOW);       // LOW = enable
  digitalWrite(M1_D2, LOW);
  digitalWrite(M2_D1, LOW);
  digitalWrite(M2_D2, LOW);
  
  // Stop moteurs
  analogWrite(M1_IN1, 0);
  analogWrite(M1_IN2, 0);
  analogWrite(M2_IN1, 0);
  analogWrite(M2_IN2, 0);
}
```

---

### Contrôle Moteur
```cpp
// === CONTRÔLE AZIMUTH ===
// speed : -255 (CCW full) à +255 (CW full)

void setMotorAz(int speed) {
  speed = constrain(speed, -255, 255);
  
  if (speed > 0) {
    // CW (sens positif)
    analogWrite(M1_IN1, speed);
    analogWrite(M1_IN2, 0);
  } else if (speed < 0) {
    // CCW (sens négatif)
    analogWrite(M1_IN1, 0);
    analogWrite(M1_IN2, -speed);  // Inverser signe
  } else {
    // Stop (freinage actif)
    analogWrite(M1_IN1, 0);
    analogWrite(M1_IN2, 0);
  }
}

// === CONTRÔLE ÉLÉVATION ===
void setMotorEl(int speed) {
  speed = constrain(speed, -255, 255);
  
  if (speed > 0) {
    // UP (sens positif)
    analogWrite(M2_IN1, speed);
    analogWrite(M2_IN2, 0);
  } else if (speed < 0) {
    // DOWN (sens négatif)
    analogWrite(M2_IN1, 0);
    analogWrite(M2_IN2, -speed);
  } else {
    // Stop (freinage actif)
    analogWrite(M2_IN1, 0);
    analogWrite(M2_IN2, 0);
  }
}

// === ARRÊT MOTEURS ===
void stopAllMotors() {
  analogWrite(M1_IN1, 0);
  analogWrite(M1_IN2, 0);
  analogWrite(M2_IN1, 0);
  analogWrite(M2_IN2, 0);
}
```

---

### Lecture Courant
```cpp
// === LECTURE COURANT MOTEURS ===
// Formule : I (A) = V_FB / 0.525
// V_FB = ADC × (5.0 / 1023.0)

float readCurrentAz() {
  int raw = analogRead(M1_FB);
  float voltage = raw * (5.0 / 1023.0);
  return voltage / 0.525;  // Gain MC33926 : 0.525 V/A
}

float readCurrentEl() {
  int raw = analogRead(M2_FB);
  float voltage = raw * (5.0 / 1023.0);
  return voltage / 0.525;
}

// === EXEMPLES VALEURS ===
// Moteur SVH3 nominal 1.9A :
//   ADC = 1.9 × 0.525 / (5.0/1023) ≈ 205
//   Lecture : 1.9A
//
// Moteur à vide ~0.5A :
//   ADC ≈ 54
//   Lecture : 0.5A
//
// Surintensité 2.8A :
//   ADC ≈ 294
//   Lecture : 2.8A
```

---

### Monitoring Safety
```cpp
// === SEUILS COURANT ===
#define CURRENT_NORMAL_MAX   1.8   // Fonctionnement normal (A)
#define CURRENT_WARNING      2.2   // Seuil avertissement (A)
#define CURRENT_EMERGENCY    2.8   // Seuil arrêt urgence (A)

// === MONITORING CONTINU ===
void monitorMotorSafety() {
  static unsigned long lastCheck = 0;
  
  if (millis() - lastCheck < 50) return;  // Check 20 Hz
  lastCheck = millis();
  
  float currentAz = readCurrentAz();
  float currentEl = readCurrentEl();
  
  // Emergency stop (>2.8A)
  if (currentAz > CURRENT_EMERGENCY || currentEl > CURRENT_EMERGENCY) {
    emergencyStopAll();
    systemError = true;
    Serial.println("EMERGENCY: Motor overcurrent!");
    return;
  }
  
  // Warning (>2.2A)
  if (currentAz > CURRENT_WARNING || currentEl > CURRENT_WARNING) {
    Serial.print("WARNING: High current - Az:");
    Serial.print(currentAz);
    Serial.print("A El:");
    Serial.print(currentEl);
    Serial.println("A");
  }
  
  // Vérifier status flags hardware
  checkMotorFaults();
}

// === VÉRIFICATION FLAGS HARDWARE ===
void checkMotorFaults() {
  // Status Flag = LOW si défaut détecté par MC33926
  bool faultAz = (digitalRead(M1_SF) == LOW);
  bool faultEl = (digitalRead(M2_SF) == LOW);
  
  if (faultAz || faultEl) {
    emergencyStopAll();
    systemError = true;
    
    if (faultAz) Serial.println("FAULT: Az motor driver error");
    if (faultEl) Serial.println("FAULT: El motor driver error");
  }
}

// === ARRÊT URGENCE COMPLET ===
void emergencyStopAll() {
  // Stop PWM
  analogWrite(M1_IN1, 0);
  analogWrite(M1_IN2, 0);
  analogWrite(M2_IN1, 0);
  analogWrite(M2_IN2, 0);
  
  // Disable tous les drivers
  digitalWrite(M1_D1, HIGH);
  digitalWrite(M1_D2, HIGH);
  digitalWrite(M2_D1, HIGH);
  digitalWrite(M2_D2, HIGH);
  
  // Disable global
  digitalWrite(MC_EN, LOW);
  
  Serial.println("EMERGENCY STOP ACTIVATED");
}
```

---

## FONCTIONS AVANCÉES

### Mode Économie Énergie
```cpp
// === POWER SAVE MODE ===
// Après idle prolongé (>5 min sans mouvement)

void setPowerSaveMode(bool enable) {
  if (enable) {
    digitalWrite(MC_SLEW, LOW);   // Commutation lente (EMI réduit)
    digitalWrite(MC_EN, LOW);     // Disable complet
    Serial.println("Power save mode ON");
  } else {
    digitalWrite(MC_EN, HIGH);
    digitalWrite(MC_SLEW, HIGH);  // Retour mode rapide
    Serial.println("Power save mode OFF");
  }
}
```

---

### Mode Test / Diagnostics
```cpp
// === TEST MODE ===
// Commutation lente pour tests bas courant

void setTestMode(bool enable) {
  if (enable) {
    digitalWrite(MC_SLEW, LOW);   // Commutation lente
    Serial.println("Test mode ON - Slow slew rate");
  } else {
    digitalWrite(MC_SLEW, HIGH);  // Commutation rapide
    Serial.println("Test mode OFF - Fast slew rate");
  }
}

// === TEST MOTEUR INDIVIDUEL ===
void testMotorAz(int speed, int duration_ms) {
  Serial.print("Testing Az motor at speed ");
  Serial.println(speed);
  
  setMotorAz(speed);
  delay(duration_ms);
  setMotorAz(0);
  
  Serial.print("Current measured: ");
  Serial.print(readCurrentAz());
  Serial.println("A");
}
```

---

### Disable Moteur Individuel
```cpp
// === DISABLE SÉLECTIF ===
// Utile pour debug ou maintenance

void disableMotorAz(bool disable) {
  if (disable) {
    digitalWrite(M1_D1, HIGH);
    digitalWrite(M1_D2, HIGH);
    analogWrite(M1_IN1, 0);
    analogWrite(M1_IN2, 0);
    Serial.println("Az motor disabled");
  } else {
    digitalWrite(M1_D1, LOW);
    digitalWrite(M1_D2, LOW);
    Serial.println("Az motor enabled");
  }
}

void disableMotorEl(bool disable) {
  if (disable) {
    digitalWrite(M2_D1, HIGH);
    digitalWrite(M2_D2, HIGH);
    analogWrite(M2_IN1, 0);
    analogWrite(M2_IN2, 0);
    Serial.println("El motor disabled");
  } else {
    digitalWrite(M2_D1, LOW);
    digitalWrite(M2_D2, LOW);
    Serial.println("El motor enabled");
  }
}
```

---

### Inversion Polarité (Software)
```cpp
// === INVERSION MOTEUR ===
// Si moteur tourne à l'envers, corriger sans recâbler

void setMotorInversion(bool invert) {
  if (invert) {
    digitalWrite(MC_INV, HIGH);  // Inversion active
    Serial.println("Motor polarity inverted");
  } else {
    digitalWrite(MC_INV, LOW);   // Normal
    Serial.println("Motor polarity normal");
  }
}

// Ou inversion sélective dans le code :
void setMotorAz_Inverted(int speed) {
  setMotorAz(-speed);  // Inverser signe
}
```

---

## DISSIPATION THERMIQUE

### Calcul Puissance Dissipée

**Formule** :
```
P_dissipée = I² × Rdson
Rdson = 0.3Ω typique
```

**Exemples** :
- 1.0A : P = 1² × 0.3 = **0.3W**
- 1.9A : P = 1.9² × 0.3 = **1.08W**
- 3.0A : P = 3² × 0.3 = **2.7W**

### Température Maximale

**Protection thermique** : 150°C jonction
**Température ambiante max** : 85°C

**Refroidissement** :
- < 2A continu : Pas de dissipateur nécessaire (convection naturelle OK)
- 2-3A continu : Dissipateur recommandé (ou ventilation forcée)
- > 3A pic : OK quelques secondes, pas continu

**Notre usage (1.9A nominal)** :
- P = 1.08W par driver
- Pas de dissipateur nécessaire (convection naturelle suffisante)

---

## PROTECTIONS & DIAGNOSTICS

### Protection Thermique

**Comportement** :
1. Température jonction >150°C
2. Shutdown automatique des sorties
3. Status Flag → LOW
4. Refroidissement naturel
5. Retour automatique si T <140°C

**Monitoring** :
```cpp
if (digitalRead(M1_SF) == LOW) {
  // Possible thermal shutdown ou court-circuit
  Serial.println("Driver fault detected - check temperature");
}
```

---

### Protection Court-Circuit

**Détection** :
- Courant > seuil interne (~6-8A typique)
- Réaction < 10µs
- Limitation courant automatique
- Status Flag → LOW

**Recovery** :
1. Disable driver (D1/D2 = HIGH)
2. Attendre 100ms
3. Vérifier cause court-circuit
4. Re-enable (D1/D2 = LOW)

---

### Protection Sous-Tension

**Seuil** : VIN < 5.5V
**Comportement** :
- Sorties désactivées
- Status Flag → LOW
- Recovery auto si VIN > 6V

**Notre cas (24V)** : Pas de risque

---

## INTERFERING EMI (ÉLECTROMAGNÉTIQUE)

### Sources EMI

Le MC33926 génère du bruit EMI :
- Commutation rapide (kHz)
- Courants élevés (pics 5A)
- Câbles moteurs = antennes

### Réduction EMI

**Hardware** :
1. **Condensateurs découplage** :
   - 100µF électrolytique près VIN
   - 100nF céramique HF près VIN
   - Sur chaque driver

2. **Câbles moteurs** :
   - Torsadés ensemble (paire)
   - Aussi courts que possible
   - Blindés si >2m (tresse → GND)

3. **Plan de masse** :
   - GND continu (pas de coupures)
   - Étoile depuis PSU

**Software** :
1. **Slew rate** :
```cpp
   digitalWrite(MC_SLEW, LOW);  // Mode lent = EMI réduit
```
   - Trade-off : Réactivité vs EMI
   - Pour tracking lent (0.048 rpm) : mode lent acceptable

2. **Fréquence PWM** :
   - Par défaut : 490 Hz (OK)
   - Augmenter : Plus d'EMI mais moteur plus doux
   - Diminuer : Moins d'EMI mais moteur plus bruyant

---

## DÉPANNAGE

### Problème : Moteur ne tourne pas

**Causes possibles** :
1. ❌ EN = LOW → Vérifier pin 8 = HIGH
2. ❌ D1/D2 = HIGH → Vérifier pins 22/4 = LOW
3. ❌ PWM = 0 → Vérifier code contrôle
4. ❌ VIN absent → Vérifier alim 24V
5. ❌ Moteur débranché → Vérifier connexions

---

### Problème : Surintensité permanente

**Causes possibles** :
1. ❌ Court-circuit moteur → Mesurer résistance (doit être ~12Ω)
2. ❌ Moteur bloqué mécaniquement → Vérifier libre rotation
3. ❌ Tension trop élevée → Vérifier VIN < 28V
4. ❌ PWM trop élevé → Réduire vitesse

**Diagnostic** :
```cpp
// Test moteur déconnecté
setMotorAz(128);  // 50% PWM
delay(100);
float current = readCurrentAz();
setMotorAz(0);

if (current > 0.2) {
  Serial.println("ERROR: Current detected without motor!");
}
```

---

### Problème : Status Flag toujours LOW

**Causes possibles** :
1. ❌ Surchauffe driver → Laisser refroidir
2. ❌ Court-circuit → Vérifier câblage
3. ❌ Driver endommagé → Remplacer module

---

### Problème : Moteur erratique

**Causes possibles** :
1. ❌ EMI perturbe signaux → Ajouter découplages, câbles blindés
2. ❌ Masse bruyante → Vérifier GND propre
3. ❌ PWM instable → Vérifier code (pas de blocking delays)

---

## CARACTÉRISTIQUES COMPARÉES

### vs L298N (Driver classique)

| Critère | MC33926 | L298N |
|---------|---------|-------|
| Courant max | 3A continu | 2A continu |
| Rdson | 0.3Ω | 1.8Ω |
| Chute tension | ~0.6V @ 2A | ~3.6V @ 2A |
| Efficacité | ~95% | ~75% |
| Protections | Oui (thermique, court-circuit) | Non |
| Feedback courant | Oui (précis) | Non |
| Prix | ~35€ (dual) | ~5€ (dual) |

**Verdict** : MC33926 largement supérieur (mais plus cher)

---

### vs BTS7960 (Alternative)

| Critère | MC33926 | BTS7960 |
|---------|---------|---------|
| Courant max | 3A | 43A (!!) |
| Rdson | 0.3Ω | 0.016Ω |
| Package | Dual (2 moteurs) | Single (1 moteur) |
| Protections | Intégrées | Intégrées |
| Feedback | 0.525 V/A | Oui (différent) |
| Prix | ~35€ (dual) | ~10€ (single) |

**Verdict** : BTS7960 = overkill pour 1.9A (mais option viable)

---

## RECOMMANDATIONS FINALES

### Pour Rotator EME (1.9A nominal)

✅ **MC33926 = choix idéal** :
- Courant 3A continu = marge confortable (1.6×)
- Pics 5A OK pour démarrages
- Protections intégrées
- Feedback courant précis (safety)
- 2 moteurs sur 1 module (économie place)

### Configuration Optimale
```cpp
EN = HIGH        // Toujours actif (sauf power save)
SLEW = HIGH      // Mode rapide (tracking précis)
INV = LOW        // Pas d'inversion (sauf test)
D1/D2 = LOW      // Enable permanent (sauf emergency)
PWM fréquence    // 490 Hz (défaut OK)
```

### Monitoring Minimal
```cpp
void loop() {
  // Check courants (20 Hz)
  if (readCurrentAz() > 2.8 || readCurrentEl() > 2.8) {
    emergencyStopAll();
  }
  
  // Check status flags (20 Hz)
  if (digitalRead(M1_SF) == LOW || digitalRead(M2_SF) == LOW) {
    emergencyStopAll();
  }
}
```

---

**Version** : 1.0
**Date** : 2026-01-30
**Auteur** : ON7KGK
**Projet** : EME 10 GHz Station Controller
```

