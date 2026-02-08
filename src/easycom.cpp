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
extern float targetAz;      // motor_stepper.cpp / motor_nano.cpp
extern float targetEl;      // motor_stepper.cpp / motor_nano.cpp

// Valeur sentinel pour "pas de cible active"
// IMPORTANT: -999.0 au lieu de -1.0 pour permettre les cibles négatives (ex: El = -5°)
#define NO_TARGET -999.0
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

    // ─────────────────────────────────────────────────────────────
    // COMMANDES ARRÊT: S, SA, SE, SA SE, STOP, ;
    // PstRotator peut envoyer "SA SE" (les deux ensemble)
    // ─────────────────────────────────────────────────────────────
    bool isStopCmd = false;

    // Vérifier différents formats de commande STOP
    if (command == "S" || command == "STOP" || command == ";") {
        isStopCmd = true;
    }
    // SA seul ou SA suivi d'espace (ex: "SA SE")
    else if (command == "SA" || command.startsWith("SA ")) {
        isStopCmd = true;
    }
    // SE seul ou SE suivi d'espace
    else if (command == "SE" || command.startsWith("SE ")) {
        isStopCmd = true;
    }
    // Contient SA ou SE quelque part (ex: "SA SE", "SASE")
    else if (command.indexOf("SA") != -1 || command.indexOf("SE") != -1) {
        // Vérifier que ce n'est pas une commande de calibration (ex: S45.0)
        if (!(command.startsWith("S") && command.length() > 1 && isDigit(command.charAt(1)))) {
            isStopCmd = true;
        }
    }

    if (isStopCmd) {
        targetAz = NO_TARGET;
        targetEl = NO_TARGET;

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
    // COMMANDES TABLE CORRECTION AZIMUTH (POT_MT uniquement)
    // ─────────────────────────────────────────────────────────────
    // C10, C20, etc. → Calibrer point de table (sans reset ADC)
    // CTABLE → Afficher table complète
    // CRESET → Réinitialiser table à valeurs linéaires

    #if (ENCODER_AZ_TYPE == ENCODER_POT_MT)
        // CTABLE: Afficher table correction
        if (command == "CTABLE") {
            printAzCorrectionTable();
            sendPositionResponse();
            return;
        }

        // CRESET: Réinitialiser table à linéaire
        if (command == "CRESET") {
            resetAzCorrectionTable();
            sendPositionResponse();
            return;
        }

        // C10, C20, etc.: Calibrer un point de table
        // (sans reset accumulatedAdcAz, juste enregistrer le point)
        if (command.startsWith("C") && command.length() > 1 && isDigit(command.charAt(1))) {
            float cal = command.substring(1).toFloat();
            calibrateAzTablePoint(cal);
            sendPositionResponse();
            return;
        }
    #endif

    // ─────────────────────────────────────────────────────────────
    // COMMANDES TABLE CORRECTION ÉLÉVATION (POT_MT uniquement)
    // ─────────────────────────────────────────────────────────────
    // E10, E20, etc. → Calibrer point de table élévation
    // ETABLE → Afficher table complète
    // ERESET → Réinitialiser table à valeurs linéaires

    #if (ENCODER_EL_TYPE == ENCODER_POT_MT)
        // ETABLE: Afficher table correction élévation
        if (command == "ETABLE") {
            printElCorrectionTable();
            sendPositionResponse();
            return;
        }

        // ERESET: Réinitialiser table élévation à linéaire
        if (command == "ERESET") {
            resetElCorrectionTable();
            sendPositionResponse();
            return;
        }

        // E10, E20, etc.: Calibrer un point de table élévation
        if (command.startsWith("E") && command.length() > 1 && isDigit(command.charAt(1))) {
            float cal = command.substring(1).toFloat();
            calibrateElTablePoint(cal);
            sendPositionResponse();
            return;
        }
    #endif

    // ─────────────────────────────────────────────────────────────
    // CALIBRATION AZIMUTH: Z123.5 (Reset ADC + calibre point table)
    // ─────────────────────────────────────────────────────────────

    if (command.startsWith("Z") && command.length() > 1) {
        float cal = command.substring(1).toFloat();
        calibrateAz(cal);  // Appel fonction calibration (gère POT_MT et SSI)
        sendPositionResponse();
        return;
    }

    // ─────────────────────────────────────────────────────────────
    // CALIBRATION ÉLÉVATION: S45.0 (S suivi d'un chiffre)
    // Note: "S" seul = STOP, "S45" = calibration
    // ─────────────────────────────────────────────────────────────

    if (command.startsWith("S") && command.length() > 1 && isDigit(command.charAt(1))) {
        float cal = command.substring(1).toFloat();
        calibrateEl(cal);  // Appel fonction calibration (gère POT_MT et SSI)
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

            // Toujours accepter le nouveau target (POSITION_TOLERANCE dans motor_nano
            // gère le seuil de mouvement, et le Nextion doit voir chaque mise à jour)
            targetAz = newTarget;

            #if DEBUG_MOTOR_CMD
                Serial.print(F("[GOTO] Az="));
                Serial.println(newTarget, 1);
            #endif
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

            // Toujours accepter le nouveau target
            targetEl = newTarget;

            #if DEBUG_MOTOR_CMD
                Serial.print(F("[GOTO] El="));
                Serial.println(newTarget, 1);
            #endif
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
