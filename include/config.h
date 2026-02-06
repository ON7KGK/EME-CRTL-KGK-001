// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Configuration centrale
// ════════════════════════════════════════════════════════════════
// Fichier: config.h
// Description: Toutes les définitions de configuration matérielle
//              et logicielle du système. Modifier ce fichier pour
//              adapter le contrôleur à différentes configurations.
// ════════════════════════════════════════════════════════════════
// Hardware: Arduino Mega Pro 2560
// Date: 2026-01-30
// Auteur: KGK
// ════════════════════════════════════════════════════════════════

#ifndef CONFIG_H
#define CONFIG_H

// ════════════════════════════════════════════════════════════════
// DÉFINITIONS DES TYPES D'ENCODEURS
// ════════════════════════════════════════════════════════════════

#define ENCODER_SSI_ABSOLUTE  1  // HH-12 SSI (encodeur absolu 12-bit)
#define ENCODER_SSI_INC       2  // HH-12 INC SSI (encodeur incrémental avec tracking tours)
#define ENCODER_HALL_SVH3     3  // Encodeur Hall intégré SVH3 (futur)
#define ENCODER_POT_1T        4  // Potentiomètre 1 tour (0-360°, mapping simple ADC)
#define ENCODER_POT_MT        5  // Potentiomètre multi-tours (tracking tours comme SSI_INC)

// Configuration des types d'encodeurs utilisés
#define ENCODER_AZ_TYPE  ENCODER_POT_MT        // Azimuth: potentiomètre multi-tours
#define ENCODER_EL_TYPE  ENCODER_POT_MT  // Élévation: encodeur absolu

// ════════════════════════════════════════════════════════════════
// DÉFINITIONS DES TYPES DE MOTEURS
// ════════════════════════════════════════════════════════════════

#define MOTOR_STEPPER      1  // Moteur pas-à-pas NEMA23 + driver TB6600
#define MOTOR_DC_BRUSHED   2  // Moteur DC brushed + driver MC33926 (futur SVH3)

// Configuration des types de moteurs utilisés
#define MOTOR_AZ_TYPE  MOTOR_STEPPER  // Azimuth: moteur pas-à-pas
#define MOTOR_EL_TYPE  MOTOR_STEPPER  // Élévation: moteur pas-à-pas

// ════════════════════════════════════════════════════════════════
// MODE NANO STEPPER (Arduino Nano dédié aux moteurs)
// ════════════════════════════════════════════════════════════════
// Si activé, le Mega envoie les commandes à un Arduino Nano via UART
// Le Nano gère les steps avec AccelStepper pour un mouvement fluide
// 0 = Contrôle direct par le Mega (motor_stepper.cpp)
// 1 = Contrôle via Nano (motor_nano.cpp)

#define USE_NANO_STEPPER    1

// Port série pour communication avec le Nano
// Serial2 = pins 16 (TX2) et 17 (RX2) sur Mega
// Serial3 = pins 14 (TX3) et 15 (RX3) sur Mega
#define NANO_SERIAL         Serial2
#define NANO_BAUD           115200

// Pin statut communication Nano (LED ou relais)
// HIGH = communication OK, LOW = communication perdue
#define NANO_STATUS_PIN     22
#define NANO_TIMEOUT_MS     2000  // Timeout 2 secondes sans réponse = communication perdue

// ════════════════════════════════════════════════════════════════
// PINS ENCODEURS SSI (Synchronous Serial Interface)
// ════════════════════════════════════════════════════════════════
// Protocole SSI: CLK commune, CS et DATA individuels par encodeur

#define SSI_CLK       10   // Clock commune pour tous encodeurs SSI

// Encodeur Azimuth
#define SSI_CS_AZ     A0   // Chip Select azimuth
#define SSI_DATA_AZ   11   // Data azimuth

// Encodeur Élévation
#define SSI_CS_EL     A1   // Chip Select élévation
#define SSI_DATA_EL   A3   // Data élévation

// ════════════════════════════════════════════════════════════════
// PINS POTENTIOMÈTRE ANALOGIQUE (Simple 1 tour OU multi-tours)
// ════════════════════════════════════════════════════════════════
// Potentiomètre 0-360° (mapping direct ADC 0-1023 → 0-360°)
// Compatible POT_1T (1 tour) et POT_MT (multi-tours avec tracking)

