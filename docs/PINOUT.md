# AFFECTATION PINS ARDUINO MEGA - Station EME Controller

## Vue d'ensemble

Ce document décrit l'affectation complète des 70 pins de l'Arduino Mega Pro 2560 pour le contrôle du rotator EME 10 GHz.

---

## TABLEAU RÉCAPITULATIF

| Pin(s) | Fonction | Périphérique | Type | Interrupt | Timer | Notes |
|--------|----------|--------------|------|-----------|-------|-------|
| **0-1** | Serial USB | Debug | UART | - | - | Programmation |
| **2** | Encodeur Az A | HH12 Azimuth | Digital IN | INT4 | - | Pull-up 4.7kΩ |
| **3** | Encodeur Az B | HH12 Azimuth | Digital IN | INT5 | - | Pull-up 4.7kΩ |
| **4** | M1D2 | MC33926 Az Disable 2 | Digital OUT | - | - | LOW = actif |
| **5** | M2D2 | MC33926 El Disable 2 | Digital OUT | - | - | LOW = actif |
| **6** | M2IN1 | MC33926 El PWM 1 | PWM OUT | - | Timer 4 | Élévation |
| **7** | M2IN2 | MC33926 El PWM 2 | PWM OUT | - | Timer 4 | Élévation |
| **8** | EN | MC33926 Enable global | Digital OUT | - | - | HIGH = ON |
| **9** | SLEW | MC33926 Slew rate | Digital OUT | - | - | HIGH = rapide |
| **10** | INV | MC33926 Inversion | Digital OUT | - | - | LOW = normal |
| **11** | M1IN1 | MC33926 Az PWM 1 | PWM OUT | - | Timer 1 | Azimuth |
| **12** | M1IN2 | MC33926 Az PWM 2 | PWM OUT | - | Timer 1 | Azimuth |
| **13** | LED | LED onboard | Digital OUT | - | - | Utilisable |
| **14-15** | Serial3 | **LIBRE** | UART | - | - | Expansion |
| **16** | TX2 | Nano R4 | UART OUT | - | - | 115200 baud |
| **17** | RX2 | Nano R4 | UART IN | - | - | 115200 baud |
| **18** | TX1 | Nextion | UART OUT | - | - | 9600 baud |
| **19** | RX1 | Nextion | UART IN | - | - | 9600 baud |
| **20** | Encodeur El A | HH12 Élévation | Digital IN | INT1 | - | Pull-up 4.7kΩ |
| **21** | Encodeur El B | HH12 Élévation | Digital IN | INT0 | - | Pull-up 4.7kΩ |
| **22** | M1D1 | MC33926 Az Disable 1 | Digital OUT | - | - | LOW = actif |
| **23** | M1SF | MC33926 Az Status | Digital IN | - | - | Pull-up interne |
| **24** | M2D1 | MC33926 El Disable 1 | Digital OUT | - | - | LOW = actif |
| **25** | M2SF | MC33926 El Status | Digital IN | - | - | Pull-up interne |
| **26** | Az CW | VMA452 Fin course | Digital IN | - | - | Pull-up interne |
| **27** | Az CCW | VMA452 Fin course | Digital IN | - | - | Pull-up interne |
| **28** | El UP | VMA452 Fin course | Digital IN | - | - | Pull-up interne |
| **29** | El DOWN | VMA452 Fin course | Digital IN | - | - | Pull-up interne |
| **30** | 1-Wire Data | DS18B20 rack | 1-Wire | - | - | Pull-up 4.7kΩ |
| **31** | **LIBRE** | Expansion | Digital | INT6 | - | Anémomètre futur |
| **32-48** | **LIBRES** | Expansion | Digital | - | - | 17 pins disponibles |
| **49** | Reset W5500 | W5500 (optionnel) | Digital OUT | - | - | |
| **50** | MISO | W5500 SPI | SPI | - | - | |
| **51** | MOSI | W5500 SPI | SPI | - | - | |
| **52** | SCK | W5500 SPI | SPI | - | - | |
| **53** | SS/CS | W5500 SPI | SPI | - | - | Doit être OUTPUT |
| **A0** | M1FB | MC33926 Courant Az | Analog IN | - | - | 0.525 V/A |
| **A1** | M2FB | MC33926 Courant El | Analog IN | - | - | 0.525 V/A |
| **A2** | V_24V | Diviseur tension | Analog IN | - | - | 47kΩ/12kΩ |
| **A3** | V_13.8V | Diviseur tension | Analog IN | - | - | 22kΩ/12kΩ |
| **A4** | V_12V | Diviseur tension | Analog IN | - | - | 18kΩ/12kΩ |
| **A5** | V_5V | Diviseur tension | Analog IN | - | - | Direct ou léger |
| **A6-A15** | **LIBRES** | Expansion | Analog IN | - | - | 10 ADC disponibles |

