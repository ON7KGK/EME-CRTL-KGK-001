// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Protocole Easycom (Implementation)
// ════════════════════════════════════════════════════════════════
// Fichier: easycom.cpp
// Description: Parseur Easycom compatible K3NG/PstRotator
//              Basé sur code fonctionnel testé
// ════════════════════════════════════════════════════════════════

#include "easycom.h"
#include "encoder_ssi.h"    // Pour currentAz, currentEl, rawCountsAz, rawCountsEl
#include "motor_stepper.h"  // Pour targetAz, targetEl, stopAllMotors
#include "network.h"        // Pour sendToClient
#include <EEPROM.h>         // Pour sauvegarde calibration

// ════════════════════════════════════════════════════════════════
// VARIABLES EXTERNES (définies dans autres modules)
// ════════════════════════════════════════════════════════════════

extern float currentAz;     // encoder_ssi.cpp
extern float currentEl;     // encoder_ssi.cpp
extern float targetAz;      // motor_stepper.cpp
extern float targetEl;      // motor_stepper.cpp
extern int rawCountsAz;     // encoder_ssi.cpp
extern int rawCountsEl;     // encoder_ssi.cpp
extern long turnsAz;        // encoder_ssi.cpp
extern long offsetStepsAz;  // encoder_ssi.cpp
extern long offsetStepsEl;  // encoder_ssi.cpp

// ════════════════════════════════════════════════════════════════
// PARSING COMMANDE PRINCIPALE (style K3NG)
// ════════════════════════════════════════════════════════════════

