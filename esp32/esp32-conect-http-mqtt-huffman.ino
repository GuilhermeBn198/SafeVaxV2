#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <time.h>
#include <Adafruit_PN532.h>
#include <LiquidCrystal.h>

// Para Huffman:
#include <map>
#include <queue>

// ----- IMPLEMENTAÇÃO DO HUFFMAN -----
struct HuffmanNode {
  char ch;
  int freq;
  HuffmanNode *left;
  HuffmanNode *right;
  HuffmanNode(char _ch, int _freq) : ch(_ch), freq(_freq), left(NULL), right(NULL) {}
};

struct CompareNode {
  bool operator()(HuffmanNode* const & n1, HuffmanNode* const & n2) {
    return n1->freq > n2->freq;
  }
};

HuffmanNode* huffmanTree = NULL;
std::map<char, String> huffmanCodes;

void buildHuffmanCodes(HuffmanNode* root, String code) {
  if (!root) return;
  if (root->left == NULL && root->right == NULL) {
    huffmanCodes[root->ch] = code;
  }
  buildHuffmanCodes(root->left, code + "0");
  buildHuffmanCodes(root->right, code + "1");
}

HuffmanNode* buildHuffmanTree(const String &data) {
  std::map<char, int> freq;
  for (int i = 0; i < data.length(); i++) {
    freq[data[i]]++;
  }
  std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareNode> pq;
  for (auto pair : freq) {
    pq.push(new HuffmanNode(pair.first, pair.second));
  }
  while (pq.size() > 1) {
    HuffmanNode* left = pq.top(); pq.pop();
    HuffmanNode* right = pq.top(); pq.pop();
    HuffmanNode* merged = new HuffmanNode('\0', left->freq + right->freq);
    merged->left = left;
    merged->right = right;
    pq.push(merged);
  }
  return pq.top();
}

String huffmanCompress(const String &data) {
  String compressed = "";
  for (int i = 0; i < data.length(); i++) {
    compressed += huffmanCodes[data[i]];
  }
  return compressed;
}

String huffmanDecompress(const String &compressed, HuffmanNode* root) {
  String decompressed = "";
  HuffmanNode* curr = root;
  for (int i = 0; i < compressed.length(); i++) {
    if (compressed[i] == '0')
      curr = curr->left;
    else
      curr = curr->right;
    if (!curr->left && !curr->right) {
      decompressed += curr->ch;
      curr = root;
    }
  }
  return decompressed;
}

// ----- FIM DO HUFFMAN -----


// --------------------------------------------------------------------------------
// Definições de pinos e constantes do projeto
// --------------------------------------------------------------------------------

// Sensor DHT interno (container)
#define DHT_PIN_IN 4
#define DHT_TYPE   DHT11 
DHT dht_in(DHT_PIN_IN, DHT_TYPE);

// Sensor DHT externo (sala)
#define DHT_PIN_EXT 12
DHT dht_ext(DHT_PIN_EXT, DHT_TYPE);

// Sensor ultrassônico
#define TRIG_PIN 5
#define ECHO_PIN 25

// LEDs de status
#define LED_OK_PIN    2   // LED Verde (status normal)
#define LED_ALERT_PIN 15  // LED Vermelho (alerta)

// Módulo PN532 (RFID)
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Configuração do LCD 16x4 (pinos de exemplo)
#define LCD_RS 26
#define LCD_E  27
#define LCD_D4 28
#define LCD_D5 29
#define LCD_D6 30
#define LCD_D7 31
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Limites e variáveis de temperatura (container)
#define TEMP_MIN 2.0
#define TEMP_MAX 8.0
float THRESHOLD_TEMP = 9.0;       // Limite para alerta de alta temperatura
unsigned long tempAboveStart = 0; // Tempo em que a temperatura ficou acima do limiar

