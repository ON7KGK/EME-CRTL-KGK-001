// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Affichage Nextion
// ════════════════════════════════════════════════════════════════
// Fichier: nextion.h
// Description: Interface affichage écran Nextion
//              Affiche positions actuelles et cibles Az/El
// ════════════════════════════════════════════════════════════════

#ifndef NEXTION_H
#define NEXTION_H

#include <Arduino.h>
#include "config.h"

// ════════════════════════════════════════════════════════════════
// FONCTIONS PUBLIQUES
// ════════════════════════════════════════════════════════════════

/**
 * Initialisation écran Nextion
 * - Configure port série (Serial1 par défaut)
 * - Envoi commande reset optionnelle
 * - Affiche message initial
 *
 * Appelé dans setup()
 */
void setupNextion();

/**
 * Mise à jour affichage Nextion
 * - Envoie positions actuelles (currentAz, currentEl)
 * - Envoie positions cibles (targetAz, targetEl)
 * - Throttling: mise à jour toutes les NEXTION_UPDATE_INTERVAL ms
 *
 * Appelé dans loop()
 *
 * Format données envoyées:
 * - tAzCur.txt="123.5"  (position azimuth actuelle)
 * - tAzTgt.txt="180.0"  (position azimuth cible)
 * - tElCur.txt="45.2"   (position élévation actuelle)
 * - tElTgt.txt="30.0"   (position élévation cible)
 * - "---" si pas de cible active (targetAz/El = -1)
 */
void updateNextion();

/**
 * Envoi commande brute vers Nextion
 *
 * @param command Commande Nextion (sans terminateurs 0xFF)
 *
 * Exemple:
 *   sendToNextion("page 0");              // Change page
 *   sendToNextion("t0.txt=\"Hello\"");    // Affiche texte
 *   sendToNextion("n0.val=1234");         // Affiche nombre
 *
 * Ajoute automatiquement 3× 0xFF (terminateur Nextion)
 */
void sendToNextion(String command);

/**
 * Affichage page accueil Nextion
 * (optionnel, personnalisable selon votre IHM)
 */
void showNextionHomePage();

/**
 * Affichage erreur sur Nextion
 * @param errorMsg Message erreur à afficher
 */
void showNextionError(String errorMsg);

/**
 * Lecture événements tactiles Nextion
 * - Lit les données disponibles sur NEXTION_SERIAL
 * - Parse les événements touch (format: 0x65 [Page] [ID] [Event])
 * - Met à jour états boutons (bCW_pressed, bCCW_pressed, etc.)
 *
 * Appelé dans loop() AVANT handleNextionButtons()
 *
 * Format événement Nextion:
 *   0x65 0x00 [Component ID] [0x01=Press / 0x00=Release] 0xFF 0xFF 0xFF
 *
 * Component IDs attendus (selon votre fichier .HMI):
 *   bCW   : Bouton CW (azimuth sens horaire)
 *   bCCW  : Bouton CCW (azimuth sens anti-horaire)
 *   bUP   : Bouton UP (élévation montante)
 *   bDOWN : Bouton DOWN (élévation descendante)
 *   bSTOP : Bouton STOP (arrêt tous mouvements)
 */
void readNextionTouch();

/**
 * Gestion boutons tactiles Nextion
 * - Envoie commandes Easycom incrémentales si bouton enfoncé
 * - Un seul bouton actif à la fois (logique mutex)
 * - Throttling: commande toutes les 500ms pendant appui
 *
 * Appelé dans loop() APRÈS readNextionTouch()
 *
 * Commandes Easycom générées:
 *   bCW   → AZ[position actuelle + MANUAL_INCREMENT_AZ]
 *   bCCW  → AZ[position actuelle - MANUAL_INCREMENT_AZ]
 *   bUP   → EL[position actuelle + MANUAL_INCREMENT_EL]
 *   bDOWN → EL[position actuelle - MANUAL_INCREMENT_EL]
 *   bSTOP → SA (Stop All)
 *
 * Les commandes sont envoyées via Easycom (Serial ou Ethernet selon config)
 * pour que PstRotator soit informé des mouvements manuels.
 */
void handleNextionButtons();

/**
 * Mise à jour indicateurs état système Nextion
 * - Affiche direction mouvement azimuth (CW / CCW / ---)
 * - Affiche direction mouvement élévation (UP / DOWN / ---)
 * - Affiche mode système (PARKING / STOP / ---)
 *
 * Appelé dans loop() après updateNextion()
 *
 * Composants Nextion mis à jour:
 *   tAzDir : Direction azimuth ("CW", "CCW", ou "---")
 *   tElDir : Direction élévation ("UP", "DOWN", ou "---")
 *   tMode  : Mode système ("PARKING", "STOP", ou "---")
 */
void updateNextionIndicators();

/**
 * Gestion calibration par appui long Nextion
 * - Détecte appui long (3 secondes) sur tAzCur ou tElCur
 * - Calibre l'axe correspondant à 0.0°
 * - Affiche feedback visuel pendant et après calibration
 *
 * Appelé dans loop() après readNextionTouch()
 *
 * Fonctionnement:
 *   1. Appui sur tAzCur/tElCur → démarrage timer
 *   2. Après 1 sec → affichage "CAL..." en bleu
 *   3. Après 3 sec → calibration effectuée, affichage "0.0°" en vert
 *   4. Relâchement avant 3 sec → annulation
 *
 * Configuration Nextion Editor:
 *   - tAzCur: Activer "Send Component ID" pour Press et Release
 *   - tElCur: Activer "Send Component ID" pour Press et Release
 *   - Vérifier que les IDs correspondent (NEXTION_ID_TAZCUR/TELCUR)
 */
void handleCalibrationTouch();

#endif // NEXTION_H
