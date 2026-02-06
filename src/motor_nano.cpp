// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Communication Nano Stepper
// ════════════════════════════════════════════════════════════════
// Fichier: motor_nano.cpp
// Description: Interface UART avec Arduino Nano dédié aux moteurs
//              Protocole combiné: une commande pour les deux axes
// ════════════════════════════════════════════════════════════════
// Connexion: Mega Serial2 (TX2=16, RX2=17) → Nano (RX=0, TX=1)
//
// Protocole Mega → Nano (COMMANDE COMBINÉE pour tracking simultané):
//   "M:dirAz:dirEl:speed\n"  - Mouvement combiné des deux axes
//                              dirAz: -1=CCW, 0=STOP, 1=CW
//                              dirEl: -1=DOWN, 0=STOP, 1=UP
//                              speed: 0=LENT (proche cible), 1=RAPIDE (loin)
//   Exemples:
//     "M:1:1:1\n"   → Az CW + El UP vitesse rapide
//     "M:1:-1:0\n"  → Az CW + El DOWN vitesse lente
//     "M:0:0:0\n"   → STOP les deux axes
//     "M:-1:0:1\n"  → Az CCW + El STOP vitesse rapide
//
//   "STOP\n"     - Arrêt d'urgence immédiat (les deux axes)
//
// Protocole Nano → Mega:
//   "OK\n"       - Commande reçue
//   "READY\n"    - Nano prêt
//   "LIMIT:AZ\n" - Fin de course Az
//   "LIMIT:EL\n" - Fin de course El
// ════════════════════════════════════════════════════════════════

#include "motor_nano.h"
#include "encoder_ssi.h"

#if USE_NANO_STEPPER

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES
// ════════════════════════════════════════════════════════════════

float targetAz = -1.0;
float targetEl = -1.0;

bool movingAz = false;
bool movingEl = false;

// Fins de course avec direction exacte
bool nanoLimitCW = false;
bool nanoLimitCCW = false;
bool nanoLimitUp = false;
bool nanoLimitDown = false;

// État direction actuelle envoyée au Nano
// 0=STOP, 1=CW/UP, -1=CCW/DOWN
int8_t currentDirAz = 0;
int8_t currentDirEl = 0;

// Mode vitesse actuel: 0=LENT, 1=RAPIDE
uint8_t currentSpeedMode = 1;

// Buffer réception
char nanoRxBuffer[64];
uint8_t nanoRxIndex = 0;

// Timing
unsigned long lastNanoUpdate = 0;
const unsigned long NANO_UPDATE_INTERVAL = 20;  // 20ms entre mises à jour (réactif)

// Statut communication Nano
unsigned long lastNanoResponse = 0;  // Timestamp dernière réponse valide
bool nanoConnected = false;          // État connexion actuel

// ════════════════════════════════════════════════════════════════
// INITIALISATION
// ════════════════════════════════════════════════════════════════

void setupMotorNano() {
    NANO_SERIAL.begin(NANO_BAUD);

    // Configuration pin statut communication
    pinMode(NANO_STATUS_PIN, OUTPUT);
    digitalWrite(NANO_STATUS_PIN, LOW);  // LOW au démarrage (pas encore connecté)
    nanoConnected = false;
    lastNanoResponse = 0;

    #if DEBUG_SERIAL
        Serial.println(F("=== MOTOR NANO INTERFACE ==="));
        Serial.print(F("UART: "));
        Serial.print(NANO_BAUD);
        Serial.println(F(" baud"));
        Serial.print(F("Status pin: "));
        Serial.println(NANO_STATUS_PIN);
    #endif

    // Attendre que le Nano soit prêt
    delay(500);

    // Vider le buffer
    while (NANO_SERIAL.available()) {
        NANO_SERIAL.read();
    }
}

// ════════════════════════════════════════════════════════════════
// MISE À JOUR PRINCIPALE
// ════════════════════════════════════════════════════════════════