#define POT_PIN_AZ    A14  // Lecture analogique azimuth (ADC 0-1023)
#define POT_PIN_EL    A15  // Lecture analogique élévation (ADC 0-1023)

// ════════════════════════════════════════════════════════════════
// PINS MOTEURS PAS-À-PAS (TB6600 Drivers)
// ════════════════════════════════════════════════════════════════
// Signal: STEP (impulsions), DIR (direction)
// ENABLE peut être forcé à 5V (toujours actif)

// Driver TB6600 Azimuth
#define AZ_STEP       9    // Impulsions step azimuth
#define AZ_DIR        8    // Direction azimuth (HIGH=CW, LOW=CCW selon câblage)

// Driver TB6600 Élévation
#define EL_STEP       13   // Impulsions step élévation
#define EL_DIR        A2   // Direction élévation (HIGH=UP, LOW=DOWN selon câblage)

// ════════════════════════════════════════════════════════════════
// PINS MOTEURS DC BRUSHED (MC33926 - FUTUR SVH3)
// ════════════════════════════════════════════════════════════════
// Driver MC33926 en mode sign-magnitude (IN1=PWM, IN2=direction)
// Voir docs/DUAL_MC33926.md pour détails complets

// Motor 1 (Azimuth DC)
#define M1_IN1        11   // PWM azimuth (pin PWM capable)
#define M1_IN2        12   // Direction azimuth
#define M1_D2         A4   // Disable M1 (LOW=enable, HIGH=disable)
#define M1_SF         A5   // Status Flag M1 (LOW=fault)
#define M1_FB         A6   // Current feedback M1 (lecture analogique)

// Motor 2 (Élévation DC)
#define M2_IN1        3    // PWM élévation (pin PWM capable)
#define M2_IN2        2    // Direction élévation
#define M2_D2         4    // Disable M2 (LOW=enable, HIGH=disable)
#define M2_SF         A7   // Status Flag M2 (LOW=fault)
#define M2_FB         A8   // Current feedback M2 (lecture analogique)

// ════════════════════════════════════════════════════════════════
// PINS BOUTONS MANUELS (Contrôle local précis)
// ════════════════════════════════════════════════════════════════
// Boutons NO (Normally Open) avec pull-ups internes
// Appui = LOW, Relâché = HIGH

#define BTN_CW        4    // Rotation azimuth CW (sens horaire)
#define BTN_CCW       5    // Rotation azimuth CCW (sens anti-horaire)
#define BTN_UP        6    // Rotation élévation UP (montée)
#define BTN_DOWN      7    // Rotation élévation DOWN (descente)
#define BTN_STOP      12   // Arrêt immédiat (annule targetAz/El)

// ════════════════════════════════════════════════════════════════
// PINS FINS DE COURSE (Sécurité NC - Normally Closed)
// ════════════════════════════════════════════════════════════════
// Chaque LIMIT = série de 2 switches NC (CW_NC et CCW_NC)
// État normal (aucun switch pressé): pin = HIGH
// État alarme (1 switch ouvert): pin = LOW → BLOCAGE MOUVEMENT

#define LIMIT_AZ      A10   // Fin de course azimuth (série NC) - pins 2/3 causaient blocage
#define LIMIT_EL      A11   // Fin de course élévation (série NC)

// ════════════════════════════════════════════════════════════════
// PINS ETHERNET W5500 (Module Mini W5500)
// ════════════════════════════════════════════════════════════════
// Connexion SPI: MISO(50), MOSI(51), SCK(52) pins hardware Mega
// CS et RST configurables

#define W5500_CS      53   // Chip Select W5500 (SS par défaut Mega)
#define W5500_RST     49   // Reset W5500 (optionnel, ou pull-up 10kΩ vers 3.3V)

// MISO, MOSI, SCK utilisent les pins hardware SPI du Mega:
// MISO = pin 50
// MOSI = pin 51
// SCK  = pin 52

