// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Sécurité (Implementation)
// ════════════════════════════════════════════════════════════════
// Fichier: safety.cpp
// Description: Implémentation gestion fins de course NC
// ════════════════════════════════════════════════════════════════

#include "safety.h"

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES
// ════════════════════════════════════════════════════════════════

bool limitAzTriggered = false;
bool limitElTriggered = false;

// ════════════════════════════════════════════════════════════════
// INITIALISATION
// ════════════════════════════════════════════════════════════════

void setupLimits() {
    // TODO: Configuration pins fins de course

    // pinMode(LIMIT_AZ, INPUT_PULLUP);
    // pinMode(LIMIT_EL, INPUT_PULLUP);

    // TODO: Vérification état initial
    // checkLimits();

    #if DEBUG_SERIAL
        Serial.println(F("=== SÉCURITÉ FINS DE COURSE INITIALISÉE ==="));
        Serial.println(F("Type: NC (Normally Closed)"));
        Serial.print(F("Pin Az: ")); Serial.println(LIMIT_AZ);
        Serial.print(F("Pin El: ")); Serial.println(LIMIT_EL);

        // Affichage état initial
        // Serial.print(F("État initial Az: "));
        // Serial.println(isAzimuthSafe() ? F("SAFE") : F("LIMIT TRIGGERED"));
        // Serial.print(F("État initial El: "));
        // Serial.println(isElevationSafe() ? F("SAFE") : F("LIMIT TRIGGERED"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// VÉRIFICATION FINS DE COURSE
// ════════════════════════════════════════════════════════════════

void checkLimits() {
    // TODO: Lecture états pins et mise à jour flags

    // ATTENTION: Logique NC avec pull-up
    // Vérifier comportement réel selon câblage!

    // Variante 1: Pin HIGH = circuit ouvert = limite atteinte
    // limitAzTriggered = (digitalRead(LIMIT_AZ) == HIGH);
    // limitElTriggered = (digitalRead(LIMIT_EL) == HIGH);

    // Variante 2: Pin LOW = circuit ouvert = limite atteinte (selon montage)
    // limitAzTriggered = (digitalRead(LIMIT_AZ) == LOW);
    // limitElTriggered = (digitalRead(LIMIT_EL) == LOW);

    // TODO: Si limite déclenchée, log message
    // #if DEBUG_SERIAL
    //     if (limitAzTriggered) {
    //         Serial.println(F("!!! LIMITE AZIMUTH ATTEINTE !!!"));
    //     }
    //     if (limitElTriggered) {
    //         Serial.println(F("!!! LIMITE ÉLÉVATION ATTEINTE !!!"));
    //     }
    // #endif
}

// ════════════════════════════════════════════════════════════════
// VÉRIFICATION LIMITE AZIMUTH
// ════════════════════════════════════════════════════════════════

bool isAzimuthSafe() {
    // TODO: Vérifier état limite azimuth

    // checkLimits();  // Mise à jour état
    // return !limitAzTriggered;  // true si mouvement sûr

    return true;  // TODO: Remplacer par implémentation réelle
}

// ════════════════════════════════════════════════════════════════
// VÉRIFICATION LIMITE ÉLÉVATION
// ════════════════════════════════════════════════════════════════

bool isElevationSafe() {
    // TODO: Vérifier état limite élévation

    // checkLimits();
    // return !limitElTriggered;

    return true;  // TODO: Remplacer par implémentation réelle
}

// ════════════════════════════════════════════════════════════════
// DEBUG AFFICHAGE
// ════════════════════════════════════════════════════════════════

void printSafetyDebug() {
    Serial.print(F("LIMITS - Az: "));
    Serial.print(limitAzTriggered ? F("TRIGGERED") : F("OK"));
    Serial.print(F(" | El: "));
    Serial.println(limitElTriggered ? F("TRIGGERED") : F("OK"));
}
