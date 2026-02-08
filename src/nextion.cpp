// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Affichage Nextion (Implementation)
// ════════════════════════════════════════════════════════════════
// Fichier: nextion.cpp
// Description: Gestion affichage écran tactile Nextion
//              Communication UART (Serial1 par défaut)
// ════════════════════════════════════════════════════════════════

#include "nextion.h"

#if ENABLE_NEXTION

#include <EEPROM.h>          // Pour sauvegarde offset
#include "encoder_ssi.h"    // Pour currentAz, currentEl
#include "motor_stepper.h"  // Pour targetAz, targetEl
#include "easycom.h"        // Pour parseEasycom() (commandes automatiques)
#include "network.h"        // Pour isClientConnected()
#if USE_NANO_STEPPER
    #include "motor_nano.h" // Pour sendManualMove(), nanoLimitAz, nanoLimitEl
#endif

// ════════════════════════════════════════════════════════════════
// VARIABLES EXTERNES
// ════════════════════════════════════════════════════════════════

extern float currentAz;  // Position actuelle azimuth
extern float currentEl;  // Position actuelle élévation
extern float targetAz;   // Position cible azimuth (NO_TARGET si aucune)
extern float targetEl;   // Position cible élévation (NO_TARGET si aucune)

// Valeur sentinel pour "pas de cible active"
// IMPORTANT: -999.0 au lieu de -1.0 pour permettre les cibles négatives (ex: El = -5°)
#define NO_TARGET -999.0

// ════════════════════════════════════════════════════════════════
// VARIABLES INTERNES
// ════════════════════════════════════════════════════════════════

unsigned long lastNextionUpdate = 0;  // Dernière mise à jour affichage

// ─────────────────────────────────────────────────────────────────
// PERSISTANCE AFFICHAGE CIBLES (pour mode tracking PstRotator)
// ─────────────────────────────────────────────────────────────────
#define TARGET_DISPLAY_PERSIST_MS  3600000UL  // Durée affichage après atteinte cible (1 heure)

float lastDisplayedTargetAz = NO_TARGET;   // Dernière cible Az affichée
float lastDisplayedTargetEl = NO_TARGET;   // Dernière cible El affichée
unsigned long lastTargetAzTime = 0;        // Timestamp dernière cible Az reçue
unsigned long lastTargetElTime = 0;        // Timestamp dernière cible El reçue

// ─────────────────────────────────────────────────────────────────
// ÉTATS BOUTONS TACTILES (mis à jour par readNextionTouch)
// ─────────────────────────────────────────────────────────────────
bool bCW_pressed   = false;  // Bouton CW enfoncé
bool bCCW_pressed  = false;  // Bouton CCW enfoncé
bool bUP_pressed   = false;  // Bouton UP enfoncé
bool bDOWN_pressed = false;  // Bouton DOWN enfoncé
bool bSTOP_pressed = false;  // Bouton STOP enfoncé

unsigned long lastButtonCommand = 0;  // Dernière commande bouton (throttling)
#define BUTTON_COMMAND_INTERVAL 50    // Intervalle minimum entre commandes (ms)

// États précédents boutons pour détecter fronts (appui/relâchement)
bool prevCW = false;
bool prevCCW = false;
bool prevUP = false;
bool prevDOWN = false;
bool prevSTOP = false;

// État mouvement manuel en cours
bool manualMoving = false;

// ─────────────────────────────────────────────────────────────────
// OFFSET AFFICHAGE ÉLÉVATION (Parabole offset)
// ─────────────────────────────────────────────────────────────────
float elDisplayOffset = 0.0;    // Offset en degrés (display-only)
bool elOffsetEnabled = false;   // Affichage offset activé

// ─────────────────────────────────────────────────────────────────
// IDs COMPOSANTS NEXTION (doivent correspondre au fichier .HMI)
// ─────────────────────────────────────────────────────────────────
// Ces IDs sont assignés automatiquement par Nextion Editor.
// Vérifiez-les dans votre fichier .HMI (attribut "id" de chaque bouton).
// Si différents, modifiez les valeurs ci-dessous.