// ════════════════════════════════════════════════════════════════
// CONFIGURATION MÉCANIQUE (Réductions et caractéristiques)
// ════════════════════════════════════════════════════════════════

// ─────────────────────────────────────────────────────────────────
// RAPPORTS DE RÉDUCTION CAPTEUR POSITION
// ─────────────────────────────────────────────────────────────────
// GEAR_RATIO = Nombre de tours CAPTEUR pour 1 tour SORTIE (antenne)
//
// S'applique à TOUS les types de capteurs (SSI, POT, etc.)
//
// Exemple: Si réducteur 5:1 entre moteur et sortie, et capteur sur moteur:
//   - Quand l'antenne fait 1 tour (360°), le capteur fait 5 tours
//   - GEAR_RATIO = 5.0
//   - angle_sortie = angle_capteur / GEAR_RATIO
//
// Si capteur directement sur la sortie (sans réduction):
//   - GEAR_RATIO = 1.0
//
// Potentiomètre multi-tours (ENCODER_POT_MT):
//   - Exemple: 10 tours pot = 1 tour antenne → GEAR_RATIO_AZ = 10.0
//   - Résolution finale = 0.35°/count × 10 = 0.035°/count
//
#define GEAR_RATIO_AZ        10   // Tours capteur pour 1 tour antenne Az
#define GEAR_RATIO_EL        10    // Tours capteur pour 1 tour antenne El

// ─────────────────────────────────────────────────────────────────
// CARACTÉRISTIQUES MOTEURS PAS-À-PAS
// ─────────────────────────────────────────────────────────────────
#define STEPS_PER_REV_MOTOR  1600  // Steps par tour moteur (1.8° par step, 1/8 microstepping = 1600)
                                    // TB6600 configuré en microstepping 1/8

// ─────────────────────────────────────────────────────────────────
// RÉSOLUTION ENCODEURS SSI
// ─────────────────────────────────────────────────────────────────
#define SSI_COUNTS_PER_REV   4096  // 12-bit encodeur = 4096 counts par tour

// ─────────────────────────────────────────────────────────────────
// CONFIGURATION POTENTIOMÈTRE 1 TOUR (Simple)
// ─────────────────────────────────────────────────────────────────
// Mapping direct ADC → degrés (pas de multi-tours, pas de tracking)
// Résolution ADC 10-bit: 0-1023 counts → 0-360°
// Résolution angulaire: 360° / 1024 = 0.35° par count ADC

#define POT_ADC_RESOLUTION   1024  // Résolution ADC 10-bit Arduino (0-1023)

// Filtrage ADC (moyenne glissante pour stabiliser affichage)
// Plus le nombre d'échantillons est élevé, plus le filtrage est fort
// mais plus la réponse est lente
#define POT_SAMPLES_AZ      16     // Nombre d'échantillons azimuth (plus filtré)
#define POT_SAMPLES_EL       8     // Nombre d'échantillons élévation (4-16 typique)

// ════════════════════════════════════════════════════════════════
// VITESSES MOTEURS PAS-À-PAS (Délais en microsecondes)
// ════════════════════════════════════════════════════════════════
// Plus le délai est petit, plus la vitesse est élevée
// Attention: vitesse trop élevée = perte de pas (TB6600 minimum ~500µs)
// Valeurs testées et validées avec TB6600 + NEMA23

#define SPEED_FAST    50     // Vitesse rapide (µs par phase) - Loin de cible (>3°)
#define SPEED_SLOW    200    // Vitesse lente (µs par phase) - Approche finale précise (<3°)

// ════════════════════════════════════════════════════════════════
// PARAMÈTRES ASSERVISSEMENT POSITION
// ════════════════════════════════════════════════════════════════

// Seuils de précision (degrés)
#define POSITION_TOLERANCE     1   // Tolérance position atteinte (0.08°)
#define SPEED_SWITCH_THRESHOLD 3.0    // Erreur pour switch vitesse rapide→lente (3.0°)
#define MICRO_MOVEMENT_FILTER  0.20   // Filtre anti-vibration PstRotator (0.15°)
                                       // Commandes < 0.15° sont ignorées

// ════════════════════════════════════════════════════════════════
// MOUVEMENT MANUEL (Boutons locaux)
// ════════════════════════════════════════════════════════════════

