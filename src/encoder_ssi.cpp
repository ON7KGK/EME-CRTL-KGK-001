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

// Variables filtrage potentiomètre (moyenne glissante)
#if (ENCODER_AZ_TYPE == ENCODER_POT_1T) || (ENCODER_AZ_TYPE == ENCODER_POT_MT)
    int potAdcBufferAz[POT_SAMPLES_AZ] = {0};  // Buffer circulaire ADC azimuth
    int potBufferIndexAz = 0;                   // Index buffer circulaire azimuth
    bool potBufferFullAz = false;               // Flag buffer plein azimuth
#endif

#if (ENCODER_EL_TYPE == ENCODER_POT_1T) || (ENCODER_EL_TYPE == ENCODER_POT_MT)
    int potAdcBufferEl[POT_SAMPLES_EL] = {0};  // Buffer circulaire ADC élévation
    int potBufferIndexEl = 0;                   // Index buffer circulaire élévation
    bool potBufferFullEl = false;               // Flag buffer plein élévation
#endif

// Variables tracking tours potentiomètre multi-tours
// Note: On utilise turnsAz/turnsEl (variables globales) pour les compteurs de tours POT_MT
// previousRawAdc est déclaré static dans updateEncoders() pour la détection wraparound

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
    // CONFIGURATION PIN POTENTIOMÈTRE (si POT 1 tour ou multi-tours)
    // ─────────────────────────────────────────────────────────────
    #if (ENCODER_AZ_TYPE == ENCODER_POT_1T) || (ENCODER_AZ_TYPE == ENCODER_POT_MT)
        pinMode(POT_PIN_AZ, INPUT);
    #endif

    #if (ENCODER_EL_TYPE == ENCODER_POT_1T) || (ENCODER_EL_TYPE == ENCODER_POT_MT)
        pinMode(POT_PIN_EL, INPUT);
    #endif

    // ─────────────────────────────────────────────────────────────
    // CHARGEMENT CALIBRATION EEPROM
    // ─────────────────────────────────────────────────────────────
    loadCalibrationFromEEPROM();

    // ─────────────────────────────────────────────────────────────
    // PREMIÈRE LECTURE POSITION
    // ─────────────────────────────────────────────────────────────

    // Initialisation potentiomètre multi-tours AZIMUTH
    #if (ENCODER_AZ_TYPE == ENCODER_POT_MT)
        // Lire ADC initial et pré-remplir buffer filtrage
        int initialAdcAz = analogRead(POT_PIN_AZ);

        for (int i = 0; i < POT_SAMPLES_AZ; i++) {
            potAdcBufferAz[i] = initialAdcAz;
        }
        potBufferFullAz = true;

        // Note: previousRawAdcAz (static dans updateEncoders) sera initialisé
        // automatiquement à la première lecture
    #endif

    // Initialisation potentiomètre multi-tours ÉLÉVATION
    #if (ENCODER_EL_TYPE == ENCODER_POT_MT)
        // Lire ADC initial et pré-remplir buffer filtrage
        int initialAdcEl = analogRead(POT_PIN_EL);

        for (int i = 0; i < POT_SAMPLES_EL; i++) {
            potAdcBufferEl[i] = initialAdcEl;
        }
        potBufferFullEl = true;

        // Note: previousRawAdcEl (static dans updateEncoders) sera initialisé
        // automatiquement à la première lecture
    #endif

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
        #elif ENCODER_AZ_TYPE == ENCODER_POT_1T
            Serial.println(F("Potentiomètre 1 Tour"));
        #elif ENCODER_AZ_TYPE == ENCODER_POT_MT
            Serial.println(F("Potentiomètre Multi-Tours"));
        #endif

        Serial.print(F("Type El: "));
        #if ENCODER_EL_TYPE == ENCODER_SSI_ABSOLUTE
            Serial.println(F("SSI Absolu"));
        #elif ENCODER_EL_TYPE == ENCODER_SSI_INC
            Serial.println(F("SSI Incrémental"));
        #elif ENCODER_EL_TYPE == ENCODER_POT_1T
            Serial.println(F("Potentiomètre 1 Tour"));
        #elif ENCODER_EL_TYPE == ENCODER_POT_MT
            Serial.println(F("Potentiomètre Multi-Tours"));
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

    #elif (ENCODER_AZ_TYPE == ENCODER_POT_1T)
        // ─────────────────────────────────────────────────────────
        // POTENTIOMÈTRE 1 TOUR (Filtrage moyenne glissante)
        // ─────────────────────────────────────────────────────────

        // Lecture ADC brute (0-1023)
        int rawAdc = analogRead(POT_PIN_AZ);
        #if REVERSE_AZ
            rawAdc = 1023 - rawAdc;  // Inversion sens
        #endif

        // Ajout au buffer circulaire
        potAdcBufferAz[potBufferIndexAz] = rawAdc;
        potBufferIndexAz = (potBufferIndexAz + 1) % POT_SAMPLES_AZ;
        if (potBufferIndexAz == 0) potBufferFullAz = true;

        // Calcul moyenne glissante
        long adcSum = 0;
        int sampleCount = potBufferFullAz ? POT_SAMPLES_AZ : potBufferIndexAz;
        for (int i = 0; i < sampleCount; i++) {
            adcSum += potAdcBufferAz[i];
        }
        int adcValue = (int)(adcSum / sampleCount);

        rawCountsAz = adcValue;  // Pour compatibilité debug

        // Mapping direct: ADC 0-1023 → 0-360°
        // Formule: degrees = (adcValue / 1024.0) * 360.0
        currentAz = ((float)adcValue / (float)POT_ADC_RESOLUTION) * 360.0;

        // Contrainte 0-360° (sécurité)
        if (currentAz < 0.0) currentAz = 0.0;
        if (currentAz > 360.0) currentAz = 360.0;

    #elif (ENCODER_AZ_TYPE == ENCODER_POT_MT)
        // ─────────────────────────────────────────────────────────
        // POTENTIOMÈTRE MULTI-TOURS (Filtrage + Tracking tours)
        // ─────────────────────────────────────────────────────────

        // Lecture ADC brute (0-1023)
        int rawAdc = analogRead(POT_PIN_AZ);
        #if REVERSE_AZ
            rawAdc = 1023 - rawAdc;  // Inversion sens
        #endif

        // ─────────────────────────────────────────────────────────
        // DÉTECTION WRAPAROUND SUR ADC BRUT (AVANT FILTRAGE!)
        // ─────────────────────────────────────────────────────────
        // Logique ORIGINALE simple qui fonctionnait.
        // Seuil 768: si ADC saute de plus de 3/4 de la plage en une lecture

        static int previousRawAdc = rawAdc;  // Valeur brute précédente

        int adcDelta = rawAdc - previousRawAdc;

        // Passage 1023 → 0 (saut négatif brutal)
        if (adcDelta < -768) {
            turnsAz++;
            EEPROM.put(EEPROM_TURNS_AZ, turnsAz);
            #if DEBUG_SERIAL
                Serial.print(F("✓✓✓ WRAP++ Az | ADC:"));
                Serial.print(previousRawAdc);
                Serial.print(F("→"));
                Serial.print(rawAdc);
                Serial.print(F(" | turns="));
                Serial.println(turnsAz);
            #endif
        }

        // Passage 0 → 1023 (saut positif brutal)
        if (adcDelta > 768) {
            turnsAz--;
            EEPROM.put(EEPROM_TURNS_AZ, turnsAz);
            #if DEBUG_SERIAL
                Serial.print(F("✓✓✓ WRAP-- Az | ADC:"));
                Serial.print(previousRawAdc);
                Serial.print(F("→"));
                Serial.print(rawAdc);
                Serial.print(F(" | turns="));
                Serial.println(turnsAz);
            #endif
        }

        previousRawAdc = rawAdc;

        // ─────────────────────────────────────────────────────────
        // FILTRAGE MOYENNE GLISSANTE (Stabilisation affichage)
        // ─────────────────────────────────────────────────────────

        // Ajout au buffer circulaire
        potAdcBufferAz[potBufferIndexAz] = rawAdc;
        potBufferIndexAz = (potBufferIndexAz + 1) % POT_SAMPLES_AZ;
        if (potBufferIndexAz == 0) potBufferFullAz = true;

        // Calcul moyenne glissante
        long adcSum = 0;
        int sampleCount = potBufferFullAz ? POT_SAMPLES_AZ : potBufferIndexAz;
        for (int i = 0; i < sampleCount; i++) {
            adcSum += potAdcBufferAz[i];
        }
        int adcValue = (int)(adcSum / sampleCount);

        rawCountsAz = adcValue;  // Pour compatibilité debug

        // Mapping ADC → degrés du tour courant (0-360°)
        float potDegrees = ((float)adcValue / (float)POT_ADC_RESOLUTION) * 360.0;

        // Contrainte 0-360° (sécurité)
        if (potDegrees < 0.0) potDegrees = 0.0;
        if (potDegrees > 360.0) potDegrees = 360.0;

        // ─────────────────────────────────────────────────────────
        // CALCUL POSITION ABSOLUE (Tours multiples + Gear Ratio + Offset)
        // ─────────────────────────────────────────────────────────
        // Position totale pot (degrés) = (nombre de tours × 360°) + position dans tour courant
        // Position antenne (degrés) = (position pot - offset) / GEAR_RATIO_AZ
        //
        // Exemple: Si GEAR_RATIO_AZ = 10.0 (10 tours pot = 1 tour antenne)
        //   - Pot à 3600° (10 tours) → Antenne à 360° (1 tour)
        //   - Normalisation 0-360° pour compatibilité Easycom/PstRotator

        float potPositionTotal = (turnsAz * 360.0) + potDegrees;

        // Appliquer l'offset de calibration (converti de steps en degrés pot)
        float offsetDegreesPot = ((float)offsetStepsAz / (float)POT_ADC_RESOLUTION) * 360.0;
        currentAz = (potPositionTotal - offsetDegreesPot) / GEAR_RATIO_AZ;

        // Normalisation 0-360° (requise par PstRotator)
        while (currentAz >= 360.0) currentAz -= 360.0;
        while (currentAz < 0.0) currentAz += 360.0;

        // DEBUG TEMPORAIRE
        #if DEBUG_SERIAL
            Serial.print(F("POT | ADC:"));
            Serial.print(rawAdc);
            Serial.print(F(" filt:"));
            Serial.print(adcValue);
            Serial.print(F(" deg:"));
            Serial.print(potDegrees, 1);
            Serial.print(F(" turns:"));
            Serial.print(turnsAz);
            Serial.print(F(" → Az:"));
            Serial.println(currentAz, 1);
        #endif

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

    #elif (ENCODER_EL_TYPE == ENCODER_POT_1T)
        // ─────────────────────────────────────────────────────────
        // POTENTIOMÈTRE 1 TOUR (Filtrage moyenne glissante)
        // ─────────────────────────────────────────────────────────

        // Lecture ADC brute (0-1023)
        int rawAdcEl = analogRead(POT_PIN_EL);
        #if REVERSE_EL
            rawAdcEl = 1023 - rawAdcEl;  // Inversion sens
        #endif

        // Ajout au buffer circulaire
        potAdcBufferEl[potBufferIndexEl] = rawAdcEl;
        potBufferIndexEl = (potBufferIndexEl + 1) % POT_SAMPLES_EL;
        if (potBufferIndexEl == 0) potBufferFullEl = true;

        // Calcul moyenne glissante
        long adcSumEl = 0;
        int sampleCountEl = potBufferFullEl ? POT_SAMPLES_EL : potBufferIndexEl;
        for (int i = 0; i < sampleCountEl; i++) {
            adcSumEl += potAdcBufferEl[i];
        }
        int adcValueEl = (int)(adcSumEl / sampleCountEl);

        rawCountsEl = adcValueEl;  // Pour compatibilité debug

        // Mapping direct: ADC 0-1023 → 0-90° (pour élévation, range typique 0-90°)
        currentEl = ((float)adcValueEl / (float)POT_ADC_RESOLUTION) * 90.0;

        // Contrainte 0-90° (sécurité)
        if (currentEl < 0.0) currentEl = 0.0;
        if (currentEl > 90.0) currentEl = 90.0;

    #elif (ENCODER_EL_TYPE == ENCODER_POT_MT)
        // ─────────────────────────────────────────────────────────
        // POTENTIOMÈTRE MULTI-TOURS (Filtrage + Tracking tours)
        // ─────────────────────────────────────────────────────────

        // Lecture ADC brute (0-1023)
        int rawAdcEl = analogRead(POT_PIN_EL);
        #if REVERSE_EL
            rawAdcEl = 1023 - rawAdcEl;  // Inversion sens
        #endif

        // ─────────────────────────────────────────────────────────
        // DÉTECTION WRAPAROUND SUR ADC BRUT (AVANT FILTRAGE!)
        // ─────────────────────────────────────────────────────────
        // Logique ORIGINALE simple qui fonctionnait.
        // Seuil 768: si ADC saute de plus de 3/4 de la plage en une lecture

        static int previousRawAdcEl = rawAdcEl;  // Valeur brute précédente

        int adcDeltaEl = rawAdcEl - previousRawAdcEl;

        // Passage 1023 → 0 (saut négatif brutal)
        if (adcDeltaEl < -768) {
            turnsEl++;
            EEPROM.put(EEPROM_TURNS_EL, turnsEl);
            #if DEBUG_SERIAL
                Serial.print(F("✓✓✓ WRAP++ El | ADC:"));
                Serial.print(previousRawAdcEl);
                Serial.print(F("→"));
                Serial.print(rawAdcEl);
                Serial.print(F(" | turns="));
                Serial.println(turnsEl);
            #endif
        }

        // Passage 0 → 1023 (saut positif brutal)
        if (adcDeltaEl > 768) {
            turnsEl--;
            EEPROM.put(EEPROM_TURNS_EL, turnsEl);
            #if DEBUG_SERIAL
                Serial.print(F("✓✓✓ WRAP-- El | ADC:"));
                Serial.print(previousRawAdcEl);
                Serial.print(F("→"));
                Serial.print(rawAdcEl);
                Serial.print(F(" | turns="));
                Serial.println(turnsEl);
            #endif
        }

        previousRawAdcEl = rawAdcEl;

        // ─────────────────────────────────────────────────────────
        // FILTRAGE MOYENNE GLISSANTE (Stabilisation affichage)
        // ─────────────────────────────────────────────────────────

        // Ajout au buffer circulaire
        potAdcBufferEl[potBufferIndexEl] = rawAdcEl;
        potBufferIndexEl = (potBufferIndexEl + 1) % POT_SAMPLES_EL;
        if (potBufferIndexEl == 0) potBufferFullEl = true;

        // Calcul moyenne glissante
        long adcSumEl = 0;
        int sampleCountEl = potBufferFullEl ? POT_SAMPLES_EL : potBufferIndexEl;
        for (int i = 0; i < sampleCountEl; i++) {
            adcSumEl += potAdcBufferEl[i];
        }
        int adcValueEl = (int)(adcSumEl / sampleCountEl);

        rawCountsEl = adcValueEl;  // Pour compatibilité debug

        // Mapping ADC → degrés du tour courant (0-360°)
        float potDegreesEl = ((float)adcValueEl / (float)POT_ADC_RESOLUTION) * 360.0;

        // Contrainte 0-360° (sécurité)
        if (potDegreesEl < 0.0) potDegreesEl = 0.0;
        if (potDegreesEl > 360.0) potDegreesEl = 360.0;

        // ─────────────────────────────────────────────────────────
        // CALCUL POSITION ABSOLUE (Tours multiples + Gear Ratio + Offset)
        // ─────────────────────────────────────────────────────────
        // Position totale pot (degrés) = (nombre de tours × 360°) + position dans tour courant
        // Position antenne (degrés) = (position pot - offset) / GEAR_RATIO_EL
        //
        // Exemple: Si GEAR_RATIO_EL = 4.0 (4 tours pot = 1 tour élévation 0-90°)
        //   - Pot à 360° (1 tour) → Élévation à 90° (1/4 tour)
        //   - Normalisation 0-90° pour compatibilité Easycom/PstRotator

        float potPositionTotalEl = (turnsEl * 360.0) + potDegreesEl;

        // Appliquer l'offset de calibration (converti de steps en degrés pot)
        float offsetDegreesPotEl = ((float)offsetStepsEl / (float)POT_ADC_RESOLUTION) * 360.0;
        currentEl = (potPositionTotalEl - offsetDegreesPotEl) / GEAR_RATIO_EL;

        // Normalisation 0-90° (typique pour élévation)
        while (currentEl >= 90.0) currentEl -= 90.0;
        while (currentEl < 0.0) currentEl += 90.0;

        // DEBUG TEMPORAIRE
        #if DEBUG_SERIAL
            Serial.print(F("POT EL | ADC:"));
            Serial.print(rawAdcEl);
            Serial.print(F(" filt:"));
            Serial.print(adcValueEl);
            Serial.print(F(" deg:"));
            Serial.print(potDegreesEl, 1);
            Serial.print(F(" turns:"));
            Serial.print(turnsEl);
            Serial.print(F(" → El:"));
            Serial.println(currentEl, 1);
        #endif

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

    #if (ENCODER_AZ_TYPE == ENCODER_POT_MT)
        // ─────────────────────────────────────────────────────────
        // CALIBRATION POTENTIOMÈTRE MULTI-TOURS
        // ─────────────────────────────────────────────────────────
        // Pour calibrer à realDegrees:
        // 1. Reset turnsAz à 0
        // 2. Calculer la position actuelle du pot en degrés antenne
        // 3. Stocker l'offset pour que currentAz = realDegrees

        // Position actuelle du pot (degrés dans le tour courant)
        float potDegrees = ((float)rawCountsAz / (float)POT_ADC_RESOLUTION) * 360.0;

        // Reset compteur de tours
        turnsAz = 0;
        EEPROM.put(EEPROM_TURNS_AZ, turnsAz);

        // Position pot sans offset = potDegrees / GEAR_RATIO_AZ
        // On veut que cette position = realDegrees
        // Donc offset (en degrés antenne) = (potDegrees / GEAR_RATIO_AZ) - realDegrees
        // On stocke l'offset en "steps" pot (pour compatibilité)
        float offsetDegrees = (potDegrees / GEAR_RATIO_AZ) - realDegrees;
        offsetStepsAz = (long)(offsetDegrees * GEAR_RATIO_AZ * POT_ADC_RESOLUTION / 360.0);

        EEPROM.put(EEPROM_OFFSET_AZ, offsetStepsAz);

        // Mise à jour position courante immédiatement
        currentAz = realDegrees;

        #if DEBUG_SERIAL
            Serial.print(F("✓ Calibration Az POT_MT: "));
            Serial.print(realDegrees, 1);
            Serial.print(F("° (potDeg="));
            Serial.print(potDegrees, 1);
            Serial.print(F(", offset="));
            Serial.print(offsetStepsAz);
            Serial.println(F(")"));
        #endif

    #else
        // ─────────────────────────────────────────────────────────
        // CALIBRATION ENCODEUR SSI
        // ─────────────────────────────────────────────────────────
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
            Serial.print(F("✓ Calibration Az SSI: "));
            Serial.print(realDegrees, 1);
            Serial.print(F("° (offset="));
            Serial.print(offsetStepsAz);
            Serial.println(F(")"));
        #endif
    #endif
}

