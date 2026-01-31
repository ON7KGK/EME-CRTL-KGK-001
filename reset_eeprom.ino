// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Utilitaire reset EEPROM
// ════════════════════════════════════════════════════════════════
// Fichier: reset_eeprom.ino
// Description: Efface les valeurs EEPROM (tours et offsets)
//              Utilisez ce programme si les positions affichées
//              sont incorrectes après un upload.
//
// UTILISATION:
//   1. Uploader ce programme sur Arduino
//   2. Ouvrir Serial Monitor (9600 baud)
//   3. Attendre message "EEPROM effacée!"
//   4. Re-uploader le programme principal
// ════════════════════════════════════════════════════════════════

#include <EEPROM.h>

// Adresses EEPROM (doivent correspondre à config.h)
#define EEPROM_TURNS_AZ    0   // Nombre de tours azimuth (long, 4 bytes)
#define EEPROM_TURNS_EL    4   // Nombre de tours élévation (long, 4 bytes)
#define EEPROM_OFFSET_AZ   8   // Offset calibration azimuth (long, 4 bytes)
#define EEPROM_OFFSET_EL   16  // Offset calibration élévation (long, 4 bytes)

void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 3000);  // Attente max 3s pour Serial Monitor

  Serial.println(F("════════════════════════════════════════════════════════════════"));
  Serial.println(F("    EME ROTATOR CONTROLLER - Reset EEPROM"));
  Serial.println(F("════════════════════════════════════════════════════════════════"));
  Serial.println(F(""));
  Serial.println(F("Effacement EEPROM en cours..."));
  Serial.println(F(""));

  // Afficher valeurs avant effacement
  long turnsAz, turnsEl, offsetAz, offsetEl;
  EEPROM.get(EEPROM_TURNS_AZ, turnsAz);
  EEPROM.get(EEPROM_TURNS_EL, turnsEl);
  EEPROM.get(EEPROM_OFFSET_AZ, offsetAz);
  EEPROM.get(EEPROM_OFFSET_EL, offsetEl);

  Serial.println(F("Valeurs AVANT effacement:"));
  Serial.print(F("  turnsAz:   ")); Serial.println(turnsAz);
  Serial.print(F("  turnsEl:   ")); Serial.println(turnsEl);
  Serial.print(F("  offsetAz:  ")); Serial.println(offsetAz);
  Serial.print(F("  offsetEl:  ")); Serial.println(offsetEl);
  Serial.println(F(""));

  // Effacer toutes les valeurs (mettre à 0)
  long zero = 0L;
  EEPROM.put(EEPROM_TURNS_AZ, zero);
  EEPROM.put(EEPROM_TURNS_EL, zero);
  EEPROM.put(EEPROM_OFFSET_AZ, zero);
  EEPROM.put(EEPROM_OFFSET_EL, zero);

  // Vérifier effacement
  EEPROM.get(EEPROM_TURNS_AZ, turnsAz);
  EEPROM.get(EEPROM_TURNS_EL, turnsEl);
  EEPROM.get(EEPROM_OFFSET_AZ, offsetAz);
  EEPROM.get(EEPROM_OFFSET_EL, offsetEl);

  Serial.println(F("Valeurs APRÈS effacement:"));
  Serial.print(F("  turnsAz:   ")); Serial.println(turnsAz);
  Serial.print(F("  turnsEl:   ")); Serial.println(turnsEl);
  Serial.print(F("  offsetAz:  ")); Serial.println(offsetAz);
  Serial.print(F("  offsetEl:  ")); Serial.println(offsetEl);
  Serial.println(F(""));

  Serial.println(F("════════════════════════════════════════════════════════════════"));
  Serial.println(F("    ✓ EEPROM EFFACÉE AVEC SUCCÈS"));
  Serial.println(F("════════════════════════════════════════════════════════════════"));
  Serial.println(F(""));
  Serial.println(F("Vous pouvez maintenant:"));
  Serial.println(F("  1. Fermer ce moniteur série"));
  Serial.println(F("  2. Re-uploader le programme principal (main.cpp)"));
  Serial.println(F("  3. Les positions devraient maintenant s'afficher correctement"));
  Serial.println(F(""));
}

void loop() {
  // Rien - programme exécuté une seule fois dans setup()
}
