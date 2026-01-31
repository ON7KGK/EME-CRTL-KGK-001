// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Moteurs Pas-à-Pas (TB6600)
// ════════════════════════════════════════════════════════════════
// Fichier: motor_stepper.h
// Description: Contrôle moteurs pas-à-pas NEMA23 avec drivers TB6600
//              Asservissement position avec vitesse variable
// ════════════════════════════════════════════════════════════════
// ÉTAPE 2 : Contrôle moteurs pas-à-pas
// ÉTAPE 3 : Boutons manuels
// ════════════════════════════════════════════════════════════════

#ifndef MOTOR_STEPPER_H
#define MOTOR_STEPPER_H

#include <Arduino.h>
#include "config.h"

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES COMMANDE
// ════════════════════════════════════════════════════════════════

// Position cible (commande PstRotator ou boutons)
// -1.0 = pas de cible active (idle/arrêt)
extern float targetAz;
extern float targetEl;

// État mouvement
extern bool movingAz;
extern bool movingEl;

// ════════════════════════════════════════════════════════════════
// FONCTIONS PUBLIQUES
// ════════════════════════════════════════════════════════════════

/**
 * Initialisation module moteurs pas-à-pas
 * - Configure pins STEP, DIR en OUTPUT
 * - État initial: arrêt (pas de mouvement)
 */
void setupMotors();

/**
 * Mise à jour contrôle moteurs (appelé dans loop)
 * - Calcul erreur position (targetAz/El - currentAz/El)
 * - Asservissement avec vitesse variable
 * - Normalisation angles (-180° à +180°)
 *
 * Logique vitesse:
 * - Erreur > SPEED_SWITCH_THRESHOLD (3°) → vitesse rapide (SPEED_MAX)
 * - Erreur < SPEED_SWITCH_THRESHOLD → vitesse lente (SPEED_SLOW)
 * - Erreur < POSITION_TOLERANCE (0.08°) → arrêt, target = -1
 */
void updateMotorControl();

/**
 * Exécution d'un step moteur avec vérification fin de course
 *
 * @param pinStep   Pin STEP du driver
 * @param pinDir    Pin DIR du driver
 * @param direction Direction (HIGH=CW/UP, LOW=CCW/DOWN)
 * @param delayUs   Délai entre pulses (µs) - détermine vitesse
 * @param limitPin  Pin fin de course (NC: LOW = limite atteinte)
 *
 * SÉCURITÉ: Si fin de course détectée (LOW), le step est ignoré
 *
 * Timing TB6600:
 * - Pulse HIGH min: 2.5µs
 * - Pulse LOW min: 2.5µs
 * - On utilise delayUs pour chaque phase (SPEED_MAX=400µs, SPEED_SLOW=4000µs)
 */
void doStep(int pinStep, int pinDir, int direction, int delayUs, int limitPin);

/**
 * Exécution d'un step moteur (sans vérification fin de course)
 * Pour compatibilité - préférer version avec limitPin
 */
void doStep(int pinStep, int pinDir, int direction, int delayUs);

/**
 * Arrêt immédiat tous moteurs
 * Met targetAz et targetEl à -1 (pas de cible active)
 * Appelé par bouton STOP ou commande Easycom "S\r"
 */
void stopAllMotors();

/**
 * Vérification boutons manuels
 * - BTN_CW / BTN_CCW : Mouvement azimuth par petits pas
 * - BTN_UP / BTN_DOWN : Mouvement élévation par petits pas
 * - BTN_STOP : Arrêt immédiat
 *
 * Mouvements manuels:
 * - MANUAL_STEP_SIZE steps par appui (défaut: 20)
 * - Vitesse lente (SPEED_SLOW) pour précision
 * - Débouncing logiciel (BUTTON_DEBOUNCE_DELAY)
 */
void checkManualButtons();

/**
 * Calcul erreur angulaire normalisée
 *
 * @param target Angle cible (degrés)
 * @param current Angle courant (degrés)
 * @return Erreur -180° à +180° (chemin le plus court)
 *
 * Exemples:
 * - target=10°, current=350° → erreur = +20° (pas -340°)
 * - target=350°, current=10° → erreur = -20° (pas +340°)
 */
float calculateAngularError(float target, float current);

/**
 * Calcul nombre de steps nécessaires pour atteindre angle
 *
 * @param angleDegrees Angle à parcourir (degrés)
 * @param gearRatio    Rapport de réduction mécanique
 * @return Nombre de steps moteur
 *
 * Formule:
 * steps = (angleDegrees / 360.0) * STEPS_PER_REV_MOTOR * gearRatio
 */
long calculateStepsForAngle(float angleDegrees, float gearRatio);

/**
 * Affichage debug moteurs (Serial)
 * Format: "Target Az: 123.5° | Current: 120.0° | Error: 3.5° | Moving: YES"
 */
void printMotorDebug();

#endif // MOTOR_STEPPER_H
