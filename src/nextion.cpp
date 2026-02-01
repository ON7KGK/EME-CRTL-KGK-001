// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Affichage Nextion (Implementation)
// ════════════════════════════════════════════════════════════════
// Fichier: nextion.cpp
// Description: Gestion affichage écran tactile Nextion
//              Communication UART (Serial1 par défaut)
// ════════════════════════════════════════════════════════════════

#include "nextion.h"

#if ENABLE_NEXTION

#include "encoder_ssi.h"    // Pour currentAz, currentEl
#include "motor_stepper.h"  // Pour targetAz, targetEl
#include "easycom.h"        // Pour parseEasycom() (commandes manuelles)

// ════════════════════════════════════════════════════════════════
// VARIABLES EXTERNES
// ════════════════════════════════════════════════════════════════

extern float currentAz;  // Position actuelle azimuth
extern float currentEl;  // Position actuelle élévation
extern float targetAz;   // Position cible azimuth (-1 si aucune)
extern float targetEl;   // Position cible élévation (-1 si aucune)

// ════════════════════════════════════════════════════════════════
// VARIABLES INTERNES
// ════════════════════════════════════════════════════════════════

unsigned long lastNextionUpdate = 0;  // Dernière mise à jour affichage

// ─────────────────────────────────────────────────────────────────
// ÉTATS BOUTONS TACTILES (mis à jour par readNextionTouch)
// ─────────────────────────────────────────────────────────────────
bool bCW_pressed   = false;  // Bouton CW enfoncé
bool bCCW_pressed  = false;  // Bouton CCW enfoncé
bool bUP_pressed   = false;  // Bouton UP enfoncé
bool bDOWN_pressed = false;  // Bouton DOWN enfoncé
bool bSTOP_pressed = false;  // Bouton STOP enfoncé

unsigned long lastButtonCommand = 0;  // Dernière commande bouton (throttling)
#define BUTTON_COMMAND_INTERVAL 500   // Intervalle commandes bouton (ms)

// ─────────────────────────────────────────────────────────────────
// IDs COMPOSANTS NEXTION (doivent correspondre au fichier .HMI)
// ─────────────────────────────────────────────────────────────────
// Ces IDs sont assignés automatiquement par Nextion Editor.
// Vérifiez-les dans votre fichier .HMI (attribut "id" de chaque bouton).
// Si différents, modifiez les valeurs ci-dessous.

#define NEXTION_ID_BCW   2   // ID bouton CW
#define NEXTION_ID_BCCW  3   // ID bouton CCW
#define NEXTION_ID_BUP   4   // ID bouton UP
#define NEXTION_ID_BDOWN 5   // ID bouton DOWN
#define NEXTION_ID_BSTOP 6   // ID bouton STOP

// ════════════════════════════════════════════════════════════════
// INITIALISATION
// ════════════════════════════════════════════════════════════════

void setupNextion() {
    // Configuration port série Nextion
    NEXTION_SERIAL.begin(NEXTION_BAUD);
    delay(100);  // Stabilisation

    #if DEBUG_SERIAL
        Serial.println(F("════════════════════════════════════════════════════════════════"));
        Serial.println(F("    NEXTION DISPLAY INITIALISATION"));
        Serial.println(F("════════════════════════════════════════════════════════════════"));
        Serial.print(F("Port série: "));
        #if NEXTION_SERIAL == Serial1
            Serial.println(F("Serial1 (pins 18/19)"));
        #elif NEXTION_SERIAL == Serial2
            Serial.println(F("Serial2 (pins 16/17)"));
        #elif NEXTION_SERIAL == Serial3
            Serial.println(F("Serial3 (pins 14/15)"));
        #endif
        Serial.print(F("Vitesse: "));
        Serial.print(NEXTION_BAUD);
        Serial.println(F(" baud"));
        Serial.print(F("Intervalle MAJ: "));
        Serial.print(NEXTION_UPDATE_INTERVAL);
        Serial.println(F(" ms"));
    #endif

    // Reset Nextion (optionnel - décommentez si nécessaire)
    // sendToNextion("rest");  // Commande reset
    // delay(500);

    // Affichage page accueil
    showNextionHomePage();

    // Message initial
    sendToNextion("tStatus.txt=\"Initialisation...\"");

    delay(100);

    #if DEBUG_SERIAL
        Serial.println(F("✓ Nextion initialisé"));
        Serial.println(F(""));
    #endif
}