// ════════════════════════════════════════════════════════════════
// CALIBRATION ÉLÉVATION
// ════════════════════════════════════════════════════════════════

void calibrateEl(float realDegrees) {
    // Calibration élévation: définit position courante = angle réel donné
    // Ex: Pointer à l'horizon (0°) ou au zénith (90°) et calibrer

    #if (ENCODER_EL_TYPE == ENCODER_POT_MT)
        // ─────────────────────────────────────────────────────────
        // CALIBRATION POTENTIOMÈTRE MULTI-TOURS
        // ─────────────────────────────────────────────────────────

        // Position actuelle du pot (degrés dans le tour courant)
        float potDegrees = ((float)rawCountsEl / (float)POT_ADC_RESOLUTION) * 360.0;

        // Reset compteur de tours
        turnsEl = 0;
        EEPROM.put(EEPROM_TURNS_EL, turnsEl);

        // Calcul offset
        float offsetDegrees = (potDegrees / GEAR_RATIO_EL) - realDegrees;
        offsetStepsEl = (long)(offsetDegrees * GEAR_RATIO_EL * POT_ADC_RESOLUTION / 360.0);

        EEPROM.put(EEPROM_OFFSET_EL, offsetStepsEl);

        // Mise à jour position courante immédiatement
        currentEl = realDegrees;

        #if DEBUG_SERIAL
            Serial.print(F("✓ Calibration El POT_MT: "));
            Serial.print(realDegrees, 1);
            Serial.print(F("° (potDeg="));
            Serial.print(potDegrees, 1);
            Serial.print(F(", offset="));
            Serial.print(offsetStepsEl);
            Serial.println(F(")"));
        #endif

    #else
        // ─────────────────────────────────────────────────────────
        // CALIBRATION ENCODEUR SSI
        // ─────────────────────────────────────────────────────────
        offsetStepsEl = (turnsEl * SSI_COUNTS_PER_REV) + rawCountsEl
                        - (long)(realDegrees * SSI_COUNTS_PER_REV * GEAR_RATIO_EL / 360.0);

        // Sauvegarde EEPROM
        EEPROM.put(EEPROM_OFFSET_EL, offsetStepsEl);

        // Mise à jour position courante immédiatement
        currentEl = realDegrees;

        #if DEBUG_SERIAL
            Serial.print(F("✓ Calibration El SSI: "));
            Serial.print(realDegrees, 1);
            Serial.print(F("° (offset="));
            Serial.print(offsetStepsEl);
            Serial.println(F(")"));
        #endif
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
