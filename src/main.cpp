// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Programme principal
// ════════════════════════════════════════════════════════════════
// Fichier: main.cpp
// Description: Point d'entrée application - Architecture modulaire
//              Tests progressifs via switches TEST_xxx
// ════════════════════════════════════════════════════════════════
// Hardware: Arduino Mega Pro 2560
// Contrôlé par: PstRotator (tracking lunaire via Easycom/Ethernet)
// ════════════════════════════════════════════════════════════════

#include <Arduino.h>
#include "config.h"

// ════════════════════════════════════════════════════════════════
// INCLUDES MODULES CONDITIONNELS (Selon config.h)
// ════════════════════════════════════════════════════════════════

#if TEST_ENCODERS
  #include "encoder_ssi.h"
#endif

#if TEST_MOTORS
  #if USE_NANO_STEPPER
    // Mode Nano: communication UART avec Arduino Nano dédié
    #include "motor_nano.h"
  #else
    // Mode direct: contrôle moteurs par le Mega
    #if (MOTOR_AZ_TYPE == MOTOR_STEPPER || MOTOR_EL_TYPE == MOTOR_STEPPER)
      #include "motor_stepper.h"
    #endif
  #endif

  #if (MOTOR_AZ_TYPE == MOTOR_DC_BRUSHED || MOTOR_EL_TYPE == MOTOR_DC_BRUSHED)
    #include "motor_dc.h"
  #endif
#endif

#if TEST_LIMITS
  #include "safety.h"
#endif

#if TEST_NETWORK
  #include "network.h"
  #include "easycom.h"
#endif

#if ENABLE_NEXTION
  #include "nextion.h"
#endif

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES TIMING
// ════════════════════════════════════════════════════════════════

unsigned long lastDebugTime = 0;

// ════════════════════════════════════════════════════════════════
// SETUP - Initialisation système
// ════════════════════════════════════════════════════════════════

