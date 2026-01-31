// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Encodeurs SSI (Implementation)
// ════════════════════════════════════════════════════════════════
// Fichier: encoder_ssi.cpp
// Description: Implémentation lecture encodeurs SSI
// ════════════════════════════════════════════════════════════════

#include "encoder_ssi.h"
#include "easycom.h"  // Pour printEncoderRawDebug()
#include <EEPROM.h>

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES
// ════════════════════════════════════════════════════════════════

// Position courante en degrés
float currentAz = 0.0;
float currentEl = 0.0;

// Valeurs brutes encodeurs
int rawCountsAz = 0;
int rawCountsEl = 0;

// Valeurs précédentes (pour détection transition)
int previousRawAz = 0;
int previousRawEl = 0;

// Tracking tours
long turnsAz = 0;
long turnsEl = 0;

// Offsets calibration
long offsetStepsAz = 0;
long offsetStepsEl = 0;

// Timing lecture encodeurs
unsigned long lastEncoderReadTime = 0;

// ════════════════════════════════════════════════════════════════
// INITIALISATION
// ════════════════════════════════════════════════════════════════

void setupEncoders() {
    // ─────────────────────────────────────────────────────────────
    // CONFIGURATION PINS SSI (si encodeurs SSI utilisés)
    // ─────────────────────────────────────────────────────────────
    #if (ENCODER_AZ_TYPE == ENCODER_SSI_ABSOLUTE) || (ENCODER_AZ_TYPE == ENCODER_SSI_INC) || \
        (ENCODER_EL_TYPE == ENCODER_SSI_ABSOLUTE) || (ENCODER_EL_TYPE == ENCODER_SSI_INC)
        pinMode(SSI_CLK, OUTPUT);
        pinMode(SSI_CS_AZ, OUTPUT);
        pinMode(SSI_DATA_AZ, INPUT);
        pinMode(SSI_CS_EL, OUTPUT);
        pinMode(SSI_DATA_EL, INPUT);

        // État initial pins (CLK et CS à HIGH = repos)
        digitalWrite(SSI_CLK, HIGH);
        digitalWrite(SSI_CS_AZ, HIGH);
        digitalWrite(SSI_CS_EL, HIGH);
    #endif

    // ─────────────────────────────────────────────────────────────
    // CHARGEMENT CALIBRATION EEPROM
    // ─────────────────────────────────────────────────────────────
    loadCalibrationFromEEPROM();

    // ─────────────────────────────────────────────────────────────
    // PREMIÈRE LECTURE POSITION
    // ─────────────────────────────────────────────────────────────
    delay(10);  // Stabilisation
    updateEncoders();

    // ─────────────────────────────────────────────────────────────
    // DEBUG AFFICHAGE
    // ─────────────────────────────────────────────────────────────
    #if DEBUG_SERIAL
        Serial.println(F("=== CAPTEURS POSITION INITIALISÉS ==="));
        Serial.print(F("Type Az: "));
        #if ENCODER_AZ_TYPE == ENCODER_SSI_ABSOLUTE
            Serial.println(F("SSI Absolu"));
        #elif ENCODER_AZ_TYPE == ENCODER_SSI_INC
            Serial.println(F("SSI Incrémental"));
        #endif

        Serial.print(F("Type El: "));
        #if ENCODER_EL_TYPE == ENCODER_SSI_ABSOLUTE
            Serial.println(F("SSI Absolu"));
        #elif ENCODER_EL_TYPE == ENCODER_SSI_INC
            Serial.println(F("SSI Incrémental"));
        #endif

        Serial.print(F("Position initiale Az: "));
        Serial.print(currentAz, 2);
        Serial.println(F("°"));
        Serial.print(F("Position initiale El: "));
        Serial.print(currentEl, 2);
        Serial.println(F("°"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// MISE À JOUR POSITION (Appelé dans loop)
// ════════════════════════════════════════════════════════════════

void updateEncoders() {
    // Throttling lecture (intervalle défini dans config.h)
    unsigned long currentTime = millis();
    if (currentTime - lastEncoderReadTime < ENCODER_READ_INTERVAL) {
        return;  // Pas encore temps de lire
    }
    lastEncoderReadTime = currentTime;

    // ═════════════════════════════════════════════════════════════
    // LECTURE AZIMUTH
    // ═════════════════════════════════════════════════════════════

    #if (ENCODER_AZ_TYPE == ENCODER_SSI_ABSOLUTE) || (ENCODER_AZ_TYPE == ENCODER_SSI_INC)
        // ─────────────────────────────────────────────────────────
        // ENCODEUR SSI (HH-12)
        // ─────────────────────────────────────────────────────────
        rawCountsAz = readSSI_Absolute(SSI_CS_AZ, SSI_DATA_AZ, REVERSE_AZ);

        // Détection transition tour (wraparound 0↔4095)
        int deltaAz = rawCountsAz - previousRawAz;
        if (deltaAz > 2048) {
            turnsAz--;  // Passage 4095→0 sens négatif
        } else if (deltaAz < -2048) {
            turnsAz++;  // Passage 0→4095 sens positif
        }
        previousRawAz = rawCountsAz;

        // Calcul position absolue en degrés
        long currentStepsAz = (turnsAz * 4096L) + rawCountsAz - offsetStepsAz;
        currentAz = (float)currentStepsAz * 360.0 / (4096.0 * GEAR_RATIO_AZ);

        // NORMALISATION 0-360° (IMPORTANT!)
        while (currentAz < 0) currentAz += 360.0;
        while (currentAz >= 360) currentAz -= 360.0;

    #endif

    // ═════════════════════════════════════════════════════════════
    // LECTURE ÉLÉVATION
    // ═════════════════════════════════════════════════════════════

    #if (ENCODER_EL_TYPE == ENCODER_SSI_ABSOLUTE) || (ENCODER_EL_TYPE == ENCODER_SSI_INC)
        // ─────────────────────────────────────────────────────────
        // ENCODEUR SSI (HH-12)
        // ─────────────────────────────────────────────────────────
        rawCountsEl = readSSI_Absolute(SSI_CS_EL, SSI_DATA_EL, REVERSE_EL);

        // Calcul position: mapping direct 0-4095 → 0-90°
        long currentStepsEl = (long)rawCountsEl - offsetStepsEl;
        currentEl = (float)currentStepsEl * 90.0 / 4095.0;

    #endif

    // ─────────────────────────────────────────────────────────────
    // DEBUG (optionnel)
    // ─────────────────────────────────────────────────────────────

    #if DEBUG_VERBOSE
        printEncoderDebug();
    #endif

    // Debug valeurs brutes (contrôlé par DEBUG_ENCODER_RAW et DEBUG_POT_ADC)
    printEncoderRawDebug();
}

// ════════════════════════════════════════════════════════════════
// LECTURE SSI ABSOLU
// ════════════════════════════════════════════════════════════════

int readSSI_Absolute(int csPin, int dataPin, bool reverse) {
    // Protocole SSI 12-bit (HH-12 SSI):
    // - 18 pulses CLK total (12 bits data + 6 bits status/parity)
    // - Data valide pendant CLK LOW
    // - MSB first (bit 11 → bit 0)

    unsigned long data = 0;

    // Activer transmission (CS LOW)
    digitalWrite(csPin, LOW);
    delayMicroseconds(5);

    // 18 pulses CLK (12 data + 6 status)
    for (int i = 0; i < 18; i++) {
        digitalWrite(SSI_CLK, LOW);
        delayMicroseconds(5);
        digitalWrite(SSI_CLK, HIGH);
        delayMicroseconds(5);

        // Lire uniquement 12 premiers bits (data utile)
        if (i < 12 && digitalRead(dataPin)) {
            data |= (1L << (11 - i));  // MSB first
        }
    }

    // Fin transmission (CS HIGH)
    digitalWrite(csPin, HIGH);

    // Conversion unsigned long → int (12-bit: 0-4095)
    int val = (int)data;

    // Inversion sens si demandé (REVERSE_AZ/EL)
    return reverse ? (4095 - val) : val;
}

// ════════════════════════════════════════════════════════════════
// LECTURE SSI INCRÉMENTAL
// ════════════════════════════════════════════════════════════════

int readSSI_Incremental(int csPin, int dataPin, bool reverse, long &turns) {
    // Encodeur incrémental: tracking tours multiples
    // Basé sur détection saut 0↔4095 (wraparound)

    // 1. Lecture valeur brute (même protocole SSI que absolu)
    int currentRaw = readSSI_Absolute(csPin, dataPin, reverse);

    // 2. Détection transition tour (passage 0↔4095)
    // Utiliser previousRawAz ou previousRawEl selon contexte
    int previousRaw = (csPin == SSI_CS_AZ) ? previousRawAz : previousRawEl;
    int turnChange = detectTurnTransition(currentRaw, previousRaw);
    turns += turnChange;

    // 3. Sauvegarde valeur précédente pour prochaine lecture
    if (csPin == SSI_CS_AZ) {
        previousRawAz = currentRaw;
    } else {
        previousRawEl = currentRaw;
    }

    // 4. Sauvegarde EEPROM si changement tour (éviter usure excessive)
    if (turnChange != 0) {
        if (csPin == SSI_CS_AZ) {
            EEPROM.put(EEPROM_TURNS_AZ, turns);
        }
        // Note: turnsEl pas sauvegardé car élévation généralement < 1 tour
    }

    return currentRaw;
}

// ════════════════════════════════════════════════════════════════
// CONVERSION COUNTS → DEGRÉS
// ════════════════════════════════════════════════════════════════

float convertCountsToDegrees(int rawCounts, long turns, long offset, float gearRatio) {
    // Conversion counts encodeur → degrés réels
    // Prend en compte: tours multiples, offset calibration, gear ratio

    // 1. Calcul steps absolus (position absolue depuis origine)
    //    = tours complets × counts/tour + position dans tour actuel - offset
    long stepsAbsolus = (turns * SSI_COUNTS_PER_REV) + rawCounts - offset;

    // 2. Conversion steps → degrés encodeur (1 tour encodeur = 360°)
    float degreesEncoder = (stepsAbsolus / (float)SSI_COUNTS_PER_REV) * 360.0;

    // 3. Application gear ratio (réduction mécanique)
    //    Si gear ratio = 5:1, alors 1° sortie = 5° encodeur
    float degreesOutput = degreesEncoder / gearRatio;

    return degreesOutput;
}

// ════════════════════════════════════════════════════════════════
// DÉTECTION TRANSITION TOUR
// ════════════════════════════════════════════════════════════════

int detectTurnTransition(int currentRaw, int previousRaw) {
    // Détection wraparound encodeur 12-bit (0 ↔ 4095)
    // Si delta > 2048, c'est un saut wraparound, pas un mouvement réel

    int delta = currentRaw - previousRaw;

    // Passage 4095 → 0 (sens positif = rotation CW)
    // Ex: prev=4090, curr=5 → delta=-4085 (< -2048)
    if (delta < -2048) {
        return +1;  // +1 tour complet
    }

    // Passage 0 → 4095 (sens négatif = rotation CCW)
    // Ex: prev=5, curr=4090 → delta=+4085 (> 2048)
    if (delta > 2048) {
        return -1;  // -1 tour (retour arrière)
    }

    // Pas de wraparound, mouvement normal
    return 0;
}

// ════════════════════════════════════════════════════════════════
// CALIBRATION AZIMUTH
// ════════════════════════════════════════════════════════════════

void calibrateAz(float realDegrees) {
    // Calibration azimuth: définit position courante = angle réel donné
    // Ex: Pointer vers Nord (0°) et envoyer commande "Z0.0"

    // Calcul offset pour que currentAz = realDegrees
    // Formule inversée de convertCountsToDegrees:
    // stepsAbsolus actuels = (turnsAz * 4096) + rawCountsAz
    // stepsAbsolus souhaités = realDegrees × 4096 × gearRatio / 360
    // offset = stepsAbsolus actuels - stepsAbsolus souhaités

    offsetStepsAz = (turnsAz * SSI_COUNTS_PER_REV) + rawCountsAz
                    - (long)(realDegrees * SSI_COUNTS_PER_REV * GEAR_RATIO_AZ / 360.0);

    // Sauvegarde EEPROM
    EEPROM.put(EEPROM_OFFSET_AZ, offsetStepsAz);

    // Mise à jour position courante immédiatement
    currentAz = realDegrees;

    #if DEBUG_SERIAL
        Serial.print(F("✓ Calibration Az: position courante = "));
        Serial.print(realDegrees, 1);
        Serial.print(F("° (offset="));
        Serial.print(offsetStepsAz);
        Serial.println(F(")"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// CALIBRATION ÉLÉVATION
// ════════════════════════════════════════════════════════════════

void calibrateEl(float realDegrees) {
    // Calibration élévation: définit position courante = angle réel donné
    // Ex: Pointer à l'horizon (0°) ou au zénith (90°) et calibrer

    offsetStepsEl = (turnsEl * SSI_COUNTS_PER_REV) + rawCountsEl
                    - (long)(realDegrees * SSI_COUNTS_PER_REV * GEAR_RATIO_EL / 360.0);

    // Sauvegarde EEPROM
    EEPROM.put(EEPROM_OFFSET_EL, offsetStepsEl);

    // Mise à jour position courante immédiatement
    currentEl = realDegrees;

    #if DEBUG_SERIAL
        Serial.print(F("✓ Calibration El: position courante = "));
        Serial.print(realDegrees, 1);
        Serial.print(F("° (offset="));
        Serial.print(offsetStepsEl);
        Serial.println(F(")"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// CHARGEMENT CALIBRATION EEPROM
// ════════════════════════════════════════════════════════════════

void loadCalibrationFromEEPROM() {
    // Chargement calibration depuis EEPROM persistante
    // Adresses définies dans config.h

    EEPROM.get(EEPROM_TURNS_AZ, turnsAz);
    EEPROM.get(EEPROM_TURNS_EL, turnsEl);
    EEPROM.get(EEPROM_OFFSET_AZ, offsetStepsAz);
    EEPROM.get(EEPROM_OFFSET_EL, offsetStepsEl);

    // Vérification EEPROM vierge (valeur 0xFFFFFFFF = -1 en signed long)
    // Si EEPROM jamais initialisée, reset à 0
    if (turnsAz == -1 || (unsigned long)turnsAz == 4294967295UL) {
        turnsAz = 0;
        EEPROM.put(EEPROM_TURNS_AZ, turnsAz);
    }

    if (turnsEl == -1 || (unsigned long)turnsEl == 4294967295UL) {
        turnsEl = 0;
        EEPROM.put(EEPROM_TURNS_EL, turnsEl);
    }

    if (offsetStepsAz == -1 || (unsigned long)offsetStepsAz == 4294967295UL) {
        offsetStepsAz = 0;
        EEPROM.put(EEPROM_OFFSET_AZ, offsetStepsAz);
    }

    if (offsetStepsEl == -1 || (unsigned long)offsetStepsEl == 4294967295UL) {
        offsetStepsEl = 0;
        EEPROM.put(EEPROM_OFFSET_EL, offsetStepsEl);
    }

    #if DEBUG_SERIAL
        Serial.println(F("Calibration chargée depuis EEPROM:"));
        Serial.print(F("  turnsAz: ")); Serial.println(turnsAz);
        Serial.print(F("  turnsEl: ")); Serial.println(turnsEl);
        Serial.print(F("  offsetAz: ")); Serial.println(offsetStepsAz);
        Serial.print(F("  offsetEl: ")); Serial.println(offsetStepsEl);
    #endif
}

// ════════════════════════════════════════════════════════════════
// SAUVEGARDE CALIBRATION EEPROM
// ════════════════════════════════════════════════════════════════

void saveCalibrationToEEPROM() {
    // TODO: Écriture valeurs EEPROM

    // EEPROM.put(EEPROM_TURNS_AZ, turnsAz);
    // EEPROM.put(EEPROM_OFFSET_AZ, offsetStepsAz);
    // EEPROM.put(EEPROM_OFFSET_EL, offsetStepsEl);

    #if DEBUG_SERIAL
        Serial.println(F("Calibration sauvegardée dans EEPROM"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// DEBUG AFFICHAGE
// ════════════════════════════════════════════════════════════════

void printEncoderDebug() {
    Serial.print(F("Az: "));
    Serial.print(currentAz, 2);
    Serial.print(F("° (raw:"));
    Serial.print(rawCountsAz);
    Serial.print(F(", turns:"));
    Serial.print(turnsAz);
    Serial.print(F(") | El: "));
    Serial.print(currentEl, 2);
    Serial.print(F("° (raw:"));
    Serial.print(rawCountsEl);
    Serial.println(F(")"));
}
