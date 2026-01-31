// ════════════════════════════════════════════════════════════════
// EME ROTATOR CONTROLLER - Communication (Implementation)
// ════════════════════════════════════════════════════════════════
// Fichier: network.cpp
// Description: Gestion communication Easycom via Serial USB ou Ethernet
// ════════════════════════════════════════════════════════════════

#include "network.h"
#include "easycom.h"  // Pour parseEasycomCommand()

// ════════════════════════════════════════════════════════════════
// VARIABLES GLOBALES
// ════════════════════════════════════════════════════════════════

// Buffer réception Serial/Ethernet
#define RX_BUFFER_SIZE 64
char rxBuffer[RX_BUFFER_SIZE];
int rxBufferIndex = 0;

// État communication
bool networkInitialized = false;

// Timing polling
unsigned long lastNetworkPollTime = 0;

// ════════════════════════════════════════════════════════════════
// MODE ETHERNET W5500 (si USE_ETHERNET=1)
// ════════════════════════════════════════════════════════════════

#if USE_ETHERNET

// Configuration réseau
byte mac[] = {MAC_ADDR_0, MAC_ADDR_1, MAC_ADDR_2, MAC_ADDR_3, MAC_ADDR_4, MAC_ADDR_5};
IPAddress ip(IP_OCTET_1, IP_OCTET_2, IP_OCTET_3, IP_OCTET_4);
IPAddress gateway(GW_OCTET_1, GW_OCTET_2, GW_OCTET_3, GW_OCTET_4);
IPAddress subnet(SUBNET_OCTET_1, SUBNET_OCTET_2, SUBNET_OCTET_3, SUBNET_OCTET_4);

// Serveur Easycom (port 4533)
EthernetServer server(EASYCOM_PORT);

// Client courant
EthernetClient currentClient;
bool clientConnected = false;

#endif  // USE_ETHERNET

// ════════════════════════════════════════════════════════════════
// INITIALISATION
// ════════════════════════════════════════════════════════════════

bool setupNetwork() {
    rxBufferIndex = 0;

    #if USE_ETHERNET
        // ─────────────────────────────────────────────────────────────
        // INITIALISATION ETHERNET W5500
        // ─────────────────────────────────────────────────────────────

        #if DEBUG_SERIAL
            Serial.println(F("=== INIT RÉSEAU W5500 ==="));
        #endif

        // Reset W5500
        resetW5500();

        // Configuration SPI et démarrage Ethernet
        Ethernet.init(W5500_CS);
        Ethernet.begin(mac, ip, gateway, gateway, subnet);

        delay(1000);

        // Vérification hardware W5500
        if (Ethernet.hardwareStatus() == EthernetNoHardware) {
            #if DEBUG_SERIAL
                Serial.println(F("ERREUR: W5500 non détecté!"));
            #endif
            networkInitialized = false;
            return false;
        }

        // Vérification câble Ethernet
        if (Ethernet.linkStatus() == LinkOFF) {
            #if DEBUG_NETWORK
                Serial.println(F("ATTENTION: Câble non connecté"));
            #endif
        }

        // Démarrage serveur TCP
        server.begin();
        networkInitialized = true;

        #if DEBUG_SERIAL
            printNetworkConfig();
        #endif

    #else
        // ─────────────────────────────────────────────────────────────
        // INITIALISATION SERIAL USB
        // ─────────────────────────────────────────────────────────────

        // Note: Serial.begin() déjà fait dans main.cpp
        networkInitialized = true;

        #if DEBUG_SERIAL
            Serial.println(F("=== EASYCOM VIA SERIAL USB ==="));
            Serial.print(F("Baudrate: "));
            Serial.println(SERIAL_BAUD);
            Serial.println(F("Prêt pour commandes Easycom..."));
        #endif

    #endif  // USE_ETHERNET

    return true;
}

// ════════════════════════════════════════════════════════════════
// GESTION COMMUNICATION (Loop principal)
// ════════════════════════════════════════════════════════════════

void handleNetwork() {
    if (!networkInitialized) {
        return;
    }

    // Throttling polling
    unsigned long currentTime = millis();
    if (currentTime - lastNetworkPollTime < NETWORK_POLL_INTERVAL) {
        return;
    }
    lastNetworkPollTime = currentTime;

    #if USE_ETHERNET
        // ─────────────────────────────────────────────────────────────
        // MODE ETHERNET
        // ─────────────────────────────────────────────────────────────

        // Écoute nouvelles connexions
        EthernetClient newClient = server.available();
        if (newClient) {
            if (!clientConnected) {
                currentClient = newClient;
                clientConnected = true;
                rxBufferIndex = 0;

                #if DEBUG_NETWORK
                    Serial.print(F("[NET] Client: "));
                    Serial.println(currentClient.remoteIP());
                #endif
            } else if (newClient != currentClient) {
                newClient.stop();
                #if DEBUG_NETWORK
                    Serial.println(F("[NET] Client rejeté"));
                #endif
            }
        }

        // Lecture données client
        if (clientConnected) {
            if (currentClient.connected()) {
                while (currentClient.available() > 0) {
                    char c = currentClient.read();
                    processReceivedChar(c);
                }
            } else {
                disconnectClient();
            }
        }

    #else
        // ─────────────────────────────────────────────────────────────
        // MODE SERIAL USB
        // ─────────────────────────────────────────────────────────────

        while (Serial.available() > 0) {
            char c = Serial.read();
            processReceivedChar(c);
        }

    #endif  // USE_ETHERNET
}

