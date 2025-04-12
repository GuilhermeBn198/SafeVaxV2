#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <time.h>
#include <Adafruit_PN532.h>
#include <LiquidCrystal.h>
#include <Arduino.h>
#include <map>
#include <queue>

// ----- IMPLEMENTAÇÃO DO HUFFMAN -----
struct HuffmanNode {
  char ch; int freq;
  HuffmanNode *left, *right;
  HuffmanNode(char _ch, int _freq) : ch(_ch), freq(_freq), left(NULL), right(NULL) {}
};
struct CompareNode {
  bool operator()(HuffmanNode* const & n1, HuffmanNode* const & n2) {
    return n1->freq > n2->freq;
  }
};

HuffmanNode*   huffmanTree;
std::map<char,String> huffmanCodes;

void buildHuffmanCodes(HuffmanNode* root, String code = "") {
  if (!root) return;
  if (!root->left && !root->right) {
    huffmanCodes[root->ch] = code;
  }
  buildHuffmanCodes(root->left,  code + "0");
  buildHuffmanCodes(root->right, code + "1");
}

HuffmanNode* buildHuffmanTree(const String &data) {
  std::map<char,int> freq;
  for (char c : data) freq[c]++;
  std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, CompareNode> pq;
  for (auto &p : freq) pq.push(new HuffmanNode(p.first, p.second));
  while (pq.size() > 1) {
    HuffmanNode* l = pq.top(); pq.pop();
    HuffmanNode* r = pq.top(); pq.pop();
    HuffmanNode* m = new HuffmanNode('\0', l->freq + r->freq);
    m->left = l; m->right = r;
    pq.push(m);
  }
  return pq.top();
}

String huffmanCompress(const String &data) {
  String out;
  for (char c : data) out += huffmanCodes[c];
  return out;
}

String huffmanDecompress(const String &bits, HuffmanNode* root) {
  String out;
  HuffmanNode* curr = root;
  for (char b : bits) {
    curr = (b == '0') ? curr->left : curr->right;
    if (!curr->left && !curr->right) {
      out += curr->ch;
      curr = root;
    }
  }
  return out;
}

void freeHuffmanTree(HuffmanNode* node) {
    if (!node) return;
    freeHuffmanTree(node->left);
    freeHuffmanTree(node->right);
    delete node;
  }

// --------------------------------------------------------------------------------
// Definições de pinos e constantes do projeto
// --------------------------------------------------------------------------------

// Sensor DHT interno (container)
const int DHT_PIN_IN = 4;
const int DHT_TYPE =   DHT11;
DHT dht_in(DHT_PIN_IN, DHT_TYPE);

// Sensor DHT externo (sala)
const int DHT_PIN_EXT = 18;
DHT dht_ext(DHT_PIN_EXT, DHT_TYPE);

// Sensor ultrassônico
const int TRIG_PIN = 5;
const int ECHO_PIN = 23;

// LEDs de status
const int LED_OK_PIN = 2;   // LED Verde (status normal)
const int LED_ALERT_PIN = 15;  // LED Vermelho (alerta)

// Módulo PN532 (RFID)
const int SDA_PIN = 21;
const int SCL_PIN = 22;
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Configuração do LCD 20x4
const int LCD_RS = 26;
const int LCD_E =  27;
const int LCD_D4 = 33;
const int LCD_D5 = 32;
const int LCD_D6 = 25;
const int LCD_D7 = 13;
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

// Limites e variáveis de temperatura (container)
float THRESHOLD_TEMP = 25.5;
unsigned long tempAboveStart = 0;

// Intervalos de tempo (ms)
const unsigned long PERIODIC_SEND_INTERVAL = 60000;
const unsigned long GREEN_BLINK_INTERVAL   = 5000;
const unsigned long RED_BLINK_INTERVAL     = 1000;
const unsigned long UNAUTH_WINDOW_BEFORE   = 60000;
const unsigned long TEMP_ALERT_DURATION    = 10000;