// ════════════════════════════════════════════════════════════════
// MISE À JOUR AFFICHAGE (Appelé dans loop)
// ════════════════════════════════════════════════════════════════

void updateNextion() {
    // ─────────────────────────────────────────────────────────────
    // THROTTLING (ne pas saturer le Nextion)
    // ─────────────────────────────────────────────────────────────
    unsigned long currentTime = millis();
    if (currentTime - lastNextionUpdate < NEXTION_UPDATE_INTERVAL) {
        return;  // Pas encore temps de mettre à jour
    }
    lastNextionUpdate = currentTime;

    // ─────────────────────────────────────────────────────────────
    // AZIMUTH - Position actuelle
    // ─────────────────────────────────────────────────────────────
    String cmd = "tAzCur.txt=\"";
    cmd += String(currentAz, 1);  // 1 décimale (ex: "123.5")
    cmd += "°\"";
    sendToNextion(cmd);

    // ─────────────────────────────────────────────────────────────
    // AZIMUTH - Position cible (si active)
    // ─────────────────────────────────────────────────────────────
    if (targetAz >= 0.0) {  // -1.0 = pas de cible active
        cmd = "tAzTgt.txt=\"";
        cmd += String(targetAz, 1);
        cmd += "°\"";
        sendToNextion(cmd);
    } else {
        // Aucune cible - afficher "---"
        sendToNextion("tAzTgt.txt=\"---\"");
    }

    // ─────────────────────────────────────────────────────────────
    // ÉLÉVATION - Position actuelle
    // ─────────────────────────────────────────────────────────────
    cmd = "tElCur.txt=\"";
    cmd += String(currentEl, 1);
    cmd += "°\"";
    sendToNextion(cmd);

    // ─────────────────────────────────────────────────────────────
    // ÉLÉVATION - Position cible (si active)
    // ─────────────────────────────────────────────────────────────
    if (targetEl >= 0.0) {
        cmd = "tElTgt.txt=\"";
        cmd += String(targetEl, 1);
        cmd += "°\"";
        sendToNextion(cmd);
    } else {
        sendToNextion("tElTgt.txt=\"---\"");
    }

    // ─────────────────────────────────────────────────────────────
    // INDICATEUR MOUVEMENT (optionnel)
    // ─────────────────────────────────────────────────────────────
    // Si cible active ET différente de position actuelle → en mouvement
    bool moving = false;
    if (targetAz >= 0.0 && abs(targetAz - currentAz) > 0.5) moving = true;
    if (targetEl >= 0.0 && abs(targetEl - currentEl) > 0.5) moving = true;

    if (moving) {
        sendToNextion("tStatus.txt=\"EN MOUVEMENT\"");
        sendToNextion("tStatus.pco=63488");  // Couleur rouge (RGB565)
    } else {
        sendToNextion("tStatus.txt=\"ARRETE\"");
        sendToNextion("tStatus.pco=2016");   // Couleur verte (RGB565)
    }

    #if DEBUG_NEXTION
        Serial.print(F("[NEXTION] Az:"));
        Serial.print(currentAz, 1);
        Serial.print(F("° → "));
        if (targetAz >= 0) Serial.print(targetAz, 1);
        else Serial.print(F("---"));
        Serial.print(F("° | El:"));
        Serial.print(currentEl, 1);
        Serial.print(F("° → "));
        if (targetEl >= 0) Serial.println(targetEl, 1);
        else Serial.println(F("---"));
    #endif
}

// ════════════════════════════════════════════════════════════════
// ENVOI COMMANDE NEXTION
// ════════════════════════════════════════════════════════════════

void sendToNextion(String command) {
    // Envoi commande texte
    NEXTION_SERIAL.print(command);

    // Terminateur Nextion: 3× 0xFF (OBLIGATOIRE!)
    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);
    NEXTION_SERIAL.write(0xFF);

    #if DEBUG_NEXTION_VERBOSE
        Serial.print(F("[→ NEXTION] "));
        Serial.println(command);
    #endif
}

// ════════════════════════════════════════════════════════════════
// PAGE ACCUEIL
// ════════════════════════════════════════════════════════════════