---

## ENCODEURS (INTERRUPTS HARDWARE)

### Encodeur Azimuth

| Pin | Signal | Type | Interrupt | Notes |
|-----|--------|------|-----------|-------|
| **2** | A (blanc) | Digital IN | INT4 | Pull-up 4.7kΩ externe |
| **3** | B (bleu) | Digital IN | INT5 | Pull-up 4.7kΩ externe |

**Encodeur** : HH12 absolu incrémental (Hall effect)
**Résolution** : 6624 counts/360° (0.054°/count)
**Connecteur** : JST-XH 6P (VCC, GND, A, B, NC, Shield)

---

### Encodeur Élévation

| Pin | Signal | Type | Interrupt | Notes |
|-----|--------|------|-----------|-------|
| **20** | A (blanc) | Digital IN | INT1 | Pull-up 4.7kΩ externe |
| **21** | B (bleu) | Digital IN | INT0 | Pull-up 4.7kΩ externe |

**Encodeur** : HH12 absolu incrémental (Hall effect)
**Résolution** : 6624 counts/360° (0.054°/count)
**Connecteur** : JST-XH 6P (VCC, GND, A, B, NC, Shield)

**Note** : Pins 20/21 DÉDIÉES aux encodeurs (pas de partage I²C)

---

## PWM MOTEURS (MC33926)

### Timer 1 (Azimuth)

| Pin | Signal | Fonction | Timer | Fréquence |
|-----|--------|----------|-------|-----------|
| **11** | M1IN1 | Az PWM direction 1 | Timer 1 | ~490 Hz |
| **12** | M1IN2 | Az PWM direction 2 | Timer 1 | ~490 Hz |

**Contrôle** : Sign-magnitude (0-255 chaque direction)

---

### Timer 4 (Élévation)

| Pin | Signal | Fonction | Timer | Fréquence |
|-----|--------|----------|-------|-----------|
| **6** | M2IN1 | El PWM direction 1 | Timer 4 | ~490 Hz |
| **7** | M2IN2 | El PWM direction 2 | Timer 4 | ~490 Hz |

**Contrôle** : Sign-magnitude (0-255 chaque direction)

**Avantage timers séparés** : Pas d'interférences PWM entre axes

---

## MC33926 CONTRÔLE & FEEDBACK

### Pins Contrôle

| Pin | Signal | Fonction | Logique | Notes |
|-----|--------|----------|---------|-------|
| **22** | M1D1 | Az Disable 1 | LOW = actif | Enable sélectif |
| **4** | M1D2 | Az Disable 2 | LOW = actif | Enable sélectif |
| **24** | M2D1 | El Disable 1 | LOW = actif | Enable sélectif |
| **5** | M2D2 | El Disable 2 | LOW = actif | Enable sélectif |
| **8** | EN | Enable global | HIGH = ON | Master enable |
| **9** | SLEW | Slew rate | HIGH = rapide | EMI control |
| **10** | INV | Inversion | LOW = normal | Polarity swap |