// Distância para considerar porta fechada
const float distancia_limite = 7.0;

// MQTT: tópicos dinâmicos
String centro    = "centroDeVacinaXYZ";
String container = "containerXYZ";
String mqtt_topic = "/" + centro + "/" + container;

// Tópico de controle para comandos
const char* control_topic = "esp32/control";

// Wi-Fi e MQTT
const char* ssid     = "Starlink_CIT";
const char* password = "Ufrr@2024Cit";
const char* mqtt_server   = "07356c1b41e34d65a6152a202151c24d.s1.eu.hivemq.cloud";
const int   mqtt_port     = 8883;
const char* mqtt_username = "hivemq.webclient.1744472320531";
const char* mqtt_password = "QOwsih0.7!tV%L6gD1B>";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Estado global
unsigned long lastPeriodicSend = 0, lastGreenBlink = 0, lastRedBlink = 0;
String usuarioAtual = "desconhecido";
bool doorOpen = false, alarmState = false;
unsigned long lastRFIDTime = 0;

// Histórico de temperaturas
#define TEMP_HISTORY_SIZE 10
float tempHistory[TEMP_HISTORY_SIZE] = {0};
int tempIndex = 0;
bool avgAlertActive = false;

// Sample para Huffman
String sampleAlphabet = "{\"temperatura_interna\": 0.00,\"umidade_interna\": 0.00,\"temperatura_externa\": 0.00,\"umidade_externa\": 0.00,\"estado_porta\": \"Fechada\",\"usuario\": \"desconhecido\",\"alarm_state\": \"OK\",\"motivo\": \"periodic\"}";

// --------------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("[DEBUG] Iniciando setup...");

  // LEDs
  pinMode(LED_OK_PIN, OUTPUT);
  pinMode(LED_ALERT_PIN, OUTPUT);

  // Ultrassônico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // DHT
  dht_in.begin();
  dht_ext.begin();
  Serial.println("[DEBUG] Sensores DHT inicializados.");

  // PN532
  Serial.println("[DEBUG] Inicializando PN532...");
  nfc.begin();
  uint32_t version = nfc.getFirmwareVersion();
  if (version) {
    nfc.SAMConfig();
    Serial.println("[DEBUG] PN532 OK.");
  } else {
    Serial.println("[ERROR] PN532 não detectado!");
  }

  // LCD
  lcd.begin(20, 4);
  lcd.clear();
  lcd.print("Iniciando...");
  Serial.println("[DEBUG] LCD inicializado.");

  // Wi-Fi
  setup_wifi();

  // MQTT
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println("[DEBUG] MQTT configurado.");

  // Huffman
  Serial.println("[DEBUG] Construindo árvore Huffman...");
  if (huffmanTree) {
    freeHuffmanTree(huffmanTree);
    huffmanCodes.clear();
  }
  huffmanTree = buildHuffmanTree(sampleAlphabet);
  buildHuffmanCodes(huffmanTree);
  Serial.println("[DEBUG] Dicionário Huffman:");
  for (auto &p : huffmanCodes) {
    Serial.printf(" '%c' -> %s\n", p.first, p.second.c_str());
  }
  Serial.println("[DEBUG] Árvore Huffman construída.");

  Serial.println("[DEBUG] Setup concluído!!!");
}