// Intervalos de tempo (milissegundos)
const unsigned long PERIODIC_SEND_INTERVAL = 60000;    // 1 minuto
const unsigned long GREEN_BLINK_INTERVAL   = 5000;     // LED verde: 5s
const unsigned long RED_BLINK_INTERVAL     = 1000;     // LED vermelho: 1s
const unsigned long UNAUTH_WINDOW_BEFORE   = 60000;     // 1 minuto antes
const unsigned long UNAUTH_WINDOW_AFTER    = 15000;     // 15 segundos depois
const unsigned long TEMP_ALERT_DURATION    = 10000;     // 10s de alta temperatura

// Distância para considerar porta fechada
const float distancia_limite = 10.0;

// MQTT: Tópicos – configurados via variáveis para facilitar alteração
String centro    = "centroDeVacinaXYZ";
String container = "containerXYZ";
String mqtt_topic = "/" + centro + "/" + container;  // Ex: /centroDeVacinaXYZ/containerXYZ

// Tópico de controle para receber comandos remotos (ex.: mudança de tópico)
const char* control_topic = "esp32/control";

// Configurações da rede Wi-Fi
const char* ssid     = "Starlink_CIT";
const char* password = "Ufrr@2024Cit";

// Configurações do HiveMQ (broker MQTT)
const char* mqtt_server   = "cd8839ea5ec5423da3aaa6691e5183a5.s1.eu.hivemq.cloud";
const int   mqtt_port     = 8883;
const char* mqtt_username = "hivemq.webclient.1734636778463";
const char* mqtt_password = "EU<pO3F7x?S%wLk4#5ib";

// Objetos de rede e MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Variáveis globais de controle
unsigned long lastPeriodicSend = 0;
unsigned long lastGreenBlink   = 0;
unsigned long lastRedBlink     = 0;

String usuarioAtual = "desconhecido";
bool doorOpen       = false;
bool alarmState     = false;

unsigned long doorOpenTime = 0;  // Tempo em que a porta foi aberta
unsigned long lastRFIDTime = 0;  // Última leitura de RFID

// Histórico de temperaturas (sensor interno) – últimos 10 valores
#define TEMP_HISTORY_SIZE 10
float tempHistory[TEMP_HISTORY_SIZE] = {0};
int tempIndex = 0;
bool avgAlertActive = false; // Indica se alerta de média está ativo

// ----- Variáveis globais do Huffman -----
// Usaremos um dicionário estático construído a partir de um sample representativo dos caracteres do payload.
String sampleAlphabet = "{\"temperatura_interna\": 0.00,\"umidade_interna\": 0.00,\"temperatura_externa\": 0.00,\"umidade_externa\": 0.00,\"estado_porta\": \"Fechada\",\"usuario\": \"desconhecido\",\"alarm_state\": \"OK\",\"motivo\": \"periodic\"}";
 
// ----- SETUP ----- 
void setup() {
  Serial.begin(115200);

  // Configura LEDs
  pinMode(LED_OK_PIN, OUTPUT);
  pinMode(LED_ALERT_PIN, OUTPUT);

  // Configura sensor ultrassônico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Inicializa sensores DHT
  dht_in.begin();
  dht_ext.begin();

  // Inicializa módulo PN532 (RFID)
  Serial.println("Inicializando PN532...");
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (versiondata) {
    nfc.SAMConfig();
    Serial.println("PN532 inicializado com sucesso.");
  } else {
    Serial.println("Módulo PN532 não detectado.");
  }

  // Inicializa display LCD
  lcd.begin(16, 4);
  lcd.clear();
  lcd.print("Iniciando...");

  // Conexão Wi-Fi
  setup_wifi();

  // Configura MQTT
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  // ----- Constrói a árvore Huffman e o dicionário estático -----
  huffmanTree = buildHuffmanTree(sampleAlphabet);
  buildHuffmanCodes(huffmanTree, "");
  
  Serial.println("Dicionário Huffman construído:");
  for (auto pair : huffmanCodes) {
    Serial.print(pair.first);
    Serial.print(": ");
    Serial.println(pair.second);
  }
}

