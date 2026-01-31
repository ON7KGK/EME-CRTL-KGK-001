# NOTES SCHÉMATIQUE EASYEDA - Station EME Controller

## Vue d'ensemble

Ce document guide la création du schéma électrique complet dans EasyEDA Pro. Le schéma est organisé en **6 feuilles** (pages) pour une lisibilité optimale.

---

## STRUCTURE SCHÉMA (6 PAGES)
```
Page 1 : Power & MCU
Page 2 : Motor Drivers
Page 3 : Encoders & Limit Switches
Page 4 : Ethernet & Communication
Page 5 : Sensors (Local Rack)
Page 6 : Connectors Summary
```

**Format recommandé** : A4 paysage (297×210 mm)

---

## PAGE 1 : POWER & MCU

### Arduino Mega Pro 2560

**Symbole** : Créer symbole custom ou utiliser générique Mega 2560

**Groupes de pins** :
- **Power** : VIN, 5V, 3.3V, GND, IOREF
- **Digital** : 0-53 (regrouper par fonction)
- **Analog** : A0-A15
- **Communication** : Serial 0-3, SPI, I²C (noter mais pas utilisé)

**Note importante** : Ajouter labels texte pour chaque groupe :
```
ENCODERS (2, 3, 20, 21)
PWM MOTORS (6, 7, 11, 12)
MC33926 CONTROL (4, 5, 8, 9, 10, 22, 23, 24, 25)
LIMIT SWITCHES (26-29)
SENSORS (30, A0-A5)
UART NEXTION (18, 19)
UART NANO (16, 17)
SPI ETHERNET (50-53)
```

---

### Alimentation 12V (Logique)

**Schéma** :
```
PSU 12V ──[F1 2A]──[D1]──[C1 100µF]──[C2 100nF]──┬──> VIN (Mega)
                                                  │
GND ──────────────────────────────────────────────┴──> GND (Mega)
```

**Composants** :

| Ref | Valeur | Package | Notes |
|-----|--------|---------|-------|
| **F1** | Fusible 2A rapide | 5×20mm | Littelfuse ou équivalent |
| **D1** | 1N5822 (Schottky 3A) | DO-201AD | Protection inversion polarité |
| **C1** | 100µF / 16V | Radial 6.3mm | Électrolytique (bulk) |
| **C2** | 100nF / 50V | 5mm pitch | Céramique X7R (HF) |
| **LED1** | LED verte 3mm | TH | Power indicator |
| **R1** | 1kΩ 1/4W | Axial | Résistance série LED |

**Connexion LED Power** :
```
5V ──[R1 1kΩ]──[LED1]──> GND
```

---

### Alimentation 24V (Moteurs)

**Schéma** :
```
PSU 24V ──[F2 5A]──[D2]──[C3 470µF]──[C4 100nF]──> VIN (MC33926)
                                                   │
GND ───────────────────────────────────────────────> GND (MC33926)
```

**Composants** :

| Ref | Valeur | Package | Notes |
|-----|--------|---------|-------|
| **F2** | Fusible 5A rapide | 5×20mm | Protection moteurs |
| **D2** | 1N5822 (Schottky 3A) | DO-201AD | Protection inversion |
| **C3** | 470µF / 35V | Radial 10mm | Électrolytique (bulk capacitor) |
| **C4** | 100nF / 50V | 5mm pitch | Céramique X7R (HF découplage) |

**Note** : Placer C3 et C4 **très proche** des bornes VIN/GND du module MC33926 (<2cm)

---

### Connecteurs Alimentation

**J1 : Alimentation 12V** (Bornier vis 2P, 5.08mm pitch)
```
Pin 1 : +12V (depuis PSU)
Pin 2 : GND
```

**J2 : Alimentation 24V** (Bornier vis 2P, 5.08mm pitch)
```
Pin 1 : +24V (depuis PSU)
Pin 2 : GND
```

**Capacité vis** : Minimum 10A pour J2 (câble 1.5mm²)

---

### Découplages Locaux

**Mega** :
```
VIN ──[C5 100µF]──[C6 100nF]── GND
5V  ──[C7 100µF]──[C8 100nF]── GND
```

**W5500** :
```
3.3V ──[C9 10µF]──[C10 100nF]── GND
```

**Tous les IC** : Ajouter 100nF **très proche** de chaque pin VCC/GND

---

## PAGE 2 : MOTOR DRIVERS

### Module Pololu Dual MC33926

**Symbole** : Bloc rectangulaire avec 19 pins signaux + 4 borniers puissance

**Connexions Mega → MC33926** :

#### PWM Moteurs

| MC33926 Pin | Mega Pin | Signal | Notes |
|-------------|----------|--------|-------|
| **M1IN1** | 11 | Az PWM 1 | Timer 1 |
| **M1IN2** | 12 | Az PWM 2 | Timer 1 |
| **M2IN1** | 6 | El PWM 1 | Timer 4 |
| **M2IN2** | 7 | El PWM 2 | Timer 4 |

**Ajouter labels** : "PWM Az (Timer 1)" et "PWM El (Timer 4)"

---

#### Contrôle Digital

| MC33926 Pin | Mega Pin | Signal | Logique |
|-------------|----------|--------|---------|
| **M1D1** | 22 | Az Disable 1 | LOW = actif |
| **M1D2** | 4 | Az Disable 2 | LOW = actif |
| **M2D1** | 24 | El Disable 1 | LOW = actif |
| **M2D2** | 5 | El Disable 2 | LOW = actif |
| **M1SF** | 23 | Az Status Flag | LOW = défaut |
| **M2SF** | 25 | El Status Flag | LOW = défaut |
| **EN** | 8 | Enable global | HIGH = ON |
| **SLEW** | 9 | Slew rate | HIGH = rapide |
| **INV** | 10 | Inversion | LOW = normal |

**Ajouter pull-ups internes sur SF** : Symbole résistance pointillée vers VCC

---

#### Feedback Courant

| MC33926 Pin | Mega Pin | Signal | Notes |
|-------------|----------|--------|-------|
| **M1FB** | A0 | Az Current | 0.525 V/A |
| **M2FB** | A1 | El Current | 0.525 V/A |

**Ajouter note** : "0.525 V/A gain"

---

### Alimentation Module

**Connexion VIN (puissance)** :
```
24V ──[C11 100µF]──[C12 100nF]──> MC33926 VIN
                                   │
GND ───────────────────────────────> MC33926 GND
```

**C11, C12** : Placer **immédiatement** à l'entrée VIN du module (<1cm)

**VDD (logique)** :
```
Mega 5V ──> MC33926 VDD (optionnel, module a régulateur interne)
```

---

### Sorties Moteurs

**Borniers vis 2P** (5.08mm pitch, 10A min) :

**J3 : Moteur Azimuth**
```
Pin 1 : M1OUT1 (depuis MC33926)
Pin 2 : M1OUT2
```

**J4 : Moteur Élévation**
```
Pin 1 : M2OUT1 (depuis MC33926)
Pin 2 : M2OUT2
```

**Ajouter symbole moteur** :
```
M1OUT1 ──┐
         ├──[M] Moteur Az
M1OUT2 ──┘
```

---

### Condensateurs Moteurs (Optionnel)

**Suppression EMI** :
```
M1OUT1 ──[C13 100nF X2]── M1OUT2
M2OUT1 ──[C14 100nF X2]── M2OUT2
```

**Type** : Céramique X2 (250V AC rated)
**Placement** : Directement sur bornes moteur (côté moteur, pas PCB)

---

## PAGE 3 : ENCODERS & LIMIT SWITCHES

### Encodeurs HH12 (×2)

**Connecteurs** : JST-XH 6P (2.54mm pitch)