// --------------------------------------------------------------------------------
// LOOP
// --------------------------------------------------------------------------------
void loop() {
  if (!client.connected()) {
    Serial.println("[DEBUG] MQTT desconectado, reconectando...");
    reconnect_mqtt();
  }
  client.loop();

  // Atualizações periódicas de estado
  atualizarTempHistory();
  verificarRFID();
  atualizarDoorState();
  checarEventos();
  atualizarLCD();

  unsigned long now = millis();
  if (now - lastPeriodicSend >= PERIODIC_SEND_INTERVAL) {
    lastPeriodicSend = now;
    Serial.println("[DEBUG] Envio periódico acionado.");
    enviarDados("periodic");
  }

  // LEDs de status
  if (!alarmState && now - lastGreenBlink >= GREEN_BLINK_INTERVAL) {
    lastGreenBlink = now;
    blinkLED(LED_OK_PIN, 100);
    Serial.println("[DEBUG] Blink verde.");
  }
  if (alarmState && now - lastRedBlink >= RED_BLINK_INTERVAL) {
    lastRedBlink = now;
    blinkLED(LED_ALERT_PIN, 100);
    Serial.println("[DEBUG] Blink vermelho.");
  }
}

// --------------------------------------------------------------------------------
// FUNÇÕES AUXILIARES
// --------------------------------------------------------------------------------

// Conecta ao Wi‑Fi e imprime debug
static void setup_wifi() {
  Serial.print("[DEBUG] Conectando ao Wi-Fi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[DEBUG] Wi-Fi conectado: %s\n", WiFi.localIP().toString().c_str());
}

// Reconecta ao broker MQTT
static void reconnect_mqtt() {
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
static void blinkLED(uint8_t pin, int duration) {
  digitalWrite(pin, HIGH);
  delay(duration);
  digitalWrite(pin, LOW);
}

// Atualiza os LEDs de status
static void atualizarLEDs() {
  digitalWrite(LED_ALERT_PIN, alarmState ? HIGH : LOW);
  digitalWrite(LED_OK_PIN,    alarmState ? LOW  : HIGH);
}

// Mede distância ultrassônica
static float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH);
  return (dur * 0.034) / 2.0;
}

void atualizarTempHistory() {
  float t = dht_in.readTemperature();
  tempHistory[tempIndex] = t;
  tempIndex = (tempIndex + 1) % TEMP_HISTORY_SIZE;
  Serial.printf("[DEBUG] Temp interna lida: %.2f°C\n", t);

  if (TEMP_HISTORY_SIZE >= 10) {
    float s5 = t, s10 = t;
    for (int i = 1; i < 5; i++) s5  += tempHistory[(tempIndex - i + TEMP_HISTORY_SIZE) % TEMP_HISTORY_SIZE];
    for (int i = 1; i < 10; i++) s10 += tempHistory[(tempIndex - i + TEMP_HISTORY_SIZE) % TEMP_HISTORY_SIZE];
    if (s5/5.0 > s10/10.0 && !avgAlertActive) {
      avgAlertActive = true;
      Serial.println("[DEBUG] Alerta média ativado.");
      enviarDados("avg_temp_alert");
    } else if (s5/5.0 <= s10/10.0 && avgAlertActive) {
      avgAlertActive = false;
      Serial.println("[DEBUG] Média estabilizada.");
      enviarDados("avg_temp_stable");
    }
  }
}

void atualizarLCD() {
  float ti = dht_in.readTemperature();
  float te = dht_ext.readTemperature();
  float he = dht_ext.readHumidity();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.printf("Int:%.1fC Ext:%.1fC", ti, te);
  lcd.setCursor(0,1);
  lcd.printf("Porta:%s", doorOpen ? "Aberta" : "Fechada");
  lcd.setCursor(0,2);
  lcd.printf("Umid Ext:%.1f%%", he);
  lcd.setCursor(0,3);
  lcd.printf("Stat:%s", alarmState ? "ALERT" : "OK");
}

void verificarRFID() {
  uint8_t success = nfc.inListPassiveTarget();
  if (success) {
    uint8_t uid[7], len;
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len)) {
      String u;
      for (int i = 0; i < len; i++) u += String(uid[i], HEX);
      if (u != usuarioAtual) {
        usuarioAtual = u;
        lastRFIDTime = millis();
        Serial.printf("[DEBUG] Novo RFID: %s\n", usuarioAtual.c_str());
        if (doorOpen) {
          Serial.println("[DEBUG] Porta aberta com RFID, enviando rfid_door.");
          enviarDados("rfid_door");
        }
      }
    }
  }
}

