// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Communication Nano Stepper
// ════════════════════════════════════════════════════════════════
// Fichier: motor_nano.h
// Description: Interface UART avec Arduino Nano dédié aux moteurs
// ════════════════════════════════════════════════════════════════
// Connexion: Mega Serial2 (TX2=pin16, RX2=pin17) → Nano (RX=pin0, TX=pin1)
//
// Protocole Mega → Nano (COMMANDE COMBINÉE pour tracking simultané):
//   "M:dirAz:dirEl\n"  - Mouvement combiné des deux axes
//                        dirAz: -1=CCW, 0=STOP, 1=CW
//                        dirEl: -1=DOWN, 0=STOP, 1=UP
//   Exemples:
//     "M:1:1\n"   → Az CW + El UP (tracking lune montante)
//     "M:1:-1\n"  → Az CW + El DOWN
//     "M:0:0\n"   → STOP les deux axes
//
// Protocole Nano → Mega:
//   "OK\n"           - Commande reçue
//   "READY\n"        - Nano démarré et prêt
//   "LIMIT:AZ:CW\n"  - Fin de course Az CW atteinte
//   "LIMIT:AZ:CCW\n" - Fin de course Az CCW atteinte
//   "LIMIT:EL:UP\n"  - Fin de course El UP atteinte
//   "LIMIT:EL:DOWN\n"- Fin de course El DOWN atteinte
// ════════════════════════════════════════════════════════════════

#ifndef MOTOR_NANO_H
#define MOTOR_NANO_H

#include <Arduino.h>
#include "config.h"

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES COMMANDE
// ════════════════════════════════════════════════════════════════

extern float targetAz;          // Position cible Az (-1 = pas de cible)
extern float targetEl;          // Position cible El (-1 = pas de cible)
extern bool movingAz;           // true si Az en mouvement
extern bool movingEl;           // true si El en mouvement

// Fins de course avec direction exacte
extern bool nanoLimitCW;        // true si fin de course Az CW active
extern bool nanoLimitCCW;       // true si fin de course Az CCW active
extern bool nanoLimitUp;        // true si fin de course El UP active
extern bool nanoLimitDown;      // true si fin de course El DOWN active

extern int8_t currentDirAz;     // Direction actuelle: 0=STOP, 1=CW, -1=CCW
extern int8_t currentDirEl;     // Direction actuelle: 0=STOP, 1=UP, -1=DOWN
extern uint8_t currentSpeedMode; // Mode vitesse: 0=LENT, 1=RAPIDE, 2=MANUEL

// ════════════════════════════════════════════════════════════════
// FONCTIONS PUBLIQUES
// ════════════════════════════════════════════════════════════════

/**
 * Initialisation communication UART avec Nano
 * Configure Serial2 à NANO_BAUD (115200)
 */
void setupMotorNano();

/**
 * Mise à jour - calcule direction et envoie commande combinée au Nano
 * Compare targetAz/El avec currentAz/El (des encodeurs)
 * Envoie "M:dirAz:dirEl" pour mouvement simultané des deux axes
 * Appelé dans loop() toutes les 20ms
 */
void updateMotorNano();

/**
 * Arrêt immédiat tous moteurs
 * Envoie "M:0:0" au Nano et reset les cibles
 */
void stopAllMotorsNano();

/**
 * Lecture réponses du Nano (non-bloquant)
 * Parse OK, READY, LIMIT:AZ, LIMIT:EL
 */
void readNanoResponse();

/**
 * Affichage debug état moteurs
 */
void printMotorNanoDebug();

/**
 * Commande manuelle directe (vitesse très lente pour pointage précis)
 * Envoie M:dirAz:dirEl:2 au Nano (speed=2 = 50 steps/s)
 * @param dirAz Direction azimuth: -1=CCW, 0=STOP, 1=CW
 * @param dirEl Direction élévation: -1=DOWN, 0=STOP, 1=UP
 */
void sendManualMove(int8_t dirAz, int8_t dirEl);

#endif // MOTOR_NANO_H
