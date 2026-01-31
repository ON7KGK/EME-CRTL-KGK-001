# ARCHITECTURE SYSTÈME - Station EME 10 GHz Controller

## Vue d'ensemble

Architecture complète du système de contrôle rotator EME avec monitoring distribué, interfaces réseau multiples et tracking automatique lunaire.

---

## DIAGRAMME GÉNÉRAL
```
┌─────────────────────────────────────────────────────────────────┐
│                        STATION EME 10 GHz                       │
└─────────────────────────────────────────────────────────────────┘
                               │
        ┌──────────────────────┼──────────────────────┐
        │                      │                      │
        ▼                      ▼                      ▼
┌───────────────┐      ┌──────────────┐      ┌──────────────┐
│   PARABOLE    │      │   RACK 19"   │      │     NUC      │
│   CAS90       │      │              │      │   (PC Linux) │
├───────────────┤      ├──────────────┤      ├──────────────┤
│               │      │              │      │              │
│ • Rotator SVH3│◄─────┤ Arduino Mega │◄─────┤ PstRotator   │
│   (Az + El)   │      │ Pro 2560     │  │   │ (Tracking)   │
│               │      │              │  │   │              │
│ • Encodeurs   │──────►│ • W5500     │──┘   │ Python App   │
│   HH12 (×2)   │      │   Ethernet   │◄─────┤ (Monitoring) │
│               │      │              │      │              │
│ • Tête RF :   │      │ • Nextion    │      │ WSJT-X       │
│   - Préampli  │      │   Display    │      │ (EME)        │
│   - PA 10W    │      │              │      │              │
│   - Transv    │      │ • Safety     │      └──────────────┘
│   - Relais    │      │   Monitoring │              │
│               │      │              │              │ USB/Ethernet
│ • Nano R4     │◄─────┤ • Serial2    │              │
│   (Monitoring)│ UART │   (Nano)     │              ▼
│               │      │              │      ┌──────────────┐
│               │      │ • Serial1    │      │  INTERNET    │
│               │      │   (Nextion)  │      │  (NTP, DX)   │
│               │      │              │      └──────────────┘
└───────────────┘      └──────────────┘
```

---

## ARCHITECTURE LOGICIELLE

### Niveaux d'abstraction
```
┌────────────────────────────────────────────────────────┐
│  NIVEAU 5 : APPLICATIONS UTILISATEUR                   │
│  • Python PyQt5 (Interface monitoring/contrôle)        │
│  • PstRotator (Tracking lune automatique)              │
│  • WSJT-X (Communications EME)                         │
└────────────────────────────────────────────────────────┘
                         │
                         │ TCP/IP (Ethernet)
                         ▼
┌────────────────────────────────────────────────────────┐
│  NIVEAU 4 : COUCHE RÉSEAU                              │
│  • Serveur Easycom (port 4533, PstRotator)            │
│  • Serveur Custom JSON (port 5000, Python)             │
│  • Client NTP (sync heure UTC)                         │
│  • Arbitrage contrôle (lock exclusif)                  │
└────────────────────────────────────────────────────────┘
                         │
                         │ Firmware Arduino Mega
                         ▼
┌────────────────────────────────────────────────────────┐
│  NIVEAU 3 : CONTRÔLE ROTATOR                           │
│  • PID Azimuth/Élévation                               │
│  • Asservissement position (goto, tracking)            │
│  • Safety monitoring temps réel                         │
│  • Emergency stop (multi-niveau)                        │
└────────────────────────────────────────────────────────┘
                         │
                         │ PWM + Interrupts + ADC
                         ▼
┌────────────────────────────────────────────────────────┐
│  NIVEAU 2 : DRIVERS HARDWARE                           │
│  • Driver moteurs (MC33926 dual)                       │
│  • Lecture encodeurs (interrupts quadrature)           │
│  • Monitoring courants/tensions/températures           │
│  • Fins de course (VMA452 optocoupleurs)               │
└────────────────────────────────────────────────────────┘
                         │
                         │ Électrique/Mécanique
                         ▼
┌────────────────────────────────────────────────────────┐
│  NIVEAU 1 : MATÉRIEL PHYSIQUE                          │
│  • Rotator SVH3 (moteurs DC 24V + encodeurs Hall)     │
│  • Parabole Kathrein CAS90                             │
│  • Équipement RF (PA, Préampli, Transverter)          │
└────────────────────────────────────────────────────────┘
```