#### Encodeur Azimuth (J5)
```
Pin 1 : A (blanc)    ──[R2 220Ω]──> Mega Pin 2  (INT4)
Pin 2 : B (bleu)     ──[R3 220Ω]──> Mega Pin 3  (INT5)
Pin 3 : VCC (rouge)  ──────────────> Mega 5V
Pin 4 : GND (noir)   ──────────────> Mega GND
Pin 5 : NC
Pin 6 : Shield       ──────────────> GND
```

**Pull-ups externes** :
```
Mega Pin 2 ──[R4 4.7kΩ]── 5V
Mega Pin 3 ──[R5 4.7kΩ]── 5V
```

**Condensateurs découplage** :
```
VCC ──[C15 100nF]── GND  (au connecteur)
```

---

#### Encodeur Élévation (J6)
```
Pin 1 : A (blanc)    ──[R6 220Ω]──> Mega Pin 20 (INT1)
Pin 2 : B (bleu)     ──[R7 220Ω]──> Mega Pin 21 (INT0)
Pin 3 : VCC (rouge)  ──────────────> Mega 5V
Pin 4 : GND (noir)   ──────────────> Mega GND
Pin 5 : NC
Pin 6 : Shield       ──────────────> GND
```

**Pull-ups externes** :
```
Mega Pin 20 ──[R8 4.7kΩ]── 5V
Mega Pin 21 ──[R9 4.7kΩ]── 5V
```

**Condensateur découplage** :
```
VCC ──[C16 100nF]── GND
```

---

### Module VMA452 (Fins de Course)

**Connecteur** : JST-XH 6P
```
Pin 1 : VCC     ──────────────> Mega 5V
Pin 2 : GND     ──────────────> Mega GND
Pin 3 : OUT1    ──────────────> Mega Pin 26 (Az CW)
Pin 4 : OUT2    ──────────────> Mega Pin 27 (Az CCW)
Pin 5 : OUT3    ──────────────> Mega Pin 28 (El UP)
Pin 6 : OUT4    ──────────────> Mega Pin 29 (El DOWN)
```

**Découplage** :
```
VCC ──[C17 100nF]── GND
```

**Pull-ups** : Internes Mega (activés software)

---

### Switches Fins de Course

**Symbole** : 4× Microswitch NO (Normally Open)

**Câblage vers VMA452** :
```
Switch Az CW :
  Commun ──> VMA452 IN1
  NO     ──> GND

Switch Az CCW :
  Commun ──> VMA452 IN2
  NO     ──> GND

Switch El UP :
  Commun ──> VMA452 IN3
  NO     ──> GND

Switch El DOWN :
  Commun ──> VMA452 IN4
  NO     ──> GND
```

**Note** : VMA452 gère la logique (optocoupleurs internes)

---

## PAGE 4 : ETHERNET & COMMUNICATION

### W5500 Ethernet Module

**Connecteur** : Header 1×10 (2.54mm pitch)
```
Pin 1  : MISO    ──> Mega Pin 50
Pin 2  : MOSI    ──> Mega Pin 51
Pin 3  : SCK     ──> Mega Pin 52
Pin 4  : CS/SS   ──> Mega Pin 53
Pin 5  : RESET   ──> Mega Pin 49 (optionnel)
Pin 6  : INT     ──> NC (non utilisé)
Pin 7  : 3.3V    ──> Mega 3.3V
Pin 8  : GND     ──> Mega GND
Pin 9  : NC
Pin 10 : NC
```

**Pull-up RESET** (optionnel) :
```
Mega Pin 49 ──[R10 10kΩ]── 3.3V
```

**Découplages** :
```
3.3V ──[C18 10µF]──[C19 100nF]── GND
```

**Note** : Module W5500 inclut RJ45 + magnetics (pas de composants externes)

---

### Nextion Display

**Connecteur** : Header 1×4 (2.54mm pitch) ou JST-XH 4P
```
Pin 1 : TX (Nextion) ──> Mega Pin 19 (RX1)
Pin 2 : RX (Nextion) ──> Mega Pin 18 (TX1)
Pin 3 : VCC          ──> Mega 5V
Pin 4 : GND          ──> Mega GND
```

**Découplage** :
```
VCC ──[C20 100nF]── GND  (au connecteur)
```

**Note** : Pas de résistances série (UART 5V TTL direct)

---

### Communication Nano R4

**Connecteur J10** : JST-XH 4P (2.54mm pitch)
```
Pin 1 : TX (Mega)  ──> Mega Pin 16 (TX2) ──> Nano RX
Pin 2 : RX (Mega)  ──> Mega Pin 17 (RX2) ──> Nano TX
Pin 3 : 5V         ──> Mega 5V
Pin 4 : GND        ──> Mega GND
```

**Découplage** :
```
5V ──[C21 100nF]── GND  (au connecteur)
```

**Note schéma** : Ajouter note "UART 115200 baud vers Nano R4 (tête RF)"

---

## PAGE 5 : SENSORS (LOCAL RACK)

### DS18B20 Bus 1-Wire

**Connecteur J8** : JST-XH 3P
```
Pin 1 : VCC (rouge)  ──────────────> Mega 5V
Pin 2 : Data (jaune) ──[R11 4.7kΩ]─┬> Mega 5V
                       │            │
                       └────────────┴> Mega Pin 30
Pin 3 : GND (noir)   ──────────────> Mega GND
```

**Découplage** :
```
VCC ──[C22 100nF]── GND  (au connecteur)
```

**Note** : Pull-up 4.7kΩ **obligatoire** sur bus 1-Wire

---

### Diviseurs de Tension ADC

#### Tension 24V (Pin A2)
```
24V ──[R12 47kΩ 1%]──┬──[R13 12kΩ 1%]── GND
                     │
                     ├──[D3 Zener 5.1V]── GND
                     │
                     ├──[C23 100nF]── GND
                     │
                     └──[R14 1kΩ]──> Mega A2
```

**Calcul** : V_ADC = V_in × 12k / (47k + 12k) = V_in × 0.203

**Composants** :
- **R12** : 47kΩ ±1% métal film 1/4W
- **R13** : 12kΩ ±1% métal film 1/4W
- **D3** : 1N4733A (Zener 5.1V 1W)
- **R14** : 1kΩ (protection série ADC)
- **C23** : 100nF céramique (filtrage bruit)

---

#### Tension 13.8V (Pin A3)
```
13.8V ──[R15 22kΩ 1%]──┬──[R16 12kΩ 1%]── GND
                       │
                       ├──[D4 Zener 5.1V]── GND
                       │
                       ├──[C24 100nF]── GND
                       │
                       └──[R17 1kΩ]──> Mega A3
```

**Calcul** : V_ADC = 13.8V × 12k / (22k + 12k) = 4.87V

---

#### Tension 12V (Pin A4)
```
12V ──[R18 18kΩ 1%]──┬──[R19 12kΩ 1%]── GND
                     │
                     ├──[D5 Zener 5.1V]── GND
                     │
                     ├──[C25 100nF]── GND
                     │
                     └──[R20 1kΩ]──> Mega A4
```

**Calcul** : V_ADC = 12V × 12k / (18k + 12k) = 4.8V

---

#### Tension 5V (Pin A5)

**Option A : Direct** (si source fiable)
```
5V ──[R21 1kΩ]──> Mega A5
```

**Option B : Diviseur léger** (sécurité)
```
5V ──[R22 2.2kΩ]──┬──[R23 10kΩ]── GND
                  │
                  ├──[C26 100nF]── GND
                  │
                  └──> Mega A5
```

**Calcul** : V_ADC = 5V × 10k / (2.2k + 10k) = 4.1V

---

### Connecteur Tensions J9