#define NEXTION_ID_BCW   6    // ID bouton CW (b0)
#define NEXTION_ID_BCCW  7    // ID bouton CCW (b1)
#define NEXTION_ID_BUP   8    // ID bouton UP (b2)
#define NEXTION_ID_BDOWN 9    // ID bouton DOWN (b3)
#define NEXTION_ID_BSTOP 10   // ID bouton STOP (b4)

// ─────────────────────────────────────────────────────────────────
// IDs COMPOSANTS CALIBRATION (Textes position pour appui long)
// ─────────────────────────────────────────────────────────────────
#define NEXTION_ID_TAZCUR  1   // ID texte tAzCur (position Az actuelle)
#define NEXTION_ID_TELCUR  3   // ID texte tElCur (position El actuelle)

// ─────────────────────────────────────────────────────────────────
// VARIABLES CALIBRATION PAR APPUI LONG
// ─────────────────────────────────────────────────────────────────
#define CALIBRATION_LONG_PRESS_MS  3000  // Durée appui long (3 secondes)

unsigned long calibTouchStartAz = 0;     // Timestamp début appui tAzCur
unsigned long calibTouchStartEl = 0;     // Timestamp début appui tElCur
bool calibTouchingAz = false;            // Appui en cours sur tAzCur
bool calibTouchingEl = false;            // Appui en cours sur tElCur
bool calibFeedbackSentAz = false;        // Feedback "CAL..." déjà envoyé pour Az
bool calibFeedbackSentEl = false;        // Feedback "CAL..." déjà envoyé pour El

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

    // ─────────────────────────────────────────────────────────────
    // CHARGEMENT OFFSET ÉLÉVATION DEPUIS EEPROM
    // ─────────────────────────────────────────────────────────────
    float savedOffset;
    EEPROM.get(EEPROM_EL_DISPLAY_OFFSET, savedOffset);
    if (!isnan(savedOffset) && savedOffset > -90.0 && savedOffset < 90.0) {
        elDisplayOffset = savedOffset;
    } else {
        elDisplayOffset = 0.0;
        EEPROM.put(EEPROM_EL_DISPLAY_OFFSET, elDisplayOffset);
    }
    uint8_t savedEnabled;
    EEPROM.get(EEPROM_EL_OFFSET_ENABLED, savedEnabled);
    elOffsetEnabled = (savedEnabled == 1);

    #if DEBUG_SERIAL
        Serial.print(F("Offset El: "));
        Serial.print(elDisplayOffset, 1);
        Serial.print(F("° ("));
        Serial.print(elOffsetEnabled ? F("ON") : F("OFF"));
        Serial.println(F(")"));
    #endif

    // Affichage page accueil
    showNextionHomePage();

    // Couleur et texte initial bouton offset
    sendToNextion(elOffsetEnabled ? "bOffsetToggle.pco=2016" : "bOffsetToggle.pco=65535");
    {
        String cmd = "bOffsetToggle.txt=\"";
        if (elDisplayOffset >= 0) cmd += "+";
        cmd += String(elDisplayOffset, 1);
        cmd += "\"";
        sendToNextion(cmd);
    }

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
    // AZIMUTH - Position cible (avec persistance pour tracking)
    // ─────────────────────────────────────────────────────────────
    if (targetAz > NO_TARGET) {
        // Nouvelle cible active - mémoriser et afficher
        lastDisplayedTargetAz = targetAz;
        lastTargetAzTime = currentTime;
        cmd = "tAzTgt.txt=\"";
        cmd += String(targetAz, 1);
        cmd += "°\"";
        sendToNextion(cmd);
    } else if (lastDisplayedTargetAz > NO_TARGET &&
               (currentTime - lastTargetAzTime) < TARGET_DISPLAY_PERSIST_MS) {
        // Pas de cible active mais on garde l'affichage pendant 5s
        cmd = "tAzTgt.txt=\"(";
        cmd += String(lastDisplayedTargetAz, 1);
        cmd += ")\"";  // Parenthèses pour indiquer "atteint"
        sendToNextion(cmd);
    } else {
        // Timeout - effacer affichage
        if (lastDisplayedTargetAz > NO_TARGET) {
            lastDisplayedTargetAz = NO_TARGET;
        }
        sendToNextion("tAzTgt.txt=\"---\"");
    }

    // ─────────────────────────────────────────────────────────────
    // ÉLÉVATION - Position actuelle (avec offset parabole si activé)
    // ─────────────────────────────────────────────────────────────
    float displayEl = elOffsetEnabled ? (currentEl + elDisplayOffset) : currentEl;
    cmd = "tElCur.txt=\"";
    cmd += String(displayEl, 1);
    cmd += "°\"";
    sendToNextion(cmd);

    // ─────────────────────────────────────────────────────────────
    // ÉLÉVATION - Position cible (avec persistance pour tracking)
    // ─────────────────────────────────────────────────────────────
    if (targetEl > NO_TARGET) {
        // Nouvelle cible active - mémoriser et afficher
        lastDisplayedTargetEl = targetEl;
        lastTargetElTime = currentTime;
        float displayTargetEl = elOffsetEnabled ? (targetEl + elDisplayOffset) : targetEl;
        cmd = "tElTgt.txt=\"";
        cmd += String(displayTargetEl, 1);
        cmd += "°\"";
        sendToNextion(cmd);
    } else if (lastDisplayedTargetEl > NO_TARGET &&
               (currentTime - lastTargetElTime) < TARGET_DISPLAY_PERSIST_MS) {
        // Pas de cible active mais on garde l'affichage
        float displayLastEl = elOffsetEnabled ? (lastDisplayedTargetEl + elDisplayOffset) : lastDisplayedTargetEl;
        cmd = "tElTgt.txt=\"(";
        cmd += String(displayLastEl, 1);
        cmd += ")\"";  // Parenthèses pour indiquer "atteint"
        sendToNextion(cmd);
    } else {
        // Timeout - effacer affichage
        if (lastDisplayedTargetEl > NO_TARGET) {
            lastDisplayedTargetEl = NO_TARGET;
        }
        sendToNextion("tElTgt.txt=\"---\"");
    }

    // ─────────────────────────────────────────────────────────────
    // OFFSET ÉLÉVATION - Valeur affichée sur le bouton toggle
    // ─────────────────────────────────────────────────────────────
    cmd = "bOffsetToggle.txt=\"";
    if (elDisplayOffset >= 0) cmd += "+";
    cmd += String(elDisplayOffset, 1);
    cmd += "\"";
    sendToNextion(cmd);

    // ─────────────────────────────────────────────────────────────
    // INDICATEUR STATUT SYSTÈME (tStatus)
    // Priorité: 1) Fins de course (avec direction)  2) Connexion client
    // ─────────────────────────────────────────────────────────────
    static uint8_t lastStatus = 255;  // Pour éviter envois répétés
    // 0=OK, 1=PST OK, 2=PST DISC, 3=LIM CW, 4=LIM CCW, 5=LIM UP, 6=LIM DOWN, 7=LIMIT!(multi)
    uint8_t newStatus = 0;

    #if USE_NANO_STEPPER
        // Compter nombre de limites actives
        uint8_t limitCount = 0;
        if (nanoLimitCW) limitCount++;
        if (nanoLimitCCW) limitCount++;
        if (nanoLimitUp) limitCount++;
        if (nanoLimitDown) limitCount++;

        // Priorité 1: Fins de course (critique!)
        if (limitCount > 1) {
            newStatus = 7;  // LIMIT! (plusieurs)
        } else if (nanoLimitCW) {
            newStatus = 3;  // LIM CW
        } else if (nanoLimitCCW) {
            newStatus = 4;  // LIM CCW
        } else if (nanoLimitUp) {
            newStatus = 5;  // LIM UP
        } else if (nanoLimitDown) {
            newStatus = 6;  // LIM DOWN
        } else
    #endif
    {
        // Priorité 2: État connexion client
        if (isClientConnected()) {
            newStatus = 1;  // PST OK
        } else {
            newStatus = 2;  // PST DISC
        }
    }

    // Envoyer seulement si changement
    if (newStatus != lastStatus) {
        lastStatus = newStatus;

        switch (newStatus) {
            case 0:  // OK (pas utilisé actuellement)
                sendToNextion("tStatus.txt=\"OK\"");
                sendToNextion("tStatus.pco=2016");   // Vert
                break;
            case 1:  // PstRotator connecté
                sendToNextion("tStatus.txt=\"PST OK\"");
                sendToNextion("tStatus.pco=2016");   // Vert
                break;
            case 2:  // PstRotator déconnecté
                sendToNextion("tStatus.txt=\"PST DISC\"");
                sendToNextion("tStatus.pco=65504");  // Jaune
                break;
            case 3:  // LIMIT CW
                sendToNextion("tStatus.txt=\"LIM CW!\"");
                sendToNextion("tStatus.pco=63488");  // Rouge
                break;
            case 4:  // LIMIT CCW
                sendToNextion("tStatus.txt=\"LIM CCW!\"");
                sendToNextion("tStatus.pco=63488");  // Rouge
                break;
            case 5:  // LIMIT UP
                sendToNextion("tStatus.txt=\"LIM UP!\"");
                sendToNextion("tStatus.pco=63488");  // Rouge
                break;
            case 6:  // LIMIT DOWN
                sendToNextion("tStatus.txt=\"LIM DOWN!\"");
                sendToNextion("tStatus.pco=63488");  // Rouge
                break;
            case 7:  // LIMIT! (plusieurs)
                sendToNextion("tStatus.txt=\"LIMIT!\"");
                sendToNextion("tStatus.pco=63488");  // Rouge
                break;
        }

        #if DEBUG_NEXTION
            const char* statusNames[] = {"OK", "PST OK", "PST DISC", "LIM CW", "LIM CCW", "LIM UP", "LIM DOWN", "LIMIT!"};
            Serial.print(F("[NEXTION] Status: "));
            Serial.println(statusNames[newStatus]);
        #endif
    }

    #if DEBUG_NEXTION
        Serial.print(F("[NEXTION] Az:"));
        Serial.print(currentAz, 1);
        Serial.print(F("° → "));
        if (targetAz > NO_TARGET) {
            Serial.print(targetAz, 1);
        } else if (lastDisplayedTargetAz > NO_TARGET) {
            Serial.print(F("("));
            Serial.print(lastDisplayedTargetAz, 1);
            Serial.print(F(")"));
        } else {
            Serial.print(F("---"));
        }
        Serial.print(F("° | El:"));
        Serial.print(currentEl, 1);
        Serial.print(F("° → "));
        if (targetEl > NO_TARGET) {
            Serial.println(targetEl, 1);
        } else if (lastDisplayedTargetEl > NO_TARGET) {
            Serial.print(F("("));
            Serial.print(lastDisplayedTargetEl, 1);
            Serial.println(F(")"));
        } else {
            Serial.println(F("---"));
        }
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
    // CONSOMMER LES BYTES NON-TOUCH (accusés réception Nextion)
    // ─────────────────────────────────────────────────────────────
    // Le Nextion envoie des accusés de réception (0x00, 0x01, etc.)
    // pour chaque commande reçue. On doit les consommer pour éviter
    // que le buffer se remplisse.

    while (NEXTION_SERIAL.available() > 0) {
        uint8_t peek = NEXTION_SERIAL.peek();

        // Si c'est un événement touch (0x65), on sort de la boucle
        if (peek == 0x65) {
            break;
        }

        // Sinon, consommer cet octet (accusé réception ou autre)
        NEXTION_SERIAL.read();
    }

    // ─────────────────────────────────────────────────────────────
    // VÉRIFICATION DONNÉES DISPONIBLES POUR TOUCH EVENT
    // ─────────────────────────────────────────────────────────────
    int avail = NEXTION_SERIAL.available();
    if (avail < 7) {
        return;  // Pas assez de données (événement touch = 7 octets)
    }

    // ─────────────────────────────────────────────────────────────
    // LECTURE ÉVÉNEMENT NEXTION TOUCH
    // ─────────────────────────────────────────────────────────────
    // Format: 0x65 [Page] [Component ID] [Event] 0xFF 0xFF 0xFF

    uint8_t header = NEXTION_SERIAL.read();

    // Double vérification (devrait être 0x65)
    if (header != 0x65) {
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
    // ─────────────────────────────────────────────────────────────
    // CALIBRATION: Appui long sur tAzCur ou tElCur
    // ─────────────────────────────────────────────────────────────
    else if (componentID == NEXTION_ID_TAZCUR) {
        if (isPressed) {
            calibTouchStartAz = millis();
            calibTouchingAz = true;
            calibFeedbackSentAz = false;
            #if DEBUG_NEXTION
                Serial.println(F("[NEXTION] Touch tAzCur START"));
            #endif
        } else {
            calibTouchingAz = false;
            calibTouchStartAz = 0;
            #if DEBUG_NEXTION
                Serial.println(F("[NEXTION] Touch tAzCur RELEASE"));
            #endif
        }
    }
    else if (componentID == NEXTION_ID_TELCUR) {
        if (isPressed) {
            calibTouchStartEl = millis();
            calibTouchingEl = true;
            calibFeedbackSentEl = false;
            #if DEBUG_NEXTION
                Serial.println(F("[NEXTION] Touch tElCur START"));
            #endif
        } else {
            calibTouchingEl = false;
            calibTouchStartEl = 0;
            #if DEBUG_NEXTION
                Serial.println(F("[NEXTION] Touch tElCur RELEASE"));
            #endif
        }
    }
    // ─────────────────────────────────────────────────────────────
    // BOUTONS OFFSET ÉLÉVATION (pas de mutex avec boutons moteur)
    // ─────────────────────────────────────────────────────────────
    else if (componentID == NEXTION_ID_OFFSET_PLUS && isPressed) {
        elDisplayOffset += EL_DISPLAY_OFFSET_STEP;
        EEPROM.put(EEPROM_EL_DISPLAY_OFFSET, elDisplayOffset);
        #if DEBUG_NEXTION
            Serial.print(F("[NEXTION] Offset El + → "));
            Serial.println(elDisplayOffset, 1);
        #endif
    }
    else if (componentID == NEXTION_ID_OFFSET_MINUS && isPressed) {
        elDisplayOffset -= EL_DISPLAY_OFFSET_STEP;
        EEPROM.put(EEPROM_EL_DISPLAY_OFFSET, elDisplayOffset);
        #if DEBUG_NEXTION
            Serial.print(F("[NEXTION] Offset El - → "));
            Serial.println(elDisplayOffset, 1);
        #endif
    }
    else if (componentID == NEXTION_ID_OFFSET_TOGGLE && isPressed) {
        elOffsetEnabled = !elOffsetEnabled;
        EEPROM.put(EEPROM_EL_OFFSET_ENABLED, (uint8_t)elOffsetEnabled);
        // Couleur bouton: vert si ON, blanc si OFF
        sendToNextion(elOffsetEnabled ? "bOffsetToggle.pco=2016" : "bOffsetToggle.pco=65535");
        #if DEBUG_NEXTION
            Serial.print(F("[NEXTION] Offset El toggle → "));
            Serial.println(elOffsetEnabled ? F("ON") : F("OFF"));
        #endif
    }
}

