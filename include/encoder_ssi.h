// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Encodeurs SSI
// ════════════════════════════════════════════════════════════════
// Fichier: encoder_ssi.h
// Description: Interface pour encodeurs SSI (Synchronous Serial Interface)
//              Support HH-12 SSI (absolu 12-bit) et HH-12 INC SSI (incrémental)
// ════════════════════════════════════════════════════════════════
// ÉTAPE 1 : Lecture position encodeurs
// ════════════════════════════════════════════════════════════════

#ifndef ENCODER_SSI_H
#define ENCODER_SSI_H

#include <Arduino.h>
#include "config.h"

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES POSITION
// ════════════════════════════════════════════════════════════════

// Position courante en degrés
extern float currentAz;  // Position azimuth (-∞ à +∞, peut faire plusieurs tours)
extern float currentEl;  // Position élévation (typiquement 0-90°)

// Position brute encodeurs (0-4095 counts)
extern int rawCountsAz;
extern int rawCountsEl;

// Tracking tours pour encodeur incrémental
extern long turnsAz;     // Nombre de tours azimuth (sauvegardé EEPROM)
extern long turnsEl;     // Nombre de tours élévation (optionnel)

// Accumulation ADC pour méthode cumulative (POT_MT)
extern long accumulatedAdcAz;  // ADC cumulé azimuth (sauvegardé EEPROM)
extern long accumulatedAdcEl;  // ADC cumulé élévation (optionnel)

// Offsets calibration (en steps absolus, sauvegardés EEPROM)
extern long offsetStepsAz;
extern long offsetStepsEl;

// ════════════════════════════════════════════════════════════════
// FONCTIONS PUBLIQUES
// ════════════════════════════════════════════════════════════════

/**
 * Initialisation module encodeurs SSI
 * - Configure pins SSI (CLK, CS, DATA)
 * - Charge calibration depuis EEPROM
 * - Effectue première lecture position
 */
void setupEncoders();

/**
 * Mise à jour position encodeurs (appelé dans loop)
 * - Lit valeurs SSI brutes
 * - Calcule position en degrés
 * - Tracking tours pour encodeur incrémental
 *
 * Appelé toutes les ENCODER_READ_INTERVAL ms (20ms défaut)
 */
void updateEncoders();

/**
 * Lecture encodeur SSI absolu (HH-12 SSI)
 *
 * @param csPin   Pin Chip Select de l'encodeur
 * @param dataPin Pin Data de l'encodeur
 * @param reverse Inverser sens lecture (true/false)
 * @return Valeur brute 0-4095
 *
 * Protocole SSI:
 * - 18 pulses CLK total (12 bits data + 6 bits status/parity)
 * - Data valide pendant CLK LOW
 * - CS LOW pour activer transmission
 */
int readSSI_Absolute(int csPin, int dataPin, bool reverse);

/**
 * Lecture encodeur SSI incrémental (HH-12 INC SSI)
 *
 * @param csPin   Pin Chip Select de l'encodeur
 * @param dataPin Pin Data de l'encodeur
 * @param reverse Inverser sens lecture (true/false)
 * @param turns   Référence au compteur de tours (modifié par fonction)
 * @return Valeur brute 0-4095 du tour courant
 *
 * Tracking tours:
 * - Si delta > 2048 (saut 4095→0) → turns++
 * - Si delta < -2048 (saut 0→4095) → turns--
 */
int readSSI_Incremental(int csPin, int dataPin, bool reverse, long &turns);

/**
 * Conversion counts bruts → position degrés
 *
 * @param rawCounts Valeur brute encodeur (0-4095)
 * @param turns     Nombre de tours (pour encodeur incrémental)
 * @param offset    Offset calibration (steps absolus)
 * @param gearRatio Rapport de réduction mécanique
 * @return Position en degrés
 *
 * Formule:
 * stepsAbsolus = (turns * 4096) + rawCounts - offset
 * degrees = (stepsAbsolus / 4096.0) * (360.0 / gearRatio)
 */
float convertCountsToDegrees(int rawCounts, long turns, long offset, float gearRatio);

/**
 * Calibration azimuth
 * Enregistre position actuelle comme angle de référence
 *
 * @param realDegrees Angle réel mesuré manuellement (boussole, etc.)
 *
 * Commande Easycom: "Z123.5\r" → calibre azimuth à 123.5°
 * Sauvegarde offset dans EEPROM
 */
void calibrateAz(float realDegrees);

