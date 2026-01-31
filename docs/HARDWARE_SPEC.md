# SPÉCIFICATIONS MATÉRIEL - Station EME 10 GHz

## Vue d'ensemble

Système complet de contrôle rotator pour station EME (Earth-Moon-Earth) 10 GHz avec tracking automatique lunaire, monitoring distribué et interface réseau multi-clients.

---

## CONTRÔLEUR PRINCIPAL

### Arduino Mega Pro 2560 (SMD)

**Spécifications** :
- MCU : ATmega2560
- Fréquence : 16 MHz
- Mémoire Flash : 256 KB
- SRAM : 8 KB
- EEPROM : 4 KB
- GPIO : 54 digitales (15 PWM)
- ADC : 16 × 10-bit
- UART : 4 ports série hardware
- I²C : 1 bus (pins 20/21)
- SPI : 1 bus (pins 50-53)
- Interrupts externes : 6 (INT0-5)
- Tension : 5V logique
- Alimentation : 7-12V DC (régulateur interne)

**Utilisation** :
- Contrôle moteurs rotator (PWM + safety)
- Lecture encodeurs absolus (interrupts)
- Communication réseau Ethernet (W5500)
- Interface Nextion (UART)
- Communication Nano R4 tête RF (UART)
- Monitoring tensions/températures local rack

**Prix** : ~15€

---

## SHIELD ETHERNET

### W5500 Ethernet Module

**Spécifications** :
- Chip : WIZnet W5500
- Interface : SPI
- Vitesse : 10/100 Mbps
- Sockets TCP/UDP : 8 indépendants
- Buffer interne : 32 KB
- Connecteur : RJ45 avec magnetics intégrés
- Alimentation : 3.3V (régulateur Mega)

**Utilisation** :
- Serveur TCP port 4533 (protocole Easycom pour PstRotator)
- Serveur TCP port 5000 (protocole custom JSON pour Python app)
- Client NTP (sync heure UTC)
- Multi-clients simultanés

**Connexion** :
- SPI (pins 50-53 Mega)
- Reset optionnel (pin 49)

**Prix** : ~8€

---

## DRIVER MOTEURS

### Pololu Dual MC33926 Motor Driver

**Spécifications** :
- Chip : Freescale MC33926 (×2, un par moteur)
- Tension moteurs : 5.5-28V DC
- Courant continu : 3A par canal
- Courant pic : 5A par canal (quelques secondes)
- Feedback courant : 0.525 V/A
- PWM : Jusqu'à 20 kHz
- Protections : Thermique (150°C), court-circuit, sous-tension
- Status flags : Pins SF (LOW = défaut détecté)
- Contrôle direction : 2× PWM par moteur (sign-magnitude)
- Enable global + Slew rate + Inversion

**URL** : https://www.pololu.com/product/1213

**Utilisation** :
- Moteur Azimuth (M1)
- Moteur Élévation (M2)
- Monitoring courants temps réel (safety)
- Contrôle vitesse PWM (PID)

**Avantages** :
- 1 module = 2 ponts H indépendants
- Protections intégrées (pas de circuit externe)
- Feedback courant précis
- Compact

**Prix** : ~35€

---

## ROTATOR

### Coresun SVH3-62B-RC-24H3033 (×2 axes)

**Spécifications moteurs** :
- Type : DC brushed 24V
- Courant nominal : 1.9A
- Courant démarrage : ~3-4A (pic court)
- Couple : 33 N.m
- Vitesse : 0.048 rpm (après réduction)
- Réduction : 552:1

**Encodeurs Hall** :
- Type : Incrémental absolu
- Résolution : 12 ppr (pulses per revolution moteur)
- Après réduction : 6624 counts/360°
- Précision : 0.054°/count
- Sortie : Push-pull 5V (A/B quadrature)

**Mécanique** :
- Charge max : 150 kg
- Vitesse rotation : 0.048 rpm (21 min pour 360°)
- Protection IP : IP65 (usage extérieur)
- Température : -30°C à +70°C

**Utilisation** :
- SVH3 #1 : Azimuth (rotation horizontale)
- SVH3 #2 : Élévation (inclinaison verticale)

