// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Encodeurs SSI (Implementation)
// ════════════════════════════════════════════════════════════════
// Fichier: encoder_ssi.cpp
// Description: Implémentation lecture encodeurs SSI
// ════════════════════════════════════════════════════════════════

#include "encoder_ssi.h"
#include "easycom.h"  // Pour printEncoderRawDebug()
#include "network.h"  // Pour sendToClient()
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

// Accumulation ADC pour méthode cumulative (POT_MT)
long accumulatedAdcAz = 0;  // ADC total accumulé depuis calibration
long accumulatedAdcEl = 0;

// Tracking tours (utilisé pour SSI_INC, gardé pour compatibilité)
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

// Filtrage EMA azimuth (module-level pour reset lors calibration)
float filteredAz = 0.0;
bool azFilterInitialized = false;

// ════════════════════════════════════════════════════════════════
// TABLE DE CORRECTION AZIMUTH (35 points)
// ════════════════════════════════════════════════════════════════
// Chaque élément stocke la valeur ADC cumulée pour l'angle correspondant
// Index 0 = 0°, Index 1 = 10°, Index 2 = 20°, ... Index 34 = 340°
// Interpolation linéaire entre les points pour angles intermédiaires

long azCorrectionTable[AZ_TABLE_POINTS];  // ADC cumulé pour chaque point
bool azTableLoaded = false;               // Table chargée depuis EEPROM

// ════════════════════════════════════════════════════════════════
// TABLE DE CORRECTION ÉLÉVATION (10 points)
// ════════════════════════════════════════════════════════════════
// Index 0 = 0°, Index 1 = 10°, ... Index 9 = 90°

long elCorrectionTable[EL_TABLE_POINTS];  // ADC cumulé pour chaque point
bool elTableLoaded = false;               // Table chargée depuis EEPROM