// ════════════════════════════════════════════════════════════════
// GESTION CALIBRATION PAR APPUI LONG (3 secondes sur tAzCur/tElCur)
// ════════════════════════════════════════════════════════════════

void handleCalibrationTouch() {
    unsigned long now = millis();

    // ─────────────────────────────────────────────────────────────
    // CALIBRATION AZIMUTH (appui long sur tAzCur)
    // ─────────────────────────────────────────────────────────────
    if (calibTouchingAz && calibTouchStartAz > 0) {
        unsigned long elapsed = now - calibTouchStartAz;

        // Feedback visuel après 1 seconde d'appui (en cours...)
        if (elapsed > 1000 && !calibFeedbackSentAz) {
            calibFeedbackSentAz = true;
            sendToNextion("tAzCur.pco=31");           // Bleu (calibration en cours)
            sendToNextion("tAzCur.txt=\"CAL...\"");
            #if DEBUG_NEXTION
                Serial.println(F("[NEXTION] Calibration Az en cours..."));
            #endif
        }

        // Calibration après 3 secondes d'appui
        if (elapsed >= CALIBRATION_LONG_PRESS_MS) {
            // Effectuer la calibration
            calibrateAz(0.0);

            // Feedback visuel
            sendToNextion("tAzCur.pco=2016");         // Vert (succès)
            sendToNextion("tAzCur.txt=\"0.0\\xB0\""); // Afficher 0.0°

            // Message status temporaire
            sendToNextion("tStatus.txt=\"AZ CAL OK\"");
            sendToNextion("tStatus.pco=2016");        // Vert

            #if DEBUG_SERIAL
                Serial.println(F("════════════════════════════════════════════════════"));
                Serial.println(F("    CALIBRATION AZIMUTH EFFECTUÉE (0.0°)"));
                Serial.println(F("════════════════════════════════════════════════════"));
            #endif

            // Reset état calibration
            calibTouchingAz = false;
            calibTouchStartAz = 0;
            calibFeedbackSentAz = false;

            // Petite pause pour voir le feedback
            delay(500);

            // Restaurer couleur normale
            sendToNextion("tAzCur.pco=65535");        // Blanc
        }
    }

    // ─────────────────────────────────────────────────────────────
    // CALIBRATION ÉLÉVATION (appui long sur tElCur)
    // ─────────────────────────────────────────────────────────────
    if (calibTouchingEl && calibTouchStartEl > 0) {
        unsigned long elapsed = now - calibTouchStartEl;

        // Feedback visuel après 1 seconde d'appui (en cours...)
        if (elapsed > 1000 && !calibFeedbackSentEl) {
            calibFeedbackSentEl = true;
            sendToNextion("tElCur.pco=31");           // Bleu (calibration en cours)
            sendToNextion("tElCur.txt=\"CAL...\"");
            #if DEBUG_NEXTION
                Serial.println(F("[NEXTION] Calibration El en cours..."));
            #endif
        }

        // Calibration après 3 secondes d'appui
        if (elapsed >= CALIBRATION_LONG_PRESS_MS) {
            // Effectuer la calibration
            calibrateEl(0.0);

            // Feedback visuel
            sendToNextion("tElCur.pco=2016");         // Vert (succès)
            sendToNextion("tElCur.txt=\"0.0\\xB0\""); // Afficher 0.0°

            // Message status temporaire
            sendToNextion("tStatus.txt=\"EL CAL OK\"");
            sendToNextion("tStatus.pco=2016");        // Vert

            #if DEBUG_SERIAL
                Serial.println(F("════════════════════════════════════════════════════"));
                Serial.println(F("    CALIBRATION ÉLÉVATION EFFECTUÉE (0.0°)"));
                Serial.println(F("════════════════════════════════════════════════════"));
            #endif

            // Reset état calibration
            calibTouchingEl = false;
            calibTouchStartEl = 0;
            calibFeedbackSentEl = false;

            // Petite pause pour voir le feedback
            delay(500);

            // Restaurer couleur normale
            sendToNextion("tElCur.pco=65535");        // Blanc
        }
    }
}

