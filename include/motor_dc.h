// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Moteurs DC Brushed (MC33926)
// ════════════════════════════════════════════════════════════════
// Fichier: motor_dc.h
// Description: Contrôle moteurs DC brushed avec driver MC33926
//              Pour rotator SVH3 (futur)
//              Mode Sign-Magnitude: IN1=PWM, IN2=Direction
// ════════════════════════════════════════════════════════════════
// ÉTAPE 7 : Intégration moteurs DC (OPTIONNEL - Futur SVH3)
// ════════════════════════════════════════════════════════════════

#ifndef MOTOR_DC_H
#define MOTOR_DC_H

#include <Arduino.h>
#include "config.h"

// ════════════════════════════════════════════════════════════════
// STRUCTURES PID
// ════════════════════════════════════════════════════════════════

// Contrôleur PID pour asservissement vitesse
struct PIDController {
    float kp;           // Gain proportionnel
    float ki;           // Gain intégral
    float kd;           // Gain dérivé

    float integral;     // Accumulation erreur intégrale
    float lastError;    // Erreur précédente (calcul dérivée)

    unsigned long lastTime;  // Timestamp dernier calcul
};

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES
// ════════════════════════════════════════════════════════════════

// Contrôleurs PID
extern PIDController pidAz;
extern PIDController pidEl;

// État moteurs DC
extern int currentPWM_Az;
extern int currentPWM_El;

// ════════════════════════════════════════════════════════════════
// FONCTIONS PUBLIQUES
// ════════════════════════════════════════════════════════════════

/**
 * Initialisation module moteurs DC
 * - Configure pins PWM (IN1), DIR (IN2), DISABLE (D2)
 * - Configure pins feedback (SF status flag, FB current)
 * - Initialise contrôleurs PID
 * - État initial: moteurs désactivés
 */
void setupMotorsDC();

/**
 * Mise à jour contrôle moteurs DC (appelé dans loop)
 * - Calcul erreur position (similaire stepper)
 * - Calcul vitesse cible via PID
 * - Application PWM + direction
 * - Monitoring courant et status flags
 */
void updateMotorControlDC();

/**
 * Commande moteur DC (PWM + direction)
 *
 * @param motor      1=Azimuth, 2=Élévation
 * @param pwmValue   Valeur PWM 0-255 (vitesse)
 * @param direction  HIGH=CW/UP, LOW=CCW/DOWN
 *
 * Mode Sign-Magnitude MC33926:
 * - IN1 (PWM): 0-255 vitesse
 * - IN2 (DIR): HIGH ou LOW direction
 * - D2: LOW=enable, HIGH=disable
 */
void setMotorDC(int motor, int pwmValue, int direction);

/**
 * Arrêt moteur DC
 *
 * @param motor 1=Azimuth, 2=Élévation
 *
 * Met PWM à 0 et désactive driver (D2=HIGH)
 */
void stopMotorDC(int motor);

/**
 * Arrêt immédiat tous moteurs DC
 */
void stopAllMotorsDC();

/**
 * Calcul sortie PID
 *
 * @param pid          Référence contrôleur PID
 * @param setpoint     Consigne (position ou vitesse cible)
 * @param measurement  Mesure actuelle
 * @return Sortie PID (PWM 0-255)
 *
 * Formule PID classique:
 * output = Kp*error + Ki*∫error + Kd*(d/dt error)
 */
float calculatePID(PIDController &pid, float setpoint, float measurement);

/**
 * Reset contrôleur PID
 * Remet à zéro intégrale et erreur précédente
 *
 * @param pid Référence contrôleur PID
 */
void resetPID(PIDController &pid);

/**
 * Lecture courant moteur (feedback MC33926)
 *
 * @param motor 1=Azimuth, 2=Élévation
 * @return Courant en mA (lecture analogique FB pin)
 *
 * MC33926: 525 mV/A
 * Formule: current_mA = (analogRead(FB_pin) * 5000.0 / 1023.0) / 0.525
 */
float readMotorCurrent(int motor);

/**
 * Vérification status flag (détection fault)
 *
 * @param motor 1=Azimuth, 2=Élévation
 * @return true si OK, false si fault (SF=LOW)
 *
 * Faults MC33926: surintensité, surchauffe, court-circuit
 * Action: arrêter moteur et log erreur
 */
bool checkMotorStatus(int motor);

/**
 * Affichage debug moteurs DC (Serial)
 * Format: "Az PWM: 128 (50%) | Current: 1200mA | Status: OK"
 */
void printMotorDCDebug();

#endif // MOTOR_DC_H
