# EME Rotator Controller

Contrôleur de rotateur EME (Earth-Moon-Earth) pour parabole CAS90 avec tracking lunaire.

## 🎯 Objectif

Système de contrôle d'antenne parabolique pour communications EME (Moon Bounce) avec :
- Tracking lunaire automatique via PstRotator
- Protocole Easycom (compatible Ham Radio Deluxe, SatPC32, etc.)
- Support encodeurs absolus/incrémentaux SSI
- Moteurs pas-à-pas NEMA23 avec drivers TB6600
- Ethernet W5500 ou Serial USB

## 🔧 Matériel

### Plateforme
- **Arduino Mega Pro 2560** (ATmega2560, 16MHz, 5V)
- **Module Ethernet W5500** (communication réseau optionnelle)

### Capteurs Position
- **Encodeurs SSI HH-12** (12-bit absolus, 4096 counts/tour)
  - Azimuth: Mode incrémental multi-tours
  - Élévation: Mode absolu 0-90°

### Moteurs
- **Moteurs Pas-à-Pas NEMA23** (1.8°/step, 1.9A)
- **Drivers TB6600** (microstepping 1/8 = 1600 steps/tour)
- Vitesse adaptative (rapide loin de cible, lente en approche finale)

### Sécurité
- **Fins de course NC** (Normally Closed) en série
- Arrêt automatique si limite atteinte
- Boutons manuels locaux (CW/CCW/UP/DOWN/STOP)

## 📁 Structure Projet

```
EME-CRTL-KGK-001/
├── include/
│   ├── config.h          # Configuration centrale (pins, vitesses, seuils)
│   ├── encoder_ssi.h     # Interface encodeurs SSI
│   ├── motor_stepper.h   # Contrôle moteurs pas-à-pas
│   ├── motor_dc.h        # Moteurs DC (futur SVH3)
│   ├── safety.h          # Gestion fins de course
│   ├── network.h         # Module Ethernet W5500
│   └── easycom.h         # Protocole Easycom
├── src/
│   ├── main.cpp          # Point d'entrée + loop principale
│   ├── encoder_ssi.cpp   # Lecture encodeurs + calibration
│   ├── motor_stepper.cpp # Asservissement position
│   ├── motor_dc.cpp      # PID moteurs DC (prévu)
│   ├── safety.cpp        # Vérification limites
│   ├── network.cpp       # TCP/IP W5500
│   └── easycom.cpp       # Parsing commandes Easycom
├── docs/
│   ├── HARDWARE_SPEC.md  # Spécifications matériel
│   ├── PINOUT.md         # Affectation pins Mega
│   ├── WIRING.md         # Câblage physique
│   ├── ARCHITECTURE.md   # Architecture logicielle
│   ├── TODO_PHASES.md    # Plan développement
│   └── DUAL_MC33926.md   # Guide driver DC (futur)
├── platformio.ini        # Configuration PlatformIO
├── reset_eeprom.ino      # Utilitaire reset calibration
└── test_blink.cpp        # Test minimal upload Arduino

```

## 🚀 Démarrage Rapide