---

## NIVEAU 1 : CONTRÔLEUR PRINCIPAL (MEGA)

### Arduino Mega Pro 2560

**Rôle** : Cerveau du système
- Contrôle moteurs rotator (PWM PID)
- Lecture encodeurs (position Az/El)
- Monitoring safety temps réel
- Serveur réseau multi-clients
- Interface Nextion locale
- Communication Nano R4 tête RF

**Fréquence boucle principale** : ~100 Hz (10ms period)

**Tâches prioritaires** :
1. **CRITIQUE** (chaque cycle) : Safety checks (50ms max)
2. **HAUTE** (10-20ms) : PID moteurs
3. **MOYENNE** (100ms) : Réseau (non-bloquant)
4. **BASSE** (1 Hz) : Télémétrie, affichage

---

## NIVEAU 2 : CONTRÔLE MOTEURS

### Architecture Contrôle
```
Position Target ──┐
                  │
Position Actuelle─┴──> [PID] ──> Commande PWM ──> MC33926 ──> Moteur
      ▲                                                          │
      │                                                          │
      └────────────────── Encodeur ◄──────────────────────────┘
```

### PID Controller

**Paramètres** (à tuner) :
```cpp
// Azimuth
float Kp_az = 2.0;   // Gain proportionnel
float Ki_az = 0.1;   // Gain intégral
float Kd_az = 0.5;   // Gain dérivé

// Élévation
float Kp_el = 2.0;
float Ki_el = 0.1;
float Kd_el = 0.5;
```

**Limites** :
- PWM max : ±200 (sur 255) → vitesse limitée
- Erreur tolérance : 0.2° → stop si dans fenêtre
- Anti-windup intégral : ±50 (saturation)

**Période update** : 20ms (50 Hz)

---

### État Machine Moteur
```
                    ┌─────────┐
                    │  IDLE   │
                    └────┬────┘
                         │ Commande goto/jog
                         ▼
                    ┌─────────┐
              ┌────►│ MOVING  │◄────┐
              │     └────┬────┘     │
              │          │          │
              │    Position atteinte│ Erreur trop grande
              │          │          │ (re-ajustement)
              │          ▼          │
              │     ┌─────────┐    │
              │     │SETTLING │────┘
              │     └────┬────┘
              │          │ Stable >1s
              │          ▼
              │     ┌─────────┐
              └─────┤TRACKING │
                    └────┬────┘
                         │ Stop/Emergency
                         ▼
                    ┌─────────┐
                    │ STOPPED │
                    └─────────┘
```

---

## NIVEAU 3 : MONITORING DISTRIBUÉ

### Arduino Nano R4 (Tête RF)

**Rôle** : Monitoring local près parabole
- ADC 14-bit haute précision
- Capteurs tensions (Préampli, PA, Relais, Transverter)
- Capteurs températures (DS18B20 PA, Transverter)
- Monitoring puissance PA (PA MON)
- Communication UART vers Mega (115200 baud)

**Avantages localisation tête RF** :
- Câbles capteurs courts (précision)
- Isolation électrique rack/RF
- Pas de bruit induit longues lignes
- ADC 14-bit (16× plus précis que Mega)

**Communication avec Mega** :

**Protocole UART bidirectionnel** :

**Mega → Nano** :
```
"REQ\n"  (polling 1 Hz)
```

**Nano → Mega** :
```
"V1:24.1,V2:13.7,V3:5.0,V4:12.0,T1:65.0,T2:42.0,P:10.2\n"
```

**Format CSV** :
- V1 : Tension préampli (V)
- V2 : Tension PA (V)
- V3 : Tension relais WR90 (V)
- V4 : Tension transverter (V)
- T1 : Température PA (°C)
- T2 : Température transverter (°C)
- P : PA MON puissance (mW ou V selon calibration)

---

## NIVEAU 4 : INTERFACES RÉSEAU