void setup() {
    // ─────────────────────────────────────────────────────────────
    // INITIALISATION SÉRIE
    // ─────────────────────────────────────────────────────────────
    // Toujours initialiser Serial si:
    // - USE_ETHERNET=0 (Easycom via USB)
    // - ou DEBUG_SERIAL=1 (debug actif)

    #if (USE_ETHERNET == 0) || DEBUG_SERIAL
        Serial.begin(SERIAL_BAUD);
        while (!Serial && millis() < 3000);  // Attente max 3s pour Serial Monitor
    #endif

    #if DEBUG_SERIAL
        Serial.println(F(""));
        Serial.println(F("════════════════════════════════════════════════════════════════"));
        Serial.println(F("    EME ROTATOR CONTROLLER v1.0"));
        Serial.println(F("════════════════════════════════════════════════════════════════"));
        Serial.println(F("Hardware: Arduino Mega Pro 2560"));
        Serial.println(F("Author: KGK"));
        Serial.println(F("Date: 2026-01-30"));
        Serial.println(F("════════════════════════════════════════════════════════════════"));
        Serial.println(F(""));

        // ─────────────────────────────────────────────────────────────
        // AFFICHAGE CONFIGURATION ACTIVE
        // ─────────────────────────────────────────────────────────────
        Serial.println(F("MODULES ACTIFS:"));
        Serial.print(F("  - TEST_ENCODERS:    ")); Serial.println(TEST_ENCODERS ? F("ON") : F("OFF"));
        Serial.print(F("  - TEST_MOTORS:      ")); Serial.println(TEST_MOTORS ? F("ON") : F("OFF"));
        Serial.print(F("  - TEST_BUTTONS:     ")); Serial.println(TEST_BUTTONS ? F("ON") : F("OFF"));
        Serial.print(F("  - TEST_LIMITS:      ")); Serial.println(TEST_LIMITS ? F("ON") : F("OFF"));
        Serial.print(F("  - TEST_NETWORK:     ")); Serial.println(TEST_NETWORK ? F("ON") : F("OFF"));
        Serial.print(F("  - TEST_CALIBRATION: ")); Serial.println(TEST_CALIBRATION ? F("ON") : F("OFF"));
        Serial.print(F("  - ENABLE_NEXTION:   ")); Serial.println(ENABLE_NEXTION ? F("ON") : F("OFF"));
        Serial.println(F(""));
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 1 : ENCODEURS SSI
    // ─────────────────────────────────────────────────────────────

    #if TEST_ENCODERS
        setupEncoders();
        delay(100);
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 2 : MOTEURS (Nano, Stepper direct ou DC selon config)
    // ─────────────────────────────────────────────────────────────

    #if TEST_MOTORS
        #if USE_NANO_STEPPER
            // Mode Nano: initialiser communication UART
            setupMotorNano();
            delay(100);
        #else
            #if (MOTOR_AZ_TYPE == MOTOR_STEPPER || MOTOR_EL_TYPE == MOTOR_STEPPER)
                setupMotors();
                delay(100);
            #endif
        #endif

        #if (MOTOR_AZ_TYPE == MOTOR_DC_BRUSHED || MOTOR_EL_TYPE == MOTOR_DC_BRUSHED)
            setupMotorsDC();
            delay(100);
        #endif
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 4 : FINS DE COURSE (Sécurité NC)
    // ─────────────────────────────────────────────────────────────

    #if TEST_LIMITS
        setupLimits();
        delay(100);
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 5 : RÉSEAU ETHERNET W5500
    // ─────────────────────────────────────────────────────────────

    #if TEST_NETWORK
        bool netOk = setupNetwork();
        if (!netOk) {
            #if DEBUG_SERIAL
                Serial.println(F(""));
                Serial.println(F("!!! ERREUR: Initialisation réseau échouée !!!"));
                Serial.println(F("!!! Vérifier W5500 et câble Ethernet !!!"));
                Serial.println(F(""));
            #endif
            // Continuer sans réseau (contrôle série USB possible)
        }
        delay(500);
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 6 : AFFICHAGE NEXTION (Optionnel)
    // ─────────────────────────────────────────────────────────────

    #if ENABLE_NEXTION && TEST_NEXTION
        setupNextion();
        delay(100);
    #endif

    // ─────────────────────────────────────────────────────────────
    // FIN SETUP
    // ─────────────────────────────────────────────────────────────

    #if DEBUG_SERIAL
        Serial.println(F("════════════════════════════════════════════════════════════════"));
        Serial.println(F("    INITIALISATION COMPLÈTE - SYSTÈME PRÊT"));
        Serial.println(F("════════════════════════════════════════════════════════════════"));
        Serial.println(F(""));
    #endif
}

// ════════════════════════════════════════════════════════════════
// LOOP - Boucle principale
// ════════════════════════════════════════════════════════════════