**Usage contrôle software** :
- **EN** : Emergency stop, power save, maintenance
- **SLEW** : Mode rapide (tracking) vs lent (EMI réduit)
- **D1/D2** : Disable moteur individuel, debug
- **INV** : Correction câblage sans recâbler

---

### Pins Status & Feedback

| Pin | Signal | Type | Notes |
|-----|--------|------|-------|
| **23** | M1SF | Digital IN | Az Status Flag (LOW = défaut) |
| **25** | M2SF | Digital IN | El Status Flag (LOW = défaut) |
| **A0** | M1FB | Analog IN | Az Courant (0.525 V/A) |
| **A1** | M2FB | Analog IN | El Courant (0.525 V/A) |

**Calcul courant** :
```cpp
float current_A = analogRead(pin) * (5.0 / 1023.0) / 0.525;
// Exemple : ADC = 205 → 1.0V → 1.9A (nominal moteur)
```

**Seuils recommandés** :
- Normal : 0-1.8A
- Warning : 1.8-2.2A
- Emergency : >2.8A → Stop immédiat

---

## FINS DE COURSE (VMA452)

### Connexions

| Pin | Signal | Logique | Notes |
|-----|--------|---------|-------|
| **26** | Az CW limit | LOW = atteinte | Pull-up interne activé |
| **27** | Az CCW limit | LOW = atteinte | Pull-up interne activé |
| **28** | El UP limit | LOW = atteinte | Pull-up interne activé |
| **29** | El DOWN limit | LOW = atteinte | Pull-up interne activé |

**Module** : VMA452 (4 optocoupleurs)
**Connecteur** : JST-XH 6P (VCC, GND, OUT1-4)

**Switches externes** :
- Type : Microswitch SPDT
- Normalement ouvert (NO)
- Fermeture → LOW détecté
- Positionner ~2° avant butée mécanique

**Code** :
```cpp
pinMode(26, INPUT_PULLUP);
if (digitalRead(26) == LOW) {
  // Fin de course Az CW atteinte
  stopMotorAz();
}
```

---

## CAPTEURS TEMPÉRATURE (1-WIRE)

### DS18B20 Local (Rack)

| Pin | Signal | Type | Notes |
|-----|--------|------|-------|
| **30** | 1-Wire Data | 1-Wire | Pull-up 4.7kΩ vers 5V |

**Capteur** : DS18B20 température ambiante rack
**Résolution** : 12-bit (0.0625°C)
**Connecteur** : JST-XH 3P (VCC, Data, GND)

**Code** :
```cpp
#include <OneWire.h>
#include <DallasTemperature.h>

OneWire oneWire(30);
DallasTemperature sensors(&oneWire);

void setup() {
  sensors.begin();
}

float temp = sensors.getTempCByIndex(0);
```

**Note** : Capteurs tête RF (PA, Transverter) sur Nano R4, pas sur Mega

---

## MONITORING TENSIONS (ADC)

### Diviseurs Résistifs

| Pin | Tension | Diviseur | Calcul | Notes |
|-----|---------|----------|--------|-------|
| **A2** | 24V moteurs | R1=47kΩ, R2=12kΩ | V = ADC × 5.0/1023 × 59/12 | Zener 5.1V |
| **A3** | 13.8V station | R1=22kΩ, R2=12kΩ | V = ADC × 5.0/1023 × 34/12 | Zener 5.1V |
| **A4** | 12V auxiliaire | R1=18kΩ, R2=12kΩ | V = ADC × 5.0/1023 × 30/12 | Zener 5.1V |
| **A5** | 5V logique | Direct ou léger | V = ADC × 5.0/1023 | Test |

**Protection** : Diode Zener 5.1V (1N4733A) + résistance série 1kΩ

**Connecteur** : Bornier vis 5P (24V, 13.8V, 12V, 5V, GND)

**Code** :
```cpp
float v24 = analogRead(A2) * (5.0 / 1023.0) * (59.0 / 12.0);
float v138 = analogRead(A3) * (5.0 / 1023.0) * (34.0 / 12.0);
float v12 = analogRead(A4) * (5.0 / 1023.0) * (30.0 / 12.0);
float v5 = analogRead(A5) * (5.0 / 1023.0);
```