### Architecture Multi-Ports
```
┌─────────────────────────────────────────────────────┐
│                   Arduino Mega                      │
│                                                     │
│  ┌──────────────────────────────────────────────┐  │
│  │         W5500 Ethernet Controller            │  │
│  │                                               │  │
│  │  Socket 0 : Serveur Easycom (port 4533)     │  │
│  │             ▲                                 │  │
│  │             │ Commandes Az/El                │  │
│  │             │                                 │  │
│  │  Socket 1 : Serveur Custom (port 5000)      │  │
│  │             ▲                                 │  │
│  │             │ JSON (monitoring + contrôle)   │  │
│  │             │                                 │  │
│  │  Socket 2 : Client NTP (port 123)           │  │
│  │             ▲                                 │  │
│  │             │ Sync heure UTC                 │  │
│  └─────────────┼─────────────────────────────────┘  │
│                │                                     │
└────────────────┼─────────────────────────────────────┘
                 │
                 │ RJ45 Ethernet
                 ▼
         ┌───────────────┐
         │    Switch     │
         │   Gigabit     │
         └───────┬───────┘
                 │
        ┌────────┴────────┐
        │                 │
        ▼                 ▼
  ┌──────────┐      ┌──────────┐
  │   NUC    │      │  Router  │
  │PstRotator│      │Internet  │
  │Python App│      │   NTP    │
  └──────────┘      └──────────┘
```

---

### Protocole Easycom (Port 4533)

**Standard** : Compatible Ham Lib / PstRotator

**Commandes** :
```
AZ###.#     → Set azimuth (ex: AZ180.5)
EL##.#      → Set élévation (ex: EL45.0)
AZ          → Query azimuth
EL          → Query élévation
SA          → Stop azimuth
SE          → Stop élévation
```

**Réponses** :
```
+0###.#     → Azimuth (ex: +0180.5)
+0##.#      → Élévation (ex: +045.0)
```

**Format** : ASCII, terminé `\r\n`

**Exemple session** :
```
Client: AZ180.5\r\n
Mega:   (commence rotation)

Client: AZ\r\n
Mega:   +0175.3\r\n

Client: AZ\r\n
Mega:   +0180.5\r\n
```

---

### Protocole Custom JSON (Port 5000)

**Usage** : Python app (monitoring détaillé + contrôle manuel)

**Commandes** :

**1. Prise de contrôle** :
```json
{"cmd":"lock"}
```
Réponse :
```json
{"status":"locked","control":"python"}
```

**2. Libération contrôle** :
```json
{"cmd":"unlock"}
```

**3. Mouvement manuel** :
```json
{"cmd":"jog","axis":"az","dir":"cw"}
{"cmd":"jog","axis":"el","dir":"up"}
```

**4. Arrêt** :
```json
{"cmd":"stop","axis":"all"}
{"cmd":"stop","axis":"az"}
```

**5. Goto position** :
```json
{"cmd":"goto","az":180.5,"el":45.0}
```

**6. Query status** :
```json
{"cmd":"status"}
```

---

**Télémétrie (broadcast 10 Hz)** :

Mega envoie automatiquement (sans requête) :
```json
{
  "az": 180.5,
  "el": 45.0,
  "target_az": 182.0,
  "target_el": 46.5,
  "i_az": 1.45,
  "i_el": 1.52,
  "t_pa": 65.0,
  "t_amb": 22.0,
  "t_transv": 42.0,
  "v_24": 24.1,
  "v_13": 13.7,
  "v_12": 11.9,
  "v_5": 5.0,
  "v_preamp": 13.8,
  "v_pa": 24.0,
  "pa_mon": 10.2,
  "control": "pstrotator",
  "state": "tracking",
  "alerts": []
}
```

**Fréquence** : 10 Hz (100ms) → Python app reçoit flux continu

---

### Gestion Contrôle (Lock Exclusif)

**États système** :
```
IDLE              → Pas de client connecté
PSTROTATOR_ACTIVE → PstRotator a le contrôle
MANUAL_ACTIVE     → Python app a le contrôle (lock)
ERROR_STATE       → Emergency stop (reset requis)
```

**Règles arbitrage** :

1. **PstRotator connecté** → Contrôle par défaut
2. **Python demande lock** :
   - Si IDLE ou PSTROTATOR_ACTIVE → Accordé
   - Si MANUAL_ACTIVE (autre client) → Refusé
3. **Python a lock** :
   - Commandes Easycom ignorées (sauf query)
   - Timeout 60s inactivité → auto-release
