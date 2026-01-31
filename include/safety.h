// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Sécurité (Fins de course)
// ════════════════════════════════════════════════════════════════
// Fichier: safety.h
// Description: Gestion fins de course NC (Normally Closed)
//              Sécurité matérielle anti-dépassement
// ════════════════════════════════════════════════════════════════
// ÉTAPE 4 : Test fins de course (sécurité NC)
// ════════════════════════════════════════════════════════════════

#ifndef SAFETY_H
#define SAFETY_H

#include <Arduino.h>
#include "config.h"

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES SÉCURITÉ
// ════════════════════════════════════════════════════════════════

// État fins de course
extern bool limitAzTriggered;
extern bool limitElTriggered;

// ════════════════════════════════════════════════════════════════
// FONCTIONS PUBLIQUES
// ════════════════════════════════════════════════════════════════

/**
 * Initialisation module sécurité
 * - Configure pins LIMIT_AZ, LIMIT_EL en INPUT_PULLUP
 * - Vérification état initial (switches fermés = sécurité OK)
 */
void setupLimits();

/**
 * Vérification fins de course (appelé avant chaque mouvement)
 * - Lecture pins LIMIT_AZ, LIMIT_EL
 * - Mise à jour flags limitAzTriggered, limitElTriggered
 *
 * Logique NC (Normally Closed):
 * - Pin HIGH (5V) = Limite atteinte (circuit ouvert) → BLOCAGE
 * - Pin LOW (GND) = Normal (circuit fermé) → Mouvement autorisé
 *
 * IMPORTANT: Inversé car pull-up + NC
 * - État normal (switches fermés): pin lit HIGH
 * - État alarme (switch ouvert): pin lit LOW
 *
 * ATTENTION: Vérifier câblage réel! Peut varier selon montage.
 */
void checkLimits();

/**
 * Vérification limite azimuth
 *
 * @return true si mouvement sûr (limite non atteinte)
 *         false si limite atteinte (bloquer mouvement)
 */
bool isAzimuthSafe();

/**
 * Vérification limite élévation
 *
 * @return true si mouvement sûr (limite non atteinte)
 *         false si limite atteinte (bloquer mouvement)
 */
bool isElevationSafe();

/**
 * Affichage debug sécurité (Serial)
 * Format: "LIMITS - Az: OK | El: TRIGGERED"
 */
void printSafetyDebug();

#endif // SAFETY_H