**Statut** : En commande, réception prévue ~5 jours

**Prix** : ~300-400€/unité

---

## FINS DE COURSE

### Module VMA452 (Velleman)

**Spécifications** :
- Optocoupleurs : 4× PC817
- Entrées : 4× (pour switches externes)
- Sorties : 4× (LOW = switch fermé)
- Logique : Non-inversée
- Tension : 5V
- Courant : <20mA par canal
- LED status : Oui (indication visuelle)

**Utilisation** :
- OUT1 : Azimuth CW limit
- OUT2 : Azimuth CCW limit
- OUT3 : Élévation UP limit
- OUT4 : Élévation DOWN limit

**Switches connectés** :
- Type : Microswitch SPDT
- Normalement ouvert (NO)
- Montage : Mécanique sur structure rotator

**Prix** : ~8€

---

## MCU SECONDAIRE (TÊTE RF)

### Arduino Nano R4 WiFi

**Spécifications** :
- MCU : Renesas RA4M1 (ARM Cortex-M4 48 MHz)
- Mémoire : 256 KB Flash, 32 KB SRAM
- ADC : **14-bit** (vs 10-bit classique Arduino)
- Résolution ADC : 0-16383 (vs 0-1023)
- Tension : 5V logique, ADC 0-3.3V
- UART : 1 hardware (Serial1)
- I²C : 1 bus
- WiFi : ESP32-S3 (non utilisé pour ce projet)

**Utilisation** :
- Monitoring tensions tête RF (préampli, PA, transverter, relais)
- Monitoring températures (PA, transverter via DS18B20)
- Monitoring puissance PA (PA MON)
- Communication UART vers Mega (115200 baud)
- Lecture capteurs ADC 14-bit haute précision

**Avantages vs Mega** :
- ADC 14-bit = 16× plus précis (critique pour tensions RF)
- Installation locale près parabole (câbles capteurs courts)
- Isolation électrique tête RF / rack

**Prix** : ~25€

---

## AFFICHAGE

### Nextion NX4832T035 (Basic Series)

**Spécifications** :
- Taille : 3.5" (89 mm diagonale)
- Résolution : 480×320 pixels
- Tactile : Résistif
- Mémoire Flash : 16 MB
- RAM : 3584 bytes
- Interface : UART (TTL 5V)
- Vitesse : 9600 baud (standard)
- Alimentation : 5V, ~250mA

**Fonctionnalités** :
- Mode veille automatique (économie énergie)
- Wake on touch
- Widgets : Texte, images, bargraphs, courbes
- Éditeur : Nextion Editor (Windows)

**Utilisation** :
- Affichage Azimuth / Élévation
- Status système (OK / TRACKING / ALARM)
- Mode veille après 15s inactivité
- Touch pour réveiller et consulter position

**Interface** : 1 page unique, affichage minimal (pas de boutons)

**Prix** : ~25€

**Alternative possédée** : NX4832K035 (Enhanced, avec RTC) - non utilisée pour ce projet

---

## CAPTEURS TEMPÉRATURE

### DS18B20 Digital Temperature Sensor

**Spécifications** :
- Interface : 1-Wire (Dallas/Maxim)
- Plage : -55°C à +125°C
- Précision : ±0.5°C (-10°C à +85°C)
- Résolution : 9 à 12 bits (0.0625°C à 12-bit)
- Alimentation : 3.0-5.5V
- Temps conversion : ~750ms (12-bit)
- Adresse ROM unique : 64-bit (plusieurs capteurs sur même bus)

**Utilisation** :
- **Mega (rack)** : 1× DS18B20 température ambiante
- **Nano R4 (tête RF)** : 2× DS18B20 (PA 10W + Transverter)

**Montage sonde PA** :
- Coller avec pâte thermique sur dissipateur PA
- Isoler électriquement (gaine thermorétractable)
- Fixer avec collier serflex

**Prix** : ~3€/unité (sonde étanche)

---

## ALIMENTATIONS

### PSU 24V 6A (Moteurs)

**Spécifications** :
- Tension sortie : 24V DC régulée
- Courant : 6A continu (144W)
- Protection : Court-circuit, surcharge, surtension
- Refroidissement : Ventilateur (selon modèle)
- Entrée : 230V AC 50Hz