void updateMotorNano() {
    unsigned long now = millis();

    // Lecture réponses du Nano (toujours)
    readNanoResponse();

    // ─────────────────────────────────────────────────────────────────
    // VÉRIFICATION TIMEOUT COMMUNICATION NANO
    // ─────────────────────────────────────────────────────────────────
    if (lastNanoResponse > 0 && (now - lastNanoResponse) > NANO_TIMEOUT_MS) {
        // Timeout: plus de réponse depuis NANO_TIMEOUT_MS
        if (nanoConnected) {
            nanoConnected = false;
            digitalWrite(NANO_STATUS_PIN, LOW);
            #if DEBUG_SERIAL
                Serial.println(F("[NANO] !!! COMMUNICATION PERDUE !!!"));
            #endif
        }
    }

    // ─────────────────────────────────────────────────────────────────
    // MODE MANUEL: ne pas interférer avec les commandes manuelles
    // ─────────────────────────────────────────────────────────────────
    // Si on est en mode manuel (speed=2), les commandes sont gérées
    // directement par sendManualMove(), on ne fait rien ici
    if (currentSpeedMode == 2) {
        return;
    }

    // Envoi commandes au Nano (périodique) - MODE AUTOMATIQUE seulement
    if (now - lastNanoUpdate >= NANO_UPDATE_INTERVAL) {
        lastNanoUpdate = now;

        // ─────────────────────────────────────────────────────────────
        // CALCUL DIRECTION AZIMUTH
        // ─────────────────────────────────────────────────────────────
        int8_t newDirAz = 0;
        float errAz = 0;

        if (targetAz >= 0) {
            // Calcul erreur avec gestion wrap-around 360°
            errAz = targetAz - currentAz;
            if (errAz > 180) errAz -= 360;
            if (errAz < -180) errAz += 360;

            if (abs(errAz) > POSITION_TOLERANCE) {
                newDirAz = (errAz > 0) ? 1 : -1;
                movingAz = true;
            } else {
                // Cible atteinte
                #if DEBUG_MOTOR_CMD
                    if (currentDirAz != 0) {
                        Serial.print(F("[NANO] Az ATTEINT: "));
                        Serial.println(currentAz, 1);
                    }
                #endif
                targetAz = -1.0;
                movingAz = false;
            }
        } else {
            movingAz = false;
        }

        // ─────────────────────────────────────────────────────────────
        // CALCUL DIRECTION ÉLÉVATION
        // ─────────────────────────────────────────────────────────────
        int8_t newDirEl = 0;
        float errEl = 0;

        if (targetEl >= 0) {
            errEl = targetEl - currentEl;

            if (abs(errEl) > POSITION_TOLERANCE) {
                newDirEl = (errEl > 0) ? 1 : -1;
                movingEl = true;
            } else {
                // Cible atteinte
                #if DEBUG_MOTOR_CMD
                    if (currentDirEl != 0) {
                        Serial.print(F("[NANO] El ATTEINT: "));
                        Serial.println(currentEl, 1);
                    }
                #endif
                targetEl = -1.0;
                movingEl = false;
            }
        } else {
            movingEl = false;
        }

        // ─────────────────────────────────────────────────────────────
        // CALCUL MODE VITESSE (selon distance max des deux axes)
        // ─────────────────────────────────────────────────────────────
        // 0 = LENT (proche de la cible), 1 = RAPIDE (loin de la cible)
        float maxErr = max(abs(errAz), abs(errEl));
        uint8_t newSpeedMode = (maxErr > SPEED_SWITCH_THRESHOLD) ? 1 : 0;

        // ─────────────────────────────────────────────────────────────
        // ENVOI COMMANDE COMBINÉE SI CHANGEMENT (dir ou vitesse)
        // ─────────────────────────────────────────────────────────────
        // Une seule commande pour les deux axes = tracking simultané!
        if (newDirAz != currentDirAz || newDirEl != currentDirEl || newSpeedMode != currentSpeedMode) {
            // Format: M:dirAz:dirEl:speed
            NANO_SERIAL.print(F("M:"));
            NANO_SERIAL.print(newDirAz);
            NANO_SERIAL.print(F(":"));
            NANO_SERIAL.print(newDirEl);
            NANO_SERIAL.print(F(":"));
            NANO_SERIAL.println(newSpeedMode);

            #if DEBUG_MOTOR_CMD
                Serial.print(F("[NANO] → M:"));
                Serial.print(newDirAz);
                Serial.print(F(":"));
                Serial.print(newDirEl);
                Serial.print(F(":"));
                Serial.print(newSpeedMode);
                Serial.print(F(" (Az="));
                if (newDirAz > 0) Serial.print(F("CW"));
                else if (newDirAz < 0) Serial.print(F("CCW"));
                else Serial.print(F("STOP"));
                Serial.print(F(", El="));
                if (newDirEl > 0) Serial.print(F("UP"));
                else if (newDirEl < 0) Serial.print(F("DOWN"));
                else Serial.print(F("STOP"));
                Serial.print(newSpeedMode ? F(", FAST)") : F(", SLOW)"));
                Serial.println();
            #endif

            currentDirAz = newDirAz;
            currentDirEl = newDirEl;
            currentSpeedMode = newSpeedMode;
        }
    }
}