/**
 * Calibration élévation
 * Enregistre position actuelle comme angle de référence
 *
 * @param realDegrees Angle réel mesuré manuellement (inclinomètre, etc.)
 *
 * Commande Easycom: "E45.0\r" → calibre élévation à 45.0°
 * Sauvegarde offset dans EEPROM
 */
void calibrateEl(float realDegrees);

/**
 * Chargement calibration depuis EEPROM
 * - turnsAz
 * - offsetStepsAz
 * - offsetStepsEl
 *
 * Vérifie valeurs non initialisées (0xFFFFFFFF) et reset à 0
 */
void loadCalibrationFromEEPROM();

/**
 * Sauvegarde calibration dans EEPROM
 * - turnsAz
 * - offsetStepsAz
 * - offsetStepsEl
 */
void saveCalibrationToEEPROM();

/**
 * Affichage debug position (Serial)
 * Format: "Az: 123.5° (raw:2048, turns:1) | El: 45.0° (raw:1024)"
 */
void printEncoderDebug();

/**
 * Réinitialisation buffer filtrage potentiomètre azimuth
 * @param adcValue Valeur ADC pour remplir le buffer
 *
 * CRITIQUE: Appelé lors de la calibration pour assurer
 * la cohérence entre l'offset calculé et le buffer de filtrage.
 * Sans cette réinitialisation, la moyenne filtrée diffère de
 * la valeur utilisée pour calculer l'offset, causant une
 * position incorrecte après calibration.
 */
void resetPotBufferAz(int adcValue);

/**
 * Réinitialisation buffer filtrage potentiomètre élévation
 * @param adcValue Valeur ADC pour remplir le buffer
 *
 * CRITIQUE: Même logique que resetPotBufferAz() pour l'élévation.
 */
void resetPotBufferEl(int adcValue);

// ════════════════════════════════════════════════════════════════
// TABLE DE CORRECTION AZIMUTH (Compensation non-linéarité)
// ════════════════════════════════════════════════════════════════

/**
 * Chargement table correction depuis EEPROM
 * Si table vide (0xFFFFFFFF), initialise avec valeurs linéaires
 */
void loadAzCorrectionTable();

/**
 * Calibration d'un point de la table
 * @param realDegrees Angle réel mesuré (0, 10, 20, ... 340)
 *
 * Enregistre la valeur ADC cumulée actuelle pour cet angle.
 * Commande Easycom: "C45" calibre le point 40° (arrondi au plus proche)
 */
void calibrateAzTablePoint(float realDegrees);

/**
 * Conversion ADC cumulé → degrés avec interpolation table
 * @param accumulatedAdc Valeur ADC cumulée depuis calibration
 * @return Angle en degrés (interpolé entre points de la table)
 */
float adcToDegrees(long accumulatedAdc);

/**
 * Affichage table correction complète (debug)
 * Format: "Point 0: ADC=0 → 0°"
 */
void printAzCorrectionTable();

/**
 * Reset table à valeurs linéaires (basées sur GEAR_RATIO_AZ)
 */
void resetAzCorrectionTable();

// ════════════════════════════════════════════════════════════════
// TABLE DE CORRECTION ÉLÉVATION (Compensation non-linéarité)
// ════════════════════════════════════════════════════════════════

/**
 * Chargement table correction élévation depuis EEPROM
 */
void loadElCorrectionTable();

/**
 * Calibration d'un point de la table élévation
 * @param realDegrees Angle réel mesuré (0, 10, 20, ... 90)
 */
void calibrateElTablePoint(float realDegrees);

/**
 * Conversion ADC cumulé → degrés élévation avec interpolation table
 */
float adcToDegreesEl(long accumulatedAdc);

/**
 * Affichage table correction élévation complète
 */
void printElCorrectionTable();

/**
 * Reset table élévation à valeurs linéaires
 */
void resetElCorrectionTable();

// ════════════════════════════════════════════════════════════════
// FONCTIONS INTERNES (Helpers)
// ════════════════════════════════════════════════════════════════

/**
 * Détection transition tour (pour encodeur incrémental)
 *
 * @param currentRaw  Valeur brute actuelle
 * @param previousRaw Valeur brute précédente
 * @return Changement tours (+1, 0, -1)
 *
 * Logique:
 * - Si delta > 2048 → passage 4095→0 → +1 tour
 * - Si delta < -2048 → passage 0→4095 → -1 tour
 */
int detectTurnTransition(int currentRaw, int previousRaw);

#endif // ENCODER_SSI_H