**Utilisation** :
- Alimentation moteurs Azimuth + Élévation
- Via Pololu Dual MC33926

**Dimensionnement** :
- 2 moteurs × 1.9A nominal = 3.8A
- Pics démarrage ~5A
- Marge : 6A suffisant

**Prix** : ~20-30€

---

### PSU 12V 2A (Logique)

**Spécifications** :
- Tension sortie : 12V DC régulée
- Courant : 2A (24W)
- Protection : Court-circuit, surcharge

**Utilisation** :
- Alimentation Arduino Mega (via VIN)
- Régulateur interne Mega : 12V → 5V (800mA max)
- Périphériques 5V : W5500, Nextion, capteurs

**Consommation estimée** :
- Mega : ~150mA
- W5500 : ~150mA
- Nextion : ~100mA (éteint la plupart du temps)
- Capteurs/MCP : ~50mA
- **Total** : ~450mA @ 5V

**Prix** : ~10-15€

---

### Alternative : PSU Unique 24V 8A + Buck Converter

**Option** : 1 seule alimentation 24V
- PSU 24V 8A (200W)
- Step-down DC-DC 24V → 12V 3A (LM2596 ou équivalent)

**Avantages** :
- 1 seule PSU
- Moins de câblage AC

**Inconvénients** :
- Panne PSU = tout down
- Bruit switching buck possible

**Recommandation** : 2 PSU séparées (isolation pannes)

---

## COMPOSANTS PASSIFS

### Résistances (1/4W, ±1% métal film)

| Valeur | Qté | Usage |
|--------|-----|-------|
| 1kΩ | 5 | Protection ADC, LED |
| 2.2kΩ | 2 | Pull-ups I²C (si besoin) |
| 4.7kΩ | 6 | Pull-ups encodeurs, 1-Wire |
| 10kΩ | 2 | Pull-ups divers |
| 12kΩ | 3 | Diviseurs tensions (R2) |
| 18kΩ | 1 | Diviseur 12V |
| 22kΩ | 1 | Diviseur 13.8V |
| 47kΩ | 1 | Diviseur 24V |
| 120Ω | 2 | Terminaisons RS-485 (si utilisé futur) |

---

### Condensateurs

| Valeur | Type | Tension | Qté | Usage |
|--------|------|---------|-----|-------|
| 100µF | Électrolytique | 16V | 2 | Découplage alim Mega, W5500 |
| 100µF | Électrolytique | 35V | 2 | Découplage 24V moteurs |
| 470µF | Électrolytique | 35V | 1 | Bulk capacitor 24V |
| 10µF | Tantale | 10V | 1 | W5500 3.3V |
| 100nF | Céramique | 50V | 20 | Découplage VCC (tous ICs) |
| 10nF | Céramique | 50V | 2 | Filtrage ADC |

---

### Semiconducteurs

| Type | Qté | Usage |
|------|-----|-------|
| 1N5822 (Schottky 3A) | 1 | Protection inversion 24V |
| 1N4733A (Zener 5.1V 1W) | 4 | Protection ADC |
| LED 3mm verte | 1 | Power indicator |

---

### Connecteurs

| Type | Qté | Usage |
|------|-----|-------|
| Bornier vis 2P (5.08mm) | 4 | Alim 12V/24V, Moteurs |
| Bornier vis 5P (5.08mm) | 1 | Tensions monitoring |
| JST-XH 6P (2.54mm) | 3 | Encodeurs Az/El, VMA452 |
| JST-XH 4P (2.54mm) | 1 | UART Nano R4 |
| JST-XH 3P (2.54mm) | 1 | DS18B20 |
| Header 1×4 (2.54mm) | 1 | Nextion |
| Fusible 5×20mm | 2 | Protection 2A (12V), 5A (24V) |
| Porte-fusible PCB | 2 | Montage traversant |

---

## CÂBLES

### Câble Cat6 STP (Blindé)