// ════════════════════════════════════════════════════════════════
// GESTION BOUTONS TACTILES (Mouvement continu tant qu'enfoncé)
// ════════════════════════════════════════════════════════════════

void handleNextionButtons() {
    unsigned long currentTime = millis();

    // ─────────────────────────────────────────────────────────────
    // DÉTECTION FRONTS (appui et relâchement)
    // ─────────────────────────────────────────────────────────────
    bool newPressCW    = (bCW_pressed && !prevCW);
    bool newPressCCW   = (bCCW_pressed && !prevCCW);
    bool newPressUP    = (bUP_pressed && !prevUP);
    bool newPressDOWN  = (bDOWN_pressed && !prevDOWN);
    bool newPressSTOP  = (bSTOP_pressed && !prevSTOP);

    bool releaseCW     = (!bCW_pressed && prevCW);
    bool releaseCCW    = (!bCCW_pressed && prevCCW);
    bool releaseUP     = (!bUP_pressed && prevUP);
    bool releaseDOWN   = (!bDOWN_pressed && prevDOWN);

    // Sauvegarder états pour prochain cycle
    prevCW   = bCW_pressed;
    prevCCW  = bCCW_pressed;
    prevUP   = bUP_pressed;
    prevDOWN = bDOWN_pressed;
    prevSTOP = bSTOP_pressed;

    // ─────────────────────────────────────────────────────────────
    // STOP IMMÉDIAT (bouton STOP ou relâchement)
    // ─────────────────────────────────────────────────────────────
    if (newPressSTOP || releaseCW || releaseCCW || releaseUP || releaseDOWN) {
        #if USE_NANO_STEPPER
            sendManualMove(0, 0);
        #else
            parseEasycomCommand("SA");
        #endif
        manualMoving = false;
        return;
    }

    // ─────────────────────────────────────────────────────────────
    // DÉMARRER MOUVEMENT (front montant)
    // ─────────────────────────────────────────────────────────────
    int8_t dirAz = 0;
    int8_t dirEl = 0;
    bool startMove = false;

    if (newPressCW) {
        dirAz = 1;
        startMove = true;
        #if DEBUG_NEXTION
            Serial.println(F("[NEXTION BTN] CW pressed"));
        #endif
    }
    else if (newPressCCW) {
        dirAz = -1;
        startMove = true;
        #if DEBUG_NEXTION
            Serial.println(F("[NEXTION BTN] CCW pressed"));
        #endif
    }
    else if (newPressUP) {
        dirEl = 1;
        startMove = true;
        #if DEBUG_NEXTION
            Serial.println(F("[NEXTION BTN] UP pressed"));
        #endif
    }
    else if (newPressDOWN) {
        dirEl = -1;
        startMove = true;
        #if DEBUG_NEXTION
            Serial.println(F("[NEXTION BTN] DOWN pressed"));
        #endif
    }

    // ─────────────────────────────────────────────────────────────
    // ENVOYER COMMANDE MOUVEMENT
    // ─────────────────────────────────────────────────────────────
    if (startMove) {
        #if USE_NANO_STEPPER
            sendManualMove(dirAz, dirEl);
        #else
            // Mode direct: utiliser commandes Easycom classiques
            if (dirAz != 0) {
                float newTarget = currentAz + (dirAz * MANUAL_INCREMENT_AZ);
                if (newTarget >= 360.0) newTarget -= 360.0;
                if (newTarget < 0.0) newTarget += 360.0;
                parseEasycomCommand("AZ" + String(newTarget, 1));
            } else if (dirEl != 0) {
                float newTarget = currentEl + (dirEl * MANUAL_INCREMENT_EL);
                // Limites élévation: -15° (parabole offset) à +95°
                if (newTarget > 95.0) newTarget = 95.0;
                if (newTarget < -15.0) newTarget = -15.0;
                parseEasycomCommand("EL" + String(newTarget, 1));
            }
        #endif
        manualMoving = true;
        lastButtonCommand = currentTime;
    }
}