---

## ETHERNET (SPI)

### W5500 Module

| Pin | Signal | Type | Notes |
|-----|--------|------|-------|
| **50** | MISO | SPI IN | Master In Slave Out |
| **51** | MOSI | SPI OUT | Master Out Slave In |
| **52** | SCK | SPI CLK | Clock |
| **53** | SS/CS | SPI CS | Chip Select (doit être OUTPUT) |
| **49** | Reset | Digital OUT | Optionnel (reset W5500) |

**Module** : W5500 Ethernet avec RJ45 intégrée
**Vitesse SPI** : Jusqu'à 80 MHz (Arduino limite ~8 MHz)
**Alimentation** : 3.3V (via régulateur Mega)

**Configuration IP** :
```cpp
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
IPAddress ip(192, 168, 1, 177);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

Ethernet.begin(mac, ip, gateway, subnet);
```

**Serveurs TCP** :
- Port 4533 : Protocole Easycom (PstRotator)
- Port 5000 : Protocole custom JSON (Python app)

---

## COMMUNICATION UART

### Serial (USB Debug)

| Pin | Signal | Fonction | Vitesse |
|-----|--------|----------|---------|
| **0** | RX0 | USB Debug IN | 9600-115200 |
| **1** | TX0 | USB Debug OUT | 9600-115200 |

**Usage** : Programmation + Serial Monitor

---

### Serial1 (Nextion)

| Pin | Signal | Fonction | Vitesse |
|-----|--------|----------|---------|
| **18** | TX1 | Nextion TX | 9600 baud |
| **19** | RX1 | Nextion RX | 9600 baud |