### Prérequis
- [PlatformIO](https://platformio.org/) (VS Code extension ou CLI)
- Arduino Mega 2560 connecté via USB

### Compilation et Upload

```bash
# Cloner le projet
git clone https://github.com/votre-username/EME-CRTL-KGK-001.git
cd EME-CRTL-KGK-001

# Compiler
pio run

# Upload sur Arduino
pio run --target upload

# Moniteur série (9600 baud)
pio device monitor -b 9600
```

### Configuration Initiale

1. **Éditer `include/config.h`** :
   - Ajuster `GEAR_RATIO_AZ` et `GEAR_RATIO_EL` selon réduction mécanique
   - Vérifier pins encodeurs et moteurs
   - Choisir mode communication (USB Serial ou Ethernet)

2. **Calibration EEPROM** :
   - Pointer antenne vers référence connue (ex: Nord = 0°)
   - Envoyer commande Easycom : `Z0.0` (calibre azimuth à 0°)
   - Envoyer commande : `E0.0` (calibre élévation à 0°)

3. **Reset EEPROM** (si positions incorrectes) :
   - Upload `reset_eeprom.ino`
   - Ouvrir Serial Monitor → voir valeurs effacées
   - Re-upload programme principal

## 📡 Protocole Easycom

### Commandes Supportées

| Commande | Description | Exemple |
|----------|-------------|---------|
| `AZ xxx.x EL yyy.y` | GOTO position | `AZ 123.5 EL 45.0` |
| `AZ` | Lire azimuth | → `AZ=123.5` |
| `EL` | Lire élévation | → `EL=45.0` |
| `SA` | Arrêt tous moteurs | `SA` |
| `Z xxx.x` | Calibration azimuth | `Z0.0` |
| `E yyy.y` | Calibration élévation | `E90.0` |
| `RESET_EEPROM` | Effacer calibration | `RESET_EEPROM` |

### Connexion PstRotator

**Mode Serial USB** (`USE_ETHERNET = 0`) :
- Port COM Arduino (ex: COM11)
- Vitesse: 9600 baud
- Protocole: Easycom

**Mode Ethernet** (`USE_ETHERNET = 1`) :
- IP: 192.168.1.177 (configurable)
- Port: 4533
- Protocole: Easycom over TCP/IP

## ⚙️ Architecture Modulaire

Le code utilise des switches conditionnels pour activer/désactiver modules durant développement :

```cpp
#define TEST_ENCODERS    1  // Lecture position
#define TEST_MOTORS      1  // Contrôle moteurs
#define TEST_BUTTONS     1  // Boutons manuels
#define TEST_LIMITS      1  // Fins de course
#define TEST_NETWORK     1  // Ethernet W5500
#define TEST_CALIBRATION 1  // EEPROM
```

Désactiver un module (mettre à `0`) le retire de la compilation → gain mémoire.

## 🔐 Sécurité

- **Fins de course NC** : Circuit ouvert = arrêt immédiat
- **Filtre micro-mouvements** : Ignore commandes < 0.15° (évite vibrations)
- **Normalisation angles** : Azimuth maintenu 0-360°, Élévation 0-90°
- **Watchdog** : Arrêt moteurs si perte communication (prévu)

## 📊 Performances

- **Résolution encodeurs** : 0.088° (4096 counts/tour, ratio 10:1)
- **Précision atteinte** : ±0.08° (tolérance position)
- **Vitesse max** : 400µs/step (rapide)
- **Vitesse approche** : 4000µs/step (précise)
- **Latence réseau** : <10ms (Ethernet W5500)

## 🛠️ Développement

### Phases Implémentation

- [x] **Phase 1** : Encodeurs SSI (lecture + tracking tours)
- [x] **Phase 2** : Moteurs stepper (asservissement position)
- [x] **Phase 3** : Boutons manuels locaux
- [x] **Phase 4** : Fins de course sécurité
- [x] **Phase 5** : Réseau Ethernet + Easycom
- [x] **Phase 6** : Calibration EEPROM
- [ ] **Phase 7** : Moteurs DC brushed (futur SVH3)
- [ ] **Phase 8** : Écran Nextion
- [ ] **Phase 9** : Nano R4 WiFi (tête RF)

### Debug

**Mode Ethernet** (`USE_ETHERNET = 1`) :
- Debug Serial USB actif
- Affiche commandes Easycom, positions, états

**Mode Serial USB** (`USE_ETHERNET = 0`) :
- Debug désactivé (Serial = Easycom)
- Utiliser LEDs ou boutons test

## 📝 Licence

Projet personnel EME - Code open source

## 👤 Auteur

**KGK** - Opérateur radioamateur EME (Earth-Moon-Earth)

## 🔗 Ressources

- [PlatformIO Docs](https://docs.platformio.org/)
- [Easycom Protocol](http://www.qsl.net/dh1ngp/onlinehilfe/help_rotator.htm)
- [HH-12 SSI Encoders](http://www.rls.si/eng/hh12-hhp12-magnetic-encoder)
- [TB6600 Stepper Driver](https://www.dfrobot.com/product-1547.html)

---

**Status** : ✅ Opérationnel (SSI encodeurs + Stepper motors + Ethernet)