// ════════════════════════════════════════════════════════════════
// LECTURE RÉPONSES NANO
// ════════════════════════════════════════════════════════════════

void readNanoResponse() {
    while (NANO_SERIAL.available()) {
        char c = NANO_SERIAL.read();

        if (c == '\n' || c == '\r') {
            if (nanoRxIndex > 0) {
                nanoRxBuffer[nanoRxIndex] = '\0';

                #if DEBUG_MOTOR_CMD
                    Serial.print(F("[NANO] ← "));
                    Serial.println(nanoRxBuffer);
                #endif

                // ═══════════════════════════════════════════════════════
                // MISE À JOUR STATUT COMMUNICATION
                // ═══════════════════════════════════════════════════════
                // Toute réponse valide = communication OK
                lastNanoResponse = millis();
                if (!nanoConnected) {
                    nanoConnected = true;
                    digitalWrite(NANO_STATUS_PIN, HIGH);
                    #if DEBUG_SERIAL
                        Serial.println(F("[NANO] Communication établie"));
                    #endif
                }

                // Parser la réponse
                // OK → Commande reçue par le Nano
                if (strcmp(nanoRxBuffer, "OK") == 0) {
                    // Accusé réception, rien à faire
                }
                // READY → Nano démarré et prêt
                else if (strcmp(nanoRxBuffer, "READY") == 0) {
                    #if DEBUG_SERIAL
                        Serial.println(F("[NANO] Nano prêt!"));
                    #endif
                    // Reset des états au démarrage
                    currentDirAz = 0;
                    currentDirEl = 0;
                }
                // LIMIT:AZ:CW → Fin de course Az CW atteinte
                else if (strcmp(nanoRxBuffer, "LIMIT:AZ:CW") == 0) {
                    nanoLimitCW = true;
                    movingAz = false;
                    currentDirAz = 0;
                    targetAz = -1.0;
                    #if DEBUG_SERIAL
                        Serial.println(F("[NANO] !!! LIMIT CW !!!"));
                    #endif
                }
                // LIMIT:AZ:CCW → Fin de course Az CCW atteinte
                else if (strcmp(nanoRxBuffer, "LIMIT:AZ:CCW") == 0) {
                    nanoLimitCCW = true;
                    movingAz = false;
                    currentDirAz = 0;
                    targetAz = -1.0;
                    #if DEBUG_SERIAL
                        Serial.println(F("[NANO] !!! LIMIT CCW !!!"));
                    #endif
                }
                // LIMIT:EL:UP → Fin de course El UP atteinte
                else if (strcmp(nanoRxBuffer, "LIMIT:EL:UP") == 0) {
                    nanoLimitUp = true;
                    movingEl = false;
                    currentDirEl = 0;
                    targetEl = -1.0;
                    #if DEBUG_SERIAL
                        Serial.println(F("[NANO] !!! LIMIT UP !!!"));
                    #endif
                }
                // LIMIT:EL:DOWN → Fin de course El DOWN atteinte
                else if (strcmp(nanoRxBuffer, "LIMIT:EL:DOWN") == 0) {
                    nanoLimitDown = true;
                    movingEl = false;
                    currentDirEl = 0;
                    targetEl = -1.0;
                    #if DEBUG_SERIAL
                        Serial.println(F("[NANO] !!! LIMIT DOWN !!!"));
                    #endif
                }
                // CLEAR:AZ:CW → Fin de course Az CW dégagée
                else if (strcmp(nanoRxBuffer, "CLEAR:AZ:CW") == 0) {
                    nanoLimitCW = false;
                    #if DEBUG_SERIAL
                        Serial.println(F("[NANO] Limit CW clear"));
                    #endif
                }
                // CLEAR:AZ:CCW → Fin de course Az CCW dégagée
                else if (strcmp(nanoRxBuffer, "CLEAR:AZ:CCW") == 0) {
                    nanoLimitCCW = false;
                    #if DEBUG_SERIAL
                        Serial.println(F("[NANO] Limit CCW clear"));
                    #endif
                }
                // CLEAR:EL:UP → Fin de course El UP dégagée
                else if (strcmp(nanoRxBuffer, "CLEAR:EL:UP") == 0) {
                    nanoLimitUp = false;
                    #if DEBUG_SERIAL
                        Serial.println(F("[NANO] Limit UP clear"));
                    #endif
                }
                // CLEAR:EL:DOWN → Fin de course El DOWN dégagée
                else if (strcmp(nanoRxBuffer, "CLEAR:EL:DOWN") == 0) {
                    nanoLimitDown = false;
                    #if DEBUG_SERIAL
                        Serial.println(F("[NANO] Limit DOWN clear"));
                    #endif
                }

                nanoRxIndex = 0;
            }
        } else if (nanoRxIndex < sizeof(nanoRxBuffer) - 1) {
            nanoRxBuffer[nanoRxIndex++] = c;
        }
    }
}