void parseEasycomCommand(String command) {
    // Normalisation commande
    command.toUpperCase();
    command.trim();

    // Ignorer commandes vides
    if (command.length() == 0) {
        return;
    }

    #if DEBUG_EASYCOM_RX
        Serial.print(F("[RX] "));
        Serial.println(command);
    #endif

    // ─────────────────────────────────────────────────────────────
    // COMMANDES ARRÊT (Priorité absolue)
    // ─────────────────────────────────────────────────────────────

    if (command == "S" || command == "SA" || command == "SE" || command == "STOP") {
        targetAz = -1.0;
        targetEl = -1.0;

        #if DEBUG_MOTOR_CMD
            Serial.println(F("[STOP] Arrêt demandé"));
        #endif

        // Feedback immédiat
        sendPositionResponse();
        return;
    }

    // ─────────────────────────────────────────────────────────────
    // RESET EEPROM: RESET_EEPROM ou RESET
    // ─────────────────────────────────────────────────────────────

    if (command == "RESET_EEPROM" || command == "RESET") {
        #if DEBUG_SERIAL
            Serial.println(F(""));
            Serial.println(F("════════════════════════════════════════════════════════════════"));
            Serial.println(F("    RESET EEPROM DEMANDÉ"));
            Serial.println(F("════════════════════════════════════════════════════════════════"));
            Serial.println(F(""));

            // Afficher valeurs AVANT
            Serial.println(F("Valeurs AVANT effacement:"));
            Serial.print(F("  turnsAz:   ")); Serial.println(turnsAz);

            long turnsElTemp;
            EEPROM.get(EEPROM_TURNS_EL, turnsElTemp);
            Serial.print(F("  turnsEl:   ")); Serial.println(turnsElTemp);
            Serial.print(F("  offsetAz:  ")); Serial.println(offsetStepsAz);
            Serial.print(F("  offsetEl:  ")); Serial.println(offsetStepsEl);
            Serial.println(F(""));
        #endif

        // Effacer toutes les valeurs
        long zero = 0L;
        EEPROM.put(EEPROM_TURNS_AZ, zero);
        EEPROM.put(EEPROM_TURNS_EL, zero);
        EEPROM.put(EEPROM_OFFSET_AZ, zero);
        EEPROM.put(EEPROM_OFFSET_EL, zero);

        // Mettre à jour variables globales
        turnsAz = 0;
        offsetStepsAz = 0;
        offsetStepsEl = 0;

        #if DEBUG_SERIAL
            Serial.println(F("Valeurs APRÈS effacement:"));
            Serial.print(F("  turnsAz:   ")); Serial.println(turnsAz);
            Serial.print(F("  turnsEl:   ")); Serial.println(zero);
            Serial.print(F("  offsetAz:  ")); Serial.println(offsetStepsAz);
            Serial.print(F("  offsetEl:  ")); Serial.println(offsetStepsEl);
            Serial.println(F(""));
            Serial.println(F("════════════════════════════════════════════════════════════════"));
            Serial.println(F("    ✓ EEPROM EFFACÉE AVEC SUCCÈS"));
            Serial.println(F("════════════════════════════════════════════════════════════════"));
            Serial.println(F(""));
            Serial.println(F("IMPORTANT: Redémarrez l'Arduino (reset) pour que les changements"));
            Serial.println(F("           soient pris en compte dans tous les modules."));
            Serial.println(F(""));
        #endif

        sendPositionResponse();
        return;
    }

    // ─────────────────────────────────────────────────────────────
    // CALIBRATION AZIMUTH: Z123.5
    // ─────────────────────────────────────────────────────────────

    if (command.startsWith("Z") && command.length() > 1) {
        float cal = command.substring(1).toFloat();
        offsetStepsAz = (turnsAz * 4096L) + rawCountsAz - (long)(cal * 4096.0 * GEAR_RATIO_AZ / 360.0);
        EEPROM.put(EEPROM_OFFSET_AZ, offsetStepsAz);

        #if DEBUG_SERIAL
            Serial.print(F("[CAL] Az = "));
            Serial.print(cal, 1);
            Serial.println(F("°"));
        #endif

        sendPositionResponse();
        return;
    }

    // ─────────────────────────────────────────────────────────────
    // CALIBRATION ÉLÉVATION: S45.0 (S suivi d'un chiffre)
    // Note: "S" seul = STOP, "S45" = calibration
    // ─────────────────────────────────────────────────────────────

    if (command.startsWith("S") && command.length() > 1 && isDigit(command.charAt(1))) {
        float cal = command.substring(1).toFloat();
        offsetStepsEl = (long)rawCountsEl - (long)(cal * 4095.0 / 90.0);
        EEPROM.put(EEPROM_OFFSET_EL, offsetStepsEl);

        #if DEBUG_SERIAL
            Serial.print(F("[CAL] El = "));
            Serial.print(cal, 1);
            Serial.println(F("°"));
        #endif

        sendPositionResponse();
        return;
    }

    // ─────────────────────────────────────────────────────────────
    // COMMANDES GOTO: AZ123.5 EL45.0
    // ─────────────────────────────────────────────────────────────

    // Parsing AZ
    int posAz = command.indexOf("AZ");
    if (posAz != -1) {
        String valueStr = "";
        for (int i = posAz + 2; i < (int)command.length(); i++) {
            char c = command.charAt(i);
            if (isDigit(c) || c == '.' || c == '-') {
                valueStr += c;
            } else if (valueStr.length() > 0) {
                break;
            }
        }

        if (valueStr.length() > 0) {
            float newTarget = valueStr.toFloat();

            // Filtre anti-vibration: ignorer si < 0.15° de position actuelle
            if (abs(newTarget - currentAz) < MICRO_MOVEMENT_FILTER) {
                targetAz = -1.0;  // Pas de mouvement

                #if DEBUG_EASYCOM_PARSE
                    Serial.print(F("[AZ] Micro-mvt ignoré: "));
                    Serial.println(abs(newTarget - currentAz), 2);
                #endif
            } else {
                targetAz = newTarget;

                #if DEBUG_MOTOR_CMD
                    Serial.print(F("[GOTO] Az="));
                    Serial.println(newTarget, 1);
                #endif
            }
        }
    }

    // Parsing EL
    int posEl = command.indexOf("EL");
    if (posEl != -1) {
        String valueStr = "";
        for (int i = posEl + 2; i < (int)command.length(); i++) {
            char c = command.charAt(i);
            if (isDigit(c) || c == '.' || c == '-') {
                valueStr += c;
            } else if (valueStr.length() > 0) {
                break;
            }
        }

        if (valueStr.length() > 0) {
            float newTarget = valueStr.toFloat();

            // Filtre anti-vibration
            if (abs(newTarget - currentEl) < MICRO_MOVEMENT_FILTER) {
                targetEl = -1.0;

                #if DEBUG_EASYCOM_PARSE
                    Serial.print(F("[EL] Micro-mvt ignoré: "));
                    Serial.println(abs(newTarget - currentEl), 2);
                #endif
            } else {
                targetEl = newTarget;

                #if DEBUG_MOTOR_CMD
                    Serial.print(F("[GOTO] El="));
                    Serial.println(newTarget, 1);
                #endif
            }
        }
    }

    // ─────────────────────────────────────────────────────────────
    // FEEDBACK: Toujours envoyer position après commande
    // ─────────────────────────────────────────────────────────────

    sendPositionResponse();
}

// ════════════════════════════════════════════════════════════════
// ENVOI RÉPONSE POSITION
// ════════════════════════════════════════════════════════════════

void sendPositionResponse() {
    // Format Easycom: "AZ123.5 EL45.0\r\n"
    String response = "AZ";
    response += String(currentAz, 1);
    response += " EL";
    response += String(currentEl, 1);
    response += "\r\n";

    sendToClient(response);

    #if DEBUG_EASYCOM_TX
        Serial.print(F("[TX] "));
        Serial.print(response);
    #endif
}

// ════════════════════════════════════════════════════════════════
// GÉNÉRATION RÉPONSE POSITION (alias pour compatibilité)
// ════════════════════════════════════════════════════════════════