void showNextionHomePage() {
    // Afficher page 0 (page principale rotator)
    sendToNextion("page 0");
    delay(50);

    // Titre (si composant tTitle existe)
    sendToNextion("tTitle.txt=\"EME ROTATOR\"");

    // Initialiser champs vides
    sendToNextion("tAzCur.txt=\"---\"");
    sendToNextion("tAzTgt.txt=\"---\"");
    sendToNextion("tElCur.txt=\"---\"");
    sendToNextion("tElTgt.txt=\"---\"");
    sendToNextion("tStatus.txt=\"INITIALISATION\"");
}

// ════════════════════════════════════════════════════════════════
// AFFICHAGE ERREUR
// ════════════════════════════════════════════════════════════════

void showNextionError(String errorMsg) {
    // Afficher message erreur rouge
    String cmd = "tStatus.txt=\"ERREUR: ";
    cmd += errorMsg;
    cmd += "\"";
    sendToNextion(cmd);
    sendToNextion("tStatus.pco=63488");  // Rouge

    #if DEBUG_SERIAL
        Serial.print(F("[NEXTION] ERREUR: "));
        Serial.println(errorMsg);
    #endif
}

// ════════════════════════════════════════════════════════════════
// LECTURE ÉVÉNEMENTS TACTILES
// ════════════════════════════════════════════════════════════════

void readNextionTouch() {
    // ─────────────────────────────────────────────────────────────
    // VÉRIFICATION DONNÉES DISPONIBLES
    // ─────────────────────────────────────────────────────────────
    if (NEXTION_SERIAL.available() < 7) {
        return;  // Pas assez de données (événement complet = 7 octets)
    }

    // ─────────────────────────────────────────────────────────────
    // LECTURE ÉVÉNEMENT NEXTION
    // ─────────────────────────────────────────────────────────────
    // Format: 0x65 [Page] [Component ID] [Event] 0xFF 0xFF 0xFF
    //         ^^^^^                       ^^^^^^ ^^^^^^^^^^^^
    //         Header                      Touch  Terminateurs
    //                                     0x01=Press
    //                                     0x00=Release

    uint8_t header = NEXTION_SERIAL.read();

    // Vérifier si c'est un événement touch (0x65)
    if (header != 0x65) {
        // Autre type événement - purger buffer
        while (NEXTION_SERIAL.available()) {
            NEXTION_SERIAL.read();
        }
        return;
    }

    // Lire les données événement
    uint8_t page = NEXTION_SERIAL.read();
    uint8_t componentID = NEXTION_SERIAL.read();
    uint8_t touchEvent = NEXTION_SERIAL.read();

    // Lire terminateurs 0xFF 0xFF 0xFF
    NEXTION_SERIAL.read();
    NEXTION_SERIAL.read();
    NEXTION_SERIAL.read();

    // ─────────────────────────────────────────────────────────────
    // PARSING ÉVÉNEMENT (Press = 0x01, Release = 0x00)
    // ─────────────────────────────────────────────────────────────
    bool isPressed = (touchEvent == 0x01);

    #if DEBUG_NEXTION_VERBOSE
        Serial.print(F("[NEXTION TOUCH] Page="));
        Serial.print(page);
        Serial.print(F(" ID="));
        Serial.print(componentID);
        Serial.print(F(" Event="));
        Serial.println(isPressed ? F("PRESS") : F("RELEASE"));
    #endif

    // ─────────────────────────────────────────────────────────────
    // MISE À JOUR ÉTATS BOUTONS (logique mutex: 1 seul bouton actif)
    // ─────────────────────────────────────────────────────────────
    if (componentID == NEXTION_ID_BCW) {
        if (isPressed) {
            // CW enfoncé → désactiver les autres
            bCW_pressed = true;
            bCCW_pressed = false;
            bUP_pressed = false;
            bDOWN_pressed = false;
            bSTOP_pressed = false;
        } else {
            bCW_pressed = false;
        }
    }
    else if (componentID == NEXTION_ID_BCCW) {
        if (isPressed) {
            bCCW_pressed = true;
            bCW_pressed = false;
            bUP_pressed = false;
            bDOWN_pressed = false;
            bSTOP_pressed = false;
        } else {
            bCCW_pressed = false;
        }
    }
    else if (componentID == NEXTION_ID_BUP) {
        if (isPressed) {
            bUP_pressed = true;
            bCW_pressed = false;
            bCCW_pressed = false;
            bDOWN_pressed = false;
            bSTOP_pressed = false;
        } else {
            bUP_pressed = false;
        }
    }
    else if (componentID == NEXTION_ID_BDOWN) {
        if (isPressed) {
            bDOWN_pressed = true;
            bCW_pressed = false;
            bCCW_pressed = false;
            bUP_pressed = false;
            bSTOP_pressed = false;
        } else {
            bDOWN_pressed = false;
        }
    }
    else if (componentID == NEXTION_ID_BSTOP) {
        if (isPressed) {
            bSTOP_pressed = true;
            bCW_pressed = false;
            bCCW_pressed = false;
            bUP_pressed = false;
            bDOWN_pressed = false;
        } else {
            bSTOP_pressed = false;
        }
    }
}

