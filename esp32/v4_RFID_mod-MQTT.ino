#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <time.h>
#include <Adafruit_PN532.h>

// Configurações do sensor DHT11
#define DHT_PIN 4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// Configurações do sensor ultrassônico HC-SR04
#define TRIG_PIN 5
#define ECHO_PIN 25

// Configuração do LED indicador
#define LED_PIN 2

// Configuração do módulo PN532
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Configurações da rede Wi-Fi
const char* ssid = "Starlink_CIT";
const char* password = "Ufrr@2024Cit";

// Configurações do HiveMQ (broker MQTT)
const char* mqtt_server = "cd8839ea5ec5423da3aaa6691e5183a5.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* mqtt_topic = "esp32/refrigerator";
const char* mqtt_username = "hivemq.webclient.1734636778463";
const char* mqtt_password = "EU<pO3F7x?S%wLk4#5ib";

// Inicialização do cliente Wi-Fi e MQTT com TLS
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Variáveis globais
unsigned long lastMsg = 0;
const int intervalo = 1000; // Intervalo de 1 segundo
const float distancia_limite = 10.0;
String usuarioAtual = "desconhecido"; // Persistência do usuário

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Inicializando PN532...");
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (versiondata) {
    nfc.SAMConfig();
    Serial.println("PN532 inicializado.");
  } else {
    Serial.println("Módulo PN532 não detectado.");
  }

  setup_wifi();
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  dht.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Atualiza continuamente o estado do RFID
  verificarRFID();

  // Envia os dados MQTT a cada 1 segundo
  unsigned long now = millis();
  if (now - lastMsg >= intervalo) {
    lastMsg = now;
    enviarDados(); // Envia os dados com o estado mais recente
  }
}

// Conexão Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Reconexão ao MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.println("Reconectando ao HiveMQ...");
    if (client.connect("ESP32_Client", mqtt_username, mqtt_password)) {
      Serial.println("Conectado ao HiveMQ!");
    } else {
      delay(5000);
    }
  }
}

// Envia dados ao servidor MQTT
void enviarDados() {
  // Leitura dos sensores
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();
  float distancia = medirDistancia();
  String estado_porta = (distancia <= distancia_limite) ? "Fechada" : "Aberta";

  // Monta a mensagem JSON
  String mensagem = "{";
  mensagem += "\"temperatura\": " + String(temperatura) + ", ";
  mensagem += "\"umidade\": " + String(umidade) + ", ";
  mensagem += "\"estado_porta\": \"" + estado_porta + "\", ";
  mensagem += "\"usuario\": \"" + usuarioAtual + "\"}";

  // Envia ao HiveMQ
  if (client.publish(mqtt_topic, mensagem.c_str())) {
    Serial.println("Dados enviados: " + mensagem);
  } else {
    Serial.println("Falha no envio.");
  }
}

// Função que verifica o cartão RFID de forma não bloqueante
void verificarRFID() {
  uint8_t success = nfc.inListPassiveTarget();

  if (success > 0) {
    uint8_t uid[7];
    uint8_t uidLength;
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      String novoUsuario = "";
      for (uint8_t i = 0; i < uidLength; i++) {
        novoUsuario += String(uid[i], HEX);
      }

      if (novoUsuario != usuarioAtual) {
        usuarioAtual = novoUsuario; // Atualiza o usuário apenas se for diferente
        Serial.println("Novo usuário identificado: " + usuarioAtual);
      }
    }
  }
}

// Mede a distância com o sensor ultrassônico
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duracao = pulseIn(ECHO_PIN, HIGH);
  return (duracao * 0.034) / 2.0;
}

// Callback para mensagens recebidas
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