**Spécifications** :
- Type : Paire torsadée blindée
- Conducteurs : 4 paires (8 fils)
- Blindage : Tresse aluminium
- Impédance : 100Ω
- Catégorie : Cat6 (jusqu'à 250 MHz)

**Utilisation** :
- UART Mega ↔ Nano R4 (1 paire)
- Alimentation 5V/GND Nano (1 paire)
- Blindage → GND

**Longueur** : 20m (suffisant pour 2-3m utiles + marge)

**Prix** : ~10€/20m

---

### Câble moteurs

**Spécifications** :
- Section : 1.0-1.5 mm²
- Type : Souple multibrins
- Couleurs : Rouge/Noir (polarité)

**Longueur** : Selon installation (3-10m typique)

**Prix** : ~5€

---

### Câbles encodeurs

**Spécifications** :
- Type : Blindé 4 conducteurs + tresse
- Section : 0.25 mm²

**Longueur** : Selon installation (3-10m)

**Prix** : ~10€

---

## RÉCAPITULATIF COÛTS

| Composant | Prix unitaire | Qté | Total |
|-----------|---------------|-----|-------|
| **MCU & Modules** | | | |
| Arduino Mega Pro 2560 | 15€ | 1 | 15€ |
| Arduino Nano R4 | 25€ | 1 | 25€ |
| W5500 Ethernet | 8€ | 1 | 8€ |
| Pololu Dual MC33926 | 35€ | 1 | 35€ |
| **Affichage** | | | |
| Nextion NX4832T035 | 25€ | 1 | 25€ |
| **Rotator** | | | |
| Coresun SVH3 | 350€ | 2 | 700€ |
| **Capteurs** | | | |
| DS18B20 sonde | 3€ | 3 | 9€ |
| VMA452 | 8€ | 1 | 8€ |
| **Alimentation** | | | |
| PSU 24V 6A | 25€ | 1 | 25€ |
| PSU 12V 2A | 12€ | 1 | 12€ |
| **Passifs & Connecteurs** | | | 30€ |
| **Câbles** | | | 25€ |
| **TOTAL ÉLECTRONIQUE** | | | **~920€** |
| **Parabole Kathrein CAS90** | Variable | 1 | (Possédée) |
| **RF (PA, Préampli, Transverter)** | Variable | - | (Existant) |

**Note** : Prix indicatifs, peuvent varier selon fournisseurs

---

## FOURNISSEURS RECOMMANDÉS

### Modules Arduino
- **AliExpress** : Mega Pro, W5500 (pas cher, 2-3 semaines)
- **Amazon** : Nano R4 officiel (rapide, garantie)

### Driver moteurs
- **Pololu** : Direct USA (officiel, 10 jours)
- **Mouser / Farnell** : Europe (rapide, stock)

### Écran Nextion
- **Itead** : Direct Chine (officiel, 2 semaines)
- **Amazon** : Stock Europe (rapide, +10€)

### Composants passifs
- **Mouser / Farnell / DigiKey** : Qualité, fiable
- **AliExpress** : Kits résistances/condensateurs (économique)

### Câbles
- **Magasin local** : Cat6 disponible partout
- **Conrad / RS Components** : Qualité pro

---

## OUTILS NÉCESSAIRES

### Développement
- PC avec VS Code + PlatformIO
- Câble USB A-B (programmation Mega)
- Câble USB micro/C (programmation Nano R4)

### Électronique
- Multimètre
- Fer à souder + étain
- Pince à sertir JST-XH
- Pince à dénuder
- Tournevis précision

### Mécanique
- Clés Allen (montage rotator)
- Perceuse (fixations)
- Serre-câbles (organisation)

---

## EXTENSIONS FUTURES POSSIBLES

### Court terme
- GPS USB sur NUC (heure précise, position)
- Anémomètre (sécurité vent fort)
- Capteur pluie (protection parabole)

### Moyen terme
- Deuxième parabole test (multi-Nano via UART)
- Caméra IP (surveillance visuelle tracking)
- UPS (onduleur batterie, continuité)

### Long terme
- Rotator 47 GHz (même architecture)
- Système multi-bandes (10/24/47 GHz)
- Station météo complète intégrée

---

**Version** : 1.0
**Date** : 2026-01-30
**Auteur** : ON7KGK
**Projet** : EME 10 GHz Station Controller
```

---