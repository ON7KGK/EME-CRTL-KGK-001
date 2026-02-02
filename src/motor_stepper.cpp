// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Moteurs Pas-à-Pas (Simple digitalWrite)
// ════════════════════════════════════════════════════════════════
// Fichier: motor_stepper.cpp
// Description: Contrôle moteurs TB6600 - micros() timing, 1 step/call
// ════════════════════════════════════════════════════════════════

#include "config.h"

// Ce fichier n'est compilé que si USE_NANO_STEPPER est désactivé
#if !USE_NANO_STEPPER

#include "motor_stepper.h"
#include "encoder_ssi.h"

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES
// ════════════════════════════════════════════════════════════════

float targetAz = -1.0;
float targetEl = -1.0;

bool movingAz = false;
bool movingEl = false;

bool buttonsConfigured = false;

// Timing micros() pour chaque axe
unsigned long lastStepTimeAz = 0;
unsigned long lastStepTimeEl = 0;

// Direction courante (pour éviter changements pendant step)
int currentDirAz = LOW;
int currentDirEl = LOW;

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

    pinMode(LIMIT_AZ, INPUT_PULLUP);
    pinMode(LIMIT_EL, INPUT_PULLUP);

    #if DEBUG_SERIAL
        Serial.println(F("=== MOTEURS TB6600 (Simple) ==="));
        Serial.print(F("AZ: STEP=")); Serial.print(AZ_STEP);
        Serial.print(F(" DIR=")); Serial.println(AZ_DIR);
        Serial.print(F("EL: STEP=")); Serial.print(EL_STEP);
        Serial.print(F(" DIR=")); Serial.println(EL_DIR);
    #endif

}

// ════════════════════════════════════════════════════════════════
// STEP UNIQUE (non-bloquant, timing géré par appelant)
// ════════════════════════════════════════════════════════════════

inline void singleStep(uint8_t stepPin) {
    digitalWrite(stepPin, HIGH);
    delayMicroseconds(5);  // Pulse minimum 2.5µs pour TB6600
    digitalWrite(stepPin, LOW);
}

// ════════════════════════════════════════════════════════════════
// CONTRÔLE PRINCIPAL MOTEURS (1 step par appel, sans timing)
// ════════════════════════════════════════════════════════════════

void updateMotorControl() {
    // ─────────────────────────────────────────────────────────────
    // AZIMUTH - 10 steps par appel, vitesse variable selon distance
    // ─────────────────────────────────────────────────────────────

    #if MOTOR_AZ_TYPE == MOTOR_STEPPER
        if (targetAz >= 0) {
            float errAz = targetAz - currentAz;
            if (errAz > 180) errAz -= 360;
            if (errAz < -180) errAz += 360;

            if (abs(errAz) > POSITION_TOLERANCE) {
                if (digitalRead(LIMIT_AZ) == HIGH) {
                    digitalWrite(AZ_DIR, (errAz > 0) ? HIGH : LOW);

                    // Vitesse variable: rapide si loin, lent si proche
                    int stepDelay = (abs(errAz) > SPEED_SWITCH_THRESHOLD) ? SPEED_FAST : SPEED_SLOW;

                    // Faire 10 steps (burst)
                    for (int i = 0; i < 10; i++) {
                        digitalWrite(AZ_STEP, HIGH);
                        delayMicroseconds(stepDelay);
                        digitalWrite(AZ_STEP, LOW);
                        delayMicroseconds(stepDelay);
                    }

                    movingAz = true;
                } else {
                    movingAz = false;
                }
            } else {
                targetAz = -1.0;
                movingAz = false;
            }
        } else {
            movingAz = false;
        }
    #endif

    // ─────────────────────────────────────────────────────────────
    // ÉLÉVATION - 10 steps par appel, vitesse variable selon distance
    // ─────────────────────────────────────────────────────────────

    #if MOTOR_EL_TYPE == MOTOR_STEPPER
        if (targetEl >= 0) {
            float errEl = targetEl - currentEl;

            if (abs(errEl) > POSITION_TOLERANCE) {
                if (digitalRead(LIMIT_EL) == HIGH) {
                    digitalWrite(EL_DIR, (errEl > 0) ? HIGH : LOW);

                    // Vitesse variable: rapide si loin, lent si proche
                    int stepDelay = (abs(errEl) > SPEED_SWITCH_THRESHOLD) ? SPEED_FAST : SPEED_SLOW;

                    for (int i = 0; i < 10; i++) {
                        digitalWrite(EL_STEP, HIGH);
                        delayMicroseconds(stepDelay);
                        digitalWrite(EL_STEP, LOW);
                        delayMicroseconds(stepDelay);
                    }

                    movingEl = true;
                } else {
                    movingEl = false;
                }
            } else {
                targetEl = -1.0;
                movingEl = false;
            }
        } else {
            movingEl = false;
        }
    #endif
}

