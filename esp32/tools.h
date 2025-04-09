#ifndef TOOLS_H
#define TOOLS_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <Arduino.h>

// -----------------------------------------------------------------------------
// Variáveis globais (definidas em main.ino)
// -----------------------------------------------------------------------------
extern const char* ssid;
extern const char* password;

extern WiFiClientSecure espClient;
extern PubSubClient client;
extern const char* mqtt_username;
extern const char* mqtt_password;
extern const char* control_topic;

extern bool alarmState;
extern const int LED_OK_PIN;
extern const int LED_ALERT_PIN;

extern const int TRIG_PIN;
extern const int ECHO_PIN;

// -----------------------------------------------------------------------------
// Funções utilitárias (inline para evitar múltiplas definições)
// -----------------------------------------------------------------------------

// Conecta ao Wi‑Fi e imprime debug
static inline void setup_wifi() {
  Serial.print("[DEBUG] Conectando ao Wi-Fi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[DEBUG] Wi-Fi conectado: %s\n", WiFi.localIP().toString().c_str());
}

// Reconecta ao broker MQTT
static inline void reconnect_mqtt() {
  while (!client.connected()) {
    Serial.println("[DEBUG] Tentando conexão MQTT...");
    if (client.connect("ESP32_Client", mqtt_username, mqtt_password)) {
      Serial.println("[DEBUG] Conectado ao MQTT.");
      client.subscribe(control_topic);
      Serial.printf("[DEBUG] Inscrito em: %s\n", control_topic);
    } else {
      Serial.printf("[ERROR] Falha MQTT, rc=%d. Tentando em 5s\n", client.state());
      delay(5000);
    }
  }
}

// Pisca um LED por `duration` ms
static inline void blinkLED(uint8_t pin, int duration) {
  digitalWrite(pin, HIGH);
  delay(duration);
  digitalWrite(pin, LOW);
}

// Atualiza os LEDs de status
static inline void atualizarLEDs() {
  digitalWrite(LED_ALERT_PIN, alarmState ? HIGH : LOW);
  digitalWrite(LED_OK_PIN,    alarmState ? LOW  : HIGH);
}

// Mede distância ultrassônica
static inline float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH);
  return (dur * 0.034) / 2.0;
}
#endif // TOOLS_H