#define MANUAL_STEP_SIZE  20   // Nombre de steps par appui bouton (ajustement fin)

// ════════════════════════════════════════════════════════════════
// CONFIGURATION COMMUNICATION
// ════════════════════════════════════════════════════════════════

// Mode de communication Easycom
// 0 = Serial USB (PstRotator via port COM)
// 1 = Ethernet W5500 (PstRotator via TCP/IP)
#define USE_ETHERNET        0   // 0=USB Serial, 1=Ethernet W5500

// Configuration Serial (si USE_ETHERNET=0)
#define SERIAL_BAUD         9600   // Vitesse Serial pour Easycom (9600 standard)

// ════════════════════════════════════════════════════════════════
// CONFIGURATION RÉSEAU (W5500 Ethernet) - si USE_ETHERNET=1
// ════════════════════════════════════════════════════════════════

// Adresse MAC (unique par device - modifier si plusieurs contrôleurs)
#define MAC_ADDR_0    0xDE
#define MAC_ADDR_1    0xAD
#define MAC_ADDR_2    0xBE
#define MAC_ADDR_3    0xEF
#define MAC_ADDR_4    0xFE
#define MAC_ADDR_5    0xED

// Configuration IP statique (192.168.1.177)
#define IP_OCTET_1    192
#define IP_OCTET_2    168
#define IP_OCTET_3    1
#define IP_OCTET_4    177   // Modifier selon votre réseau

// Gateway et subnet (standard réseau local)
#define GW_OCTET_1    192
#define GW_OCTET_2    168
#define GW_OCTET_3    1
#define GW_OCTET_4    1     // Gateway (typiquement routeur)

#define SUBNET_OCTET_1  255
#define SUBNET_OCTET_2  255
#define SUBNET_OCTET_3  255
#define SUBNET_OCTET_4  0

// Port Easycom (standard ham radio tracking)
#define EASYCOM_PORT  4533  // Port TCP pour PstRotator

// ════════════════════════════════════════════════════════════════
// ADRESSES EEPROM (Sauvegarde calibration)
// ════════════════════════════════════════════════════════════════
// EEPROM ATmega2560 = 4096 bytes
// Structure: [turnsAz(4)][offsetAz(4)][offsetEl(4)][...réservé]

#define EEPROM_TURNS_AZ    0   // Nombre de tours azimuth (long, 4 bytes)
#define EEPROM_TURNS_EL    4   // Nombre de tours élévation (long, 4 bytes)
#define EEPROM_OFFSET_AZ   8   // Offset calibration azimuth (long, 4 bytes)
#define EEPROM_OFFSET_EL   16  // Offset calibration élévation (long, 4 bytes)

// Adresses réservées pour futures extensions
#define EEPROM_RESERVED_1  24
#define EEPROM_RESERVED_2  32

// ════════════════════════════════════════════════════════════════
// INVERSION SENS ENCODEURS (SSI et Potentiomètres)
// ════════════════════════════════════════════════════════════════
// Inverser si le sens physique ne correspond pas aux commandes
// S'applique à tous les types d'encodeurs: SSI, POT_1T, POT_MT

#define REVERSE_AZ    true  // Inverser sens azimuth (true/false)
#define REVERSE_EL    true  // Inverser sens élévation (true/false)

// ════════════════════════════════════════════════════════════════
// MODULES ACTIFS (Switches pour tests progressifs)
// ════════════════════════════════════════════════════════════════
// Activer/désactiver chaque module indépendamment durant développement
// 1 = actif, 0 = désactivé

#define TEST_ENCODERS       1  // ÉTAPE 1: Test lecture encodeurs SSI
#define TEST_MOTORS         1  // ÉTAPE 2: Test contrôle moteurs
#define TEST_BUTTONS        1  // ÉTAPE 3: Test boutons manuels
#define TEST_LIMITS         1  // ÉTAPE 4: Test fins de course
#define TEST_NETWORK        1  // ÉTAPE 5: Test Ethernet + Easycom
#define TEST_CALIBRATION    1  // ÉTAPE 6: Test calibration EEPROM