// Filtrage EMA élévation (module-level pour reset lors calibration)
float filteredEl = 0.0;
bool elFilterInitialized = false;

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

    // Chargement table correction azimuth (POT_MT)
    #if (ENCODER_AZ_TYPE == ENCODER_POT_MT)
        loadAzCorrectionTable();
    #endif

    // Chargement table correction élévation (POT_MT)
    #if (ENCODER_EL_TYPE == ENCODER_POT_MT)
        loadElCorrectionTable();
    #endif

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
        // ═══════════════════════════════════════════════════════════
        // POTENTIOMÈTRE MULTI-TOURS AZIMUTH - MÉTHODE CUMULATIVE
        // ═══════════════════════════════════════════════════════════
        // Cette méthode accumule les petits changements ADC au lieu
        // de détecter des "tours" discrets. Plus robuste et simple.
        //
        // Fonctionnement:
        // 1. Lire ADC (avec REVERSE)
        // 2. Calculer delta depuis dernière lecture
        // 3. Corriger le delta si wraparound (±512 threshold)
        // 4. Accumuler le delta dans accumulatedAdcAz
        // 5. Calculer position = accumulatedAdc / (1024 * GEAR_RATIO) * 360
        // ═══════════════════════════════════════════════════════════

        // ─────────────────────────────────────────────────────────
        // ÉTAPE 1: LECTURE ADC
        // ─────────────────────────────────────────────────────────
        int rawAdc = analogRead(POT_PIN_AZ);
        #if REVERSE_AZ
            rawAdc = 1023 - rawAdc;
        #endif

        // ─────────────────────────────────────────────────────────
        // ÉTAPE 2: ACCUMULATION AVEC CORRECTION WRAPAROUND
        // ─────────────────────────────────────────────────────────
        // On accumule les petits changements. Si le delta est > 512,
        // c'est un wraparound qu'on corrige.

        static bool azAccumInitialized = false;

        if (!azAccumInitialized) {
            previousRawAz = rawAdc;
            azAccumInitialized = true;
        }

        int delta = rawAdc - previousRawAz;

        // Correction wraparound: si delta trop grand, c'est un passage 0↔1023
        if (delta > 512) {
            delta -= 1024;  // Passage 1023→0 en sens inverse (CCW)
        } else if (delta < -512) {
            delta += 1024;  // Passage 0→1023 en sens inverse (CW)
        }

        // Accumuler (turnsAz stocke maintenant l'ADC accumulé, pas les tours)
        // On réutilise offsetStepsAz pour stocker la position de référence
        accumulatedAdcAz += delta;

        previousRawAz = rawAdc;

        // Sauvegarder périodiquement (pas à chaque cycle pour éviter usure EEPROM)
        static unsigned long lastEepromSaveAz = 0;
        if (millis() - lastEepromSaveAz > 5000) {  // Toutes les 5 secondes
            EEPROM.put(EEPROM_TURNS_AZ, accumulatedAdcAz);
            lastEepromSaveAz = millis();
        }

        // ─────────────────────────────────────────────────────────
        // ÉTAPE 3: CALCUL POSITION EN DEGRÉS (Table d'interpolation)
        // ─────────────────────────────────────────────────────────
        // Utilise la table de correction avec interpolation linéaire
        // au lieu du simple GEAR_RATIO (compense non-linéarité pot)

        float rawAzDeg = adcToDegrees(accumulatedAdcAz);

        // ─────────────────────────────────────────────────────────
        // ÉTAPE 4: FILTRAGE EMA (Exponential Moving Average)
        // ─────────────────────────────────────────────────────────
        // Lisse les fluctuations ADC à l'arrêt
        // Alpha = 0.25 → 25% nouvelle valeur, 75% ancienne (bon compromis)
        // Variables filteredAz et azFilterInitialized sont module-level
        // pour permettre reset lors de la calibration

        if (!azFilterInitialized) {
            filteredAz = rawAzDeg;
            azFilterInitialized = true;
        } else {
            filteredAz = 0.25 * rawAzDeg + 0.75 * filteredAz;
        }
        currentAz = filteredAz;

        // Normalisation 0-360° pour Easycom/PstRotator
        while (currentAz >= 360.0) currentAz -= 360.0;
        while (currentAz < 0.0) currentAz += 360.0;

        // Snap à 0° si très proche de 360° (évite 359.9° après calibration à 0°)
        // Avec limite mécanique à ~343°, on ne sera jamais légitimement à 359°
        if (currentAz > 359.5) currentAz = 0.0;

        rawCountsAz = rawAdc;  // Pour debug

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
        // POTENTIOMÈTRE 1 TOUR (Filtrage + Offset + Gear Ratio)
        // ─────────────────────────────────────────────────────────
        // Utilisé quand la plage mécanique < 1 tour du pot
        // (ex: élévation -10° à +60° = 70° → pot fait ~0.97 tour)

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

        // ─────────────────────────────────────────────────────────
        // CALCUL POSITION AVEC GEAR_RATIO ET OFFSET
        // ─────────────────────────────────────────────────────────
        // potDegrees = position du pot (0-360°)
        // offsetDegreesPot = offset en degrés pot (depuis calibration)
        // currentEl = (potDegrees - offset) / GEAR_RATIO

        float potDegreesEl = ((float)adcValueEl / (float)POT_ADC_RESOLUTION) * 360.0;
        float offsetDegreesPotEl = ((float)offsetStepsEl / (float)POT_ADC_RESOLUTION) * 360.0;
        currentEl = (potDegreesEl - offsetDegreesPotEl) / GEAR_RATIO_EL;

        // Contrainte -15° à +95° (marge de sécurité)
        if (currentEl < -15.0) currentEl = -15.0;
        if (currentEl > 95.0) currentEl = 95.0;

    #elif (ENCODER_EL_TYPE == ENCODER_POT_MT)
        // ═══════════════════════════════════════════════════════════
        // POTENTIOMÈTRE MULTI-TOURS ÉLÉVATION (MÉTHODE CUMULATIVE)
        // ═══════════════════════════════════════════════════════════
        // Même méthode que l'azimuth: accumulation ADC + table correction
        // Compense la non-linéarité du potentiomètre

        // ─────────────────────────────────────────────────────────
        // ÉTAPE 1: LECTURE ADC
        // ─────────────────────────────────────────────────────────
        int rawAdcEl = analogRead(POT_PIN_EL);
        #if REVERSE_EL
            rawAdcEl = 1023 - rawAdcEl;
        #endif

        // ─────────────────────────────────────────────────────────
        // ÉTAPE 2: ACCUMULATION AVEC CORRECTION WRAPAROUND
        // ─────────────────────────────────────────────────────────
        static bool elAccumInitialized = false;
        static int previousRawEl = 0;

        if (!elAccumInitialized) {
            previousRawEl = rawAdcEl;
            elAccumInitialized = true;
        }

        int deltaEl = rawAdcEl - previousRawEl;

        // Correction wraparound
        if (deltaEl > 512) {
            deltaEl -= 1024;
        } else if (deltaEl < -512) {
            deltaEl += 1024;
        }

        // Accumuler
        accumulatedAdcEl += deltaEl;
        previousRawEl = rawAdcEl;

        // Sauvegarder périodiquement
        static unsigned long lastEepromSaveEl = 0;
        if (millis() - lastEepromSaveEl > 5000) {
            EEPROM.put(EEPROM_TURNS_EL, accumulatedAdcEl);
            lastEepromSaveEl = millis();
        }

        // ─────────────────────────────────────────────────────────
        // ÉTAPE 3: CALCUL POSITION EN DEGRÉS (Table d'interpolation)
        // ─────────────────────────────────────────────────────────
        float rawElDeg = adcToDegreesEl(accumulatedAdcEl);

        // ─────────────────────────────────────────────────────────
        // ÉTAPE 4: FILTRAGE EMA
        // ─────────────────────────────────────────────────────────
        if (!elFilterInitialized) {
            filteredEl = rawElDeg;
            elFilterInitialized = true;
        } else {
            filteredEl = 0.25 * rawElDeg + 0.75 * filteredEl;
        }
        currentEl = filteredEl;

        // Contrainte -15° à +95°
        if (currentEl < -15.0) currentEl = -15.0;
        if (currentEl > 95.0) currentEl = 95.0;

        rawCountsEl = rawAdcEl;  // Pour debug

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
// RESET BUFFER FILTRAGE POTENTIOMÈTRE
// ════════════════════════════════════════════════════════════════