**Bornier vis 5P** (5.08mm pitch)
```
Pin 1 : 24V input   (depuis PSU moteurs, après fusible)
Pin 2 : 13.8V input (depuis alim station)
Pin 3 : 12V input   (depuis PSU Mega)
Pin 4 : 5V input    (depuis pin 5V Mega, test)
Pin 5 : GND commun
```

---

## PAGE 6 : CONNECTORS SUMMARY

### Tableau Récapitulatif Connecteurs

| Ref | Type | Pins | Fonction | Câble Type |
|-----|------|------|----------|------------|
| **J1** | Bornier vis 2P | VIN, GND | Alim 12V Mega | 0.75mm² |
| **J2** | Bornier vis 2P | VIN, GND | Alim 24V moteurs | 1.5mm² |
| **J3** | Bornier vis 2P | OUT1, OUT2 | Moteur Azimuth | 1.0mm² |
| **J4** | Bornier vis 2P | OUT1, OUT2 | Moteur Élévation | 1.0mm² |
| **J5** | JST-XH 6P | A, B, VCC, GND, NC, Shield | Encodeur Az | Blindé 4 fils |
| **J6** | JST-XH 6P | A, B, VCC, GND, NC, Shield | Encodeur El | Blindé 4 fils |
| **J7** | JST-XH 6P | VCC, GND, OUT1-4 | VMA452 fins course | - |
| **J8** | JST-XH 3P | VCC, Data, GND | DS18B20 rack | - |
| **J9** | Bornier vis 5P | 24V, 13.8V, 12V, 5V, GND | Tensions monitoring | - |
| **J10** | JST-XH 4P | TX, RX, 5V, GND | UART Nano R4 | Blindé Cat6 |
| **J11** | Header 1×10 | MISO, MOSI, SCK... | W5500 module | - |
| **J12** | Header 1×4 | TX, RX, VCC, GND | Nextion display | - |
| **J13** | Header 1×19 | IN1/2, D1/2, SF, FB... | Dual MC33926 | - |

---

### Schéma Bloc Connexions Externes
```
┌─────────────────────────────────────────────────┐
│              PCB ARDUINO MEGA                   │
│                                                 │
│  J1 ◄─── 12V PSU                                │
│  J2 ◄─── 24V PSU                                │
│                                                 │
│  J3 ───► Moteur Az                              │
│  J4 ───► Moteur El                              │
│                                                 │
│  J5 ◄─── Encodeur Az (6P blindé)               │
│  J6 ◄─── Encodeur El (6P blindé)               │
│                                                 │
│  J7 ◄─── VMA452 + 4× Switches                  │
│  J8 ◄─── DS18B20 rack                           │
│  J9 ◄─── Tensions (24V, 13.8V, 12V, 5V)        │
│                                                 │
│  J10 ◄──► Nano R4 (UART, 4P Cat6)              │
│  J11 ◄──► W5500 + RJ45 Ethernet                │
│  J12 ◄──► Nextion Display                      │
│                                                 │
│  J13 ◄──► Pololu Dual MC33926 Module           │
│                                                 │
└─────────────────────────────────────────────────┘
```

---

## NOTES LAYOUT PCB

### Format & Dimensions