// ════════════════════════════════════════════════════════════════
// MISE À JOUR INDICATEURS ÉTAT SYSTÈME
// ════════════════════════════════════════════════════════════════

// Variables pour éviter envois répétés
static int8_t lastSentDirAz = 0;
static int8_t lastSentDirEl = 0;
static uint8_t lastSentMode = 255;  // 255 = invalide pour forcer premier envoi

void updateNextionIndicators() {
    // ─────────────────────────────────────────────────────────────
    // ENVOYER DIRECTION AU NEXTION (pour gestion couleurs boutons)
    // vaDirAz: -1=CCW, 0=STOP, 1=CW
    // vaDirEl: -1=DOWN, 0=STOP, 1=UP
    // ─────────────────────────────────────────────────────────────
    #if USE_NANO_STEPPER
        if (currentDirAz != lastSentDirAz) {
            lastSentDirAz = currentDirAz;
            // Envoyer valeur +1 pour éviter les négatifs (0=CCW, 1=STOP, 2=CW)
            sendToNextion("vaDirAz.val=" + String(currentDirAz + 1));
        }
        if (currentDirEl != lastSentDirEl) {
            lastSentDirEl = currentDirEl;
            // Envoyer valeur +1 pour éviter les négatifs (0=DOWN, 1=STOP, 2=UP)
            sendToNextion("vaDirEl.val=" + String(currentDirEl + 1));
        }
    #endif

    // ─────────────────────────────────────────────────────────────
    // INDICATEUR MODE SYSTÈME (tMode)
    // Modes: STOP, AUTO, MANUAL
    // ─────────────────────────────────────────────────────────────
    #if USE_NANO_STEPPER
        // Déterminer mode actuel
        // 0=STOP, 1=AUTO, 2=MANUAL
        uint8_t currentMode = 0;  // STOP par défaut

        if (currentSpeedMode == 2) {
            // Mode manuel (boutons Nextion) - speed=2
            currentMode = 2;  // MANUAL
        } else if (currentDirAz != 0 || currentDirEl != 0) {
            // Moteurs en mouvement en mode automatique
            currentMode = 1;  // AUTO
        }

        // Envoyer seulement si changement
        if (currentMode != lastSentMode) {
            lastSentMode = currentMode;

            switch (currentMode) {
                case 0:  // STOP
                    sendToNextion("tMode.txt=\"STOP\"");
                    sendToNextion("tMode.pco=65535");  // Blanc
                    break;
                case 1:  // AUTO
                    sendToNextion("tMode.txt=\"AUTO\"");
                    sendToNextion("tMode.pco=2016");   // Vert
                    break;
                case 2:  // MANUAL
                    sendToNextion("tMode.txt=\"MANUAL\"");
                    sendToNextion("tMode.pco=31");     // Bleu
                    break;
            }

            #if DEBUG_NEXTION
                const char* modeNames[] = {"STOP", "AUTO", "MANUAL"};
                Serial.print(F("[NEXTION] Mode: "));
                Serial.println(modeNames[currentMode]);
            #endif
        }
    #else
        // Mode sans Nano - afficher STOP ou AUTO basé sur les cibles
        static bool lastWasMoving = false;
        bool isMoving = (targetAz > NO_TARGET || targetEl > NO_TARGET);

        if (isMoving != lastWasMoving) {
            lastWasMoving = isMoving;
            if (isMoving) {
                sendToNextion("tMode.txt=\"AUTO\"");
                sendToNextion("tMode.pco=2016");  // Vert
            } else {
                sendToNextion("tMode.txt=\"STOP\"");
                sendToNextion("tMode.pco=65535"); // Blanc
            }
        }
    #endif
}

#endif // ENABLE_NEXTION