void resetPotBufferAz(int adcValue) {
    #if (ENCODER_AZ_TYPE == ENCODER_POT_1T) || (ENCODER_AZ_TYPE == ENCODER_POT_MT)
        // Remplir le buffer avec la valeur actuelle pour cohérence
        for (int i = 0; i < POT_SAMPLES_AZ; i++) {
            potAdcBufferAz[i] = adcValue;
        }
        potBufferIndexAz = 0;
        potBufferFullAz = true;
        rawCountsAz = adcValue;

        // Reset valeur précédente pour éviter faux wraparound
        previousRawAz = adcValue;

        #if DEBUG_SERIAL
            Serial.print(F("✓ Buffer Az reset ADC="));
            Serial.println(adcValue);
        #endif
    #endif
}

void resetPotBufferEl(int adcValue) {
    #if (ENCODER_EL_TYPE == ENCODER_POT_1T) || (ENCODER_EL_TYPE == ENCODER_POT_MT)
        // Remplir le buffer avec la valeur actuelle pour cohérence
        for (int i = 0; i < POT_SAMPLES_EL; i++) {
            potAdcBufferEl[i] = adcValue;
        }
        potBufferIndexEl = 0;
        potBufferFullEl = true;
        rawCountsEl = adcValue;

        #if DEBUG_SERIAL
            Serial.print(F("✓ Buffer El réinitialisé avec ADC="));
            Serial.println(adcValue);
        #endif
    #endif
}

// ════════════════════════════════════════════════════════════════
// CALIBRATION AZIMUTH
// ════════════════════════════════════════════════════════════════