// ════════════════════════════════════════════════════════════════
// GESTION BOUTONS TACTILES (Envoi commandes Easycom)
// ════════════════════════════════════════════════════════════════

void handleNextionButtons() {
    // ─────────────────────────────────────────────────────────────
    // THROTTLING (ne pas saturer Easycom)
    // ─────────────────────────────────────────────────────────────
    unsigned long currentTime = millis();
    if (currentTime - lastButtonCommand < BUTTON_COMMAND_INTERVAL) {
        return;  // Pas encore temps d'envoyer commande
    }

    // ─────────────────────────────────────────────────────────────
    // VÉRIFICATION BOUTON ACTIF (logique mutex: 1 seul à la fois)
    // ─────────────────────────────────────────────────────────────
    String command = "";

    if (bSTOP_pressed) {
        // ─────────────────────────────────────────────────────────
        // BOUTON STOP → Commande SA (Stop All)
        // ─────────────────────────────────────────────────────────
        command = "SA";
        lastButtonCommand = currentTime;

        #if DEBUG_NEXTION
            Serial.println(F("[NEXTION BTN] STOP → Commande SA"));
        #endif
    }
    else if (bCW_pressed) {
        // ─────────────────────────────────────────────────────────
        // BOUTON CW → Incrément azimuth (sens horaire)
        // ─────────────────────────────────────────────────────────
        float newTarget = currentAz + MANUAL_INCREMENT_AZ;

        // Gérer wraparound 0-360°
        if (newTarget >= 360.0) newTarget -= 360.0;

        command = "AZ";
        command += String(newTarget, 1);  // 1 décimale
        lastButtonCommand = currentTime;

        #if DEBUG_NEXTION
            Serial.print(F("[NEXTION BTN] CW → AZ "));
            Serial.print(currentAz, 1);
            Serial.print(F("° + "));
            Serial.print(MANUAL_INCREMENT_AZ, 1);
            Serial.print(F("° = "));
            Serial.println(newTarget, 1);
        #endif
    }
    else if (bCCW_pressed) {
        // ─────────────────────────────────────────────────────────
        // BOUTON CCW → Décrément azimuth (sens anti-horaire)
        // ─────────────────────────────────────────────────────────
        float newTarget = currentAz - MANUAL_INCREMENT_AZ;

        // Gérer wraparound 0-360°
        if (newTarget < 0.0) newTarget += 360.0;

        command = "AZ";
        command += String(newTarget, 1);
        lastButtonCommand = currentTime;

        #if DEBUG_NEXTION
            Serial.print(F("[NEXTION BTN] CCW → AZ "));
            Serial.print(currentAz, 1);
            Serial.print(F("° - "));
            Serial.print(MANUAL_INCREMENT_AZ, 1);
            Serial.print(F("° = "));
            Serial.println(newTarget, 1);
        #endif
    }
    else if (bUP_pressed) {
        // ─────────────────────────────────────────────────────────
        // BOUTON UP → Incrément élévation (montante)
        // ─────────────────────────────────────────────────────────
        float newTarget = currentEl + MANUAL_INCREMENT_EL;

        // Limiter à 0-90° (ou limite config)
        if (newTarget > 90.0) newTarget = 90.0;

        command = "EL";
        command += String(newTarget, 1);
        lastButtonCommand = currentTime;

        #if DEBUG_NEXTION
            Serial.print(F("[NEXTION BTN] UP → EL "));
            Serial.print(currentEl, 1);
            Serial.print(F("° + "));
            Serial.print(MANUAL_INCREMENT_EL, 1);
            Serial.print(F("° = "));
            Serial.println(newTarget, 1);
        #endif
    }
    else if (bDOWN_pressed) {
        // ─────────────────────────────────────────────────────────
        // BOUTON DOWN → Décrément élévation (descendante)
        // ─────────────────────────────────────────────────────────
        float newTarget = currentEl - MANUAL_INCREMENT_EL;

        // Limiter à 0-90°
        if (newTarget < 0.0) newTarget = 0.0;

        command = "EL";
        command += String(newTarget, 1);
        lastButtonCommand = currentTime;

        #if DEBUG_NEXTION
            Serial.print(F("[NEXTION BTN] DOWN → EL "));
            Serial.print(currentEl, 1);
            Serial.print(F("° - "));
            Serial.print(MANUAL_INCREMENT_EL, 1);
            Serial.print(F("° = "));
            Serial.println(newTarget, 1);
        #endif
    }

    // ─────────────────────────────────────────────────────────────
    // ENVOI COMMANDE EASYCOM (si bouton actif)
    // ─────────────────────────────────────────────────────────────
    if (command.length() > 0) {
        // Parser commande via fonction Easycom existante
        // Cela met à jour targetAz/targetEl et démarre les moteurs
        parseEasycomCommand(command);

        #if DEBUG_SERIAL
            Serial.print(F("[NEXTION → EASYCOM] "));
            Serial.println(command);
        #endif
    }
}