void loop() {
    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 1 : LECTURE ENCODEURS (toutes les 150ms)
    // ─────────────────────────────────────────────────────────────

    #if TEST_ENCODERS
        updateEncoders();
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 3 : BOUTONS MANUELS (CW/CCW/UP/DOWN/STOP)
    // ─────────────────────────────────────────────────────────────

    #if TEST_BUTTONS && TEST_MOTORS && !USE_NANO_STEPPER
        // Boutons manuels uniquement en mode direct (pas en mode Nano)
        #if (MOTOR_AZ_TYPE == MOTOR_STEPPER || MOTOR_EL_TYPE == MOTOR_STEPPER)
            checkManualButtons();
        #endif
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 4 : VÉRIFICATION FINS DE COURSE (Avant chaque mouvement)
    // ─────────────────────────────────────────────────────────────

    #if TEST_LIMITS
        checkLimits();
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 2 : ASSERVISSEMENT MOTEURS
    // ─────────────────────────────────────────────────────────────

    #if TEST_MOTORS
        #if USE_NANO_STEPPER
            // Mode Nano: le Nano gère les fins de course localement
            // On envoie juste les commandes et on lit les réponses
            updateMotorNano();
        #else
            // Mode direct: vérifier sécurité avant mouvement
            #if TEST_LIMITS
                bool azSafe = isAzimuthSafe();
                bool elSafe = isElevationSafe();
            #else
                bool azSafe = true;
                bool elSafe = true;
            #endif

            // Asservissement stepper direct
            #if (MOTOR_AZ_TYPE == MOTOR_STEPPER || MOTOR_EL_TYPE == MOTOR_STEPPER)
                if (azSafe && elSafe) {
                    updateMotorControl();
                } else {
                    if (!azSafe) {
                        extern float targetAz;
                        targetAz = -1.0;
                    }
                    if (!elSafe) {
                        extern float targetEl;
                        targetEl = -1.0;
                    }
                }
            #endif
        #endif

        // Asservissement DC
        #if (MOTOR_AZ_TYPE == MOTOR_DC_BRUSHED || MOTOR_EL_TYPE == MOTOR_DC_BRUSHED)
            #if TEST_LIMITS
                bool azSafeDC = isAzimuthSafe();
                bool elSafeDC = isElevationSafe();
            #else
                bool azSafeDC = true;
                bool elSafeDC = true;
            #endif
            if (azSafeDC && elSafeDC) {
                updateMotorControlDC();
            } else {
                if (!azSafeDC) stopMotorDC(1);
                if (!elSafeDC) stopMotorDC(2);
            }
        #endif
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 5 : GESTION RÉSEAU ETHERNET (Polling clients)
    // ─────────────────────────────────────────────────────────────

    #if TEST_NETWORK
        handleNetwork();
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉTAPE 6 : AFFICHAGE NEXTION ET CONTRÔLE TACTILE
    // ─────────────────────────────────────────────────────────────

    #if ENABLE_NEXTION && TEST_NEXTION
        // Lecture événements tactiles (boutons CW, CCW, UP, DOWN, STOP)
        readNextionTouch();

        // Gestion calibration par appui long (3 sec sur tAzCur/tElCur)
        handleCalibrationTouch();

        // Gestion boutons → envoi commandes Easycom incrémentales
        handleNextionButtons();

        // Mise à jour affichage positions (Az/El actuelle et cible)
        updateNextion();

        // Mise à jour indicateurs état (direction moteurs, mode)
        updateNextionIndicators();
    #endif

    // ─────────────────────────────────────────────────────────────
    // DEBUG PÉRIODIQUE (Toutes les DEBUG_INTERVAL ms)
    // ─────────────────────────────────────────────────────────────

    #if DEBUG_SERIAL && DEBUG_VERBOSE
        unsigned long currentTime = millis();
        if (currentTime - lastDebugTime >= DEBUG_INTERVAL) {
            lastDebugTime = currentTime;

            Serial.println(F("─────────────────────────────────────────"));

            #if TEST_ENCODERS
                printEncoderDebug();
            #endif

            #if TEST_MOTORS
                #if USE_NANO_STEPPER
                    printMotorNanoDebug();
                #else
                    #if (MOTOR_AZ_TYPE == MOTOR_STEPPER || MOTOR_EL_TYPE == MOTOR_STEPPER)
                        printMotorDebug();
                    #endif
                #endif
                #if (MOTOR_AZ_TYPE == MOTOR_DC_BRUSHED || MOTOR_EL_TYPE == MOTOR_DC_BRUSHED)
                    printMotorDCDebug();
                #endif
            #endif

            #if TEST_LIMITS
                printSafetyDebug();
            #endif

            #if TEST_NETWORK
                printNetworkDebug();
            #endif

            Serial.println(F("─────────────────────────────────────────"));
        }
    #endif

    // ─────────────────────────────────────────────────────────────
    // YIELD (Optionnel, pour compatibilité future RTOS)
    // ─────────────────────────────────────────────────────────────

    // yield();  // Pas nécessaire sur Arduino AVR, mais bonne pratique
}

// ════════════════════════════════════════════════════════════════
// FIN PROGRAMME PRINCIPAL
// ════════════════════════════════════════════════════════════════