void atualizarDoorState() {
  float dist = medirDistancia();
  bool nowOpen = dist > distancia_limite;
  Serial.printf("[DEBUG] Distância lida: %.2f cm\n", dist);
  if (nowOpen != doorOpen) {
    doorOpen = nowOpen;
    if (doorOpen) {
      Serial.println("[DEBUG] Porta abriu.");
      if (millis() - lastRFIDTime > UNAUTH_WINDOW_BEFORE) {
        Serial.println("[DEBUG] Porta não autorizada, enviando unauthorized_door.");
        alarmState = true;
        enviarDados("unauthorized_door");
      }
    } else {
      Serial.println("[DEBUG] Porta fechou.");
      alarmState = false;
    }
  }
}

void checarEventos() {
  float t = dht_in.readTemperature();
  if (t > THRESHOLD_TEMP) {
    if (tempAboveStart == 0) {
      tempAboveStart = millis();
      Serial.println("[DEBUG] Temperatura acima do limiar, iniciando contador.");
    } else if (millis() - tempAboveStart >= TEMP_ALERT_DURATION) {
      Serial.println("[DEBUG] Temperatura alta por 10s, enviando temp_high.");
      alarmState = true;
      enviarDados("temp_high");
    }
  } else {
    tempAboveStart = 0;
  }
}

void enviarDados(String motivo) {
  Serial.printf("[DEBUG] Montando payload para motivo: %s\n", motivo.c_str());
  float ti = dht_in.readTemperature();
  float hi = dht_in.readHumidity();
  float te = dht_ext.readTemperature();
  float he = dht_ext.readHumidity();
  String portState = (medirDistancia() <= distancia_limite) ? "Fechada" : "Aberta";

  String payload = "{";
  payload += "\"temperatura_interna\":" + String(ti,2) + ",";
  payload += "\"umidade_interna\":"   + String(hi,2) + ",";
  payload += "\"temperatura_externa\":" + String(te,2) + ",";
  payload += "\"umidade_externa\":"   + String(he,2) + ",";
  payload += "\"estado_porta\":\""     + portState + "\",";
  payload += "\"usuario\":\""          + usuarioAtual + "\",";
  payload += "\"alarm_state\":\""      + String(alarmState?"ALERT":"OK") + "\",";
  payload += "\"motivo\":\""           + motivo + "\"}";
  Serial.println("[DEBUG] Payload JSON: " + payload);

  Serial.println("[DEBUG] Compactando payload com Huffman...");
  String compressed = huffmanCompress(payload);
  Serial.println("[DEBUG] Payload compactado.");
  Serial.println("[DEBUG] Payload Huffman: " + compressed);

  bool ok = client.publish(mqtt_topic.c_str(), compressed.c_str());
  Serial.printf("[DEBUG] Publish em %s %s\n", mqtt_topic.c_str(), ok ? "SUCESSO" : "FALHA");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[DEBUG] Mensagem recebida em [%s], tamanho %u\n", topic, length);
  String msg;
  for (unsigned int i = 0; i < length; i++) msg += (char)payload[i];
  Serial.println("[DEBUG] Mensagem bruta: " + msg);
  if (String(topic) == control_topic) {
    String decompressed = huffmanDecompress(msg, huffmanTree);
    Serial.println("[DEBUG] Mensagem descompactada: " + decompressed);
    if (decompressed.startsWith("change_topic")) {
      int a = decompressed.indexOf(';');
      int b = decompressed.indexOf(';', a + 1);
      centro    = decompressed.substring(a+1, b);
      container = decompressed.substring(b+1);
      mqtt_topic = "/" + centro + "/" + container;
      Serial.println("[DEBUG] Tópico alterado para: " + mqtt_topic);
    }
  }
}