// ════════════════════════════════════════════════════════════════
// ARRÊT IMMÉDIAT
// ════════════════════════════════════════════════════════════════

void stopAllMotorsNano() {
    // Envoyer commande combinée STOP (les deux axes à 0)
    NANO_SERIAL.println(F("M:0:0:0"));

    // Reset état local
    targetAz = -1.0;
    targetEl = -1.0;
    movingAz = false;
    movingEl = false;
    currentDirAz = 0;
    currentDirEl = 0;
    currentSpeedMode = 1;

    #if DEBUG_MOTOR_CMD
        Serial.println(F("[NANO] → M:0:0:0 (STOP all)"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// COMMANDE MANUELLE (Vitesse très lente pour pointage précis)
// ════════════════════════════════════════════════════════════════

void sendManualMove(int8_t dirAz, int8_t dirEl) {
    // Annuler les cibles automatiques (mode manuel prioritaire)
    targetAz = -1.0;
    targetEl = -1.0;

    // Si STOP (0,0), repasser en mode automatique
    if (dirAz == 0 && dirEl == 0) {
        // Envoyer STOP et revenir en mode automatique
        NANO_SERIAL.println(F("M:0:0:0"));
        currentDirAz = 0;
        currentDirEl = 0;
        currentSpeedMode = 1;  // Retour mode automatique
        movingAz = false;
        movingEl = false;

        #if DEBUG_MOTOR_CMD
            Serial.println(F("[NANO] → M:0:0:0 (MANUAL STOP → AUTO)"));
        #endif
        return;
    }

    // Envoyer commande avec speed=2 (vitesse manuelle 100 steps/s)
    NANO_SERIAL.print(F("M:"));
    NANO_SERIAL.print(dirAz);
    NANO_SERIAL.print(F(":"));
    NANO_SERIAL.print(dirEl);
    NANO_SERIAL.println(F(":2"));

    // Mettre à jour état local
    currentDirAz = dirAz;
    currentDirEl = dirEl;
    currentSpeedMode = 2;
    movingAz = (dirAz != 0);
    movingEl = (dirEl != 0);

    #if DEBUG_MOTOR_CMD
        Serial.print(F("[NANO] → M:"));
        Serial.print(dirAz);
        Serial.print(F(":"));
        Serial.print(dirEl);
        Serial.println(F(":2 (MANUAL)"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// DEBUG
// ════════════════════════════════════════════════════════════════

void printMotorNanoDebug() {
    // Azimuth
    Serial.print(F("[NANO] Az: "));
    if (targetAz >= 0) {
        Serial.print(targetAz, 1);
    } else {
        Serial.print(F("---"));
    }
    Serial.print(F(" → "));
    Serial.print(currentAz, 1);
    Serial.print(F("° "));

    // Direction Az
    if (currentDirAz > 0) {
        Serial.print(F("[CW]"));
    } else if (currentDirAz < 0) {
        Serial.print(F("[CCW]"));
    } else {
        Serial.print(F("[STOP]"));
    }
    if (nanoLimitCW) Serial.print(F(" LIM_CW!"));
    if (nanoLimitCCW) Serial.print(F(" LIM_CCW!"));

    // Élévation
    Serial.print(F(" | El: "));
    if (targetEl >= 0) {
        Serial.print(targetEl, 1);
    } else {
        Serial.print(F("---"));
    }
    Serial.print(F(" → "));
    Serial.print(currentEl, 1);
    Serial.print(F("° "));

    // Direction El
    if (currentDirEl > 0) {
        Serial.print(F("[UP]"));
    } else if (currentDirEl < 0) {
        Serial.print(F("[DOWN]"));
    } else {
        Serial.print(F("[STOP]"));
    }
    if (nanoLimitUp) Serial.print(F(" LIM_UP!"));
    if (nanoLimitDown) Serial.print(F(" LIM_DOWN!"));

    Serial.println();
}

#endif // USE_NANO_STEPPER
