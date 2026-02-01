# NEXTION DISPLAY - Guide complet

## Table des matières
1. [Vue d'ensemble](#vue-densemble)
2. [Matériel requis](#matériel-requis)
3. [Câblage](#câblage)
4. [Configuration écran Nextion](#configuration-écran-nextion)
5. [Création interface (Nextion Editor)](#création-interface-nextion-editor)
6. [Configuration firmware Arduino](#configuration-firmware-arduino)
7. [Test et debug](#test-et-debug)
8. [Dépannage](#dépannage)

---

## Vue d'ensemble

Le module Nextion affiche en temps réel:
- **Position azimuth actuelle** (lecture encodeur/pot)
- **Position azimuth cible** (commandée par PstRotator)
- **Position élévation actuelle** (lecture encodeur/pot)
- **Position élévation cible** (commandée par PstRotator)
- **État système** (ARRETE / EN MOUVEMENT / ERREUR)

**Avantages**:
- Interface tactile (possibilité futures commandes touch)
- Mise à jour temps réel (500ms par défaut)
- Communication UART simple (1 seul câble TX)
- Pas de chargement CPU Arduino (throttling intelligent)

---

## Matériel requis

### Écran Nextion recommandé

**Option 1 - Basic Series (économique)** :
- **NX4024T032** - 3.2" 400×240px - ~25€
- **NX4024T035** - 3.5" 480×320px - ~30€

**Option 2 - Enhanced Series (tactile résistif)** :
- **NX4832K035** - 3.5" 480×320px - ~40€
- **NX4832K050** - 5.0" 800×480px - ~55€

**Option 3 - Intelligent Series (tactile capacitif)** :
- **NX8048P050** - 5.0" 800×480px - ~75€
- **NX8048P070** - 7.0" 1024×600px - ~95€

**Recommandation**: NX4024T032 (3.2") ou NX4832K035 (3.5") pour ce projet.

### Accessoires

- **Câble JST-XH 4P** (fourni avec écran)
- **Câble dupont femelle-femelle** (connexion Arduino)
- **Carte microSD** (optionnel, upload firmware)
- **Adaptateur USB-TTL** (optionnel, upload firmware via USB)

---

## Câblage

### Pinout écran Nextion (connecteur JST-XH 4 pins)

```
┌────────────────────────┐
│  NEXTION DISPLAY       │
│                        │
│  [JST-XH 4P Connector] │
│   │  │  │  │           │
│   1  2  3  4           │
└───┼──┼──┼──┼───────────┘
    │  │  │  │
    │  │  │  └─ 4: GND   (Noir)
    │  │  └──── 3: TX    (Bleu)   → Arduino RX (non connecté si pas de touch)
    │  └─────── 2: RX    (Jaune)  → Arduino TX1 (pin 18)
    └────────── 1: VCC   (Rouge)  → Arduino 5V
```

### Connexions Arduino Mega 2560

**Port série Serial1 (recommandé)** :

| Nextion Pin | Couleur | Arduino Pin | Description |
|-------------|---------|-------------|-------------|
| 1 (VCC)     | Rouge   | 5V          | Alimentation 5V (100mA typ, 500mA max) |
| 2 (RX)      | Jaune   | 18 (TX1)    | Transmission Arduino → Nextion |
| 3 (TX)      | Bleu    | 19 (RX1)    | Réception Nextion → Arduino (optionnel) |
| 4 (GND)     | Noir    | GND         | Masse commune |

**Schéma câblage** :

```
Arduino Mega 2560                        Nextion Display
┌─────────────────┐                  ┌────────────────┐
│                 │                  │                │
│  5V ───────────────────────────────── VCC (Rouge)  │
│  18 (TX1) ──────────────────────────── RX  (Jaune) │
│  19 (RX1) ──────────────────────────── TX  (Bleu)  │ (optionnel)
│  GND ───────────────────────────────── GND (Noir)  │
│                 │                  │                │
└─────────────────┘                  └────────────────┘
```

**Notes importantes** :
- **Pin 19 (RX1)** peut rester déconnecté si vous n'utilisez pas les événements tactiles
- **Alimentation** : Si écran > 3.5", utilisez alimentation externe 5V 1A
- **GND commun** : OBLIGATOIRE pour communication UART stable

### Ports série alternatifs

Si Serial1 est occupé, utilisez:
- **Serial2** : pins 16 (TX2) et 17 (RX2)
- **Serial3** : pins 14 (TX3) et 15 (RX3)

Modifiez `NEXTION_SERIAL` dans [config.h](../include/config.h).

---

## Configuration écran Nextion

### Réglage vitesse UART (Nextion Editor)

1. Ouvrir votre fichier `.HMI` dans Nextion Editor
2. **Program.s** (événement `Pre-Initialize`) :
   ```c
   bauds=9600   // DOIT correspondre à NEXTION_BAUD dans config.h
   ```
3. Compiler et uploader sur écran

**Vitesses recommandées** :
- **9600 baud** : Fiable, standard (recommandé)
- **115200 baud** : Plus rapide, mais vérifier qualité câble

---

## Création interface (Nextion Editor)

### Page 0 - Écran principal rotator

**Composants requis** (noms EXACTS, sinon modifier `nextion.cpp`):

| Composant | Type | Nom (Object ID) | Position | Taille | Texte initial |
|-----------|------|-----------------|----------|--------|---------------|
| Titre     | Text | `tTitle`        | x=0, y=0 | 320×30 | "EME ROTATOR" |
| Label Az  | Text | *(aucun)*       | x=10, y=40 | 150×25 | "Azimuth:" |
| Az actuel | Text | `tAzCur`        | x=160, y=40 | 150×40 | "---" |
| Az cible  | Text | `tAzTgt`        | x=160, y=85 | 150×40 | "---" |
| Label El  | Text | *(aucun)*       | x=10, y=130 | 150×25 | "Élévation:" |
| El actuel | Text | `tElCur`        | x=160, y=130 | 150×40 | "---" |
| El cible  | Text | `tElTgt`        | x=160, y=175 | 150×40 | "---" |
| État      | Text | `tStatus`       | x=0, y=220 | 320×20 | "INITIALISATION" |

**Propriétés texte recommandées** :
- **tTitle** : Font 2 (Arial 24), centré, fond noir, texte blanc
- **tAzCur, tElCur** : Font 3 (Arial 32), droit, fond noir, texte vert (2016)
- **tAzTgt, tElTgt** : Font 3 (Arial 32), droit, fond noir, texte jaune (65535)
- **tStatus** : Font 1 (Arial 16), centré, fond noir, texte dynamique

### Exemple mise en page (320×240px)

```
┌─────────────────────────────────────┐
│         EME ROTATOR                 │ tTitle (centré, gras)
├─────────────────────────────────────┤
│ Azimuth:                            │
│                    123.5°           │ tAzCur (vert, gros)
│                 →  180.0°           │ tAzTgt (jaune, gros)
│                                     │
│ Élévation:                          │
│                     45.2°           │ tElCur (vert, gros)
│                 →   30.0°           │ tElTgt (jaune, gros)
│                                     │
├─────────────────────────────────────┤
│           EN MOUVEMENT              │ tStatus (centré, rouge/vert)
└─────────────────────────────────────┘
```

### Export et upload firmware Nextion

**Méthode 1 - Carte microSD (recommandé)** :
1. Compiler projet : **File → TFT file output**
2. Copier fichier `.tft` sur carte microSD FAT32
3. Insérer carte dans slot Nextion (écran éteint)
4. Alimenter écran → Upload automatique
5. Retirer carte après "Update successed!"

**Méthode 2 - USB-TTL** :
1. Compiler projet : **File → TFT file output**
2. Connecter adaptateur USB-TTL:
   - USB-TTL TX → Nextion RX
   - USB-TTL RX → Nextion TX
   - GND commun
3. **File → Upload** → Sélectionner port COM

---

## Configuration firmware Arduino

### Activation module dans config.h

```cpp
// ════════════════════════════════════════════════════════════════
// CONFIGURATION NEXTION DISPLAY
// ════════════════════════════════════════════════════════════════

#define ENABLE_NEXTION      1           // Activer affichage Nextion (1=ON, 0=OFF)
#define TEST_NEXTION        1           // Test Nextion (nécessite ENABLE_NEXTION=1)

#define NEXTION_SERIAL      Serial1     // Port série (Serial1 = pins 18/19)
#define NEXTION_BAUD        9600        // Vitesse (DOIT correspondre à écran!)
#define NEXTION_UPDATE_INTERVAL  500    // Mise à jour toutes les 500ms
```

### Debug Nextion

Pour activer les messages debug:

```cpp
#if USE_ETHERNET == 1
    #define DEBUG_NEXTION       1  // Affiche résumé mises à jour Nextion
    #define DEBUG_NEXTION_VERBOSE 1  // Affiche CHAQUE commande envoyée
#endif
```

**Sortie debug typique** :
```
[NEXTION] Az:123.5° → 180.0° | El:45.2° → 30.0°
[→ NEXTION] tAzCur.txt="123.5°"
[→ NEXTION] tAzTgt.txt="180.0°"
[→ NEXTION] tElCur.txt="45.2°"
[→ NEXTION] tElTgt.txt="30.0°"
[→ NEXTION] tStatus.txt="EN MOUVEMENT"
```

---

## Test et debug

### Test communication UART

**Sketch test simple** (sans module Nextion):

```cpp
void setup() {
  Serial.begin(115200);  // Debug
  Serial1.begin(9600);   // Nextion
  delay(100);

  // Envoi commande test
  Serial1.print("page 0");  // Change page 0
  Serial1.write(0xFF);
  Serial1.write(0xFF);
  Serial1.write(0xFF);

  delay(100);

  Serial1.print("tTitle.txt=\"TEST OK\"");
  Serial1.write(0xFF);
  Serial1.write(0xFF);
  Serial1.write(0xFF);

  Serial.println("Commande envoyée!");
}

void loop() {
  // Rien
}
```

**Résultat attendu** : "TEST OK" s'affiche dans composant `tTitle`.

### Vérification affichage positions

1. **Activer module** : `ENABLE_NEXTION = 1` dans config.h
2. **Upload firmware** Arduino
3. **Ouvrir Serial Monitor** (115200 baud)
4. **Déplacer pot azimuth** → vérifier `tAzCur` se met à jour
5. **Envoyer commande Easycom** `AZ180.0` → vérifier `tAzTgt` affiche "180.0°"
6. **Vérifier état** : "ARRETE" si cible atteinte, "EN MOUVEMENT" sinon

---

## Dépannage

### Écran blanc / pas d'affichage

**Cause** : Firmware non uploadé ou erreur upload

**Solutions** :
1. Vérifier fichier `.tft` correct modèle Nextion
2. Réessayer upload carte microSD (formater FAT32)
3. Vérifier alimentation 5V stable (multimètre)

### Texte ne se met pas à jour

**Cause** : Noms composants incorrects ou vitesse UART incorrecte

**Solutions** :
1. **Vérifier noms composants** dans Nextion Editor:
   - Double-clic composant → onglet **Attribute** → `objname`
   - Doit correspondre EXACTEMENT : `tAzCur`, `tAzTgt`, etc.
2. **Vérifier vitesse UART** :
   - Nextion : `Program.s` → `bauds=9600`
   - Arduino : `NEXTION_BAUD = 9600`
   - Les deux DOIVENT être identiques!
3. **Test commande manuelle** via Serial Monitor:
   ```
   Envoyer: page 0ÿÿÿ
   Envoyer: tTitle.txt="TEST"ÿÿÿ
   ```
   (ÿ = caractère 0xFF)

### Affichage incorrect (caractères bizarres)

**Cause** : Vitesse UART incorrecte ou terminateurs manquants

**Solutions** :
1. Vérifier `NEXTION_BAUD` = vitesse écran
2. Vérifier 3× 0xFF envoyés après CHAQUE commande (voir `sendToNextion()`)
3. Vérifier câble TX/RX non inversés

### Positions cibles toujours "---"

**Cause** : Aucune cible active (PstRotator pas connecté)

**Solutions** :
1. **Normal si pas de commande GOTO** : "---" = pas de cible
2. **Connecter PstRotator** et envoyer commande `AZ180.0 EL45.0`
3. **Vérifier debug** : `targetAz` et `targetEl` doivent être >= 0.0

### État toujours "INITIALISATION"

**Cause** : Fonction `updateNextion()` pas appelée dans loop

**Solutions** :
1. Vérifier `TEST_NEXTION = 1` dans config.h
2. Vérifier `updateNextion()` dans [main.cpp](../src/main.cpp) ligne ~240
3. Vérifier debug: messages `[NEXTION]` doivent apparaître toutes les 500ms

---

## Références

- **Nextion Wiki officiel** : https://nextion.tech/instruction-set/
- **Nextion Editor download** : https://nextion.tech/nextion-editor/
- **Protocole UART Nextion** : https://nextion.tech/wp-content/uploads/2020/03/Nextion-Instruction-Set.pdf
- **Exemples projets** : https://github.com/topics/nextion

---

## Évolutions futures

**Fonctionnalités possibles** (si temps disponible):

1. **Boutons tactiles** :
   - Boutons STOP azimuth/élévation
   - Preset positions (ex: "Zenith", "Nord", "Parking")
   - Calibration tactile (boutons Z, E)

2. **Graphiques** :
   - Barre progression mouvement
   - Indicateur vitesse rotation
   - Historique positions (waveform)

3. **Alarmes visuelles** :
   - Clignotement si limite atteinte
   - Popup erreur réseau
   - Indicateur perte signal encodeur

4. **Informations système** :
   - Température (DS18B20)
   - Uptime
   - Adresse IP Ethernet

---

---

## Configuration boutons tactiles (Contrôle manuel)

### Vue d'ensemble

Le firmware implémente maintenant un contrôle tactile complet:
- **5 boutons tactiles**: CW, CCW, UP, DOWN, STOP
- **Commandes incrémentales**: Envoient commandes Easycom tant que bouton enfoncé
- **3 indicateurs d'état**: Direction azimuth, direction élévation, mode système
- **Communication bidirectionnelle**: Arduino ← Nextion (touch events)

### Fonctionnement

**Boutons CW / CCW** (Azimuth):
- **CW** : Déplace azimuth vers position actuelle + `MANUAL_INCREMENT_AZ` (5.0° par défaut)
- **CCW** : Déplace azimuth vers position actuelle - `MANUAL_INCREMENT_AZ`
- Wraparound automatique 0-360°
- Commande Easycom: `AZ180.0` (exemple)

**Boutons UP / DOWN** (Élévation):
- **UP** : Déplace élévation vers position actuelle + `MANUAL_INCREMENT_EL` (2.0° par défaut)
- **DOWN** : Déplace élévation vers position actuelle - `MANUAL_INCREMENT_EL`
- Limite automatique 0-90°
- Commande Easycom: `EL45.0` (exemple)

**Bouton STOP**:
- Arrêt immédiat azimuth ET élévation
- Commande Easycom: `SA` (Stop All)

**Logique mutex**:
- Un seul bouton actif à la fois
- Appui nouveau bouton → désactive les autres
- Throttling 500ms (2 commandes/seconde max)

**Indicateurs d'état**:
- **tAzDir** : Direction azimuth ("CW" / "CCW" / "---")
- **tElDir** : Direction élévation ("UP" / "DOWN" / "---")
- **tMode** : Mode système ("PARKING" / "STOP" / "---")

---

## Configuration Nextion Editor - Interface tactile

### Page 0 - Layout complet recommandé (480×320px)

```
┌──────────────────────────────────────────────────────┐
│                EME ROTATOR                           │ tTitle
├──────────────────────────────────────────────────────┤
│                                                      │
│  Azimuth:                                            │
│                 123.5°           [CW]  [CCW]         │ tAzCur, bCW, bCCW
│              →  180.0°                               │ tAzTgt
│                                                      │
│  Élévation:                                          │
│                  45.2°           [UP] [DOWN]         │ tElCur, bUP, bDOWN
│              →   30.0°                               │ tElTgt
│                                                      │
│                              [  STOP  ]              │ bSTOP (gros bouton)
│                                                      │
│  Az: CW    El: UP    Mode: ---                      │ tAzDir, tElDir, tMode
├──────────────────────────────────────────────────────┤
│              EN MOUVEMENT                            │ tStatus
└──────────────────────────────────────────────────────┘
```

### Composants requis

#### Textes existants (déjà configurés)

| Composant | Type | Nom (objname) | Position | Taille | Texte initial |
|-----------|------|---------------|----------|--------|---------------|
| Titre     | Text | `tTitle`      | 0, 0     | 480×30 | "EME ROTATOR" |
| Az actuel | Text | `tAzCur`      | 160, 50  | 120×40 | "---" |
| Az cible  | Text | `tAzTgt`      | 160, 95  | 120×40 | "---" |
| El actuel | Text | `tElCur`      | 160, 150 | 120×40 | "---" |
| El cible  | Text | `tElTgt`      | 160, 195 | 120×40 | "---" |
| État      | Text | `tStatus`     | 0, 300   | 480×20 | "INITIALISATION" |

#### Boutons tactiles (NOUVEAUX)

| Composant | Type   | Nom (objname) | Position | Taille | Texte | ID (vérifier!) |
|-----------|--------|---------------|----------|--------|-------|----------------|
| CW        | Button | `bCW`         | 300, 50  | 80×40  | "CW"  | 2 (par défaut) |
| CCW       | Button | `bCCW`        | 390, 50  | 80×40  | "CCW" | 3 (par défaut) |
| UP        | Button | `bUP`         | 300, 150 | 80×40  | "UP"  | 4 (par défaut) |
| DOWN      | Button | `bDOWN`       | 390, 150 | 80×40  | "DOWN"| 5 (par défaut) |
| STOP      | Button | `bSTOP`       | 190, 240 | 100×50 | "STOP"| 6 (par défaut) |

**IMPORTANT - Vérification IDs composants**:

Les IDs sont assignés automatiquement par Nextion Editor lors de la création des composants. Pour vérifier l'ID d'un bouton:

1. Double-clic sur le bouton dans Nextion Editor
2. Onglet **Attribute** → champ **id** (lecture seule)
3. Noter l'ID réel (peut différer de celui par défaut!)

Si vos IDs diffèrent, modifiez les valeurs dans [src/nextion.cpp](../src/nextion.cpp) ligne ~55:

```cpp
#define NEXTION_ID_BCW   2   // ID bouton CW (VÉRIFIER!)
#define NEXTION_ID_BCCW  3   // ID bouton CCW
#define NEXTION_ID_BUP   4   // ID bouton UP
#define NEXTION_ID_BDOWN 5   // ID bouton DOWN
#define NEXTION_ID_BSTOP 6   // ID bouton STOP
```

#### Indicateurs d'état (NOUVEAUX)

| Composant | Type | Nom (objname) | Position | Taille | Texte initial | Couleur |
|-----------|------|---------------|----------|--------|---------------|---------|
| Az Dir    | Text | `tAzDir`      | 30, 270  | 80×25  | "Az: ---"     | Vert    |
| El Dir    | Text | `tElDir`      | 130, 270 | 80×25  | "El: ---"     | Vert    |
| Mode      | Text | `tMode`       | 230, 270 | 100×25 | "Mode: ---"   | Vert    |

---

## Configuration boutons dans Nextion Editor

### Étape 1 - Créer les boutons

Pour chaque bouton (bCW, bCCW, bUP, bDOWN, bSTOP):

1. **Ajouter composant Button** :
   - Toolbox → **Button** → glisser sur page 0
   - Positionner selon layout ci-dessus

2. **Configurer attributs** (onglet Attribute):
   - **objname** : `bCW`, `bCCW`, `bUP`, `bDOWN`, ou `bSTOP` (EXACT!)
   - **txt** : Texte affiché ("CW", "CCW", "UP", "DOWN", "STOP")
   - **font** : Font 2 ou 3 (Arial 24 ou 32)
   - **bco** : Couleur fond (31 = bleu foncé)
   - **pco** : Couleur texte (65535 = blanc)
   - **bco2** : Couleur fond pressé (2016 = vert)
   - **pco2** : Couleur texte pressé (0 = noir)

3. **Activer événements tactiles** (onglet Event):
   - **Touch Press Event** : Cocher ✓ "Send Component ID"
   - **Touch Release Event** : Cocher ✓ "Send Component ID"

   **CRITIQUE**: Sans ces options cochées, aucun événement ne sera envoyé à l'Arduino!

4. **Vérifier ID composant** (onglet Attribute):
   - Noter valeur champ **id** (lecture seule)
   - Modifier `nextion.cpp` si ID différent

### Étape 2 - Créer les indicateurs d'état

Pour chaque indicateur (tAzDir, tElDir, tMode):

1. **Ajouter composant Text** :
   - Toolbox → **Text** → glisser sur page 0

2. **Configurer attributs**:
   - **objname** : `tAzDir`, `tElDir`, ou `tMode` (EXACT!)
   - **txt** : Texte initial ("Az: ---", "El: ---", "Mode: ---")
   - **font** : Font 1 (Arial 16)
   - **xcen** : 0 (alignement gauche)
   - **ycen** : 1 (centré verticalement)
   - **bco** : 0 (fond noir)
   - **pco** : 2016 (texte vert par défaut)

3. **Pas d'événement requis** (lecture seule)

### Étape 3 - Vérifier configuration UART bidirectionnelle

**Dans Nextion Editor** (Program.s - Pre-Initialize):

```c
bauds=9600
```

**Dans config.h Arduino**:

```cpp
#define NEXTION_SERIAL      Serial1     // Port série (pins 18/19)
#define NEXTION_BAUD        9600        // DOIT correspondre!
```

**Câblage CRITIQUE pour touch**:

| Nextion Pin | Arduino Pin | Description |
|-------------|-------------|-------------|
| 1 (VCC)     | 5V          | Alimentation |
| 2 (RX)      | 18 (TX1)    | Arduino → Nextion (affichage) |
| 3 (TX)      | 19 (RX1)    | **Nextion → Arduino (touch events)** |
| 4 (GND)     | GND         | Masse |

**IMPORTANT**: Pin 19 (RX1) DOIT être connectée pour recevoir les événements tactiles!

---

## Configuration firmware Arduino - Contrôle manuel

### Paramètres ajustables (config.h)

```cpp
// ─────────────────────────────────────────────────────────────────
// CONTRÔLE MANUEL TACTILE
// ─────────────────────────────────────────────────────────────────

#define MANUAL_INCREMENT_AZ  5.0        // Incrément manuel azimuth (degrés)
#define MANUAL_INCREMENT_EL  2.0        // Incrément manuel élévation (degrés)
```

**Ajustement selon vitesse moteurs**:

- **Moteurs lents EME** (21 min/360° = 0.29°/sec):
  - Azimuth: 5.0° à 10.0° (10-20 sec de mouvement)
  - Élévation: 2.0° à 5.0° (5-10 sec de mouvement)

- **Moteurs rapides**:
  - Azimuth: 1.0° à 2.0°
  - Élévation: 0.5° à 1.0°

**Throttling boutons** (nextion.cpp ligne ~47):

```cpp
#define BUTTON_COMMAND_INTERVAL 500   // Commandes toutes les 500ms
```

- 500ms = 2 commandes/seconde (recommandé moteurs lents)
- 200ms = 5 commandes/seconde (moteurs rapides)

---

## Test et debug - Contrôle tactile

### Test communication bidirectionnelle

**Upload firmware Nextion** avec boutons configurés, puis:

1. **Activer debug verbose** dans config.h:
   ```cpp
   #define DEBUG_NEXTION_VERBOSE 1
   ```

2. **Upload firmware Arduino**

3. **Ouvrir Serial Monitor** (115200 baud)

4. **Appuyer bouton CW** sur Nextion

**Sortie attendue** (Serial Monitor):

```
[NEXTION TOUCH] Page=0 ID=2 Event=PRESS
[NEXTION BTN] CW → AZ 123.5° + 5.0° = 128.5°
[NEXTION → EASYCOM] AZ128.5
```

5. **Relâcher bouton CW**

```
[NEXTION TOUCH] Page=0 ID=2 Event=RELEASE
```

6. **Vérifier mouvement moteur** : Azimuth doit se déplacer vers 128.5°

### Vérification indicateurs d'état

Pendant mouvement azimuth:
- **tAzDir** doit afficher "CW" (jaune)
- **tElDir** doit afficher "---" (vert)
- **tStatus** doit afficher "EN MOUVEMENT" (rouge)

Quand cible atteinte:
- **tAzDir** doit afficher "---" (vert)
- **tStatus** doit afficher "ARRETE" (vert)

### Dépannage touch events

**Problème**: Aucun événement tactile reçu (pas de debug `[NEXTION TOUCH]`)

**Solutions**:

1. **Vérifier câblage pin 19 (RX1)** :
   - DOIT être connecté Nextion TX → Arduino RX1
   - Tester continuité avec multimètre

2. **Vérifier configuration boutons** (Nextion Editor):
   - Onglet Event → "Send Component ID" coché pour Press ET Release
   - Recompiler .TFT et re-uploader sur écran

3. **Vérifier vitesse UART identique**:
   - Nextion: `bauds=9600`
   - Arduino: `NEXTION_BAUD = 9600`

4. **Test manuel événement** (Serial Monitor):
   - Débrancher Arduino pin 19
   - Envoyer manuellement: `0x65 0x00 0x02 0x01 0xFF 0xFF 0xFF`
   - Si debug apparaît → problème câblage Nextion TX
   - Si rien → problème code Arduino

**Problème**: Boutons envoyés mais moteur ne bouge pas

**Solutions**:

1. **Vérifier commandes Easycom générées** :
   - Debug doit afficher `[NEXTION → EASYCOM] AZ128.5`
   - Vérifier format: `AZ` ou `EL` suivi nombre avec 1 décimale

2. **Vérifier IDs composants**:
   - IDs dans `nextion.cpp` doivent correspondre aux IDs réels
   - Vérifier dans Nextion Editor (attribut `id` de chaque bouton)

3. **Vérifier logique mutex**:
   - Un seul bouton peut être actif
   - Appuyer nouveau bouton désactive l'ancien

**Problème**: Indicateurs d'état ne se mettent pas à jour

**Solutions**:

1. **Vérifier noms composants** :
   - Doivent être EXACTEMENT: `tAzDir`, `tElDir`, `tMode`
   - Vérifier dans Nextion Editor (attribut `objname`)

2. **Vérifier fonction appelée** :
   - main.cpp doit appeler `updateNextionIndicators()`
   - Vérifier ligne ~265 (après `updateNextion()`)

---

## Résumé configuration - Checklist complète

### Nextion Editor (.HMI)

- [ ] **Boutons tactiles créés** : bCW, bCCW, bUP, bDOWN, bSTOP
- [ ] **Attribut objname correct** (EXACT, sensible casse)
- [ ] **Event "Send Component ID" coché** (Press ET Release)
- [ ] **IDs composants notés** (onglet Attribute → id)
- [ ] **Indicateurs créés** : tAzDir, tElDir, tMode
- [ ] **Program.s configuré** : `bauds=9600`
- [ ] **Fichier .TFT compilé et uploadé** sur écran

### Arduino firmware

- [ ] **config.h - NEXTION_BAUD = 9600** (correspond écran)
- [ ] **config.h - MANUAL_INCREMENT_AZ/EL** ajustés selon moteurs
- [ ] **nextion.cpp - IDs composants** correspondent fichier .HMI
- [ ] **main.cpp - Fonctions appelées** : `readNextionTouch()`, `handleNextionButtons()`, `updateNextionIndicators()`
- [ ] **Firmware Arduino uploadé**

### Câblage

- [ ] **VCC (rouge)** : Nextion → Arduino 5V
- [ ] **RX (jaune)** : Nextion → Arduino TX1 (pin 18)
- [ ] **TX (bleu)** : Nextion → Arduino RX1 (pin 19) ← **CRITIQUE pour touch!**
- [ ] **GND (noir)** : Nextion → Arduino GND

### Test

- [ ] **Debug NEXTION_VERBOSE activé**
- [ ] **Serial Monitor ouvert** (115200 baud)
- [ ] **Touch events reçus** (`[NEXTION TOUCH]` dans debug)
- [ ] **Commandes Easycom générées** (`[NEXTION → EASYCOM]`)
- [ ] **Moteurs bougent** quand boutons pressés
- [ ] **Indicateurs se mettent à jour** (tAzDir, tElDir, tMode)

---

**Note finale**: Le firmware implémente maintenant une communication **bidirectionnelle complète** Arduino ↔ Nextion. Les boutons tactiles envoient des commandes Easycom à PstRotator, garantissant que le logiciel de contrôle est toujours informé des mouvements manuels effectués via l'écran tactile.