**Écran** : Nextion NX4832T035 (3.5")
**Protocole** : Commandes texte terminées par 0xFF×3
**Mode veille** : Automatique après 15s, wake on touch

**Affichage** :
- Azimuth (°)
- Élévation (°)
- Status (OK / TRACKING / ALARM)

**Code** :
```cpp
void nextion(String cmd) {
  Serial1.print(cmd);
  Serial1.write(0xFF);
  Serial1.write(0xFF);
  Serial1.write(0xFF);
}

nextion("t_az.txt=\"180.5°\"");
```

---

### Serial2 (Nano R4 Tête RF)

| Pin | Signal | Fonction | Vitesse |
|-----|--------|----------|---------|
| **16** | TX2 | Nano R4 TX | 115200 baud |
| **17** | RX2 | Nano R4 RX | 115200 baud |

**Communication bidirectionnelle** :
- **Mega → Nano** : `POS:180.5,45.0` (position Az/El)
- **Nano → Mega** : `V1:24.1,V2:13.7,T1:65.0,...` (télémétrie)

**Protocole** : CSV texte, polling 1 Hz

**Câblage** : Cat6 STP (1 paire TX/RX, 1 paire 5V/GND, blindage→GND)

---

### Serial3 (Expansion)

| Pin | Signal | Fonction | État |
|-----|--------|----------|------|
| **14** | TX3 | **LIBRE** | Expansion future |
| **15** | RX3 | **LIBRE** | Expansion future |

**Usages potentiels** :
- Deuxième Nano R4 (parabole test)
- GPS série
- Module radio (LoRa, etc.)

---

## PINS LIBRES (EXPANSION)

### Digitales Libres

| Pins | Qté | Notes |
|------|-----|-------|
| **13** | 1 | LED onboard (utilisable) |
| **14-15** | 2 | Serial3 UART |
| **31** | 1 | Interrupt capable (INT6) |
| **32-48** | 17 | GPIO standard |

**Total** : **21 pins digitales libres**

**Exemples usage** :
- Anémomètre (pin 31, interrupt)
- Capteur pluie (digital IN)
- Relais contrôle (digital OUT)
- Boutons physiques (pull-up)
- LEDs status (OUT)

---

### Analogiques Libres

| Pins | Qté | Notes |
|------|-----|-------|
| **A6-A15** | 10 | ADC 10-bit |

**Total** : **10 ADC libres**

**Exemples usage** :
- Capteurs supplémentaires (LDR, potentiomètres)
- Monitoring autres tensions
- Capteurs analogiques température (LM35, etc.)

---

## ARCHITECTURE INTERRUPTS

### Interrupts Externes Mega

| Interrupt | Pin | Usage | Priorité |
|-----------|-----|-------|----------|
| **INT0** | 21 | Encodeur El B | Haute |
| **INT1** | 20 | Encodeur El A | Haute |
| **INT2** | 19 | Nextion RX | Basse |
| **INT3** | 18 | Nextion TX | Basse |
| **INT4** | 2 | Encodeur Az A | Haute |
| **INT5** | 3 | Encodeur Az B | Haute |
| **INT6** | 31 | LIBRE | - |

**Note** : Encodeurs utilisent 4 interrupts (optimal pour lecture quadrature)

---

## TIMERS PWM

### Affectation Timers

| Timer | Pins | Usage | Fréquence |
|-------|------|-------|-----------|
| **Timer 0** | 4, 13 | millis() / delay() | **Réservé** |
| **Timer 1** | 11, 12 | **PWM Az** | ~490 Hz |
| **Timer 2** | 9, 10 | Utilisé (SLEW, INV) | - |
| **Timer 3** | 2, 3, 5 | - | Libre |
| **Timer 4** | 6, 7, 8 | **PWM El** | ~490 Hz |
| **Timer 5** | 44, 45, 46 | - | Libre |

**Note** : Ne jamais toucher Timer 0 (millis() dysfonctionnel)

---

## ALIMENTATION

### Pins Alimentation

| Pin | Fonction | Tension | Notes |
|-----|----------|---------|-------|
| **VIN** | Alim externe | 7-12V DC | Régulateur interne → 5V |
| **5V** | Sortie régulée | 5V | Max 800mA disponible |
| **3.3V** | Sortie régulée | 3.3V | Max 50mA |
| **GND** | Masse | 0V | Plusieurs pins GND disponibles |
| **IOREF** | Référence logique | 5V | Pour shields |

**Consommation 5V** :
- Mega : ~150mA
- W5500 : ~150mA
- Nextion : ~100mA (veille), ~250mA (actif)
- Capteurs : ~50mA
- **Total** : ~450mA (marge OK avec 800mA max)

---

## CONFIGURATION FINALE

### Pins Utilisées : 42 / 70

**Répartition** :
- PWM Moteurs : 4
- Encodeurs : 4
- MC33926 Contrôle : 10
- Fins course : 4
- Capteurs locaux : 5
- Communication : 7
- SPI Ethernet : 5
- LED : 1
- Alimentation : 2

### Pins Disponibles : 28

**Répartition** :
- Digitales : 21
- Analogiques : 10
- Serial3 UART : 2 (inclus dans digitales)

**Marge confortable pour extensions futures !**

---

## NOTES IMPORTANTES

### Pull-ups

**Externes obligatoires** :
- Encodeurs A/B : 4.7kΩ → 5V (×4)
- DS18B20 1-Wire : 4.7kΩ → 5V

**Internes activés (code)** :
- Fins de course (pins 26-29)
- MC33926 Status Flags (pins 23, 25)

---

### Protection ADC

**Toutes les entrées ADC tensions** :
- Diode Zener 5.1V (1N4733A) vers GND
- Résistance série 1kΩ
- Condensateur 100nF filtrage

---

### Blindages Câbles

**Tous les câbles blindés** :
- Encodeurs : Tresse → GND
- UART Nano : Tresse → GND (1 seul côté si boucle masse)
- Masse commune : Point étoile sur PCB

---

**Version** : 1.0
**Date** : 2026-01-30
**Auteur** : ON7KGK
**Projet** : EME 10 GHz Station Controller
```