// ----- LOOP -----
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  atualizarTempHistory();
  verificarRFID();
  atualizarDoorState();
  checarEventos();
  atualizarLCD();

  unsigned long now = millis();
  if (now - lastPeriodicSend >= PERIODIC_SEND_INTERVAL) {
    lastPeriodicSend = now;
    enviarDados("periodic");
  }

  if (!alarmState && (now - lastGreenBlink >= GREEN_BLINK_INTERVAL)) {
    lastGreenBlink = now;
    blinkLED(LED_OK_PIN, 100);
  }

  if (alarmState && (now - lastRedBlink >= RED_BLINK_INTERVAL)) {
    lastRedBlink = now;
    blinkLED(LED_ALERT_PIN, 100);
  }

  atualizarLEDs();
}

// ----- FUNÇÕES AUXILIARES ----- 

void setup_wifi() {
  delay(10);
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Reconectando ao HiveMQ...");
    if (client.connect("ESP32_Client", mqtt_username, mqtt_password)) {
      Serial.println("Conectado ao HiveMQ!");
      client.subscribe(control_topic);
    } else {
      delay(5000);
    }
  }
}

void blinkLED(uint8_t pin, int duration) {
  digitalWrite(pin, HIGH);
  delay(duration);
  digitalWrite(pin, LOW);
}

void atualizarTempHistory() {
  float tempAtual = dht_in.readTemperature();
  tempHistory[tempIndex] = tempAtual;
  tempIndex = (tempIndex + 1) % TEMP_HISTORY_SIZE;
  
  if (TEMP_HISTORY_SIZE >= 10) {
    float soma5 = tempAtual;
    float soma10 = tempAtual;
    for (int i = 1; i < 5; i++) {
      int idx = (tempIndex - i + TEMP_HISTORY_SIZE) % TEMP_HISTORY_SIZE;
      soma5 += tempHistory[idx];
    }
    for (int i = 1; i < 10; i++) {
      int idx = (tempIndex - i + TEMP_HISTORY_SIZE) % TEMP_HISTORY_SIZE;
      soma10 += tempHistory[idx];
    }
    float avg5 = soma5 / 5.0;
    float avg10 = soma10 / 10.0;
    if (avg5 > avg10) {
      if (!avgAlertActive) {
        Serial.println("Alerta de média: condição alterada.");
        avgAlertActive = true;
        enviarDados("avg_temp_alert");
      }
    } else {
      if (avgAlertActive) {
        Serial.println("Média estabilizada.");
        avgAlertActive = false;
        enviarDados("avg_temp_stable");
      }
    }
  }
}

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
        usuarioAtual = novoUsuario;
        lastRFIDTime = millis();
        Serial.println("Novo usuário identificado: " + usuarioAtual);
        if (doorOpen) {
          enviarDados("rfid_door");
        }
      }
    }
  }
}

void atualizarDoorState() {
  float distancia = medirDistancia();
  bool newDoorOpen = (distancia > distancia_limite);
  if (newDoorOpen != doorOpen) {
    doorOpen = newDoorOpen;
    if (doorOpen) {
      doorOpenTime = millis();
      Serial.println("Porta aberta (ultrassônico).");
      if (millis() - lastRFIDTime > UNAUTH_WINDOW_BEFORE) {
        enviarDados("unauthorized_door");
        alarmState = true;
      }
    } else {
      Serial.println("Porta fechada (ultrassônico).");
      alarmState = false;
    }
  }
}

void checarEventos() {
  float temperatura = dht_in.readTemperature();
  if (temperatura > THRESHOLD_TEMP) {
    if (tempAboveStart == 0) {
      tempAboveStart = millis();
    } else if (millis() - tempAboveStart >= TEMP_ALERT_DURATION) {
      Serial.println("Temperatura alta por mais de 10 segundos.");
      enviarDados("temp_high");
      alarmState = true;
    }
  } else {
    tempAboveStart = 0;
  }
}