String generatePositionResponse() {
    String response = "AZ";
    response += String(currentAz, 1);
    response += " EL";
    response += String(currentEl, 1);
    response += "\r\n";
    return response;
}

// ════════════════════════════════════════════════════════════════
// PARSING VALEUR NUMÉRIQUE (utilitaire)
// ════════════════════════════════════════════════════════════════

float parseNumericValue(String command, String keyword, bool &valueFound) {
    int pos = command.indexOf(keyword);

    if (pos == -1) {
        valueFound = false;
        return 0.0;
    }

    String valueStr = "";
    int startPos = pos + keyword.length();

    for (int i = startPos; i < (int)command.length(); i++) {
        char c = command.charAt(i);
        if (isDigit(c) || c == '.' || c == '-' || c == '+') {
            valueStr += c;
        } else if (valueStr.length() > 0) {
            break;
        }
    }

    if (valueStr.length() > 0) {
        valueFound = true;
        return valueStr.toFloat();
    }

    valueFound = false;
    return 0.0;
}

// ════════════════════════════════════════════════════════════════
// DÉTECTION QUERY POSITION
// ════════════════════════════════════════════════════════════════

bool isPositionQuery(String command) {
    return (command == "AZ" || command == "EL" || command == "AZ EL");
}

// ════════════════════════════════════════════════════════════════
// DÉTECTION COMMANDE STOP
// ════════════════════════════════════════════════════════════════

bool isStopCommand(String command) {
    return (command == "S" || command == "SA" || command == "SE" || command == "STOP");
}

// ════════════════════════════════════════════════════════════════
// DEBUG AFFICHAGE
// ════════════════════════════════════════════════════════════════

void printEasycomDebug(String command) {
    Serial.print(F("[EASYCOM] "));
    Serial.println(command);
}

// ════════════════════════════════════════════════════════════════
// DEBUG ENCODEURS RAW
// ════════════════════════════════════════════════════════════════

void printEncoderRawDebug() {
    static unsigned long lastRawDebugTime = 0;
    if (millis() - lastRawDebugTime < DEBUG_INTERVAL) {
        return;  // Pas encore temps d'afficher
    }
    lastRawDebugTime = millis();

    // Debug encodeurs SSI (valeurs brutes 0-4095)
    #if DEBUG_ENCODER_RAW && ((ENCODER_AZ_TYPE == ENCODER_SSI_ABSOLUTE) || (ENCODER_AZ_TYPE == ENCODER_SSI_INC) || \
                               (ENCODER_EL_TYPE == ENCODER_SSI_ABSOLUTE) || (ENCODER_EL_TYPE == ENCODER_SSI_INC))
        Serial.print(F("[SSI RAW] "));
        #if (ENCODER_AZ_TYPE == ENCODER_SSI_ABSOLUTE) || (ENCODER_AZ_TYPE == ENCODER_SSI_INC)
            Serial.print(F("Az="));
            Serial.print(rawCountsAz);
            Serial.print(F("/4095"));
        #endif
        #if (ENCODER_EL_TYPE == ENCODER_SSI_ABSOLUTE) || (ENCODER_EL_TYPE == ENCODER_SSI_INC)
            Serial.print(F(" El="));
            Serial.print(rawCountsEl);
            Serial.print(F("/4095"));
        #endif
        Serial.println();
    #endif

    // Debug potentiomètres (valeurs ADC 0-1023)
    #if DEBUG_POT_ADC && ((ENCODER_AZ_TYPE == ENCODER_POT_ABSOLUTE) || (ENCODER_AZ_TYPE == ENCODER_POT_MULTI) || \
                          (ENCODER_EL_TYPE == ENCODER_POT_ABSOLUTE) || (ENCODER_EL_TYPE == ENCODER_POT_MULTI))
        Serial.print(F("[POT ADC] "));
        #if (ENCODER_AZ_TYPE == ENCODER_POT_ABSOLUTE) || (ENCODER_AZ_TYPE == ENCODER_POT_MULTI)
            extern int potAdcAz;  // Variable depuis encoder_ssi.cpp
            Serial.print(F("Az="));
            Serial.print(potAdcAz);
            Serial.print(F("/1023"));
        #endif
        #if (ENCODER_EL_TYPE == ENCODER_POT_ABSOLUTE) || (ENCODER_EL_TYPE == ENCODER_POT_MULTI)
            extern int potAdcEl;  // Variable depuis encoder_ssi.cpp
            Serial.print(F(" El="));
            Serial.print(potAdcEl);
            Serial.print(F("/1023"));
        #endif
        Serial.println();
    #endif

    // Debug positions en degrés (commun SSI et POT)
    #if DEBUG_ENCODER_DEG || DEBUG_POT_DEG
        extern float currentAz, currentEl;
        Serial.print(F("[POS °] Az="));
        Serial.print(currentAz, 2);
        Serial.print(F("° El="));
        Serial.print(currentEl, 2);
        Serial.println(F("°"));
    #endif
}