// ════════════════════════════════════════════════════════════════
// MISE À JOUR INDICATEURS ÉTAT SYSTÈME
// ════════════════════════════════════════════════════════════════

void updateNextionIndicators() {
    // ─────────────────────────────────────────────────────────────
    // INDICATEUR DIRECTION AZIMUTH (tAzDir)
    // ─────────────────────────────────────────────────────────────
    // Logique:
    //   - Si cible active ET différente de position actuelle
    //   - Afficher "CW" si cible > actuelle, "CCW" sinon
    //   - Sinon "---" (arrêté)

    if (targetAz >= 0.0 && abs(targetAz - currentAz) > 0.5) {
        // Mouvement azimuth en cours
        if (targetAz > currentAz) {
            // Sens horaire (shortest path peut être CCW, mais simplifié ici)
            sendToNextion("tAzDir.txt=\"CW\"");
            sendToNextion("tAzDir.pco=65535");  // Jaune
        } else {
            sendToNextion("tAzDir.txt=\"CCW\"");
            sendToNextion("tAzDir.pco=65535");  // Jaune
        }
    } else {
        // Arrêté
        sendToNextion("tAzDir.txt=\"---\"");
        sendToNextion("tAzDir.pco=2016");  // Vert
    }

    // ─────────────────────────────────────────────────────────────
    // INDICATEUR DIRECTION ÉLÉVATION (tElDir)
    // ─────────────────────────────────────────────────────────────
    if (targetEl >= 0.0 && abs(targetEl - currentEl) > 0.5) {
        // Mouvement élévation en cours
        if (targetEl > currentEl) {
            sendToNextion("tElDir.txt=\"UP\"");
            sendToNextion("tElDir.pco=65535");  // Jaune
        } else {
            sendToNextion("tElDir.txt=\"DOWN\"");
            sendToNextion("tElDir.pco=65535");  // Jaune
        }
    } else {
        sendToNextion("tElDir.txt=\"---\"");
        sendToNextion("tElDir.pco=2016");  // Vert
    }

    // ─────────────────────────────────────────────────────────────
    // INDICATEUR MODE SYSTÈME (tMode)
    // ─────────────────────────────────────────────────────────────
    // TODO: Implémenter détection mode PARKING depuis PstRotator
    // Pour l'instant, afficher STOP si bouton STOP enfoncé

    if (bSTOP_pressed) {
        sendToNextion("tMode.txt=\"STOP\"");
        sendToNextion("tMode.pco=63488");  // Rouge
    } else {
        // Mode normal
        sendToNextion("tMode.txt=\"---\"");
        sendToNextion("tMode.pco=2016");  // Vert
    }
}

#endif // ENABLE_NEXTION
