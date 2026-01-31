// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Protocole Easycom
// ════════════════════════════════════════════════════════════════
// Fichier: easycom.h
// Description: Parseur protocole Easycom (standard ham radio tracking)
//              Compatible PstRotator, HRD, N1MM, etc.
// ════════════════════════════════════════════════════════════════
// ÉTAPE 5 : Test Ethernet W5500 + Easycom
// ════════════════════════════════════════════════════════════════

#ifndef EASYCOM_H
#define EASYCOM_H

#include <Arduino.h>
#include "config.h"

// ════════════════════════════════════════════════════════════════
// PROTOCOLE EASYCOM - RÉFÉRENCE
// ════════════════════════════════════════════════════════════════
/*
Format commandes (terminées par \r ou \r\n):

COMMANDES MOUVEMENT:
  "AZ123.5\r"     → Goto azimuth 123.5°
  "EL45.0\r"      → Goto élévation 45.0°
  "AZ123.5 EL45\r" → Goto azimuth ET élévation (optionnel)

COMMANDES ARRÊT:
  "S\r"           → Stop tous mouvements (arrêt immédiat)
  "SA\r"          → Stop azimuth seulement
  "SE\r"          → Stop élévation seulement
  "STOP\r"        → Stop tous (alias de "S")

COMMANDES STATUS (Query):
  "AZ\r"          → Demande position → Réponse "AZ123.5 EL45.0\r\n"
  "EL\r"          → Demande position (même réponse)

COMMANDES CALIBRATION (Extensions custom):
  "Z123.5\r"      → Calibre azimuth position courante à 123.5°
  "E45.0\r"       → Calibre élévation position courante à 45.0°
  (Note: 'Z' et 'E' seuls = calibration, pas 'AZ'/'EL')

RÉPONSE STANDARD:
  "AZ123.5 EL45.0\r\n"  → Position courante (1 décimale)

COMPORTEMENT ANTI-VIBRATION:
  - Si |target - current| < MICRO_MOVEMENT_FILTER (0.15°) → Ignorer commande
  - Évite micro-mouvements continus de PstRotator
*/
// ════════════════════════════════════════════════════════════════

// ════════════════════════════════════════════════════════════════
// FONCTIONS PUBLIQUES
// ════════════════════════════════════════════════════════════════

/**
 * Parsing commande Easycom reçue
 *
 * @param command Chaîne commande (sans \r\n final)
 *
 * Analyse commande et exécute action appropriée:
 * - Parse "AZxxx" → Met à jour targetAz (motor_stepper.cpp)
 * - Parse "ELxxx" → Met à jour targetEl
 * - Parse "S" / "STOP" → Arrêt moteurs
 * - Parse "AZ" / "EL" (query) → Envoie position courante
 * - Parse "Zxxx" / "Exxx" → Calibration encodeurs
 *
 * IMPORTANT: Filtre micro-mouvements (seuil MICRO_MOVEMENT_FILTER)
 */
void parseEasycomCommand(String command);

/**
 * Envoi réponse position via Serial/Ethernet
 *
 * Envoie directement "AZ123.5 EL45.0\r\n" au client
 * Appelé après chaque commande reçue (comportement K3NG)
 */
void sendPositionResponse();

/**
 * Génération réponse position Easycom
 *
 * @return String format "AZ123.5 EL45.0\r\n"
 *
 * Utilise currentAz, currentEl (encoder_ssi.cpp)
 * Format: 1 décimale (norme Easycom)
 */
String generatePositionResponse();

/**
 * Parsing valeur numérique depuis commande
 *
 * @param command    Chaîne complète
 * @param keyword    Mot-clé à chercher ("AZ", "EL", "Z", "E")
 * @param valueFound Référence bool (true si valeur trouvée)
 * @return Valeur numérique extraite (float)
 *
 * Exemple:
 *   parseNumericValue("AZ123.5 EL45", "AZ", found) → 123.5
 *   parseNumericValue("AZ123.5 EL45", "EL", found) → 45.0
 */
float parseNumericValue(String command, String keyword, bool &valueFound);

/**
 * Vérification commande = query position
 *
 * @param command Chaîne commande
 * @return true si commande query ("AZ", "EL" seuls, sans valeur)
 *
 * Exemples:
 *   "AZ" → true (query)
 *   "AZ123" → false (commande goto)
 */
bool isPositionQuery(String command);

/**
 * Vérification commande = arrêt
 *
 * @param command Chaîne commande
 * @return true si commande stop ("S", "SA", "SE", "STOP")
 */
bool isStopCommand(String command);

/**
 * Affichage debug commande reçue
 *
 * @param command Commande parsée
 */
void printEasycomDebug(String command);

/**
 * Affichage debug valeurs brutes encodeurs
 *
 * Affiche rawCountsAz et rawCountsEl (0-4095)
 * Contrôlé par DEBUG_ENCODER_RAW dans config.h
 */
void printEncoderRawDebug();

#endif // EASYCOM_H
