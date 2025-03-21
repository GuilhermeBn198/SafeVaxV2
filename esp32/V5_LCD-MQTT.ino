#include <LiquidCrystal.h>
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

// Configuração do LCD
#define RS 19
#define E  18
#define D4 32
#define D5 35
#define D6 34
#define D7 27

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

// Pinos do LCD
LiquidCrystal lcd(RS, E, D4, D5, D6, D7); 

// Variáveis globais
unsigned long lastMsg = 0;
const int intervalo = 1000; // Intervalo de 1 segundo
const float distancia_limite = 10.0;
String usuarioAtual = "desconhecido"; // Persistência do usuário
unsigned long portaAbertaDesde = 0;  // Momento em que a porta foi detectada aberta
bool ledEstado = false;              // Estado atual do LED

void setup() {
  Serial.begin(115200);
  lcd.begin(20, 4);  // Inicializa o LCD e identifica que ele tem 20 colunas e 4 linhas
  lcd.clear(); // Apaga todos os caracteres do Display
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
void verificarRFID() {
  // Verifica se há um cartão presente de forma não bloqueante
  uint8_t success = nfc.inListPassiveTarget();
  if (success > 0) { // Há um cartão detectado
    uint8_t uid[7];
    uint8_t uidLength;
    String novoUsuario = "";

    // Tenta ler o UID do cartão
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
      for (uint8_t i = 0; i < uidLength; i++) {
        novoUsuario += String(uid[i], HEX);
      }

      // Atualiza o usuário se for diferente do atual
      if (novoUsuario != usuarioAtual) {
        Serial.println("Novo usuário identificado: " + novoUsuario);
        usuarioAtual = novoUsuario;
      }
    } else {
      // Caso a leitura do UID falhe
      Serial.println("Falha na leitura do UID do cartão.");
    }
  } else {
    // Caso nenhum cartão seja detectado
    Serial.println("Nenhum cartão detectado.");
  }

  // Exibe o estado atual do usuário
  Serial.println("Usuário atual: " + usuarioAtual);
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
  // escreveTela();
 // Atualiza o estado do LED
  gerenciarLED();
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

  if (estado_porta == "Aberta") {
    if (portaAbertaDesde == 0) {
      portaAbertaDesde = millis();
    }
  } else {
    portaAbertaDesde = 0;
    digitalWrite(LED_PIN, LOW); // Desliga o LED quando a porta é fechada
  }
  // Envia ao HiveMQ
  if (client.publish(mqtt_topic, mensagem.c_str())) {
    Serial.println("Dados enviados: " + mensagem);
  } else {
    Serial.println("Falha no envio.");
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

// Função para gerenciar o estado do LED
void gerenciarLED() {
  if (portaAbertaDesde > 0 && millis() - portaAbertaDesde > 10000) {
    // Pisca o LED se a porta estiver aberta por mais de 10 segundos
    ledEstado = !ledEstado;
    digitalWrite(LED_PIN, ledEstado ? HIGH : LOW);
    delay(500); // Tempo de piscada
  }
}


// Callback para mensagens recebidas
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void escreveTela() {
  float distancia = medirDistancia();
  String estado_porta = (distancia <= distancia_limite) ? "Fechada" : "Aberta";

  lcd.setCursor(0, 0); // Marca o cursor na posição coluna 0, linha 0
  lcd.print("Temperatura: "+ String(dht.readTemperature())); // Escreve a mensagem na posição coluna 0, linha 0
  lcd.setCursor(0, 1);
  lcd.print("Umidade: "+ String(dht.readHumidity()));
  lcd.setCursor(0, 2);
  lcd.print("Estado da porta: "+ estado_porta);
  lcd.setCursor(0, 3);
  lcd.print("Ultimo usuario: " + usuarioAtual);
}

