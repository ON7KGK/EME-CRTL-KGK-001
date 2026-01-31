// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Communication Easycom
// ════════════════════════════════════════════════════════════════
// Fichier: network.h
// Description: Gestion communication Easycom via Serial USB ou Ethernet
//              Mode sélectionné par USE_ETHERNET dans config.h
// ════════════════════════════════════════════════════════════════
// USE_ETHERNET=0 : Communication via Serial USB (PstRotator port COM)
// USE_ETHERNET=1 : Communication via Ethernet W5500 (TCP port 4533)
// ════════════════════════════════════════════════════════════════

#ifndef NETWORK_H
#define NETWORK_H

#include <Arduino.h>
#include "config.h"

// Include Ethernet uniquement si mode réseau activé
#if USE_ETHERNET
    #include <Ethernet.h>
#endif

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES
// ════════════════════════════════════════════════════════════════

// État communication
extern bool networkInitialized;

#if USE_ETHERNET
    // Configuration réseau (mode Ethernet uniquement)
    extern byte mac[];
    extern IPAddress ip;
    extern IPAddress gateway;
    extern IPAddress subnet;

    // Serveur Easycom
    extern EthernetServer server;

    // Client courant
    extern EthernetClient currentClient;
    extern bool clientConnected;
#endif

// ════════════════════════════════════════════════════════════════
// FONCTIONS PUBLIQUES
// ════════════════════════════════════════════════════════════════

/**
 * Initialisation communication
 *
 * Mode Serial (USE_ETHERNET=0):
 * - Utilise Serial déjà initialisé dans main.cpp
 * - Prêt pour commandes Easycom via USB
 *
 * Mode Ethernet (USE_ETHERNET=1):
 * - Configure W5500 via SPI (CS, RST pins)
 * - Configuration IP statique (MAC, IP, Gateway, Subnet)
 * - Démarre serveur TCP port EASYCOM_PORT (4533)
 *
 * @return true si initialisation OK, false si échec
 */
bool setupNetwork();

/**
 * Gestion communication (appelé dans loop)
 *
 * Mode Serial:
 * - Lit caractères disponibles sur Serial
 * - Parse commandes Easycom
 *
 * Mode Ethernet:
 * - Écoute nouvelles connexions
 * - Lit commandes clients
 * - Gère déconnexions
 *
 * Polling toutes les NETWORK_POLL_INTERVAL ms
 */
void handleNetwork();

/**
 * Traitement caractère reçu
 *
 * @param c Caractère reçu (Serial ou Ethernet)
 *
 * Accumule caractères jusqu'à \r ou \n, puis parse la commande
 */
void processReceivedChar(char c);

/**
 * Envoi message (Serial ou Ethernet selon mode)
 *
 * @param message Chaîne à envoyer (format Easycom: terminé par \r\n)
 *
 * Exemple: sendToClient("AZ123.5 EL45.0\r\n");
 */
void sendToClient(const char* message);

/**
 * Envoi message (String)
 *
 * @param message String à envoyer
 */
void sendToClient(String message);

/**
 * Vérification connexion active
 *
 * Mode Serial: toujours true
 * Mode Ethernet: true si client TCP connecté
 *
 * @return true si connecté
 */
bool isClientConnected();

/**
 * Déconnexion client (Ethernet uniquement)
 */
void disconnectClient();

/**
 * Reset module W5500 (Ethernet uniquement)
 */
void resetW5500();

/**
 * Affichage debug communication (Serial)
 */
void printNetworkDebug();

/**
 * Affichage configuration au démarrage
 */
void printNetworkConfig();

#endif // NETWORK_H