// ════════════════════════════════════════════════════════════════
// TRAITEMENT CARACTÈRE REÇU
// ════════════════════════════════════════════════════════════════

void processReceivedChar(char c) {
    // Détection fin de commande (\r ou \n)
    if (c == '\r' || c == '\n') {
        if (rxBufferIndex > 0) {
            // Commande complète reçue
            rxBuffer[rxBufferIndex] = '\0';
            String command = String(rxBuffer);

            // Parser commande Easycom
            parseEasycomCommand(command);

            // Reset buffer
            rxBufferIndex = 0;
        }
    } else if (rxBufferIndex < RX_BUFFER_SIZE - 1) {
        // Ajouter caractère au buffer
        rxBuffer[rxBufferIndex++] = c;
    }
    // Si buffer plein, ignorer caractères supplémentaires
}

// ════════════════════════════════════════════════════════════════
// ENVOI MESSAGE (Serial ou Ethernet selon mode)
// ════════════════════════════════════════════════════════════════

void sendToClient(const char* message) {
    #if USE_ETHERNET
        if (clientConnected && currentClient.connected()) {
            currentClient.print(message);
        }
    #else
        Serial.print(message);
    #endif
}

void sendToClient(String message) {
    sendToClient(message.c_str());
}

// ════════════════════════════════════════════════════════════════
// VÉRIFICATION CLIENT CONNECTÉ
// ════════════════════════════════════════════════════════════════

bool isClientConnected() {
    #if USE_ETHERNET
        return clientConnected && currentClient.connected();
    #else
        return true;  // Serial toujours "connecté"
    #endif
}

// ════════════════════════════════════════════════════════════════
// DÉCONNEXION CLIENT (Ethernet uniquement)
// ════════════════════════════════════════════════════════════════

void disconnectClient() {
    #if USE_ETHERNET
        if (clientConnected) {
            currentClient.stop();
            clientConnected = false;
            rxBufferIndex = 0;

            #if DEBUG_NETWORK
                Serial.println(F("[NET] Déconnecté"));
            #endif
        }
    #endif
}

// ════════════════════════════════════════════════════════════════
// RESET W5500 (Ethernet uniquement)
// ════════════════════════════════════════════════════════════════

void resetW5500() {
    #if USE_ETHERNET
        #if defined(W5500_RST) && W5500_RST > 0
            pinMode(W5500_RST, OUTPUT);
            digitalWrite(W5500_RST, LOW);
            delay(10);
            digitalWrite(W5500_RST, HIGH);
            delay(200);

            #if DEBUG_NETWORK
                Serial.println(F("[NET] W5500 reset"));
            #endif
        #endif
    #endif
}

// ════════════════════════════════════════════════════════════════
// DEBUG AFFICHAGE
// ════════════════════════════════════════════════════════════════

void printNetworkDebug() {
    #if USE_ETHERNET
        Serial.print(F("[NET] IP: "));
        Serial.print(Ethernet.localIP());
        Serial.print(F(" Port: "));
        Serial.print(EASYCOM_PORT);
        Serial.print(F(" Client: "));
        Serial.println(clientConnected ? F("OUI") : F("NON"));
    #else
        Serial.print(F("[COM] Serial USB @ "));
        Serial.print(SERIAL_BAUD);
        Serial.println(F(" baud"));
    #endif
}

void printNetworkConfig() {
    #if USE_ETHERNET
        Serial.println(F("─────────────────────────────────"));
        Serial.println(F("Config Ethernet:"));
        Serial.print(F("  MAC: "));
        for (int i = 0; i < 6; i++) {
            if (mac[i] < 16) Serial.print(F("0"));
            Serial.print(mac[i], HEX);
            if (i < 5) Serial.print(F(":"));
        }
        Serial.println();
        Serial.print(F("  IP: "));
        Serial.println(Ethernet.localIP());
        Serial.print(F("  Port: "));
        Serial.println(EASYCOM_PORT);
        Serial.print(F("  Link: "));
        Serial.println(Ethernet.linkStatus() == LinkON ? F("OK") : F("NON"));
        Serial.println(F("─────────────────────────────────"));
    #endif
}