4. **Emergency stop** → ERROR_STATE (tous clients bloqués)

**Exemple workflow** :
```
1. PstRotator se connecte (4533)
   → État : PSTROTATOR_ACTIVE
   → Tracking lune actif

2. Python app demande lock (5000)
   → État : MANUAL_ACTIVE
   → PstRotator commandes ignorées
   → Python peut jog/goto

3. Python unlock ou timeout 60s
   → État : PSTROTATOR_ACTIVE
   → PstRotator reprend contrôle

4. Emergency stop (surintensité)
   → État : ERROR_STATE
   → Tous moteurs stoppés
   → Nécessite reset (commande spéciale)
```

---

## NIVEAU 5 : AFFICHAGE LOCAL

### Nextion NX4832T035

**Rôle** : Affichage passif rapide (coup d'œil)

**Interface minimaliste** :
- Azimuth (°)
- Élévation (°)
- Status (OK / TRACKING / ALARM)

**Mode veille automatique** :
- Écran éteint après 15s inactivité
- Wake on touch (n'importe où)
- Affichage pendant 15s
- Retour veille

**Communication** :
- UART Serial1 (9600 baud)
- Commandes texte terminées 0xFF×3
- Update 1 Hz (pas besoin plus)

**Exemple commandes** :
```cpp
nextion("t_az.txt=\"180.5°\"");
nextion("t_el.txt=\"45.0°\"");
nextion("t_status.txt=\"TRACKING\"");
nextion("t_status.pco=2016");  // Vert
```

---

## NIVEAU 6 : SÉCURITÉ MULTI-NIVEAUX

### Architecture Safety
```
┌────────────────────────────────────────────────────┐
│  NIVEAU 4 : OPÉRATEUR                              │
│  • Observation visuelle                            │
│  • Interrupteur urgence physique (optionnel)       │
│  • Disjoncteur alim 24V                            │
└────────────────────────────────────────────────────┘
                        │
                        ▼
┌────────────────────────────────────────────────────┐
│  NIVEAU 3 : LOGICIEL PYTHON                        │
│  • Alarmes visuelles/sonores                       │
│  • Logs événements                                 │
│  • Bouton Emergency Stop UI                        │
│  • Timeout commandes                               │
└────────────────────────────────────────────────────┘
                        │
                        ▼
┌────────────────────────────────────────────────────┐
│  NIVEAU 2 : FIRMWARE MEGA                          │
│  • Monitoring courants temps réel (50ms)           │
│  • Seuils progressifs (normal→warning→emergency)   │
│  • Fins de course hardware (VMA452)                │
│  • Timeout mouvement (5s sans changement encodeur) │
│  • Watchdog timer                                  │
└────────────────────────────────────────────────────┘
                        │
                        ▼
┌────────────────────────────────────────────────────┐
│  NIVEAU 1 : HARDWARE MC33926                       │
│  • Protection thermique (shutdown 150°C)           │
│  • Protection court-circuit                        │
│  • Status Flag (SF pins)                           │
└────────────────────────────────────────────────────┘
```

---

### Seuils Sécurité

**Courants moteurs** :
- **Normal** : 0-1.8A → OK
- **Warning** : 1.8-2.2A → Log warning
- **Emergency** : >2.8A → Stop immédiat

**Températures** :
- **Normal** : <70°C → OK
- **Warning** : 70-85°C → Log warning
- **Emergency** : >85°C → Stop immédiat

**Tensions** :
- **24V** : 22-26V OK, <22V ou >26V → Warning
- **13.8V** : 12-15V OK
- **12V** : 11-13V OK

**Timeout mouvement** :
- Si PWM actif >5s SANS changement encodeur → Moteur bloqué → Stop

---

### Actions Emergency Stop

**Séquence complète** :

1. **PWM → 0** (arrêt commande)
2. **Disable drivers** (D1/D2/EN → OFF)
3. **Flag systemError** (état global)
4. **Log événement** (timestamp + cause)
5. **Alert réseau** (broadcast clients)
6. **Nextion réveil** (affichage ALARM rouge)
7. **Optionnel** : Buzzer/LED physique

**Recovery** :
- Opérateur doit vérifier cause
- Commande reset spéciale
- Re-enable système

---

## PERFORMANCES ATTENDUES

### Précision Pointage

**Résolution encodeurs** : 0.054°/count
**Précision mécanique** : ±0.2° (après tuning PID)
**Beamwidth 10 GHz** : ~1° (CAS90)
**Marge** : 0.2° = 20% du beamwidth ✅ Acceptable

**Drift thermique** : <0.05°/min (après stabilisation)

---

### Vitesse Tracking

**Vitesse rotator** : 0.048 rpm (21 min/360°)
**Vitesse lune** : ~0.5°/min (drift apparent)
**Tracking** : PID ajuste en continu → suivi précis

**Temps réponse** :
- Commande → début mouvement : <500ms
- Atteinte position (10°) : ~4 minutes
- Stabilisation finale : <10 secondes

---

### Latences Système

**Boucle PID** : 20ms (50 Hz)
**Safety checks** : 50ms (20 Hz)
**Télémétrie Python** : 100ms (10 Hz)
**Nextion update** : 1000ms (1 Hz)
**Polling Nano R4** : 1000ms (1 Hz)

**Latence totale commande réseau → mouvement moteur** : ~50ms

---

### Uptime & Fiabilité

**Objectif** : >24h continu sans redémarrage
**Watchdog** : Reset auto si freeze (8s timeout)
**Monitoring** : Logs erreurs persistants (EEPROM)
**Recovery** : Auto-retry connexion réseau

---

## EXTENSIONS FUTURES

### Court Terme

**Anémomètre** :
- Pin 31 (INT6)
- Seuil vent >60 km/h → Stop tracking + parking position

**Capteur pluie** :
- Pin digitale libre
- Détection → Parking automatique

**GPS PPS** :
- Pin 31 (interrupt)
- Sync précise (<1µs) pour mesures RF

---

### Moyen Terme

**Deuxième parabole test** :
- Nano R4 #2 sur Serial3 (pins 14/15)
- Protocole identique
- Télémétrie séparée

**Caméra IP** :
- Surveillance visuelle tracking
- Détection obstacles
- Enregistrement sessions

---

### Long Terme

**Station multi-bandes** :
- Rotators 10/24/47 GHz
- Architecture identique
- Mega = master, Nanos = slaves

**Système météo complet** :
- Température, humidité, pression
- Anémomètre, pluviomètre
- Intégration décisions tracking

---

## CALIBRATION & TUNING

### Encodeurs

**Offset initial** :
```cpp
// Mesurer position réelle avec inclinomètre/boussole
float az_offset = 0.0;   // Correction offset (°)
float el_offset = 0.0;

float azimuth_real = azimuth_raw + az_offset;
```

**Résolution** :
```cpp
// Vérifier 6624 counts = 360°
// Sinon ajuster facteur
float counts_per_degree = 6624.0 / 360.0;  // 18.4
```

---

### PID Tuning

**Méthode Ziegler-Nichols** :

1. **Ki = 0, Kd = 0**
2. **Augmenter Kp** jusqu'à oscillations soutenues
3. **Ku = Kp critique**, **Pu = période oscillations**
4. **Calculer** :
   - Kp = 0.6 × Ku
   - Ki = 2 × Kp / Pu
   - Kd = Kp × Pu / 8

**Ajustements fins** :
- Trop d'overshoot → Réduire Kp
- Oscillations → Réduire Kd
- Erreur statique → Augmenter Ki

---

### Seuils Courant

**Test moteurs charge réelle** :

1. **Goto Az 180°** (charge normale)
2. **Mesurer courant** pendant mouvement
3. **Seuil warning** = 1.2 × courant max mesuré
4. **Seuil emergency** = 1.5 × courant max mesuré

---

## DOCUMENTATION ASSOCIÉE

**Fichiers liés** :
- `HARDWARE_SPEC.md` : Composants détaillés
- `PINOUT.md` : Affectation pins complète
- `DUAL_MC33926.md` : Driver moteurs
- `SCHEMATIC_NOTES.md` : Schéma électrique
- `WIRING.md` : Câblage physique
- `TODO_PHASES.md` : Plan développement

---

**Version** : 1.0
**Date** : 2026-01-30
**Auteur** : ON7KGK - JO20BM85DP
**Projet** : EME 10 GHz Station Controller
```

