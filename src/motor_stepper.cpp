// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Moteurs Pas-à-Pas (Implementation)
// ════════════════════════════════════════════════════════════════
// Fichier: motor_stepper.cpp
// Description: Contrôle moteurs TB6600 - Code fonctionnel K3NG
// ════════════════════════════════════════════════════════════════

#include "motor_stepper.h"
#include "encoder_ssi.h"  // Pour accès currentAz, currentEl

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES
// ════════════════════════════════════════════════════════════════

// Position cible (-1 = pas de cible active)
float targetAz = -1.0;
float targetEl = -1.0;

// État mouvement
bool movingAz = false;
bool movingEl = false;

// Boutons configurés
bool buttonsConfigured = false;

// ════════════════════════════════════════════════════════════════
// INITIALISATION
// ════════════════════════════════════════════════════════════════

void setupMotors() {
    #if MOTOR_AZ_TYPE == MOTOR_STEPPER
        pinMode(AZ_STEP, OUTPUT);
        pinMode(AZ_DIR, OUTPUT);
        digitalWrite(AZ_STEP, LOW);
        digitalWrite(AZ_DIR, LOW);
    #endif

    #if MOTOR_EL_TYPE == MOTOR_STEPPER
        pinMode(EL_STEP, OUTPUT);
        pinMode(EL_DIR, OUTPUT);
        digitalWrite(EL_STEP, LOW);
        digitalWrite(EL_DIR, LOW);
    #endif

    // Configuration fins de course (INPUT_PULLUP pour NC)
    pinMode(LIMIT_AZ, INPUT_PULLUP);
    pinMode(LIMIT_EL, INPUT_PULLUP);

    #if DEBUG_SERIAL
        Serial.println(F("=== MOTEURS TB6600 INITIALISÉS ==="));
        Serial.print(F("Vitesse max: ")); Serial.print(SPEED_MAX); Serial.println(F(" µs"));
        Serial.print(F("Vitesse lente: ")); Serial.print(SPEED_SLOW); Serial.println(F(" µs"));
        Serial.print(F("Tolérance: ")); Serial.print(POSITION_TOLERANCE); Serial.println(F("°"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// EXÉCUTION STEP (avec sécurité fin de course)
// ════════════════════════════════════════════════════════════════

void doStep(int pinStep, int pinDir, int direction, int delayUs, int limitPin) {
    // SÉCURITÉ: Vérifier fin de course AVANT de bouger
    // NC en série: LOW = circuit ouvert = fin de course atteinte
    if (digitalRead(limitPin) == LOW) {
        #if DEBUG_MOTOR_STEP
            Serial.println(F("[LIMIT] Fin de course!"));
        #endif
        return;  // Ne pas bouger
    }

    // Exécuter le step
    digitalWrite(pinDir, direction);
    digitalWrite(pinStep, HIGH);
    delayMicroseconds(delayUs);
    digitalWrite(pinStep, LOW);
    delayMicroseconds(delayUs);
}

// Surcharge sans limitPin (pour compatibilité)
void doStep(int pinStep, int pinDir, int direction, int delayUs) {
    digitalWrite(pinDir, direction);
    digitalWrite(pinStep, HIGH);
    delayMicroseconds(delayUs);
    digitalWrite(pinStep, LOW);
    delayMicroseconds(delayUs);
}

// ════════════════════════════════════════════════════════════════
// CONTRÔLE PRINCIPAL MOTEURS (Asservissement)
// ════════════════════════════════════════════════════════════════

void updateMotorControl() {
    // ─────────────────────────────────────────────────────────────
    // ASSERVISSEMENT AZIMUTH
    // ─────────────────────────────────────────────────────────────

    #if MOTOR_AZ_TYPE == MOTOR_STEPPER
        if (targetAz >= 0) {
            // Calcul erreur normalisée (-180° à +180°)
            float errAz = targetAz - currentAz;
            if (errAz > 180) errAz -= 360;
            if (errAz < -180) errAz += 360;

            // Vérifier si position atteinte
            if (abs(errAz) > POSITION_TOLERANCE) {
                // Sélection vitesse (rapide si loin, lent si proche)
                int speed = (abs(errAz) > SPEED_SWITCH_THRESHOLD) ? SPEED_MAX : SPEED_SLOW;

                // Direction: HIGH si erreur positive (CW), LOW si négative (CCW)
                int direction = (errAz > 0) ? HIGH : LOW;

                // Exécuter 1 step avec vérification fin de course
                doStep(AZ_STEP, AZ_DIR, direction, speed, LIMIT_AZ);
                movingAz = true;

                #if DEBUG_MOTOR_STEP
                    Serial.print(F("[AZ] err=")); Serial.print(errAz, 2);
                    Serial.print(F(" spd=")); Serial.println(speed);
                #endif
            } else {
                // Position atteinte
                targetAz = -1.0;
                movingAz = false;

                #if DEBUG_MOTOR_CMD
                    Serial.println(F("[MOTOR] Az: Position atteinte"));
                #endif
            }
        } else {
            movingAz = false;
        }
    #endif

    // ─────────────────────────────────────────────────────────────
    // ASSERVISSEMENT ÉLÉVATION
    // ─────────────────────────────────────────────────────────────

    #if MOTOR_EL_TYPE == MOTOR_STEPPER
        if (targetEl >= 0) {
            float errEl = targetEl - currentEl;

            if (abs(errEl) > POSITION_TOLERANCE) {
                int speed = (abs(errEl) > SPEED_SWITCH_THRESHOLD) ? SPEED_MAX : SPEED_SLOW;
                int direction = (errEl > 0) ? HIGH : LOW;

                doStep(EL_STEP, EL_DIR, direction, speed, LIMIT_EL);
                movingEl = true;

                #if DEBUG_MOTOR_STEP
                    Serial.print(F("[EL] err=")); Serial.print(errEl, 2);
                    Serial.print(F(" spd=")); Serial.println(speed);
                #endif
            } else {
                targetEl = -1.0;
                movingEl = false;

                #if DEBUG_MOTOR_CMD
                    Serial.println(F("[MOTOR] El: Position atteinte"));
                #endif
            }
        } else {
            movingEl = false;
        }
    #endif
}

// ════════════════════════════════════════════════════════════════
// ARRÊT IMMÉDIAT
// ════════════════════════════════════════════════════════════════

void stopAllMotors() {
    targetAz = -1.0;
    targetEl = -1.0;
    movingAz = false;
    movingEl = false;

    #if DEBUG_MOTOR_CMD
        Serial.println(F("[MOTOR] STOP - Arrêt immédiat"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// BOUTONS MANUELS
// ════════════════════════════════════════════════════════════════

void checkManualButtons() {
    // Configuration pins boutons (une seule fois)
    if (!buttonsConfigured) {
        pinMode(BTN_CW, INPUT_PULLUP);
        pinMode(BTN_CCW, INPUT_PULLUP);
        pinMode(BTN_UP, INPUT_PULLUP);
        pinMode(BTN_DOWN, INPUT_PULLUP);
        pinMode(BTN_STOP, INPUT_PULLUP);
        buttonsConfigured = true;
    }

    // ─────────────────────────────────────────────────────────────
    // BOUTON STOP (Priorité absolue)
    // ─────────────────────────────────────────────────────────────

    if (digitalRead(BTN_STOP) == LOW) {
        stopAllMotors();
        return;
    }

    // ─────────────────────────────────────────────────────────────
    // BOUTONS AZIMUTH (CW / CCW)
    // ─────────────────────────────────────────────────────────────

    #if MOTOR_AZ_TYPE == MOTOR_STEPPER
        if (digitalRead(BTN_CW) == LOW) {
            for (int i = 0; i < MANUAL_STEP_SIZE; i++) {
                doStep(AZ_STEP, AZ_DIR, HIGH, SPEED_SLOW, LIMIT_AZ);
            }
            #if DEBUG_MOTOR_CMD
                Serial.println(F("[BTN] CW"));
            #endif
        }

        if (digitalRead(BTN_CCW) == LOW) {
            for (int i = 0; i < MANUAL_STEP_SIZE; i++) {
                doStep(AZ_STEP, AZ_DIR, LOW, SPEED_SLOW, LIMIT_AZ);
            }
            #if DEBUG_MOTOR_CMD
                Serial.println(F("[BTN] CCW"));
            #endif
        }
    #endif

    // ─────────────────────────────────────────────────────────────
    // BOUTONS ÉLÉVATION (UP / DOWN)
    // ─────────────────────────────────────────────────────────────

    #if MOTOR_EL_TYPE == MOTOR_STEPPER
        if (digitalRead(BTN_UP) == LOW) {
            for (int i = 0; i < MANUAL_STEP_SIZE; i++) {
                doStep(EL_STEP, EL_DIR, HIGH, SPEED_SLOW, LIMIT_EL);
            }
            #if DEBUG_MOTOR_CMD
                Serial.println(F("[BTN] UP"));
            #endif
        }

        if (digitalRead(BTN_DOWN) == LOW) {
            for (int i = 0; i < MANUAL_STEP_SIZE; i++) {
                doStep(EL_STEP, EL_DIR, LOW, SPEED_SLOW, LIMIT_EL);
            }
            #if DEBUG_MOTOR_CMD
                Serial.println(F("[BTN] DOWN"));
            #endif
        }
    #endif
}

// ════════════════════════════════════════════════════════════════
// CALCUL ERREUR ANGULAIRE NORMALISÉE
// ════════════════════════════════════════════════════════════════

float calculateAngularError(float target, float current) {
    float error = target - current;

    // Normaliser dans plage -180° à +180° (chemin le plus court)
    if (error > 180.0) {
        error -= 360.0;
    } else if (error < -180.0) {
        error += 360.0;
    }

    return error;
}

// ════════════════════════════════════════════════════════════════
// CALCUL STEPS POUR ANGLE
// ════════════════════════════════════════════════════════════════

long calculateStepsForAngle(float angleDegrees, float gearRatio) {
    // Formule: steps = (angle / 360°) × steps_per_rev × gear_ratio
    return (long)((angleDegrees / 360.0) * STEPS_PER_REV_MOTOR * gearRatio);
}

// ════════════════════════════════════════════════════════════════
// DEBUG AFFICHAGE
// ════════════════════════════════════════════════════════════════

void printMotorDebug() {
    Serial.print(F("[MOTOR] Az: tgt="));
    if (targetAz >= 0) {
        Serial.print(targetAz, 1);
    } else {
        Serial.print(F("--"));
    }
    Serial.print(F(" cur="));
    Serial.print(currentAz, 1);
    Serial.print(F(" mov="));
    Serial.print(movingAz ? F("Y") : F("N"));

    Serial.print(F(" | El: tgt="));
    if (targetEl >= 0) {
        Serial.print(targetEl, 1);
    } else {
        Serial.print(F("--"));
    }
    Serial.print(F(" cur="));
    Serial.print(currentEl, 1);
    Serial.print(F(" mov="));
    Serial.println(movingEl ? F("Y") : F("N"));
}