// ════════════════════════════════════════════════════════════════
// CONFIGURATION DEBUG
// ════════════════════════════════════════════════════════════════
// IMPORTANT: Quand USE_ETHERNET=0, le debug est DÉSACTIVÉ car
// Serial USB est utilisé pour le protocole Easycom.
// Quand USE_ETHERNET=1, Serial USB est libre pour le debug.
// ════════════════════════════════════════════════════════════════

#if USE_ETHERNET == 1
    // ─────────────────────────────────────────────────────────────
    // MODE ETHERNET: Debug activé sur Serial USB
    // ─────────────────────────────────────────────────────────────
    #define DEBUG_SERIAL        1  // Activer messages debug Serial (1=ON, 0=OFF)
    #define DEBUG_VERBOSE       0  // Debug verbeux (affiche chaque step) - RALENTIT CODE

    // Encodeurs HH-12
    #define DEBUG_ENCODER_RAW   1  // Affiche valeurs brutes encodeurs (0-4095)
    #define DEBUG_ENCODER_DEG   1  // Affiche position en degrés (Az/El réels)

    // Protocole Easycom
    #define DEBUG_EASYCOM_RX    1  // Affiche commandes reçues de PstRotator
    #define DEBUG_EASYCOM_TX    1  // Affiche réponses envoyées à PstRotator
    #define DEBUG_EASYCOM_PARSE 0  // Affiche détails parsing (verbeux)

    // Réseau
    #define DEBUG_NETWORK       1  // Affiche connexions/déconnexions clients

    // Moteurs
    #define DEBUG_MOTOR_CMD     1  // Affiche commandes moteur (GOTO, STOP)
    #define DEBUG_MOTOR_STEP    0  // Affiche chaque step (TRÈS verbeux)

    // Nextion
    #define DEBUG_NEXTION       0  // Affiche mise à jour Nextion (résumé)
    #define DEBUG_NEXTION_VERBOSE 0  // Affiche chaque commande Nextion (TRÈS verbeux)

#else
    // ─────────────────────────────────────────────────────────────
    // MODE SERIAL USB: Debug DÉSACTIVÉ (Serial = Easycom)
    // ─────────────────────────────────────────────────────────────
    #define DEBUG_SERIAL        0
    #define DEBUG_VERBOSE       0
    #define DEBUG_ENCODER_RAW   0
    #define DEBUG_ENCODER_DEG   0
    #define DEBUG_EASYCOM_RX    0
    #define DEBUG_EASYCOM_TX    0
    #define DEBUG_EASYCOM_PARSE 0
    #define DEBUG_NETWORK       0
    #define DEBUG_MOTOR_CMD     0
    #define DEBUG_MOTOR_STEP    0
    #define DEBUG_NEXTION       0
    #define DEBUG_NEXTION_VERBOSE 0
#endif

// Intervalle affichage debug (millisecondes) - utilisé si debug actif
#define DEBUG_INTERVAL      500  // Affiche position toutes les 500ms

// ════════════════════════════════════════════════════════════════
// TIMINGS SYSTÈME (Millisecondes)
// ════════════════════════════════════════════════════════════════

#define ENCODER_READ_INTERVAL    20    // Lecture encodeurs toutes les 20ms
                                       // IMPORTANT: Si trop lent, perte de tours lors rotation rapide
                                       // 20ms = max ~25 rev/sec avant perte wraparound
#define NETWORK_POLL_INTERVAL    10    // Poll Ethernet toutes les 10ms
#define BUTTON_DEBOUNCE_DELAY    50    // Debounce boutons 50ms

// ════════════════════════════════════════════════════════════════
// PARAMÈTRES PID (Futur moteurs DC brushed MC33926)
// ════════════════════════════════════════════════════════════════
// Non utilisés pour moteurs stepper, réservés pour SVH3

#define PID_KP_AZ    1.0   // Proportional gain azimuth
#define PID_KI_AZ    0.0   // Integral gain azimuth
#define PID_KD_AZ    0.0   // Derivative gain azimuth

#define PID_KP_EL    1.0   // Proportional gain élévation
#define PID_KI_EL    0.0   // Integral gain élévation
#define PID_KD_EL    0.0   // Derivative gain élévation