// ════════════════════════════════════════════════════════════════
// FONCTIONS LEGACY doStep
// ════════════════════════════════════════════════════════════════

void doStep(int pinStep, int pinDir, int direction, int delayUs, int limitPin) {
    if (digitalRead(limitPin) == LOW) return;
    digitalWrite(pinDir, direction);
    digitalWrite(pinStep, HIGH);
    delayMicroseconds(delayUs);
    digitalWrite(pinStep, LOW);
    delayMicroseconds(delayUs);
}

void doStep(int pinStep, int pinDir, int direction, int delayUs) {
    digitalWrite(pinDir, direction);
    digitalWrite(pinStep, HIGH);
    delayMicroseconds(delayUs);
    digitalWrite(pinStep, LOW);
    delayMicroseconds(delayUs);
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
        Serial.println(F("[MOTOR] STOP"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// BOUTONS MANUELS
// ════════════════════════════════════════════════════════════════

void checkManualButtons() {
    if (!buttonsConfigured) {
        pinMode(BTN_CW, INPUT_PULLUP);
        pinMode(BTN_CCW, INPUT_PULLUP);
        pinMode(BTN_UP, INPUT_PULLUP);
        pinMode(BTN_DOWN, INPUT_PULLUP);
        pinMode(BTN_STOP, INPUT_PULLUP);
        buttonsConfigured = true;
    }

    if (digitalRead(BTN_STOP) == LOW) {
        stopAllMotors();
        return;
    }

    #if MOTOR_AZ_TYPE == MOTOR_STEPPER
        if (digitalRead(BTN_CW) == LOW) {
            doStep(AZ_STEP, AZ_DIR, HIGH, SPEED_SLOW, LIMIT_AZ);
        }
        if (digitalRead(BTN_CCW) == LOW) {
            doStep(AZ_STEP, AZ_DIR, LOW, SPEED_SLOW, LIMIT_AZ);
        }
    #endif

    #if MOTOR_EL_TYPE == MOTOR_STEPPER
        if (digitalRead(BTN_UP) == LOW) {
            doStep(EL_STEP, EL_DIR, HIGH, SPEED_SLOW, LIMIT_EL);
        }
        if (digitalRead(BTN_DOWN) == LOW) {
            doStep(EL_STEP, EL_DIR, LOW, SPEED_SLOW, LIMIT_EL);
        }
    #endif
}

// ════════════════════════════════════════════════════════════════
// FONCTIONS UTILITAIRES
// ════════════════════════════════════════════════════════════════

float calculateAngularError(float target, float current) {
    float error = target - current;
    if (error > 180.0) error -= 360.0;
    else if (error < -180.0) error += 360.0;
    return error;
}

long calculateStepsForAngle(float angleDegrees, float gearRatio) {
    return (long)((angleDegrees / 360.0) * STEPS_PER_REV_MOTOR * gearRatio);
}

void printMotorDebug() {
    Serial.print(F("[MOTOR] Az:"));
    Serial.print(targetAz >= 0 ? targetAz : -1, 1);
    Serial.print(F("/"));
    Serial.print(currentAz, 1);
    Serial.print(movingAz ? F(" MOV") : F(" ---"));

    Serial.print(F(" El:"));
    Serial.print(targetEl >= 0 ? targetEl : -1, 1);
    Serial.print(F("/"));
    Serial.print(currentEl, 1);
    Serial.println(movingEl ? F(" MOV") : F(" ---"));
}

#endif // !USE_NANO_STEPPER