void atualizarLEDs() {
  if (alarmState) {
    digitalWrite(LED_ALERT_PIN, HIGH);
    digitalWrite(LED_OK_PIN, LOW);
  } else {
    digitalWrite(LED_ALERT_PIN, LOW);
    digitalWrite(LED_OK_PIN, HIGH);
  }
}

void atualizarLCD() {
  float tempInterna = dht_in.readTemperature();
  float tempExterna = dht_ext.readTemperature();
  float umidadeExterna = dht_ext.readHumidity();
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Int:");
  lcd.print(tempInterna, 1);
  lcd.print("C Ext:");
  lcd.print(tempExterna, 1);
  lcd.print("C");
  
  lcd.setCursor(0, 1);
  lcd.print("Porta:");
  lcd.print(doorOpen ? "Aberta" : "Fechada");
  
  lcd.setCursor(0, 2);
  lcd.print("Umid Ext:");
  lcd.print(umidadeExterna, 1);
  lcd.print("%");
  
  lcd.setCursor(0, 3);
  lcd.print("Status:");
  lcd.print(alarmState ? "ALERT" : "OK");
}

// Função para enviar dados via MQTT com compressão Huffman
void enviarDados(String motivo) {
  float temperatura = dht_in.readTemperature();
  float umidade     = dht_in.readHumidity();
  float distancia   = medirDistancia();
  String estado_porta = (distancia <= distancia_limite) ? "Fechada" : "Aberta";
  
  float temperatura_ext = dht_ext.readTemperature();
  float umidade_ext     = dht_ext.readHumidity();
  
  String payload = "{";
  payload += "\"temperatura_interna\": " + String(temperatura, 2) + ",";
  payload += "\"umidade_interna\": " + String(umidade, 2) + ",";
  payload += "\"temperatura_externa\": " + String(temperatura_ext, 2) + ",";
  payload += "\"umidade_externa\": " + String(umidade_ext, 2) + ",";
  payload += "\"estado_porta\": \"" + estado_porta + "\",";
  payload += "\"usuario\": \"" + usuarioAtual + "\",";
  payload += "\"alarm_state\": \"" + String(alarmState ? "ALERT" : "OK") + "\",";
  payload += "\"motivo\": \"" + motivo + "\"";
  payload += "}";
  
  // Comprima o payload usando Huffman
  String compressedPayload = huffmanCompress(payload);
  
  if (client.publish(mqtt_topic.c_str(), compressedPayload.c_str())) {
    Serial.println("Dados enviados (comprimidos) via MQTT: " + compressedPayload);
  } else {
    Serial.println("Falha no envio via MQTT.");
  }
}

// Mede a distância usando o sensor ultrassônico
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duracao = pulseIn(ECHO_PIN, HIGH);
  return (duracao * 0.034) / 2.0;
}

// Callback para mensagens MQTT recebidas; descomprime se necessário
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  // Se a mensagem veio do tópico de controle, a descomprime
  if (String(topic) == control_topic) {
    String decompressed = huffmanDecompress(message, huffmanTree);
    Serial.println("Mensagem recebida (descomprimida) em [" + String(topic) + "]: " + decompressed);
    // Aqui você pode processar o comando, por exemplo, alterar o tópico MQTT.
    if (decompressed.indexOf("change_topic") >= 0) {
      int firstSep = decompressed.indexOf(";");
      int secondSep = decompressed.indexOf(";", firstSep + 1);
      if (firstSep > 0 && secondSep > firstSep) {
        String novoCentro = decompressed.substring(firstSep + 1, secondSep);
        String novoContainer = decompressed.substring(secondSep + 1);
        centro = novoCentro;
        container = novoContainer;
        mqtt_topic = "/" + centro + "/" + container;
        Serial.println("Novo MQTT topic: " + mqtt_topic);
      }
    }
  } else {
    Serial.println("Mensagem recebida em [" + String(topic) + "]: " + message);
  }
}
