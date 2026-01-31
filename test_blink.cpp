// Test minimal - Blink + Serial
#include <Arduino.h>

void setup() {
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    delay(2000); // Attendre que Serial Monitor s'ouvre

    Serial.println("=== TEST ARDUINO MEGA ===");
    Serial.println("Si vous voyez ce message, l'Arduino fonctionne!");
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);
    Serial.println("LED ON");
    delay(1000);

    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("LED OFF");
    delay(1000);
}
