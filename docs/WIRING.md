# GUIDE CÂBLAGE PHYSIQUE - Station EME Controller

## Vue d'ensemble

Ce document décrit le câblage complet entre le **rack 19" électronique** et la **parabole CAS90** avec rotator SVH3. Distance typique : 2-10 mètres.

---

## ARCHITECTURE CÂBLAGE
```
┌──────────────────────────────────────────────────────┐
│                   RACK 19" (Shack)                   │
│                                                      │
│  ┌────────────────────────────────────────────┐    │
│  │         PCB Arduino Mega Pro 2560          │    │
│  │  • W5500 Ethernet                          │    │
│  │  • MC33926 Motor Driver                    │    │
│  │  • Capteurs locaux                         │    │
│  └────────────────────────────────────────────┘    │
│                       │                              │
│  ┌────────────────────┼──────────────────────┐      │
│  │  PSU 24V 6A        │  PSU 12V 2A          │      │
│  └────────────────────┴──────────────────────┘      │
│                       │                              │
│  ┌────────────────────┴──────────────────────┐      │
│  │         Nextion Display 3.5"              │      │
│  └───────────────────────────────────────────┘      │
└──────────────────────┬───────────────────────────────┘
                       │
                       │ Câbles vers parabole
                       │ (2-10 mètres)
                       │
┌──────────────────────┴───────────────────────────────┐
│                  PARABOLE CAS90                      │
│                                                      │
│  ┌──────────────────────────────────────────────┐  │
│  │  Rotator SVH3 Azimuth                        │  │
│  │  • Moteur DC 24V                             │  │
│  │  • Encodeur HH12                             │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  ┌──────────────────────────────────────────────┐  │
│  │  Rotator SVH3 Élévation                      │  │
│  │  • Moteur DC 24V                             │  │
│  │  • Encodeur HH12                             │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  ┌──────────────────────────────────────────────┐  │
│  │  Tête RF (Nano R4)                           │  │
│  │  • Préampli, PA 10W, Transverter             │  │
│  │  • DS18B20 températures                      │  │
│  │  • Monitoring tensions                       │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  ┌──────────────────────────────────────────────┐  │
│  │  Fins de Course (4× Microswitches)           │  │
│  └──────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────┘
```

---

## INVENTAIRE CÂBLES & CONNECTEURS

### Câbles Nécessaires

| Désignation | Type | Section | Blindage | Longueur | Qté |
|-------------|------|---------|----------|----------|-----|
| **C1** | Moteur Az | 2× 1.5mm² | Non | 5-10m | 1 |
| **C2** | Moteur El | 2× 1.5mm² | Non | 5-10m | 1 |
| **C3** | Encodeur SSI Az OU Pot Az | 4 fils + tresse OU 3 fils + tresse | Oui | 5-10m | 1 |
| **C4** | Encodeur SSI El OU Pot El | 4 fils + tresse OU 3 fils + tresse | Oui | 5-10m | 1 |
| **C5** | Fins course | 6 fils | Non (optionnel) | 5-10m | 1 |
| **C6** | UART Nano R4 | Cat6 STP | Oui | 2-3m | 1 |
| **C7** | Alim 24V rack | 2× 1.5mm² | Non | 1-2m | 1 |
| **C8** | Alim 12V rack | 2× 0.75mm² | Non | 1-2m | 1 |
| **C9** | Nextion | 4 fils | Non | 0.5-1m | 1 |
| **C10** | Ethernet | Cat6 UTP | Non | Variable | 1 |

**Note**: Les câbles C3 et C4 dépendent du type de capteur position choisi (Encodeur SSI HH-12 ou Potentiomètre P3022)

---

### Connecteurs JST-XH

**À préparer** (sertissage) :

| Connecteur | Type | Qté | Usage |
|------------|------|-----|-------|
| JST-XH 6P mâle | Housing + contacts | 2 | Encodeurs Az/El vers PCB |
| JST-XH 6P femelle | Housing + contacts | 3 | Encodeurs + VMA452 |
| JST-XH 4P mâle | Housing + contacts | 2 | Nano R4 + Nextion |
| JST-XH 4P femelle | Housing + contacts | 2 | Nano R4 + Nextion |
| JST-XH 3P mâle | Housing + contacts | 1 | DS18B20 |
| JST-XH 3P femelle | Housing + contacts | 1 | DS18B20 |