**Taille recommandée** : 15×20 cm (compatible rack 19" avec brackets)
**Couches** : 2 couches (Top + Bottom, suffisant)
**Épaisseur cuivre** : 35µm (1 oz) standard

---

### Zones Fonctionnelles
```
┌──────────────────────────────────────────────┐
│  ZONE PUISSANCE (gauche)                     │
│  • Borniers alim J1, J2                      │
│  • MC33926 module (avec dissipation)         │
│  • Gros condensateurs C3, C11                │
│  • Borniers moteurs J3, J4                   │
├──────────────────────────────────────────────┤
│  ZONE LOGIQUE (centre)                       │
│  • Arduino Mega Pro 2560                     │
│  • W5500 Ethernet                            │
│  • Connecteurs encodeurs J5, J6              │
│  • VMA452 module                             │
├──────────────────────────────────────────────┤
│  ZONE CAPTEURS (droite)                      │
│  • Diviseurs tensions (R/D/C)                │
│  • DS18B20 connecteur J8                     │
│  • Connecteurs communication (J10, J12)      │
└──────────────────────────────────────────────┘
```

**Séparation claire** : Éviter croisement pistes puissance/signaux

---

### Règles de Routage

#### Pistes Puissance

| Signal | Largeur | Notes |
|--------|---------|-------|
| **24V / GND moteurs** | **2.0 mm** | 4A continu |
| **12V** | 1.0 mm | <1A |
| **5V** | 0.8 mm | <500mA |

**Plan de masse** : Couche Bottom = GND plane continu

---

#### Pistes Signaux

| Type | Largeur | Notes |
|------|---------|-------|
| **PWM moteurs** | 0.5 mm | Court, direct Mega→MC33926 |
| **Encodeurs** | 0.3 mm | Paires différentielles si possible |
| **UART/SPI** | 0.4 mm | Longueurs équilibrées |
| **ADC** | 0.3 mm | Éloigner des PWM/SPI |

---

### Placement Composants

**Découplages** :
- Condensateurs 100nF **très proches** pins VCC/GND des ICs (<5mm)
- Via GND immédiatement après condensateur

**MC33926** :
- Condensateurs C11/C12 à **<1cm** de VIN/GND
- Garder espace autour pour dissipation thermique

**Diviseurs tensions** :
- Résistances proches les unes des autres
- Zener proche de la résistance R2
- Condensateur filtrage proche de l'ADC

---

### Sérigraphie (Silkscreen)

**Face TOP** :
- Tous les designators (R1, C1, U1, J1...)
- Polarités électrolytiques (+/-)
- Pin 1 des connecteurs (carré ou triangle)
- Tensions nominales près borniers (24V, 12V, 5V)
- Noms signaux importants (Az, El, TX, RX, SDA, SCL)

**Face BOTTOM** :
- Logo projet : "EME Controller ON7KGK"
- Version : "v1.0"
- Date : "2026"

---

### Test Points

**Ajouter pads de test** (1mm, non masqués) :

- 5V
- 3.3V
- 24V
- GND (×3 minimum)
- PWM M1IN1, M1IN2, M2IN1, M2IN2
- Encodeur signals (A/B)
- UART TX/RX

**Utile** : Debug avec oscilloscope/multimètre

---

## VÉRIFICATIONS PRÉ-FABRICATION

### Checklist Schéma ERC (Electrical Rule Check)

- [ ] Tous les pins Mega connectés ou marqués NC
- [ ] Pas de pins en conflit (ex: même interrupt)
- [ ] Pull-ups sur toutes entrées digitales nécessaires
- [ ] Découplages sur tous les VCC/GND
- [ ] Toutes les masses connectées (GND unique)
- [ ] Polarités condensateurs électrolytiques correctes
- [ ] Valeurs résistances diviseurs vérifiées (calcul)
- [ ] Fusibles dimensionnés (2A pour 12V, 5A pour 24V)
- [ ] Connecteurs repérés (J1, J2... dans l'ordre)
- [ ] Pas de nets non connectés (erreurs)

---

### Checklist Layout DRC (Design Rule Check)

- [ ] Clearance > 0.3mm (signaux)
- [ ] Clearance > 0.5mm (puissance)
- [ ] Largeur pistes selon courant (voir tableau)
- [ ] Pas de pistes à angle droit (préférer 45° ou arcs)
- [ ] Vias thermiques sur pads composants puissance
- [ ] Trous montage M3 (4 coins minimum, ⌀3.2mm)
- [ ] Zone keep-out autour trous (⌀5mm)
- [ ] Pas de composants sous Mega (risque court-circuit)
- [ ] Test points accessibles (pas de composants dessus)
- [ ] Sérigraphie lisible (taille texte >0.8mm)

---

## FABRICATION

### Fournisseurs Recommandés

**JLCPCB** (Chine) :
- Prix : ~5€ les 5 PCB (10×15 cm, 2 couches)
- Délai : 2-3 jours fabrication + 7-10 jours livraison
- Couleur : Vert (standard, gratuit)
- Finition : HASL (standard, soudure facile)
- URL : https://jlcpcb.com

**PCBWay** (Chine) :
- Prix : ~10€ les 5 PCB
- Délai : Similaire JLCPCB
- Qualité : Légèrement supérieure
- Options : Plus de finitions disponibles

**Options upgrades** :
- **ENIG** (finition or) : +10€ mais meilleure soudabilité long terme
- **Masque blanc** : +5€ mais sérigraphie plus lisible
- **Épaisseur 1.6mm** : Standard, robuste

---

### Fichiers Gerber à Fournir

**Export depuis EasyEDA** → Fichier → Export → Gerber

**Fichiers générés** :
```
.GTL  → Top Copper (cuivre supérieur)
.GBL  → Bottom Copper (cuivre inférieur)
.GTO  → Top Silkscreen (sérigraphie supérieure)
.GBO  → Bottom Silkscreen (sérigraphie inférieure)
.GTS  → Top Soldermask (vernis supérieur)
.GBS  → Bottom Soldermask (vernis inférieur)
.GKO  → Board Outline (contour PCB)
.TXT  → Drill file (perçages)
```

**Compression** : Tout zipper en `gerber.zip`

**Upload** : Sur site fabricant → Vérification automatique → Commande

---

## ASSEMBLAGE (ORDRE RECOMMANDÉ)

### Étape 1 : Composants SMD (si présents)

Souder en premier (accès plus facile avant composants traversants)

---

### Étape 2 : Résistances

**Ordre** : Plat sur PCB, pattes pliées 90°
- Identifier valeur (code couleur ou marquage)
- Souder
- Couper pattes ras

---

### Étape 3 : Diodes

**ATTENTION polarité** : Bande = cathode (côté marqué sur PCB)
- D1, D2 (Schottky)
- D3, D4, D5 (Zener)

---

### Étape 4 : Condensateurs Céramiques

**Pas de polarité**, souder directement

---

### Étape 5 : Condensateurs Électrolytiques

**ATTENTION polarité** : Patte longue = +, marquage PCB
- C1, C3, C5, C7, C11 (gros électrolytiques)

---

### Étape 6 : Sockets / Headers

- Sockets IC (si utilisés pour Mega, modules)
- Headers mâles (pour modules enfichables)
- Borniers à vis (alimentation, moteurs, tensions)

**Astuce** : Maintenir perpendiculaire avec ruban adhésif pendant soudure

---

### Étape 7 : Fuseholders

Porte-fusibles PCB (F1, F2)

---

### Étape 8 : LEDs

**ATTENTION polarité** : Patte longue = anode (+)

---

### Étape 9 : Connecteurs JST-XH

Headers mâles pour JST (encodeurs, capteurs, UART)

---

### Étape 10 : Modules (dernier)

- Arduino Mega (socketé ou soudé direct)
- W5500 Ethernet
- Pololu MC33926
- VMA452

**Recommandation** : Utiliser sockets/headers pour faciliter remplacement

---

## INSPECTION AVANT MISE SOUS TENSION

### Vérifications Visuelles

- [ ] Pas de ponts de soudure (court-circuits)
- [ ] Pas de soudures froides (brillantes)
- [ ] Toutes les pattes soudées
- [ ] Polarités correctes (électrolytiques, diodes, LEDs)
- [ ] Composants bien positionnés (pas de décalage)

---

### Tests Multimètre (HORS TENSION)

**Continuité GND** :
- [ ] Tous les GND connectés (0Ω)

**Pas de court-circuits** :
- [ ] 5V ↔ GND : ∞ (ou >10kΩ)
- [ ] 12V ↔ GND : ∞
- [ ] 24V ↔ GND : ∞
- [ ] 3.3V ↔ GND : ∞

**Résistances diviseurs** :
- [ ] Mesurer R12+R13, R15+R16, R18+R19 → Valeurs attendues

---

## PREMIER POWER-ON (MISE SOUS TENSION)

### Étape 1 : Mega Seul (Sans Modules)

1. **Connecter 12V sur J1** (sans 24V)
2. **Observer** :
   - LED power (LED1) allumée → ✅
   - Pas de fumée, pas de composant chaud → ✅
3. **Mesurer** :
   - Pin 5V Mega : 5.0V ±0.1V → ✅
   - Pin 3.3V Mega : 3.3V ±0.1V → ✅

**Si OK** : Éteindre, passer étape 2

---

### Étape 2 : Ajouter W5500

1. **Enficher W5500** sur header J11
2. **Alimenter 12V**
3. **Mesurer** :
   - 3.3V sur W5500 : 3.3V ±0.1V → ✅
4. **Connecter Ethernet**
5. **Upload firmware test** (ping)
6. **Ping 192.168.1.177** → ✅

---

### Étape 3 : Ajouter MC33926

1. **Enficher MC33926** sur header J13
2. **Connecter 24V sur J2**
3. **NE PAS connecter moteurs encore**
4. **Alimenter 12V + 24V**
5. **Mesurer** :
   - 24V sur VIN MC33926 → ✅
6. **Upload firmware test** (PWM basique)
7. **Oscillo sur M1IN1/M1IN2** : Signaux PWM OK → ✅

---

### Étape 4 : Ajouter Moteurs (À Vide)

1. **Connecter moteurs sur J3, J4** (sans charge mécanique)
2. **Upload firmware contrôle**
3. **Test rotation courte** (1 seconde)
4. **Vérifier** :
   - Sens rotation correct → ✅
   - Courant <0.5A (à vide) → ✅
5. **Mesurer** avec pince ampèremétrique

---

### Étape 5 : Tous Périphériques

1. **Connecter** : Encodeurs (J5, J6), VMA452 (J7), DS18B20 (J8), Nextion (J12), Nano (J10)
2. **Tests unitaires** chaque bloc (voir TODO_PHASES.md)

---

## TROUBLESHOOTING HARDWARE

| Problème | Cause Probable | Solution |
|----------|----------------|----------|
| **Mega ne boot pas** | Court-circuit alim | Vérifier continuité 5V-GND |
| **LED power éteinte** | Fusible F1 grillé | Remplacer, chercher cause |
| **W5500 pas de ping** | Pas de 3.3V | Vérifier régulateur Mega |
| **Moteur ne tourne pas** | MC33926 disabled | Vérifier EN=HIGH, D1/D2=LOW |
| **Surintensité permanente** | Court-circuit moteur | Mesurer résistance moteur (~12Ω) |
| **Encodeurs pas de signal** | Câble inversé | Vérifier pinout VCC/GND |
| **ADC lecture erratique** | Bruit, pas de filtrage | Vérifier C23-C26 présents |
| **Nextion écran noir** | Pas d'alim ou mauvais baud | Vérifier 5V + baud 9600 |

---

## FICHIERS COMPLÉMENTAIRES

**Fichiers à créer avec schéma** :
- `BOM.csv` : Bill of Materials (export EasyEDA)
- `CPL.csv` : Component Placement List (si assemblage automatique)
- `Gerber.zip` : Fichiers fabrication
- `Schematic.pdf` : PDF du schéma (6 pages)

---

**Version** : 1.0
**Date** : 2026-01-30
**Auteur** : ON7KGK - JO20BM85DP
**Projet** : EME 10 GHz Station Controller
```

# NOTES SCHÉMATIQUE EASYEDA - Station EME Controller

## Vue d'ensemble

Ce document guide la création du schéma électrique complet dans EasyEDA Pro. Le schéma est organisé en **6 feuilles** (pages) pour une lisibilité optimale.

---

## STRUCTURE SCHÉMA (6 PAGES)
```
Page 1 : Power & MCU
Page 2 : Motor Drivers
Page 3 : Encoders & Limit Switches
Page 4 : Ethernet & Communication
Page 5 : Sensors (Local Rack)
Page 6 : Connectors Summary
```

**Format recommandé** : A4 paysage (297×210 mm)

---

## PAGE 1 : POWER & MCU

### Arduino Mega Pro 2560

**Symbole** : Créer symbole custom ou utiliser générique Mega 2560

**Groupes de pins** :
- **Power** : VIN, 5V, 3.3V, GND, IOREF
- **Digital** : 0-53 (regrouper par fonction)
- **Analog** : A0-A15
- **Communication** : Serial 0-3, SPI, I²C (noter mais pas utilisé)

**Note importante** : Ajouter labels texte pour chaque groupe :
```
ENCODERS (2, 3, 20, 21)
PWM MOTORS (6, 7, 11, 12)
MC33926 CONTROL (4, 5, 8, 9, 10, 22, 23, 24, 25)
LIMIT SWITCHES (26-29)
SENSORS (30, A0-A5)
UART NEXTION (18, 19)
UART NANO (16, 17)
SPI ETHERNET (50-53)
```

---

### Alimentation 12V (Logique)

**Schéma** :
```
PSU 12V ──[F1 2A]──[D1]──[C1 100µF]──[C2 100nF]──┬──> VIN (Mega)
                                                  │
GND ──────────────────────────────────────────────┴──> GND (Mega)
```

**Composants** :

| Ref | Valeur | Package | Notes |
|-----|--------|---------|-------|
| **F1** | Fusible 2A rapide | 5×20mm | Littelfuse ou équivalent |
| **D1** | 1N5822 (Schottky 3A) | DO-201AD | Protection inversion polarité |
| **C1** | 100µF / 16V | Radial 6.3mm | Électrolytique (bulk) |
| **C2** | 100nF / 50V | 5mm pitch | Céramique X7R (HF) |
| **LED1** | LED verte 3mm | TH | Power indicator |
| **R1** | 1kΩ 1/4W | Axial | Résistance série LED |

**Connexion LED Power** :
```
5V ──[R1 1kΩ]──[LED1]──> GND
```

---

### Alimentation 24V (Moteurs)

**Schéma** :
```
PSU 24V ──[F2 5A]──[D2]──[C3 470µF]──[C4 100nF]──> VIN (MC33926)
                                                   │
GND ───────────────────────────────────────────────> GND (MC33926)
```

**Composants** :

| Ref | Valeur | Package | Notes |
|-----|--------|---------|-------|
| **F2** | Fusible 5A rapide | 5×20mm | Protection moteurs |
| **D2** | 1N5822 (Schottky 3A) | DO-201AD | Protection inversion |
| **C3** | 470µF / 35V | Radial 10mm | Électrolytique (bulk capacitor) |
| **C4** | 100nF / 50V | 5mm pitch | Céramique X7R (HF découplage) |

**Note** : Placer C3 et C4 **très proche** des bornes VIN/GND du module MC33926 (<2cm)

---

### Connecteurs Alimentation

**J1 : Alimentation 12V** (Bornier vis 2P, 5.08mm pitch)
```
Pin 1 : +12V (depuis PSU)
Pin 2 : GND
```

**J2 : Alimentation 24V** (Bornier vis 2P, 5.08mm pitch)
```
Pin 1 : +24V (depuis PSU)
Pin 2 : GND
```

**Capacité vis** : Minimum 10A pour J2 (câble 1.5mm²)

---

### Découplages Locaux

**Mega** :
```
VIN ──[C5 100µF]──[C6 100nF]── GND
5V  ──[C7 100µF]──[C8 100nF]── GND
```

**W5500** :
```
3.3V ──[C9 10µF]──[C10 100nF]── GND
```

**Tous les IC** : Ajouter 100nF **très proche** de chaque pin VCC/GND

---

## PAGE 2 : MOTOR DRIVERS

### Module Pololu Dual MC33926

**Symbole** : Bloc rectangulaire avec 19 pins signaux + 4 borniers puissance

**Connexions Mega → MC33926** :

#### PWM Moteurs

| MC33926 Pin | Mega Pin | Signal | Notes |
|-------------|----------|--------|-------|
| **M1IN1** | 11 | Az PWM 1 | Timer 1 |
| **M1IN2** | 12 | Az PWM 2 | Timer 1 |
| **M2IN1** | 6 | El PWM 1 | Timer 4 |
| **M2IN2** | 7 | El PWM 2 | Timer 4 |

**Ajouter labels** : "PWM Az (Timer 1)" et "PWM El (Timer 4)"

---

#### Contrôle Digital

| MC33926 Pin | Mega Pin | Signal | Logique |
|-------------|----------|--------|---------|
| **M1D1** | 22 | Az Disable 1 | LOW = actif |
| **M1D2** | 4 | Az Disable 2 | LOW = actif |
| **M2D1** | 24 | El Disable 1 | LOW = actif |
| **M2D2** | 5 | El Disable 2 | LOW = actif |
| **M1SF** | 23 | Az Status Flag | LOW = défaut |
| **M2SF** | 25 | El Status Flag | LOW = défaut |
| **EN** | 8 | Enable global | HIGH = ON |
| **SLEW** | 9 | Slew rate | HIGH = rapide |
| **INV** | 10 | Inversion | LOW = normal |

**Ajouter pull-ups internes sur SF** : Symbole résistance pointillée vers VCC

---

#### Feedback Courant

| MC33926 Pin | Mega Pin | Signal | Notes |
|-------------|----------|--------|-------|
| **M1FB** | A0 | Az Current | 0.525 V/A |
| **M2FB** | A1 | El Current | 0.525 V/A |

**Ajouter note** : "0.525 V/A gain"

---

### Alimentation Module

**Connexion VIN (puissance)** :
```
24V ──[C11 100µF]──[C12 100nF]──> MC33926 VIN
                                   │
GND ───────────────────────────────> MC33926 GND
```

**C11, C12** : Placer **immédiatement** à l'entrée VIN du module (<1cm)

**VDD (logique)** :
```
Mega 5V ──> MC33926 VDD (optionnel, module a régulateur interne)
```

---

### Sorties Moteurs

**Borniers vis 2P** (5.08mm pitch, 10A min) :

**J3 : Moteur Azimuth**
```
Pin 1 : M1OUT1 (depuis MC33926)
Pin 2 : M1OUT2
```

**J4 : Moteur Élévation**
```
Pin 1 : M2OUT1 (depuis MC33926)
Pin 2 : M2OUT2
```

**Ajouter symbole moteur** :
```
M1OUT1 ──┐
         ├──[M] Moteur Az
M1OUT2 ──┘
```

---

### Condensateurs Moteurs (Optionnel)

**Suppression EMI** :
```
M1OUT1 ──[C13 100nF X2]── M1OUT2
M2OUT1 ──[C14 100nF X2]── M2OUT2
```

**Type** : Céramique X2 (250V AC rated)
**Placement** : Directement sur bornes moteur (côté moteur, pas PCB)

---

## PAGE 3 : ENCODERS & LIMIT SWITCHES

### Encodeurs HH12 (×2)

**Connecteurs** : JST-XH 6P (2.54mm pitch)

#### Encodeur Azimuth (J5)
```
Pin 1 : A (blanc)    ──[R2 220Ω]──> Mega Pin 2  (INT4)
Pin 2 : B (bleu)     ──[R3 220Ω]──> Mega Pin 3  (INT5)
Pin 3 : VCC (rouge)  ──────────────> Mega 5V
Pin 4 : GND (noir)   ──────────────> Mega GND
Pin 5 : NC
Pin 6 : Shield       ──────────────> GND
```

**Pull-ups externes** :
```
Mega Pin 2 ──[R4 4.7kΩ]── 5V
Mega Pin 3 ──[R5 4.7kΩ]── 5V
```

**Condensateurs découplage** :
```
VCC ──[C15 100nF]── GND  (au connecteur)
```

---

#### Encodeur Élévation (J6)
```
Pin 1 : A (blanc)    ──[R6 220Ω]──> Mega Pin 20 (INT1)
Pin 2 : B (bleu)     ──[R7 220Ω]──> Mega Pin 21 (INT0)
Pin 3 : VCC (rouge)  ──────────────> Mega 5V
Pin 4 : GND (noir)   ──────────────> Mega GND
Pin 5 : NC
Pin 6 : Shield       ──────────────> GND
```

**Pull-ups externes** :
```
Mega Pin 20 ──[R8 4.7kΩ]── 5V
Mega Pin 21 ──[R9 4.7kΩ]── 5V
```

**Condensateur découplage** :
```
VCC ──[C16 100nF]── GND
```

---

### Module VMA452 (Fins de Course)

**Connecteur** : JST-XH 6P
```
Pin 1 : VCC     ──────────────> Mega 5V
Pin 2 : GND     ──────────────> Mega GND
Pin 3 : OUT1    ──────────────> Mega Pin 26 (Az CW)
Pin 4 : OUT2    ──────────────> Mega Pin 27 (Az CCW)
Pin 5 : OUT3    ──────────────> Mega Pin 28 (El UP)
Pin 6 : OUT4    ──────────────> Mega Pin 29 (El DOWN)
```

**Découplage** :
```
VCC ──[C17 100nF]── GND
```

**Pull-ups** : Internes Mega (activés software)

---

### Switches Fins de Course

**Symbole** : 4× Microswitch NO (Normally Open)

**Câblage vers VMA452** :
```
Switch Az CW :
  Commun ──> VMA452 IN1
  NO     ──> GND

Switch Az CCW :
  Commun ──> VMA452 IN2
  NO     ──> GND

Switch El UP :
  Commun ──> VMA452 IN3
  NO     ──> GND

Switch El DOWN :
  Commun ──> VMA452 IN4
  NO     ──> GND
```

**Note** : VMA452 gère la logique (optocoupleurs internes)

---

## PAGE 4 : ETHERNET & COMMUNICATION

### W5500 Ethernet Module

**Connecteur** : Header 1×10 (2.54mm pitch)
```
Pin 1  : MISO    ──> Mega Pin 50
Pin 2  : MOSI    ──> Mega Pin 51
Pin 3  : SCK     ──> Mega Pin 52
Pin 4  : CS/SS   ──> Mega Pin 53
Pin 5  : RESET   ──> Mega Pin 49 (optionnel)
Pin 6  : INT     ──> NC (non utilisé)
Pin 7  : 3.3V    ──> Mega 3.3V
Pin 8  : GND     ──> Mega GND
Pin 9  : NC
Pin 10 : NC
```

**Pull-up RESET** (optionnel) :
```
Mega Pin 49 ──[R10 10kΩ]── 3.3V
```

**Découplages** :
```
3.3V ──[C18 10µF]──[C19 100nF]── GND
```

**Note** : Module W5500 inclut RJ45 + magnetics (pas de composants externes)

---

### Nextion Display

**Connecteur** : Header 1×4 (2.54mm pitch) ou JST-XH 4P
```
Pin 1 : TX (Nextion) ──> Mega Pin 19 (RX1)
Pin 2 : RX (Nextion) ──> Mega Pin 18 (TX1)
Pin 3 : VCC          ──> Mega 5V
Pin 4 : GND          ──> Mega GND
```

**Découplage** :
```
VCC ──[C20 100nF]── GND  (au connecteur)
```

**Note** : Pas de résistances série (UART 5V TTL direct)

---

### Communication Nano R4

**Connecteur J10** : JST-XH 4P (2.54mm pitch)
```
Pin 1 : TX (Mega)  ──> Mega Pin 16 (TX2) ──> Nano RX
Pin 2 : RX (Mega)  ──> Mega Pin 17 (RX2) ──> Nano TX
Pin 3 : 5V         ──> Mega 5V
Pin 4 : GND        ──> Mega GND
```

**Découplage** :
```
5V ──[C21 100nF]── GND  (au connecteur)
```

**Note schéma** : Ajouter note "UART 115200 baud vers Nano R4 (tête RF)"

---

## PAGE 5 : SENSORS (LOCAL RACK)

### DS18B20 Bus 1-Wire

**Connecteur J8** : JST-XH 3P
```
Pin 1 : VCC (rouge)  ──────────────> Mega 5V
Pin 2 : Data (jaune) ──[R11 4.7kΩ]─┬> Mega 5V
                       │            │
                       └────────────┴> Mega Pin 30
Pin 3 : GND (noir)   ──────────────> Mega GND
```

**Découplage** :
```
VCC ──[C22 100nF]── GND  (au connecteur)
```

**Note** : Pull-up 4.7kΩ **obligatoire** sur bus 1-Wire

---

### Diviseurs de Tension ADC

#### Tension 24V (Pin A2)
```
24V ──[R12 47kΩ 1%]──┬──[R13 12kΩ 1%]── GND
                     │
                     ├──[D3 Zener 5.1V]── GND
                     │
                     ├──[C23 100nF]── GND
                     │
                     └──[R14 1kΩ]──> Mega A2
```

**Calcul** : V_ADC = V_in × 12k / (47k + 12k) = V_in × 0.203

**Composants** :
- **R12** : 47kΩ ±1% métal film 1/4W
- **R13** : 12kΩ ±1% métal film 1/4W
- **D3** : 1N4733A (Zener 5.1V 1W)
- **R14** : 1kΩ (protection série ADC)
- **C23** : 100nF céramique (filtrage bruit)

---

#### Tension 13.8V (Pin A3)
```
13.8V ──[R15 22kΩ 1%]──┬──[R16 12kΩ 1%]── GND
                       │
                       ├──[D4 Zener 5.1V]── GND
                       │
                       ├──[C24 100nF]── GND
                       │
                       └──[R17 1kΩ]──> Mega A3
```

**Calcul** : V_ADC = 13.8V × 12k / (22k + 12k) = 4.87V

---

#### Tension 12V (Pin A4)
```
12V ──[R18 18kΩ 1%]──┬──[R19 12kΩ 1%]── GND
                     │
                     ├──[D5 Zener 5.1V]── GND
                     │
                     ├──[C25 100nF]── GND
                     │
                     └──[R20 1kΩ]──> Mega A4
```

**Calcul** : V_ADC = 12V × 12k / (18k + 12k) = 4.8V

---

#### Tension 5V (Pin A5)

**Option A : Direct** (si source fiable)
```
5V ──[R21 1kΩ]──> Mega A5
```

**Option B : Diviseur léger** (sécurité)
```
5V ──[R22 2.2kΩ]──┬──[R23 10kΩ]── GND
                  │
                  ├──[C26 100nF]── GND
                  │
                  └──> Mega A5
```

**Calcul** : V_ADC = 5V × 10k / (2.2k + 10k) = 4.1V

---

### Connecteur Tensions J9

**Bornier vis 5P** (5.08mm pitch)
```
Pin 1 : 24V input   (depuis PSU moteurs, après fusible)
Pin 2 : 13.8V input (depuis alim station)
Pin 3 : 12V input   (depuis PSU Mega)
Pin 4 : 5V input    (depuis pin 5V Mega, test)
Pin 5 : GND commun
```

---

## PAGE 6 : CONNECTORS SUMMARY

### Tableau Récapitulatif Connecteurs

| Ref | Type | Pins | Fonction | Câble Type |
|-----|------|------|----------|------------|
| **J1** | Bornier vis 2P | VIN, GND | Alim 12V Mega | 0.75mm² |
| **J2** | Bornier vis 2P | VIN, GND | Alim 24V moteurs | 1.5mm² |
| **J3** | Bornier vis 2P | OUT1, OUT2 | Moteur Azimuth | 1.0mm² |
| **J4** | Bornier vis 2P | OUT1, OUT2 | Moteur Élévation | 1.0mm² |
| **J5** | JST-XH 6P | A, B, VCC, GND, NC, Shield | Encodeur Az | Blindé 4 fils |
| **J6** | JST-XH 6P | A, B, VCC, GND, NC, Shield | Encodeur El | Blindé 4 fils |
| **J7** | JST-XH 6P | VCC, GND, OUT1-4 | VMA452 fins course | - |
| **J8** | JST-XH 3P | VCC, Data, GND | DS18B20 rack | - |
| **J9** | Bornier vis 5P | 24V, 13.8V, 12V, 5V, GND | Tensions monitoring | - |
| **J10** | JST-XH 4P | TX, RX, 5V, GND | UART Nano R4 | Blindé Cat6 |
| **J11** | Header 1×10 | MISO, MOSI, SCK... | W5500 module | - |
| **J12** | Header 1×4 | TX, RX, VCC, GND | Nextion display | - |
| **J13** | Header 1×19 | IN1/2, D1/2, SF, FB... | Dual MC33926 | - |

---

### Schéma Bloc Connexions Externes
```
┌─────────────────────────────────────────────────┐
│              PCB ARDUINO MEGA                   │
│                                                 │
│  J1 ◄─── 12V PSU                                │
│  J2 ◄─── 24V PSU                                │
│                                                 │
│  J3 ───► Moteur Az                              │
│  J4 ───► Moteur El                              │
│                                                 │
│  J5 ◄─── Encodeur Az (6P blindé)               │
│  J6 ◄─── Encodeur El (6P blindé)               │
│                                                 │
│  J7 ◄─── VMA452 + 4× Switches                  │
│  J8 ◄─── DS18B20 rack                           │
│  J9 ◄─── Tensions (24V, 13.8V, 12V, 5V)        │
│                                                 │
│  J10 ◄──► Nano R4 (UART, 4P Cat6)              │
│  J11 ◄──► W5500 + RJ45 Ethernet                │
│  J12 ◄──► Nextion Display                      │
│                                                 │
│  J13 ◄──► Pololu Dual MC33926 Module           │
│                                                 │
└─────────────────────────────────────────────────┘
```

---

## NOTES LAYOUT PCB

### Format & Dimensions

**Taille recommandée** : 15×20 cm (compatible rack 19" avec brackets)
**Couches** : 2 couches (Top + Bottom, suffisant)
**Épaisseur cuivre** : 35µm (1 oz) standard

---

### Zones Fonctionnelles
```
┌──────────────────────────────────────────────┐
│  ZONE PUISSANCE (gauche)                     │
│  • Borniers alim J1, J2                      │
│  • MC33926 module (avec dissipation)         │
│  • Gros condensateurs C3, C11                │
│  • Borniers moteurs J3, J4                   │
├──────────────────────────────────────────────┤
│  ZONE LOGIQUE (centre)                       │
│  • Arduino Mega Pro 2560                     │
│  • W5500 Ethernet                            │
│  • Connecteurs encodeurs J5, J6              │
│  • VMA452 module                             │
├──────────────────────────────────────────────┤
│  ZONE CAPTEURS (droite)                      │
│  • Diviseurs tensions (R/D/C)                │
│  • DS18B20 connecteur J8                     │
│  • Connecteurs communication (J10, J12)      │
└──────────────────────────────────────────────┘
```

**Séparation claire** : Éviter croisement pistes puissance/signaux

---

### Règles de Routage

#### Pistes Puissance

| Signal | Largeur | Notes |
|--------|---------|-------|
| **24V / GND moteurs** | **2.0 mm** | 4A continu |
| **12V** | 1.0 mm | <1A |
| **5V** | 0.8 mm | <500mA |

**Plan de masse** : Couche Bottom = GND plane continu

---

#### Pistes Signaux

| Type | Largeur | Notes |
|------|---------|-------|
| **PWM moteurs** | 0.5 mm | Court, direct Mega→MC33926 |
| **Encodeurs** | 0.3 mm | Paires différentielles si possible |
| **UART/SPI** | 0.4 mm | Longueurs équilibrées |
| **ADC** | 0.3 mm | Éloigner des PWM/SPI |

---

### Placement Composants

**Découplages** :
- Condensateurs 100nF **très proches** pins VCC/GND des ICs (<5mm)
- Via GND immédiatement après condensateur

**MC33926** :
- Condensateurs C11/C12 à **<1cm** de VIN/GND
- Garder espace autour pour dissipation thermique

**Diviseurs tensions** :
- Résistances proches les unes des autres
- Zener proche de la résistance R2
- Condensateur filtrage proche de l'ADC

---

### Sérigraphie (Silkscreen)

**Face TOP** :
- Tous les designators (R1, C1, U1, J1...)
- Polarités électrolytiques (+/-)
- Pin 1 des connecteurs (carré ou triangle)
- Tensions nominales près borniers (24V, 12V, 5V)
- Noms signaux importants (Az, El, TX, RX, SDA, SCL)

**Face BOTTOM** :
- Logo projet : "EME Controller ON7KGK"
- Version : "v1.0"
- Date : "2026"

---

### Test Points

**Ajouter pads de test** (1mm, non masqués) :

- 5V
- 3.3V
- 24V
- GND (×3 minimum)
- PWM M1IN1, M1IN2, M2IN1, M2IN2
- Encodeur signals (A/B)
- UART TX/RX

**Utile** : Debug avec oscilloscope/multimètre

---

## VÉRIFICATIONS PRÉ-FABRICATION

### Checklist Schéma ERC (Electrical Rule Check)

- [ ] Tous les pins Mega connectés ou marqués NC
- [ ] Pas de pins en conflit (ex: même interrupt)
- [ ] Pull-ups sur toutes entrées digitales nécessaires
- [ ] Découplages sur tous les VCC/GND
- [ ] Toutes les masses connectées (GND unique)
- [ ] Polarités condensateurs électrolytiques correctes
- [ ] Valeurs résistances diviseurs vérifiées (calcul)
- [ ] Fusibles dimensionnés (2A pour 12V, 5A pour 24V)
- [ ] Connecteurs repérés (J1, J2... dans l'ordre)
- [ ] Pas de nets non connectés (erreurs)

---

### Checklist Layout DRC (Design Rule Check)

- [ ] Clearance > 0.3mm (signaux)
- [ ] Clearance > 0.5mm (puissance)
- [ ] Largeur pistes selon courant (voir tableau)
- [ ] Pas de pistes à angle droit (préférer 45° ou arcs)
- [ ] Vias thermiques sur pads composants puissance
- [ ] Trous montage M3 (4 coins minimum, ⌀3.2mm)
- [ ] Zone keep-out autour trous (⌀5mm)
- [ ] Pas de composants sous Mega (risque court-circuit)
- [ ] Test points accessibles (pas de composants dessus)
- [ ] Sérigraphie lisible (taille texte >0.8mm)

---

## FABRICATION

### Fournisseurs Recommandés

**JLCPCB** (Chine) :
- Prix : ~5€ les 5 PCB (10×15 cm, 2 couches)
- Délai : 2-3 jours fabrication + 7-10 jours livraison
- Couleur : Vert (standard, gratuit)
- Finition : HASL (standard, soudure facile)
- URL : https://jlcpcb.com

**PCBWay** (Chine) :
- Prix : ~10€ les 5 PCB
- Délai : Similaire JLCPCB
- Qualité : Légèrement supérieure
- Options : Plus de finitions disponibles

**Options upgrades** :
- **ENIG** (finition or) : +10€ mais meilleure soudabilité long terme
- **Masque blanc** : +5€ mais sérigraphie plus lisible
- **Épaisseur 1.6mm** : Standard, robuste

---

### Fichiers Gerber à Fournir

**Export depuis EasyEDA** → Fichier → Export → Gerber

**Fichiers générés** :
```
.GTL  → Top Copper (cuivre supérieur)
.GBL  → Bottom Copper (cuivre inférieur)
.GTO  → Top Silkscreen (sérigraphie supérieure)
.GBO  → Bottom Silkscreen (sérigraphie inférieure)
.GTS  → Top Soldermask (vernis supérieur)
.GBS  → Bottom Soldermask (vernis inférieur)
.GKO  → Board Outline (contour PCB)
.TXT  → Drill file (perçages)
```

**Compression** : Tout zipper en `gerber.zip`

**Upload** : Sur site fabricant → Vérification automatique → Commande

---

## ASSEMBLAGE (ORDRE RECOMMANDÉ)

### Étape 1 : Composants SMD (si présents)

Souder en premier (accès plus facile avant composants traversants)

---

### Étape 2 : Résistances

**Ordre** : Plat sur PCB, pattes pliées 90°
- Identifier valeur (code couleur ou marquage)
- Souder
- Couper pattes ras

---

### Étape 3 : Diodes

**ATTENTION polarité** : Bande = cathode (côté marqué sur PCB)
- D1, D2 (Schottky)
- D3, D4, D5 (Zener)

---

### Étape 4 : Condensateurs Céramiques

**Pas de polarité**, souder directement

---

### Étape 5 : Condensateurs Électrolytiques

**ATTENTION polarité** : Patte longue = +, marquage PCB
- C1, C3, C5, C7, C11 (gros électrolytiques)

---

### Étape 6 : Sockets / Headers

- Sockets IC (si utilisés pour Mega, modules)
- Headers mâles (pour modules enfichables)
- Borniers à vis (alimentation, moteurs, tensions)

**Astuce** : Maintenir perpendiculaire avec ruban adhésif pendant soudure

---

### Étape 7 : Fuseholders

Porte-fusibles PCB (F1, F2)

---

### Étape 8 : LEDs

**ATTENTION polarité** : Patte longue = anode (+)

---

### Étape 9 : Connecteurs JST-XH

Headers mâles pour JST (encodeurs, capteurs, UART)

---

### Étape 10 : Modules (dernier)

- Arduino Mega (socketé ou soudé direct)
- W5500 Ethernet
- Pololu MC33926
- VMA452

**Recommandation** : Utiliser sockets/headers pour faciliter remplacement

---

## INSPECTION AVANT MISE SOUS TENSION

### Vérifications Visuelles

- [ ] Pas de ponts de soudure (court-circuits)
- [ ] Pas de soudures froides (brillantes)
- [ ] Toutes les pattes soudées
- [ ] Polarités correctes (électrolytiques, diodes, LEDs)
- [ ] Composants bien positionnés (pas de décalage)

---

### Tests Multimètre (HORS TENSION)

**Continuité GND** :
- [ ] Tous les GND connectés (0Ω)

**Pas de court-circuits** :
- [ ] 5V ↔ GND : ∞ (ou >10kΩ)
- [ ] 12V ↔ GND : ∞
- [ ] 24V ↔ GND : ∞
- [ ] 3.3V ↔ GND : ∞

**Résistances diviseurs** :
- [ ] Mesurer R12+R13, R15+R16, R18+R19 → Valeurs attendues

---

## PREMIER POWER-ON (MISE SOUS TENSION)

### Étape 1 : Mega Seul (Sans Modules)

1. **Connecter 12V sur J1** (sans 24V)
2. **Observer** :
   - LED power (LED1) allumée → ✅
   - Pas de fumée, pas de composant chaud → ✅
3. **Mesurer** :
   - Pin 5V Mega : 5.0V ±0.1V → ✅
   - Pin 3.3V Mega : 3.3V ±0.1V → ✅

**Si OK** : Éteindre, passer étape 2

---

### Étape 2 : Ajouter W5500

1. **Enficher W5500** sur header J11
2. **Alimenter 12V**
3. **Mesurer** :
   - 3.3V sur W5500 : 3.3V ±0.1V → ✅
4. **Connecter Ethernet**
5. **Upload firmware test** (ping)
6. **Ping 192.168.1.177** → ✅

---

### Étape 3 : Ajouter MC33926

1. **Enficher MC33926** sur header J13
2. **Connecter 24V sur J2**
3. **NE PAS connecter moteurs encore**
4. **Alimenter 12V + 24V**
5. **Mesurer** :
   - 24V sur VIN MC33926 → ✅
6. **Upload firmware test** (PWM basique)
7. **Oscillo sur M1IN1/M1IN2** : Signaux PWM OK → ✅

---

### Étape 4 : Ajouter Moteurs (À Vide)

1. **Connecter moteurs sur J3, J4** (sans charge mécanique)
2. **Upload firmware contrôle**
3. **Test rotation courte** (1 seconde)
4. **Vérifier** :
   - Sens rotation correct → ✅
   - Courant <0.5A (à vide) → ✅
5. **Mesurer** avec pince ampèremétrique

---

### Étape 5 : Tous Périphériques

1. **Connecter** : Encodeurs (J5, J6), VMA452 (J7), DS18B20 (J8), Nextion (J12), Nano (J10)
2. **Tests unitaires** chaque bloc (voir TODO_PHASES.md)

---

## TROUBLESHOOTING HARDWARE

| Problème | Cause Probable | Solution |
|----------|----------------|----------|
| **Mega ne boot pas** | Court-circuit alim | Vérifier continuité 5V-GND |
| **LED power éteinte** | Fusible F1 grillé | Remplacer, chercher cause |
| **W5500 pas de ping** | Pas de 3.3V | Vérifier régulateur Mega |
| **Moteur ne tourne pas** | MC33926 disabled | Vérifier EN=HIGH, D1/D2=LOW |
| **Surintensité permanente** | Court-circuit moteur | Mesurer résistance moteur (~12Ω) |
| **Encodeurs pas de signal** | Câble inversé | Vérifier pinout VCC/GND |
| **ADC lecture erratique** | Bruit, pas de filtrage | Vérifier C23-C26 présents |
| **Nextion écran noir** | Pas d'alim ou mauvais baud | Vérifier 5V + baud 9600 |

---

## FICHIERS COMPLÉMENTAIRES

**Fichiers à créer avec schéma** :
- `BOM.csv` : Bill of Materials (export EasyEDA)
- `CPL.csv` : Component Placement List (si assemblage automatique)
- `Gerber.zip` : Fichiers fabrication
- `Schematic.pdf` : PDF du schéma (6 pages)

---

**Version** : 1.0
**Date** : 2026-01-30
**Auteur** : ON7KGK - JO20BM85DP
**Projet** : EME 10 GHz Station Controller
```