// Limites PWM (0-255)
#define PWM_MIN      0     // PWM minimum (arrêt)
#define PWM_MAX      255   // PWM maximum (pleine vitesse)

// ════════════════════════════════════════════════════════════════
// CONFIGURATION NEXTION DISPLAY (Affichage tactile optionnel)
// ════════════════════════════════════════════════════════════════
// Écran Nextion pour affichage positions Az/El (actuelle et cible)
// Communication UART avec terminateurs 0xFF 0xFF 0xFF
// ════════════════════════════════════════════════════════════════

#define ENABLE_NEXTION      1           // Activer affichage Nextion (1=ON, 0=OFF)
#define TEST_NEXTION        1           // Test Nextion (nécessite ENABLE_NEXTION=1)

// ─────────────────────────────────────────────────────────────────
// PORT SÉRIE NEXTION
// ─────────────────────────────────────────────────────────────────
// Arduino Mega 2560 dispose de 4 ports série:
//   Serial  : USB (utilisé pour debug ou Easycom selon USE_ETHERNET)
//   Serial1 : pins 18 (TX1) et 19 (RX1) - RECOMMANDÉ pour Nextion
//   Serial2 : pins 16 (TX2) et 17 (RX2)
//   Serial3 : pins 14 (TX3) et 15 (RX3)
//
// IMPORTANT: Seul TX (18/16/14) est connecté au Nextion RX
//            RX Nextion (19/17/15) peut rester déconnecté si pas de touch events

#define NEXTION_SERIAL      Serial1     // Port série Nextion (Serial1 = pins 18/19)
#define NEXTION_BAUD        9600        // Vitesse communication Nextion
                                        // Valeurs standards: 9600, 115200
                                        // DOIT correspondre au réglage écran Nextion!

// ─────────────────────────────────────────────────────────────────
// TIMING MISE À JOUR NEXTION
// ─────────────────────────────────────────────────────────────────
#define NEXTION_UPDATE_INTERVAL  500    // Intervalle mise à jour affichage (ms)
                                        // 500ms = rafraîchissement 2× par seconde
                                        // Ne pas descendre sous 100ms (surcharge)

// ─────────────────────────────────────────────────────────────────
// CONTRÔLE MANUEL TACTILE (Boutons Nextion)
// ─────────────────────────────────────────────────────────────────
// Incréments utilisés lors de l'appui sur les boutons CW, CCW, UP, DOWN
// Les boutons envoient des commandes Easycom incrémentales tant qu'ils
// sont maintenus enfoncés (position cible = position actuelle + incrément)
//
// Recommandations:
//   - Moteurs lents EME: 5.0° à 10.0° pour azimuth
//   - Moteurs lents EME: 2.0° à 5.0° pour élévation
//   - Ajuster selon vitesse moteurs (21 min/360° = 0.29°/sec)

#define MANUAL_INCREMENT_AZ  5.0        // Incrément manuel azimuth (degrés, 1 décimale)
#define MANUAL_INCREMENT_EL  2.0        // Incrément manuel élévation (degrés, 1 décimale)

// ─────────────────────────────────────────────────────────────────
// NOMS COMPOSANTS NEXTION (personnalisable selon votre IHM)
// ─────────────────────────────────────────────────────────────────
// Ces noms doivent correspondre aux composants texte dans votre
// fichier .HMI Nextion Editor.
//
// Composants texte attendus (sur page 0):
//   tTitle    : Titre écran (ex: "EME ROTATOR")
//   tAzCur    : Position azimuth actuelle (ex: "123.5°")
//   tAzTgt    : Position azimuth cible (ex: "180.0°" ou "---")
//   tElCur    : Position élévation actuelle (ex: "45.2°")
//   tElTgt    : Position élévation cible (ex: "30.0°" ou "---")
//   tStatus   : État système (ex: "ARRETE", "EN MOUVEMENT", "ERREUR")
//
// Si vos composants ont des noms différents, modifiez nextion.cpp

// ════════════════════════════════════════════════════════════════
// FIN CONFIGURATION
// ════════════════════════════════════════════════════════════════

#endif // CONFIG_H