void calibrateAz(float realDegrees) {
    // Calibration azimuth: définit position courante = angle réel donné
    // Ex: Pointer vers Nord (0°) et envoyer commande "Z0.0"

    #if (ENCODER_AZ_TYPE == ENCODER_POT_MT)
        // ═══════════════════════════════════════════════════════════
        // CALIBRATION POTENTIOMÈTRE MULTI-TOURS (TABLE DE CORRECTION)
        // ═══════════════════════════════════════════════════════════
        // La commande Z calibre UN point de la table:
        // - Reset accumulatedAdcAz = 0 (point de référence)
        // - Enregistre ce point dans la table de correction
        //
        // Pour calibrer d'autres points, utiliser la commande C:
        // C10, C20, C30, etc.

        // Lire ADC actuel
        int currentAdc = analogRead(POT_PIN_AZ);
        #if REVERSE_AZ
            currentAdc = 1023 - currentAdc;
        #endif

        // Reset accumulation à 0 (point de référence)
        accumulatedAdcAz = 0;
        EEPROM.put(EEPROM_TURNS_AZ, accumulatedAdcAz);

        // Reset previousRawAz pour éviter faux delta au prochain cycle
        previousRawAz = currentAdc;

        // Reset buffer filtrage
        resetPotBufferAz(currentAdc);

        // Reset filtre EMA
        filteredAz = realDegrees;
        azFilterInitialized = true;

        // ─────────────────────────────────────────────────────────
        // RECALCUL COMPLET TABLE DE CORRECTION
        // ─────────────────────────────────────────────────────────
        // La commande Z recalcule TOUTE la table avec realDegrees comme référence ADC=0
        // Formule: ADC = (angle - realDegrees) * GEAR_RATIO * 1024 / 360

        const float RATIO_AZ = 4.4;  // Même ratio que dans resetAzCorrectionTable()

        for (int i = 0; i < AZ_TABLE_POINTS; i++) {
            float angle = (float)(i * AZ_TABLE_STEP);  // 0, 10, 20, ... 340
            // ADC relatif à la position de calibration
            azCorrectionTable[i] = (long)((angle - realDegrees) * RATIO_AZ * 1024.0 / 360.0);
        }

        // Sauvegarder dans EEPROM
        for (int i = 0; i < AZ_TABLE_POINTS; i++) {
            EEPROM.put(EEPROM_AZ_TABLE + (i * sizeof(long)), azCorrectionTable[i]);
        }

        // Mise à jour position courante immédiatement
        currentAz = realDegrees;

        #if DEBUG_SERIAL
            Serial.println(F("═══════════════════════════════════════"));
            Serial.print(F("✓ CALIBRATION Az: "));
            Serial.print(realDegrees, 1);
            Serial.println(F("°"));
            Serial.print(F("  ADC actuel: ")); Serial.println(currentAdc);
            Serial.println(F("  Table recalculée avec cette ref:"));
            Serial.print(F("    0° = ADC ")); Serial.println(azCorrectionTable[0]);
            Serial.print(F("    ")); Serial.print((int)realDegrees); Serial.print(F("° = ADC "));
            int refIndex = (int)(realDegrees / AZ_TABLE_STEP);
            Serial.println(azCorrectionTable[refIndex]);
            Serial.print(F("    340° = ADC ")); Serial.println(azCorrectionTable[34]);
            Serial.println(F("═══════════════════════════════════════"));
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

    #if (ENCODER_EL_TYPE == ENCODER_POT_1T)
        // ─────────────────────────────────────────────────────────
        // CALIBRATION POTENTIOMÈTRE 1 TOUR
        // ─────────────────────────────────────────────────────────
        // Pour calibrer à realDegrees:
        // 1. Lire ADC directement (pas la moyenne filtrée)
        // 2. Réinitialiser le buffer avec la valeur ADC actuelle
        // 3. Calculer et stocker l'offset

        // Lire ADC directement pour avoir la valeur actuelle exacte
        int currentAdc = analogRead(POT_PIN_EL);
        #if REVERSE_EL
            currentAdc = 1023 - currentAdc;
        #endif

        // Position actuelle du pot (degrés)
        float potDegrees = ((float)currentAdc / (float)POT_ADC_RESOLUTION) * 360.0;

        // Réinitialiser le buffer avec la valeur ADC actuelle
        resetPotBufferEl(currentAdc);

        // Calcul offset: on veut que (potDegrees - offset) / GEAR_RATIO = realDegrees
        // Donc offset = potDegrees - (realDegrees * GEAR_RATIO)
        float offsetDegrees = potDegrees - (realDegrees * GEAR_RATIO_EL);
        offsetStepsEl = (long)(offsetDegrees * POT_ADC_RESOLUTION / 360.0);

        EEPROM.put(EEPROM_OFFSET_EL, offsetStepsEl);

        // Mise à jour position courante immédiatement
        currentEl = realDegrees;

        #if DEBUG_SERIAL
            Serial.print(F("✓ Calibration El POT_1T: "));
            Serial.print(realDegrees, 1);
            Serial.print(F("° (ADC="));
            Serial.print(currentAdc);
            Serial.print(F(", potDeg="));
            Serial.print(potDegrees, 1);
            Serial.print(F(", offset="));
            Serial.print(offsetStepsEl);
            Serial.println(F(")"));
        #endif

    #elif (ENCODER_EL_TYPE == ENCODER_POT_MT)
        // ═══════════════════════════════════════════════════════════
        // CALIBRATION POTENTIOMÈTRE MULTI-TOURS (TABLE DE CORRECTION)
        // ═══════════════════════════════════════════════════════════
        // Même logique que calibrateAz:
        // - Reset accumulatedAdcEl = 0
        // - Enregistre le point dans la table

        // Lire ADC actuel
        int currentAdc = analogRead(POT_PIN_EL);
        #if REVERSE_EL
            currentAdc = 1023 - currentAdc;
        #endif

        // Reset accumulation à 0
        accumulatedAdcEl = 0;
        EEPROM.put(EEPROM_TURNS_EL, accumulatedAdcEl);

        // Reset buffer filtrage
        resetPotBufferEl(currentAdc);

        // Reset filtre EMA
        filteredEl = realDegrees;
        elFilterInitialized = true;

        // ─────────────────────────────────────────────────────────
        // RECALCUL COMPLET TABLE DE CORRECTION
        // ─────────────────────────────────────────────────────────
        // La commande S recalcule TOUTE la table avec realDegrees comme référence ADC=0
        // Formule: ADC = (angle - realDegrees) * GEAR_RATIO * 1024 / 360

        for (int i = 0; i < EL_TABLE_POINTS; i++) {
            float angle = (float)(i * EL_TABLE_STEP);  // 0, 10, 20, ... 90
            // ADC relatif à la position de calibration
            elCorrectionTable[i] = (long)((angle - realDegrees) * GEAR_RATIO_EL * 1024.0 / 360.0);
        }

        // Sauvegarder dans EEPROM
        for (int i = 0; i < EL_TABLE_POINTS; i++) {
            EEPROM.put(EEPROM_EL_TABLE + (i * sizeof(long)), elCorrectionTable[i]);
        }

        // Mise à jour position courante immédiatement
        currentEl = realDegrees;

        #if DEBUG_SERIAL
            Serial.println(F("═══════════════════════════════════════"));
            Serial.print(F("✓ CALIBRATION El: "));
            Serial.print(realDegrees, 1);
            Serial.println(F("°"));
            Serial.print(F("  ADC actuel: ")); Serial.println(currentAdc);
            Serial.println(F("  Table recalculée avec cette ref:"));
            Serial.print(F("    0° = ADC ")); Serial.println(elCorrectionTable[0]);
            int refIndex = (int)(realDegrees / EL_TABLE_STEP);
            Serial.print(F("    ")); Serial.print((int)realDegrees); Serial.print(F("° = ADC "));
            Serial.println(elCorrectionTable[refIndex]);
            Serial.print(F("    90° = ADC ")); Serial.println(elCorrectionTable[9]);
            Serial.println(F("═══════════════════════════════════════"));
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

    // ─────────────────────────────────────────────────────────────
    // CHARGEMENT AZIMUTH
    // ─────────────────────────────────────────────────────────────
    #if (ENCODER_AZ_TYPE == ENCODER_POT_MT)
        // POT_MT: Charger accumulatedAdcAz (méthode cumulative)
        EEPROM.get(EEPROM_TURNS_AZ, accumulatedAdcAz);
        if (accumulatedAdcAz == -1 || (unsigned long)accumulatedAdcAz == 4294967295UL) {
            accumulatedAdcAz = 0;
            EEPROM.put(EEPROM_TURNS_AZ, accumulatedAdcAz);
        }
    #else
        // SSI/autres: Charger turnsAz (compteur de tours)
        EEPROM.get(EEPROM_TURNS_AZ, turnsAz);
        if (turnsAz == -1 || (unsigned long)turnsAz == 4294967295UL) {
            turnsAz = 0;
            EEPROM.put(EEPROM_TURNS_AZ, turnsAz);
        }
    #endif

    EEPROM.get(EEPROM_OFFSET_AZ, offsetStepsAz);
    if (offsetStepsAz == -1 || (unsigned long)offsetStepsAz == 4294967295UL) {
        offsetStepsAz = 0;
        EEPROM.put(EEPROM_OFFSET_AZ, offsetStepsAz);
    }

    // ─────────────────────────────────────────────────────────────
    // CHARGEMENT ÉLÉVATION
    // ─────────────────────────────────────────────────────────────
    #if (ENCODER_EL_TYPE == ENCODER_POT_MT)
        // POT_MT: Charger accumulatedAdcEl (méthode cumulative)
        EEPROM.get(EEPROM_TURNS_EL, accumulatedAdcEl);
        if (accumulatedAdcEl == -1 || (unsigned long)accumulatedAdcEl == 4294967295UL) {
            accumulatedAdcEl = 0;
            EEPROM.put(EEPROM_TURNS_EL, accumulatedAdcEl);
        }
    #else
        // SSI/autres: Charger turnsEl
        EEPROM.get(EEPROM_TURNS_EL, turnsEl);
        if (turnsEl == -1 || (unsigned long)turnsEl == 4294967295UL) {
            turnsEl = 0;
            EEPROM.put(EEPROM_TURNS_EL, turnsEl);
        }
    #endif

    EEPROM.get(EEPROM_OFFSET_EL, offsetStepsEl);
    if (offsetStepsEl == -1 || (unsigned long)offsetStepsEl == 4294967295UL) {
        offsetStepsEl = 0;
        EEPROM.put(EEPROM_OFFSET_EL, offsetStepsEl);
    }

    #if DEBUG_SERIAL
        Serial.println(F("Calibration chargée depuis EEPROM:"));
        #if (ENCODER_AZ_TYPE == ENCODER_POT_MT)
            Serial.print(F("  accumulatedAdcAz: ")); Serial.println(accumulatedAdcAz);
        #else
            Serial.print(F("  turnsAz: ")); Serial.println(turnsAz);
        #endif
        #if (ENCODER_EL_TYPE == ENCODER_POT_MT)
            Serial.print(F("  accumulatedAdcEl: ")); Serial.println(accumulatedAdcEl);
        #else
            Serial.print(F("  turnsEl: ")); Serial.println(turnsEl);
        #endif
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

// ════════════════════════════════════════════════════════════════
// TABLE DE CORRECTION AZIMUTH
// ════════════════════════════════════════════════════════════════

void loadAzCorrectionTable() {
    // Chargement table correction depuis EEPROM
    // Si premier élément = 0xFFFFFFFF (EEPROM vierge), initialiser avec valeurs linéaires

    long firstValue;
    EEPROM.get(EEPROM_AZ_TABLE, firstValue);

    if (firstValue == -1 || (unsigned long)firstValue == 4294967295UL) {
        // Table vierge → initialiser avec valeurs linéaires basées sur GEAR_RATIO_AZ
        #if DEBUG_SERIAL
            Serial.println(F("Table correction Az vierge → initialisation linéaire"));
        #endif
        resetAzCorrectionTable();
    } else {
        // Charger table depuis EEPROM
        for (int i = 0; i < AZ_TABLE_POINTS; i++) {
            EEPROM.get(EEPROM_AZ_TABLE + (i * sizeof(long)), azCorrectionTable[i]);
        }

        #if DEBUG_SERIAL
            Serial.println(F("Table correction Az chargée depuis EEPROM"));
            Serial.print(F("  Point 0° = ")); Serial.println(azCorrectionTable[0]);
            Serial.print(F("  Point 170° = ")); Serial.println(azCorrectionTable[17]);
            Serial.print(F("  Point 340° = ")); Serial.println(azCorrectionTable[34]);
        #endif
    }

    azTableLoaded = true;
}

void resetAzCorrectionTable() {
    // Initialiser table avec valeurs linéaires
    // GEAR_RATIO_AZ = 4.4 (codé en dur pour éviter problèmes de cache VSCode)
    // Formule: accumulatedAdc = angle * 4.4 * 1024 / 360

    const float RATIO_AZ = 4.4;  // Codé en dur ici!

    for (int i = 0; i < AZ_TABLE_POINTS; i++) {
        float angle = (float)(i * AZ_TABLE_STEP);  // 0, 10, 20, ... 340
        // ADC cumulé pour atteindre cet angle (formule linéaire)
        azCorrectionTable[i] = (long)(angle * RATIO_AZ * 1024.0 / 360.0);
    }

    // Sauvegarder dans EEPROM
    for (int i = 0; i < AZ_TABLE_POINTS; i++) {
        EEPROM.put(EEPROM_AZ_TABLE + (i * sizeof(long)), azCorrectionTable[i]);
    }

    #if DEBUG_SERIAL
        Serial.println(F("═══════════════════════════════════════"));
        Serial.println(F("Table correction Az réinitialisée (linéaire)"));
        Serial.print(F("  RATIO_AZ = ")); Serial.println(RATIO_AZ);
        Serial.print(F("  Point 0° = ADC ")); Serial.println(azCorrectionTable[0]);
        Serial.print(F("  Point 170° = ADC ")); Serial.println(azCorrectionTable[17]);
        Serial.print(F("  Point 340° = ADC ")); Serial.println(azCorrectionTable[34]);
        Serial.println(F("═══════════════════════════════════════"));
    #endif
}

float adcToDegrees(long accumulatedAdc) {
    // Conversion ADC cumulé → degrés avec interpolation linéaire
    //
    // 1. Trouver les deux points de la table qui encadrent l'ADC
    // 2. Interpoler linéairement entre ces deux points
    //
    // Cas spéciaux:
    // - ADC < point 0: extrapolation avant (négatif)
    // - ADC > point 34: extrapolation après (>340°)

    // Cas trivial: table non chargée → fallback linéaire
    if (!azTableLoaded) {
        float potDegreesTotal = (float)accumulatedAdc * 360.0 / 1024.0;
        return potDegreesTotal / GEAR_RATIO_AZ;
    }

    // Trouver l'intervalle contenant cet ADC
    int lowerIndex = -1;
    int upperIndex = -1;

    for (int i = 0; i < AZ_TABLE_POINTS - 1; i++) {
        if (accumulatedAdc >= azCorrectionTable[i] && accumulatedAdc <= azCorrectionTable[i + 1]) {
            lowerIndex = i;
            upperIndex = i + 1;
            break;
        }
    }

    // Cas: ADC avant le premier point (ex: valeurs négatives)
    if (lowerIndex == -1 && accumulatedAdc < azCorrectionTable[0]) {
        // Extrapolation linéaire basée sur les deux premiers points
        lowerIndex = 0;
        upperIndex = 1;
    }

    // Cas: ADC après le dernier point
    if (lowerIndex == -1 && accumulatedAdc > azCorrectionTable[AZ_TABLE_POINTS - 1]) {
        // Extrapolation linéaire basée sur les deux derniers points
        lowerIndex = AZ_TABLE_POINTS - 2;
        upperIndex = AZ_TABLE_POINTS - 1;
    }

    // Sécurité: si toujours pas trouvé, utiliser premier intervalle
    if (lowerIndex == -1) {
        lowerIndex = 0;
        upperIndex = 1;
    }

    // Interpolation linéaire
    long adcLow = azCorrectionTable[lowerIndex];
    long adcHigh = azCorrectionTable[upperIndex];
    float angleLow = (float)(lowerIndex * AZ_TABLE_STEP);   // ex: 170°
    float angleHigh = (float)(upperIndex * AZ_TABLE_STEP);  // ex: 180°

    // Éviter division par zéro
    if (adcHigh == adcLow) {
        return angleLow;
    }

    // Interpolation: angle = angleLow + (adc - adcLow) * (angleHigh - angleLow) / (adcHigh - adcLow)
    float ratio = (float)(accumulatedAdc - adcLow) / (float)(adcHigh - adcLow);
    float interpolatedAngle = angleLow + ratio * (angleHigh - angleLow);

    return interpolatedAngle;
}

void calibrateAzTablePoint(float realDegrees) {
    // Calibration d'un point de la table
    // Arrondit au point le plus proche (multiple de 10°)
    //
    // Exemple: realDegrees = 173.5 → calibre point 170° (index 17)
    // Enregistre la valeur ADC cumulée actuelle pour ce point

    // Arrondir au multiple de 10 le plus proche
    int pointIndex = (int)((realDegrees + 5.0) / AZ_TABLE_STEP);

    // Limiter à la plage valide
    if (pointIndex < 0) pointIndex = 0;
    if (pointIndex >= AZ_TABLE_POINTS) pointIndex = AZ_TABLE_POINTS - 1;

    int calibratedAngle = pointIndex * AZ_TABLE_STEP;

    // Enregistrer la valeur ADC cumulée actuelle pour ce point
    azCorrectionTable[pointIndex] = accumulatedAdcAz;

    // Sauvegarder dans EEPROM
    EEPROM.put(EEPROM_AZ_TABLE + (pointIndex * sizeof(long)), azCorrectionTable[pointIndex]);

    // Mettre à jour position courante immédiatement
    currentAz = (float)calibratedAngle;
    filteredAz = currentAz;

    #if DEBUG_SERIAL
        Serial.println(F("═══════════════════════════════════════"));
        Serial.print(F("✓ CALIBRATION TABLE Az point "));
        Serial.print(calibratedAngle);
        Serial.println(F("°"));
        Serial.print(F("  Demandé: ")); Serial.print(realDegrees, 1); Serial.println(F("°"));
        Serial.print(F("  Index: ")); Serial.println(pointIndex);
        Serial.print(F("  ADC cumulé: ")); Serial.println(accumulatedAdcAz);
        Serial.println(F("═══════════════════════════════════════"));
    #endif
}

void printAzCorrectionTable() {
    // Affichage complet de la table de correction via sendToClient
    sendToClient("\r\n");
    sendToClient("=== TABLE CORRECTION AZIMUTH (35 points) ===\r\n");
    sendToClient("Angle  ADC_cumule  Delta\r\n");

    for (int i = 0; i < AZ_TABLE_POINTS; i++) {
        int angle = i * AZ_TABLE_STEP;
        long adc = azCorrectionTable[i];

        String line = "";
        if (angle < 100) line += " ";
        if (angle < 10) line += " ";
        line += String(angle) + "   ";

        if (adc >= 0) line += " ";
        line += String(adc);

        // Delta depuis point précédent
        if (i > 0) {
            long delta = azCorrectionTable[i] - azCorrectionTable[i - 1];
            line += "  ";
            if (delta >= 0) line += "+";
            line += String(delta);
        }

        line += "\r\n";
        sendToClient(line);
    }

    String status = "Pos: " + String(currentAz, 1) + "  ADC: " + String(accumulatedAdcAz) + "\r\n";
    sendToClient(status);
}

// ════════════════════════════════════════════════════════════════
// TABLE DE CORRECTION ÉLÉVATION
// ════════════════════════════════════════════════════════════════

void loadElCorrectionTable() {
    // Chargement table correction élévation depuis EEPROM

    long firstValue;
    EEPROM.get(EEPROM_EL_TABLE, firstValue);

    if (firstValue == -1 || (unsigned long)firstValue == 4294967295UL) {
        #if DEBUG_SERIAL
            Serial.println(F("Table correction El vierge → initialisation linéaire"));
        #endif
        resetElCorrectionTable();
    } else {
        for (int i = 0; i < EL_TABLE_POINTS; i++) {
            EEPROM.get(EEPROM_EL_TABLE + (i * sizeof(long)), elCorrectionTable[i]);
        }

        #if DEBUG_SERIAL
            Serial.println(F("Table correction El chargée depuis EEPROM"));
            Serial.print(F("  Point 0° = ")); Serial.println(elCorrectionTable[0]);
            Serial.print(F("  Point 50° = ")); Serial.println(elCorrectionTable[5]);
            Serial.print(F("  Point 90° = ")); Serial.println(elCorrectionTable[9]);
        #endif
    }

    elTableLoaded = true;
}

void resetElCorrectionTable() {
    // Initialiser table avec valeurs linéaires basées sur GEAR_RATIO_EL

    for (int i = 0; i < EL_TABLE_POINTS; i++) {
        float angle = (float)(i * EL_TABLE_STEP);  // 0, 10, 20, ... 90
        elCorrectionTable[i] = (long)(angle * GEAR_RATIO_EL * 1024.0 / 360.0);
    }

    // Sauvegarder dans EEPROM
    for (int i = 0; i < EL_TABLE_POINTS; i++) {
        EEPROM.put(EEPROM_EL_TABLE + (i * sizeof(long)), elCorrectionTable[i]);
    }

    #if DEBUG_SERIAL
        Serial.println(F("═══════════════════════════════════════"));
        Serial.println(F("Table correction El réinitialisée (linéaire)"));
        Serial.print(F("  GEAR_RATIO_EL = ")); Serial.println(GEAR_RATIO_EL);
        Serial.print(F("  Point 0° = ADC ")); Serial.println(elCorrectionTable[0]);
        Serial.print(F("  Point 50° = ADC ")); Serial.println(elCorrectionTable[5]);
        Serial.print(F("  Point 90° = ADC ")); Serial.println(elCorrectionTable[9]);
        Serial.println(F("═══════════════════════════════════════"));
    #endif
}

float adcToDegreesEl(long accumulatedAdc) {
    // Conversion ADC cumulé → degrés élévation avec interpolation linéaire

    if (!elTableLoaded) {
        float potDegreesTotal = (float)accumulatedAdc * 360.0 / 1024.0;
        return potDegreesTotal / GEAR_RATIO_EL;
    }

    // Trouver l'intervalle contenant cet ADC
    int lowerIndex = -1;
    int upperIndex = -1;

    for (int i = 0; i < EL_TABLE_POINTS - 1; i++) {
        if (accumulatedAdc >= elCorrectionTable[i] && accumulatedAdc <= elCorrectionTable[i + 1]) {
            lowerIndex = i;
            upperIndex = i + 1;
            break;
        }
    }

    // ADC avant premier point
    if (lowerIndex == -1 && accumulatedAdc < elCorrectionTable[0]) {
        lowerIndex = 0;
        upperIndex = 1;
    }

    // ADC après dernier point
    if (lowerIndex == -1 && accumulatedAdc > elCorrectionTable[EL_TABLE_POINTS - 1]) {
        lowerIndex = EL_TABLE_POINTS - 2;
        upperIndex = EL_TABLE_POINTS - 1;
    }

    if (lowerIndex == -1) {
        lowerIndex = 0;
        upperIndex = 1;
    }

    // Interpolation linéaire
    long adcLow = elCorrectionTable[lowerIndex];
    long adcHigh = elCorrectionTable[upperIndex];
    float angleLow = (float)(lowerIndex * EL_TABLE_STEP);
    float angleHigh = (float)(upperIndex * EL_TABLE_STEP);

    if (adcHigh == adcLow) {
        return angleLow;
    }

    float ratio = (float)(accumulatedAdc - adcLow) / (float)(adcHigh - adcLow);
    return angleLow + ratio * (angleHigh - angleLow);
}

void calibrateElTablePoint(float realDegrees) {
    // Calibration d'un point de la table élévation

    int pointIndex = (int)((realDegrees + 5.0) / EL_TABLE_STEP);
    if (pointIndex < 0) pointIndex = 0;
    if (pointIndex >= EL_TABLE_POINTS) pointIndex = EL_TABLE_POINTS - 1;

    int calibratedAngle = pointIndex * EL_TABLE_STEP;

    elCorrectionTable[pointIndex] = accumulatedAdcEl;
    EEPROM.put(EEPROM_EL_TABLE + (pointIndex * sizeof(long)), elCorrectionTable[pointIndex]);

    currentEl = (float)calibratedAngle;
    filteredEl = currentEl;

    #if DEBUG_SERIAL
        Serial.println(F("═══════════════════════════════════════"));
        Serial.print(F("✓ CALIBRATION TABLE El point "));
        Serial.print(calibratedAngle);
        Serial.println(F("°"));
        Serial.print(F("  Demandé: ")); Serial.print(realDegrees, 1); Serial.println(F("°"));
        Serial.print(F("  Index: ")); Serial.println(pointIndex);
        Serial.print(F("  ADC cumulé: ")); Serial.println(accumulatedAdcEl);
        Serial.println(F("═══════════════════════════════════════"));
    #endif
}

void printElCorrectionTable() {
    // Affichage complet de la table de correction via sendToClient
    sendToClient("\r\n");
    sendToClient("=== TABLE CORRECTION ELEVATION (10 points) ===\r\n");
    sendToClient("Angle  ADC_cumule  Delta\r\n");

    for (int i = 0; i < EL_TABLE_POINTS; i++) {
        int angle = i * EL_TABLE_STEP;
        long adc = elCorrectionTable[i];

        String line = "";
        if (angle < 10) line += " ";
        line += String(angle) + "   ";

        if (adc >= 0) line += " ";
        line += String(adc);

        // Delta depuis point précédent
        if (i > 0) {
            long delta = elCorrectionTable[i] - elCorrectionTable[i - 1];
            line += "  ";
            if (delta >= 0) line += "+";
            line += String(delta);
        }

        line += "\r\n";
        sendToClient(line);
    }

    String status = "Pos: " + String(currentEl, 1) + "  ADC: " + String(accumulatedAdcEl) + "\r\n";
    sendToClient(status);
}