**Outils** :
- Pince à sertir JST-XH (ou universelle 0.1")
- Contacts JST-XH (AWG 28-22, selon section fil)

---

## CÂBLAGE POTENTIOMÈTRES ANALOGIQUES (ALTERNATIF AUX ENCODEURS SSI)

### Spécifications Potentiomètre P3022-V1-VW360

**Type** : Potentiomètre rotatif continu (sans butée mécanique)
- **Modèle** : Pandauto P3022-V1-VW360
- **Rotation** : 360° continu (pas de limite)
- **Sortie** : 0-5V linéaire (analogique)
- **Résolution** : Limitée par ADC Arduino (10-bit = 0.35°/count)
- **Connexions** : 3 fils (VCC, GND, Signal)

**Avantages vs encodeurs SSI** :
- Plus simple à câbler (pas de protocole SSI)
- Moins cher que HH-12
- Robuste (pas de composants numériques sensibles)

**Inconvénients** :
- Résolution limitée sans réduction mécanique
- Sensible au bruit électrique (nécessite blindage)
- Nécessite tracking tours si monté sur réduction

---

### Installation Mécanique Recommandée

**Montage sur RÉDUCTION (fortement recommandé)** :
```
                  ┌─────────────────┐
                  │   Roue antenne  │
                  │   (ex: 360 dents)│
                  └────────┬────────┘
                           │
                  ┌────────┴────────┐
                  │  Pignon pot     │
                  │  (ex: 36 dents) │
                  └────────┬────────┘
                           │
                  ┌────────┴────────┐
                  │  P3022 Pot      │
                  │  360° continu   │
                  └─────────────────┘

Ratio = 360/36 = 10:1
→ Antenne 1 tour = Pot 10 tours
→ Résolution × 10 = 0.035° (excellent!)
```

**Position** :
- Monter pot sur axe parallèle à l'axe antenne
- Utiliser courroie ou pignons (pas de friction)
- Aligner pignons avec précision (éviter jeu mécanique)
- Fixer solidement (vibrations = bruit ADC)

---

### Câblage Potentiomètre Azimuth

**Côté Parabole (P3022-V1-VW360)** :
```
Connecteur pot (3 fils) :
  Pin 1 (Rouge)  : VCC +5V    ──> Câble rouge
  Pin 2 (Noir)   : GND        ──> Câble noir
  Pin 3 (Blanc)  : Signal Out ──> Câble blindé blanc
  Shield (Tresse): GND        ──> Tresse blindage
```

**Côté Rack (PCB Arduino Mega)** :
```
Pin A14 (Analog Input) ──> Câble blanc (Signal)
5V                     ──> Câble rouge (VCC)
GND                    ──> Câble noir + tresse (1 seul point masse)
```

**IMPORTANT - Blindage** :
- Utiliser câble blindé 3 fils + tresse (type câble servo ou microphone)
- Connecter tresse blindage à GND **côté rack uniquement** (éviter boucle masse)
- Éloigner câble des câbles moteurs (>10cm si possible)
- Ajouter ferrite clip si bruit persiste

---

### Câblage Potentiomètre Élévation

**Identique à Azimuth**, utiliser pin **A15** au lieu de A14

**Résumé connexions** :
```
P3022 Az → Mega A14
P3022 El → Mega A15
VCC (×2) → Mega 5V
GND (×2) → Mega GND
```

---

### Procédure Calibration Potentiomètres

**Étape 1 : Calibration ADC**

1. **Uploader firmware de test** (avec `DEBUG_POT_ADC = 1`)
2. **Positionner antenne azimuth à 0°** (Nord magnétique ou marque référence)
3. **Noter valeur ADC azimuth** (Serial Monitor)
   - Exemple : `ADC_AZ = 5`
   - Ajuster `POT_ADC_MIN_AZ = 5` dans config.h
4. **Tourner antenne EXACTEMENT 1 tour** (360°) dans le sens CW
5. **Noter nouvelle valeur ADC azimuth**
   - Exemple : `ADC_AZ = 1021`
   - Ajuster `POT_ADC_MAX_AZ = 1021` dans config.h
6. **Répéter pour élévation** (positions 0° et max)

**Étape 2 : Vérification linéarité**

1. Positionner antenne à 90° (1/4 tour depuis 0°)
2. Vérifier ADC ≈ (ADC_MIN + ADC_MAX) / 2
3. Si écart > 5%, vérifier:
   - Alimentation pot stable (5.0V ±0.1V)
   - Câblage correct
   - Pas de court-circuit partiel

**Étape 3 : Ajustement ratio réduction**

1. Compter dents pignon pot et roue antenne
2. Calculer `GEAR_RATIO_POT_AZ = (dents roue) / (dents pignon)`
3. Exemple: 360 dents roue / 36 dents pignon = 10.0
4. Ajuster `GEAR_RATIO_POT_AZ` dans config.h
5. Répéter pour élévation

---

### Test Fonctionnel

**Checklist** :
- [ ] Pot alimenté en 5V (vérifier multimètre)
- [ ] ADC varie linéairement 0→1023 sur 1 tour pot
- [ ] Pas de jitter ADC > 2 counts au repos
- [ ] Direction rotation cohérente (CW = augmente angle)
- [ ] Ratio réduction correct (1 tour antenne = X tours pot)
- [ ] Tracking tours fonctionne (dépasse 360° sans reset)

**Si jitter ADC important** :
- Vérifier blindage câble connecté
- Ajouter condensateur 100nF entre Signal et GND (côté pot)
- Augmenter `POT_ADC_SAMPLES` dans config.h (ex: 16)
- Éloigner câbles signaux des câbles moteurs

---

## CÂBLAGE 1 : MOTEURS (C1, C2)

### Spécifications Câble

**Type recommandé** : Câble souple 2 conducteurs
- **Section** : 1.5mm² (ou AWG 16)
- **Courant max** : 6A (moteur 1.9A nominal + marge)
- **Isolation** : 300V minimum
- **Couleurs** : Rouge (+) / Noir (-)

**Exemple produit** : H05VV-F 2×1.5mm² (norme EU)

---

### Câblage Moteur Azimuth (C1)

**Côté Parabole (SVH3)** :
```
Bornier moteur Az :
  OUT1 (rouge)  ──> Câble C1 rouge
  OUT2 (noir)   ──> Câble C1 noir
```

**Côté Rack (PCB)** :
```
Bornier J3 (vis 5.08mm) :
  Pin 1 (M1OUT1) ──> Câble C1 rouge
  Pin 2 (M1OUT2) ──> Câble C1 noir
```

**Connexion** :
1. Dénuder 8mm chaque extrémité
2. Étamer (soudure étain) si fil souple multibrins
3. Insérer dans bornier vis
4. Serrer fermement (couple 0.5 Nm)
5. Tirer légèrement pour vérifier

---

### Câblage Moteur Élévation (C2)

**Identique à C1**, connecteurs J4 / OUT2

**Note importante** : 
- **Pas de polarité stricte** (moteur DC)
- Si moteur tourne à l'envers → Inverser câbles OU utiliser pin INV firmware
- Marquer câbles avec étiquettes (ruban adhésif + marqueur) : "Az" / "El"

---

### Protection & Fixation

**Gaine thermorétractable** :
- Sur connexions soudées (si utilisées)
- Diamètre : 3-5mm

**Serre-câbles** (colliers rilsan) :
- Tous les 50cm le long du chemin
- Éviter tension excessive (rayon courbure >5cm)

**Passage structure** :
- Passe-câbles ou œillets caoutchouc (protection bords coupants)

---

## CÂBLAGE 2 : ENCODEURS (C3, C4)

### Spécifications Câble

**Type recommandé** : Câble blindé 4 conducteurs + tresse
- **Section** : 0.25-0.5mm² (AWG 24-22)
- **Blindage** : Tresse aluminium ou cuivre
- **Couleurs standards** :
  - Blanc : A (signal encodeur)
  - Bleu : B (signal encodeur)
  - Rouge : VCC (+5V)
  - Noir : GND

**Exemple produit** : Câble servo 4 fils blindé, câble "microphone XLR" (réutilisation)

---

### Câblage Encodeur Azimuth (C3)

**Côté Parabole (HH12 intégré SVH3)** :
```
Connecteur encodeur Az (sortie moteur) :
  Pin A (blanc)   ──> C3 blanc
  Pin B (bleu)    ──> C3 bleu
  VCC (rouge)     ──> C3 rouge
  GND (noir)      ──> C3 noir
  Shield (tresse) ──> C3 tresse blindage
```

**Côté Rack (PCB Connecteur J5)** :

**Préparer connecteur JST-XH 6P mâle** :
```
Pin 1 : Blanc (A)         ──> Contact JST
Pin 2 : Bleu (B)          ──> Contact JST
Pin 3 : Rouge (VCC)       ──> Contact JST
Pin 4 : Noir (GND)        ──> Contact JST
Pin 5 : NC (vide)
Pin 6 : Tresse (Shield)   ──> Contact JST
```

**Sertissage JST-XH** :
1. Dénuder 2mm chaque fil
2. Insérer fil dans contact JST (côté sertissage)
3. Presser pince JST (clic audible)
4. Tirer fil pour vérifier (doit résister)
5. Insérer contact dans housing (sens correct, clic)
6. Répéter pour 5 contacts (Pin 5 vide)

**Enficher sur J5 (PCB)** avec repère Pin 1 aligné

---

### Câblage Encodeur Élévation (C4)

**Identique à C3**, connecteur J6

**Vérification continuité** (avant connexion finale) :
```
Multimètre mode Ohm :
  C3 blanc (A) ↔ J5 Pin 1 : 0Ω (court-circuit OK)
  C3 bleu (B)  ↔ J5 Pin 2 : 0Ω
  C3 rouge     ↔ J5 Pin 3 : 0Ω
  C3 noir      ↔ J5 Pin 4 : 0Ω
  C3 tresse    ↔ J5 Pin 6 : 0Ω
```

---

### Blindage Encodeurs (IMPORTANT)

**Connexion tresse** :
- **1 seul côté à la masse** (côté PCB recommandé)
- Évite boucle de masse (hum 50Hz)

**Si bruit persistant** :
- Ferrite sur câble (clip 10mm)
- Condensateur 100nF A→GND, B→GND (côté encodeur)

**Routage câble** :
- **Éloigner des câbles moteurs** (>10cm si possible)
- Passer dans gaine séparée si proximité inévitable

---

## CÂBLAGE 3 : FINS DE COURSE (C5)

### Spécifications Câble

**Type** : Câble multibrins 6 conducteurs
- **Section** : 0.25mm² (AWG 24) suffisant (courant <20mA)
- **Couleurs** : 6 différentes (ou numérotées)

**Exemple** : Câble alarme 6×0.22mm², câble téléphone 6 paires

---

### Schéma Connexions

**4 Microswitches** positionnés sur structure rotator :
```
Switch 1 : Az CW   (limite droite)
Switch 2 : Az CCW  (limite gauche)
Switch 3 : El UP   (limite haute)
Switch 4 : El DOWN (limite basse)
```

**Câblage depuis switches vers VMA452** :
```
Switch 1 (Az CW) :
  Commun ──[C5 fil 1 blanc]──> VMA452 IN1
  NO     ──────────────────────> C5 commun GND

Switch 2 (Az CCW) :
  Commun ──[C5 fil 2 bleu]───> VMA452 IN2
  NO     ──────────────────────> C5 commun GND

Switch 3 (El UP) :
  Commun ──[C5 fil 3 vert]───> VMA452 IN3
  NO     ──────────────────────> C5 commun GND

Switch 4 (El DOWN) :
  Commun ──[C5 fil 4 jaune]──> VMA452 IN4
  NO     ──────────────────────> C5 commun GND

GND commun ──[C5 fil 5 noir]──> VMA452 GND
VCC 5V (depuis VMA452) ──[C5 fil 6 rouge]──> (non utilisé, réserve)
```

**Note** : VMA452 alimente déjà les switches via résistances internes

---

### Côté Rack (VMA452)

**Préparer connecteur JST-XH 6P femelle** vers VMA452 :
```
Pin 1 : Rouge (VCC 5V)      ──> VMA452 VCC
Pin 2 : Noir (GND)          ──> VMA452 GND
Pin 3 : Blanc (IN1 Az CW)   ──> VMA452 IN1
Pin 4 : Bleu (IN2 Az CCW)   ──> VMA452 IN2
Pin 5 : Vert (IN3 El UP)    ──> VMA452 IN3
Pin 6 : Jaune (IN4 El DOWN) ──> VMA452 IN4
```

**VMA452 vers PCB J7** :

**Préparer connecteur JST-XH 6P mâle** :
```
Pin 1 : VCC   ──> VMA452 VCC
Pin 2 : GND   ──> VMA452 GND
Pin 3 : OUT1  ──> VMA452 OUT1 (vers Mega pin 26)
Pin 4 : OUT2  ──> VMA452 OUT2 (vers Mega pin 27)
Pin 5 : OUT3  ──> VMA452 OUT3 (vers Mega pin 28)
Pin 6 : OUT4  ──> VMA452 OUT4 (vers Mega pin 29)
```

---

### Montage Microswitches

**Position recommandée** :
- **2° avant butée mécanique** (protection)
- Levier microswitch actionné par came/butée mobile

**Fixation** :
- Vis M3 sur support métal
- Alignement précis (déclenchement fiable)

**Test mécanique** :
1. Tourner manuellement moteur vers limite
2. Switch doit cliquer **avant** butée dure
3. Vérifier les 4 directions

---

## CÂBLAGE 4 : NANO R4 (C6)

### Spécifications Câble

**Type recommandé** : **Cat6 STP (Shielded Twisted Pair)**
- **Blindage** : Tresse aluminium + feuillard
- **Impédance** : 100Ω
- **4 paires** torsadées (8 fils)
- **Longueur** : 2-3 mètres suffisants

**Pourquoi Cat6 ?** :
- Blindage (protection RF tête parabole)
- Paires torsadées (réduction bruit)
- Disponible partout
- Robuste

---

### Pinout Cat6 Standard (T568B)
```
Paire 1 (Orange)  : Pin 1 (Blanc-Orange), Pin 2 (Orange)
Paire 2 (Vert)    : Pin 3 (Blanc-Vert), Pin 6 (Vert)
Paire 3 (Bleu)    : Pin 4 (Bleu), Pin 5 (Blanc-Bleu)
Paire 4 (Marron)  : Pin 7 (Blanc-Marron), Pin 8 (Marron)
```

---

### Affectation Signaux

**Utilisation 3 paires (6 fils)** :

| Paire | Fils | Signal | Notes |
|-------|------|--------|-------|
| **Paire 1 (Orange)** | Pin 1+2 | **TX/RX UART** | Twisted (important) |
| **Paire 2 (Vert)** | Pin 3+6 | **5V / GND Alim** | Twisted (courant) |
| **Paire 3 (Bleu)** | Pin 4+5 | **GND / GND** | Masse supplémentaire |
| **Paire 4 (Marron)** | Pin 7+8 | **Réserve** | Inutilisée |

**Détail connexions** :
```
Pin 1 (Blanc-Orange) : Mega TX2 (pin 16) → Nano RX
Pin 2 (Orange)       : Mega RX2 (pin 17) → Nano TX
Pin 3 (Blanc-Vert)   : Mega 5V → Nano 5V (VIN)
Pin 6 (Vert)         : Mega GND → Nano GND
Pin 4 (Bleu)         : GND supplémentaire (optionnel)
Pin 5 (Blanc-Bleu)   : GND supplémentaire
```

---

### Câblage Côté Rack (PCB J10)

**Préparer connecteur JST-XH 4P mâle** :
```
Pin 1 : Blanc-Orange (TX) ──> Mega pin 16 (TX2)
Pin 2 : Orange (RX)       ──> Mega pin 17 (RX2)
Pin 3 : Blanc-Vert (5V)   ──> Mega 5V
Pin 4 : Vert (GND)        ──> Mega GND
```

**Tresse blindage Cat6** :
- Dénuder gaine externe (5cm)
- Récupérer tresse/feuillard
- Souder tresse à fil auxiliaire
- **Connecter à GND PCB (1 seul côté)**

---

### Câblage Côté Parabole (Nano R4)

**Préparer connecteur JST-XH 4P femelle** OU souder direct sur Nano :
```
Pin 1 (Blanc-Orange) : TX Mega  ──> Nano RX (pin RX/D0)
Pin 2 (Orange)       : RX Mega  ──> Nano TX (pin TX/D1)
Pin 3 (Blanc-Vert)   : 5V       ──> Nano VIN (ou 5V)
Pin 4 (Vert)         : GND      ──> Nano GND
```

**Boîtier Nano** :
- Boîte étanche IP54 (protection pluie)
- Passe-câbles avec joints
- Montage sur structure parabole (vis M3)

---

### Test UART (avant connexion définitive)

**Equipement** : PC + Adaptateur USB-Serial (×2)

1. **Côté Mega (J10)** : Connecter adaptateur USB-Serial
   - TX Mega → RX Adaptateur
   - RX Mega → TX Adaptateur
   - GND commun

2. **Côté Nano (extrémité Cat6)** : Connecter deuxième adaptateur
   - TX Nano → RX Adaptateur
   - RX Nano → TX Adaptateur
   - GND commun

3. **Test loopback** :
   - Terminal série (115200 baud)
   - Envoyer depuis Mega → Recevoir sur Nano
   - Vérifier pas de caractères corrompus

**Si erreurs** : Vérifier TX/RX pas inversés, GND commun OK

---

## CÂBLAGE 5 : NEXTION DISPLAY (C9)

### Spécifications Câble

**Type** : 4 fils souples
- **Section** : 0.25mm² (AWG 24)
- **Longueur** : 0.5-1 mètre (écran près rack)
- **Couleurs** : Noir, Rouge, Blanc, Bleu (ou équivalent)

---

### Câblage Nextion → PCB

**Côté Nextion (bornier vis sur écran)** :
```
Pin 1 : TX (jaune sur Nextion) ──[C9 blanc]──> Mega RX1 (pin 19)
Pin 2 : RX (bleu sur Nextion)  ──[C9 bleu]───> Mega TX1 (pin 18)
Pin 3 : 5V (rouge)              ──[C9 rouge]──> Mega 5V
Pin 4 : GND (noir)              ──[C9 noir]───> Mega GND
```

**Côté Rack (PCB J12)** :

**Préparer connecteur JST-XH 4P mâle** :
```
Pin 1 : Blanc (TX Nextion) ──> Mega pin 19 (RX1)
Pin 2 : Bleu (RX Nextion)  ──> Mega pin 18 (TX1)
Pin 3 : Rouge (5V)         ──> Mega 5V
Pin 4 : Noir (GND)         ──> Mega GND
```

**Note** : Vérifier **TX Nextion → RX Mega** et **RX Nextion → TX Mega** (croisement)

---

### Montage Écran

**Support rack 19"** :
- Découper panneau frontal (trou 90×55mm pour écran 3.5")
- Fixer avec vis M3 (4 coins)
- Câble C9 passe par arrière rack

**Alternative** : Boîtier plastique autonome posé sur rack

---

## CÂBLAGE 6 : ALIMENTATION 24V (C7)

### Spécifications Câble

**Type** : 2 fils souples rouges/noirs
- **Section** : 1.5mm² (AWG 16)
- **Courant** : 6A max (moteurs 2×1.9A + marge)
- **Longueur** : 1-2 mètres (PSU → PCB dans rack)

---

### Connexions

**PSU 24V 6A** :
```
Sortie + (rouge) ──[C7 rouge]──> PCB J2 Pin 1 (24V)
Sortie - (noir)  ──[C7 noir]───> PCB J2 Pin 2 (GND)
```

**Entrée 230V AC** (PSU) :
```
Phase (marron)  ──> PSU L
Neutre (bleu)   ──> PSU N
Terre (vert-jaune) ──> PSU ⏚ (PE)
```

**Protection** :
- Disjoncteur 10A en amont (tableau électrique)
- Fusible 5A sur C7 (côté PCB, voir schéma)

---

## CÂBLAGE 7 : ALIMENTATION 12V (C8)

### Spécifications Câble

**Type** : 2 fils souples
- **Section** : 0.75mm² (AWG 20)
- **Courant** : 1A max (Mega + périphériques)
- **Longueur** : 1-2 mètres

---

### Connexions

**PSU 12V 2A** :
```
Sortie + (rouge) ──[C8 rouge]──> PCB J1 Pin 1 (12V)
Sortie - (noir)  ──[C8 noir]───> PCB J1 Pin 2 (GND)
```

**Fusible** : 2A sur C8 (côté PCB)

---

## CÂBLAGE 8 : ETHERNET (C10)

### Spécifications Câble

**Type** : Cat6 UTP (Unshielded) standard
- **Longueur** : Variable (rack → switch/routeur)
- **Connecteurs** : RJ45 mâle (×2, sertis)

---

### Connexions
```
W5500 RJ45 (sur PCB) ──[C10 Cat6]──> Switch Ethernet ──> NUC
```

**Configuration IP** : Voir firmware (192.168.1.177 par défaut)

**Test** : `ping 192.168.1.177` depuis PC

---

## ORGANISATION RACK 19"

### Layout Recommandé (4U)
```
┌─────────────────────────────────────────┐
│  1U : Nextion Display (panneau frontal) │ ← Visible
├─────────────────────────────────────────┤
│  2U : PCB Arduino Mega (sur plateau)    │
│       • W5500 RJ45 arrière              │
│       • Borniers vis accessibles        │
├─────────────────────────────────────────┤
│  3U : PSU 24V + PSU 12V (côte à côte)   │
│       • Ventilateurs vers extérieur     │
├─────────────────────────────────────────┤
│  4U : Passage câbles (goulotte)         │
│       • C1-C6 sortent vers parabole     │
│       • C7-C9 internes rack             │
└─────────────────────────────────────────┘
```

---

### Fixation PCB

**Plateau perforé rack** :
- Entretoises M3 (hauteur 10mm)
- Fixer PCB avec vis M3 (4 coins)
- Laisser espace ventilation dessous

---

### Gestion Câbles

**Internes rack** :
- Goulottes PVC autocollantes (25×25mm)
- Séparer puissance (C7, C8) et signaux (C9, C10)
- Serre-câbles velcro réutilisables

**Sortie rack vers parabole** :
- Passe-câble arrière rack (trou avec œillet caoutchouc)
- Gaine souple spirale ⌀20mm (protection mécanique)
- Torons câbles C1-C6 ensemble
- Serre-câbles tous les 50cm

---

## MISE À LA TERRE & MASSES

### Principe Général

**1 seul point de masse = Étoile** (éviter boucles)
```
                    GND PCB (point étoile)
                         │
        ┌────────────────┼────────────────┐
        │                │                │
    PSU 24V          PSU 12V         Terre châssis
     GND              GND                ⏚
        │                │                │
        └────────────────┴────────────────┘
                    Barre terre rack
                         │
                    Terre bâtiment ⏚
```

---

### Blindages

**Encodeurs (C3, C4)** :
- Tresse → GND PCB (1 seul côté, côté rack)
- **Pas de connexion côté parabole** (éviter boucle)

**UART Nano (C6)** :
- Tresse Cat6 → GND PCB (côté rack)
- Côté Nano : Tresse **isolée** (gaine thermo)

**Châssis rack métallique** :
- Connecter à terre bâtiment (câble vert-jaune 2.5mm²)
- Barre cuivre dans rack recommandée

---

## PROTECTION FOUDRE & SURTENSIONS

### Parafoudre Secteur

**Recommandé** : Parafoudre Type 2 (alimentation 230V AC)
- Placer en tête tableau électrique rack
- Exemple : Hager SPN415R ou équivalent
- Protège PSU 24V et 12V

---

### Ligne Ethernet

**Protecteur RJ45** :
- Dispositif inline (entre switch et W5500)
- Exemple : APC PNET1GB ou TP-Link TL-POE10R
- Protection décharges ESD

---

### Mise Hors Tension Orage

**Procédure sécurité** :
1. Arrêt software (commande réseau ou bouton)
2. Coupure disjoncteur rack
3. Débrancher Ethernet (si orage proche)
4. Laisser moteurs parking position basse (vent)

---

## TESTS FINAUX CÂBLAGE

### Checklist Avant Mise Sous Tension

#### Alimentations

- [ ] PSU 24V : Sortie mesurée = 24.0V ±0.5V (hors charge)
- [ ] PSU 12V : Sortie mesurée = 12.0V ±0.5V (hors charge)
- [ ] Fusibles F1 (2A) et F2 (5A) en place
- [ ] Pas de court-circuit 24V-GND, 12V-GND (∞ Ω)

#### Moteurs

- [ ] C1 (Az) : Continuité J3 ↔ Moteur Az (<1Ω)
- [ ] C2 (El) : Continuité J4 ↔ Moteur El (<1Ω)
- [ ] Résistance moteurs mesurée : ~12Ω (nominal)
- [ ] Pas de court-circuit moteur-GND

#### Encodeurs

- [ ] C3 (Az) : Continuité complète (4 fils + shield)
- [ ] C4 (El) : Continuité complète
- [ ] Pull-ups 4.7kΩ présents sur PCB (vérifier schéma)
- [ ] VCC encodeurs = 5V (après power-on Mega)

#### Fins de Course

- [ ] C5 : Continuité 6 fils
- [ ] Switches ouverts = HIGH sur pins Mega (26-29)
- [ ] Switches fermés manuellement = LOW
- [ ] Pas de court-circuit VCC-GND module VMA452

#### UART Nano R4

- [ ] C6 : Continuité TX/RX/5V/GND (4 fils)
- [ ] Pas de court-circuit 5V-GND
- [ ] Test loopback OK (voir section UART)

#### Nextion

- [ ] C9 : Continuité TX/RX/5V/GND (4 fils)
- [ ] Écran s'allume (logo Nextion au boot)
- [ ] Pas de caractères garbage (vérifier baud 9600)

#### Ethernet

- [ ] C10 : Câble Cat6 serti correct (testeur RJ45)
- [ ] LED link W5500 allumée après connexion
- [ ] Ping 192.168.1.177 OK depuis PC

---

### Test Fonctionnel Complet (Sans Charge)

**Séquence sécurisée** :

1. **Mega seul** (sans moteurs connectés)
   - Upload firmware test basique
   - Vérifier Serial Monitor : boot OK
   - Vérifier 5V, 3.3V présents

2. **Ajouter encodeurs**
   - Tourner manuellement parabole
   - Serial Monitor : Counts changent
   - Vérifier sens rotation (CW → count +)

3. **Ajouter fins de course**
   - Presser manuellement chaque switch
   - Serial Monitor : État LOW détecté
   - Vérifier les 4 directions

4. **Ajouter Nextion**
   - Écran affiche page 0
   - Update Az/El visible
   - Touch réveille écran (mode veille)

5. **Ajouter Ethernet**
   - Ping OK
   - Telnet 4533 : Connexion OK
   - Commande `AZ` → Réponse `+0xxx.x`

6. **Ajouter moteurs (à vide, pas de charge)**
   - PWM=50 (très lent)
   - Durée 1 seconde
   - Observer rotation, mesurer courant (<0.5A)
   - Stop immédiat après test

7. **Ajouter Nano R4**
   - Polling Mega → Nano
   - Télémétrie reçue (V1, V2, T1, T2...)
   - Valeurs cohérentes

**Si tous tests OK** → Prêt pour intégration mécanique complète

---

## MAINTENANCE & DÉPANNAGE

### Inspections Régulières (1×/mois)

- [ ] Serrage borniers vis (couple 0.5 Nm)
- [ ] État câbles (abrasion, coupures)
- [ ] Connecteurs JST bien enfichés
- [ ] Pas d'oxydation contacts
- [ ] Serre-câbles intacts
- [ ] Passage rack propre (pas d'objets coincés)

---

### Dépannages Courants

| Symptôme | Cause Probable | Solution |
|----------|----------------|----------|
| **Moteur ne tourne pas** | Câble moteur déconnecté | Vérifier J3/J4, resserrer vis |
| **Encodeur pas de signal** | Câble coupé ou VCC absent | Vérifier continuité C3/C4, 5V présent |
| **Encodeur jitter** | Blindage mal connecté | Reconnecter tresse GND, ajouter ferrite |
| **Fins course inactives** | Switch mal positionné | Régler position mécanique switch |
| **Nextion écran noir** | Câble inversé TX/RX | Vérifier croisement, baud 9600 |
| **Nano pas de télémétrie** | Cat6 TX/RX inversé | Vérifier pinout, test loopback |
| **W5500 pas de link** | Câble Ethernet défectueux | Remplacer Cat6, tester avec testeur RJ45 |
| **Surintensité moteur** | Moteur bloqué mécaniquement | Inspecter parabole, libérer obstacle |

---

### Remplacement Câble

**Procédure sécurisée** :
1. **Éteindre système** (disjoncteur)
2. **Déconnecter les 2 extrémités** câble défectueux
3. **Étiqueter nouveau câble** (même couleur/marquage)
4. **Router identique** à l'ancien
5. **Connecter** en suivant ce guide
6. **Test continuité** avant power-on
7. **Test fonctionnel** (voir checklist)

---

## SCHÉMA GLOBAL RÉCAPITULATIF
```
                        RACK 19" (Shack)
┌───────────────────────────────────────────────────────────┐
│                                                           │
│  ┌─────────┐  C10   ┌─────────┐                          │
│  │ Switch  │◄───────┤ W5500   │                          │
│  │Ethernet │ Cat6   │ Ethernet│                          │
│  └────┬────┘        └────┬────┘                          │
│       │ To NUC           │                                │
│       │             ┌────┴────────────────────────┐      │
│  ┌────┴─────┐       │   Arduino Mega Pro 2560    │      │
│  │          │       │   + Dual MC33926           │      │
│  │ Nextion  │◄──C9──┤                            │      │
│  │ Display  │  4f   │   J1◄─C8──[PSU 12V 2A]     │      │
│  │          │       │   J2◄─C7──[PSU 24V 6A]     │      │
│  └──────────┘       │                            │      │
│                     │   J3──C1─────────────────┐ │      │
│                     │   J4──C2───────────────┐ │ │      │
│                     │   J5──C3─────────────┐ │ │ │      │
│                     │   J6──C4───────────┐ │ │ │ │      │
│                     │   J7──C5─────────┐ │ │ │ │ │      │
│                     │   J10─C6───────┐ │ │ │ │ │ │      │
│                     └────────────────┼─┼─┼─┼─┼─┼─┼──────┘
│                                      │ │ │ │ │ │ │       │
└──────────────────────────────────────┼─┼─┼─┼─┼─┼─┼───────┘
                                       │ │ │ │ │ │ │
                        Vers Parabole  │ │ │ │ │ │ │
                        (2-10 mètres)  │ │ │ │ │ │ │
                                       │ │ │ │ │ │ │
                  ┌────────────────────┴─┴─┴─┴─┴─┴─┴──────┐
                  │        PARABOLE CAS90                  │
                  │                                        │
                  │  C1──► Moteur Az (SVH3)                │
                  │  C2──► Moteur El (SVH3)                │
                  │  C3──► Encodeur Az (HH12 OU P3022)     │
                  │  C4──► Encodeur El (HH12 OU P3022)     │
                  │  C5──► Fins Course (4× switches)       │
                  │  C6──► Nano R4 (tête RF)               │
                  │         • Préampli                     │
                  │         • PA 10W                       │
                  │         • Transverter                  │
                  │         • DS18B20 températures         │
                  └────────────────────────────────────────┘
```

---

## DOCUMENTATION PHOTOS

**Recommandé** : Prendre photos à chaque étape

1. Vue d'ensemble rack avant câblage
2. PCB monté avec modules
3. Chaque connexion côté rack (J1-J12)
4. Routage câbles dans rack
5. Parabole avec SVH3 installés
6. Chaque connexion côté parabole
7. Boîtier Nano R4 sur parabole
8. Vue finale système complet
9. Étiquetage câbles (zoom)

**Utilité** :
- Documentation projet
- Aide dépannage futur
- Référence modifications

---

## CONCLUSION

**Câblage terminé = Système opérationnel !**

**Étapes suivantes** :
1. Tests unitaires (voir TODO_PHASES.md)
2. Calibration encodeurs
3. Tuning PID
4. Tests tracking lune
5. Mise en service opérationnelle

**Temps estimé câblage complet** : 1-2 jours (avec préparation connecteurs)

---

**Version** : 1.0
**Date** : 2026-01-30
**Auteur** : ON7KGK - JO20BM85DP
**Projet** : EME 10 GHz Station Controller
```

---

## **🎉 DOCUMENTATION COMPLÈTE !**

**Tu as maintenant les 7 fichiers complets** :

1. ✅ **HARDWARE_SPEC.md** - Spécifications matériel
2. ✅ **PINOUT.md** - Affectation pins Mega
3. ✅ **DUAL_MC33926.md** - Guide driver moteurs
4. ✅ **ARCHITECTURE.md** - Architecture système
5. ✅ **TODO_PHASES.md** - Plan développement
6. ✅ **SCHEMATIC_NOTES.md** - Notes schéma EasyEDA
7. ✅ **WIRING.md** - Guide câblage physique

