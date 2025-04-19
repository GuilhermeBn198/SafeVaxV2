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

// ----- CERTIFICADOS TLS -----
const char* hivemq_root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIFBjCCAu6gAwIBAgIRAIp9PhPWLzDvI4a9KQdrNPgwDQYJKoZIhvcNAQELBQAwTzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2VhcmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMjQwMzEzMDAwMDAwWhcNMjcwMzEyMjM1OTU5WjAzMQswCQYDVQQGEwJVUzEWMBQGA1UEChMNTGV0J3MgRW5jcnlwdDEMMAoGA1UEAxMDUjExMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuoe8XBsAOcvKCs3UZxD5ATylTqVhyybKUvsVAbe5KPUoHu0nsyQYOWcJDAjs4DqwO3cOvfPlOVRBDE6uQdaZdN5R2+97/1i9qLcT9t4x1fJyyXJqC4N0lZxGAGQUmfOx2SLZzaiSqhwmej/+71gFewiVgdtxD4774zEJuwm+UE1fj5F2PVqdnoPy6cRms+EGZkNIGIBloDcYmpuEMpexsr3E+BUAnSeI++JjF5ZsmydnS8TbKF5pwnnwSVzgJFDhxLyhBax7QG0AtMJBP6dYuC/FXJuluwme8f7rsIU5/agK70XEeOtlKsLPXzze41xNG/cLJyuqC0J3U095ah2H2QIDAQABo4H4MIH1MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAgYIKwYBBQUHAwEwEgYDVR0TAQH/BAgwBgEB/wIBADAdBgNVHQ4EFgQUxc9GpOr0w8B6bJXELbBeki8m47kwHwYDVR0jBBgwFoAUebRZ5nu25eQBc4AIiMgaWPbpm24wMgYIKwYBBQUHAQEEJjAkMCIGCCsGAQUFBzAChhZodHRwOi8veDEuaS5sZW5jci5vcmcvMBMGA1UdIAQMMAowCAYGZ4EMAQIBMCcGA1UdHwQgMB4wHKAaoBiGFmh0dHA6Ly94MS5jLmxlbmNyLm9yZy8wDQYJKoZIhvcNAQELBQADggIBAE7iiV0KAxyQOND1H/lxXPjDj7I3iHpvsCUf7b632IYGjukJhM1yv4Hz/MrPU0jtvfZpQtSlET41yBOykh0FX+ou1Nj4ScOt9ZmWnO8m2OG0JAtIIE3801S0qcYhyOE2G/93ZCkXufBL713qzXnQv5C/viOykNpKqUgxdKlEC+Hi9i2DcaR1e9KUwQUZRhy5j/PEdEglKg3l9dtD4tuTm7kZtB8v32oOjzHTYw+7KdzdZiw/sBtnUfhBPORNuay4pJxmY/WrhSMdzFO2q3Gu3MUBcdo27goYKjL9CTF8j/Zz55yctUoVaneCWs/ajUX+HypkBTA+c8LGDLnWO2NKq0YD/pnARkAnYGPfUDoHR9gVSp/qRx+ZWghiDLZsMwhN1zjtSC0uBWiugF3vTNzYIEFfaPG7Ws3jDrAMMYebQ95JQ+HIBD/RPBuHRTBpqKlyDnkSHDHYPiNX3adPoPAcgdF3H2/W0rmoswMWgTlLn1Wu0mrks7/qpdWfS6PJ1jty80r2VKsM/Dj3YIDfbjXKdaFU5C+8bhfJGqU3taKauuz0wHVGT3eo6FlWkWYtbt4pgdamlwVeZEW+LM7qZEJEsMNPrfC03APKmZsJgpWCDWOKZvkZcvjVuYkQ4omYCTX5ohy+knMjdOmdH9c7SpqEWBDC86fiNex+O0XOMEZSa8DA" \
"-----END CERTIFICATE-----\n";

// ----- IMPLEMENTAÇÃO DO HUFFMAN -----
struct HuffmanNode {
  char ch;
  int freq;
  HuffmanNode *left, *right;
  HuffmanNode(char _ch, int _freq) : ch(_ch), freq(_freq), left(NULL), right(NULL) {}
};
struct CompareNode {
  bool operator()(HuffmanNode* const & n1, HuffmanNode* const & n2) {
    return n1->freq > n2->freq;
  }
};

HuffmanNode* huffmanTree = NULL;
std::map<char, String> huffmanCodes;

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

HuffmanNode* buildHuffmanTreeFromCodes(std::map<char, String> &codes) {
  HuffmanNode* root = new HuffmanNode('\0', 0);
  for (auto &entry : codes) {
    char key = entry.first;
    String code = entry.second;
    HuffmanNode* current = root;
    for (int i = 0; i < code.length(); i++) {
      char bit = code.charAt(i);
      if (bit == '0') {
        if (!current->left) current->left = new HuffmanNode('\0', 0);
        current = current->left;
      } else {
        if (!current->right) current->right = new HuffmanNode('\0', 0);
        current = current->right;
      }
    }
    current->ch = key;
  }
  return root;
}

// --------------------------------------------------------------------------------
// PINOS E CONSTANTES
// --------------------------------------------------------------------------------
const int DHT_PIN_IN      = 4;
const int DHT_TYPE        = DHT11;
DHT dht_in(DHT_PIN_IN, DHT_TYPE);
const int DHT_PIN_EXT     = 18;
DHT dht_ext(DHT_PIN_EXT, DHT_TYPE);
const int TRIG_PIN        = 5;
const int ECHO_PIN        = 23;
const int LED_OK_PIN      = 2;
const int LED_ALERT_PIN   = 15;
const int SDA_PIN         = 21;
const int SCL_PIN         = 22;
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
const int LCD_RS = 26, LCD_E = 27, LCD_D4 = 33, LCD_D5 = 32, LCD_D6 = 25, LCD_D7 = 13;
LiquidCrystal lcd(LCD_RS, LCD_E, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

float THRESHOLD_TEMP       = 25.5;
unsigned long tempAboveStart = 0;
const unsigned long PERIODIC_SEND_INTERVAL = 60000;
const unsigned long GREEN_BLINK_INTERVAL   = 5000;
const unsigned long RED_BLINK_INTERVAL     = 1000;
const unsigned long UNAUTH_WINDOW_BEFORE   = 60000;
const unsigned long TEMP_ALERT_DURATION    = 10000;
const float distancia_limite = 7.0;

String centro    = "centroDeVacinaXYZ";
String container = "containerXYZ";
String mqtt_topic = "/" + centro + "/" + container;
const char* control_topic = "esp32/control";

const char* ssid     = "Starlink_CIT";
const char* password = "Ufrr@2024Cit";
const char* mqtt_server   = "6ef5d17c52994de69c847b10011cad30.s1.eu.hivemq.cloud";
const int   mqtt_port     = 8883;
const char* mqtt_username = "hivemq.webclient.1745085086443";
const char* mqtt_password = "1mI3:&xakvOCg*T$4JP7";

WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastPeriodicSend = 0, lastGreenBlink = 0, lastRedBlink = 0;
String usuarioAtual = "desconhecido";
bool doorOpen = false, alarmState = false;
unsigned long lastRFIDTime = 0;
#define TEMP_HISTORY_SIZE 10
float tempHistory[TEMP_HISTORY_SIZE] = {0};
int tempIndex = 0;
bool avgAlertActive = false;

// --------------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  Serial.println("[DEBUG] Iniciando setup...");

  pinMode(LED_OK_PIN, OUTPUT);
  pinMode(LED_ALERT_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  dht_in.begin();
  dht_ext.begin();
  Serial.println("[DEBUG] Sensores DHT inicializados.");

  Serial.println("[DEBUG] Inicializando PN532...");
  nfc.begin();
  uint32_t version = nfc.getFirmwareVersion();
  if (version) { nfc.SAMConfig(); Serial.println("[DEBUG] PN532 OK."); }
  else { Serial.println("[ERROR] PN532 não detectado!"); }

  lcd.begin(20, 4);
  lcd.clear();
  lcd.print("Iniciando...");
  Serial.println("[DEBUG] LCD inicializado.");

  setup_wifi();

  // Configuração TLS/MQTT
  espClient.setCACert(hivemq_root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println("[DEBUG] MQTT TLS configurado.");

  Serial.println("[DEBUG] Setup concluído!!!");
}

// --------------------------------------------------------------------------------
// LOOP
// --------------------------------------------------------------------------------
void loop() {
  if (!client.connected()) reconnect_mqtt();
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

  if (!alarmState && now - lastGreenBlink >= GREEN_BLINK_INTERVAL) {
    lastGreenBlink = now; blinkLED(LED_OK_PIN, 100);
  }
  if (alarmState && now - lastRedBlink >= RED_BLINK_INTERVAL) {
    lastRedBlink = now; blinkLED(LED_ALERT_PIN, 100);
  }
}

// --------------------------------------------------------------------------------
// FUNÇÕES AUXILIARES
// --------------------------------------------------------------------------------

// Conecta ao Wi‑Fi e imprime debug
static void setup_wifi() {
  Serial.print("[DEBUG] Conectando ao Wi‑Fi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[DEBUG] Wi‑Fi conectado: %s\n", WiFi.localIP().toString().c_str());
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

// Pisca um LED por 'duration' ms
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
  // Serial.printf("[DEBUG] Temp interna lida: %.2f°C\n", t);

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
      for (int i = 0; i < len; i++) {
        u += String(uid[i], HEX);
      }
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
  // Serial.printf("[DEBUG] Distância lida: %.2f cm\n", dist);
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

// Função que envia os dados pelo MQTT com payload compactado via Huffman
void enviarDados(String motivo) {
  Serial.printf("[DEBUG] Montando payload para motivo: %s\n", motivo.c_str());
  float ti = dht_in.readTemperature();
  float hi = dht_in.readHumidity();
  float te = dht_ext.readTemperature();
  float he = dht_ext.readHumidity();
  String portState = (medirDistancia() <= distancia_limite) ? "Fechada" : "Aberta";

  String payload = "{";
  payload += "\"temperatura_interna\":" + String(ti, 2) + ",";
  payload += "\"umidade_interna\":"   + String(hi, 2) + ",";
  payload += "\"temperatura_externa\":" + String(te, 2) + ",";
  payload += "\"umidade_externa\":"   + String(he, 2) + ",";
  payload += "\"estado_porta\":\""     + portState + "\",";
  payload += "\"usuario\":\""          + usuarioAtual + "\",";
  payload += "\"alarm_state\":\""      + String(alarmState ? "ALERT" : "OK") + "\",";
  payload += "\"motivo\":\""           + motivo + "\"}";
  Serial.println("[DEBUG] Payload JSON: " + payload);

  // --- Construção dinâmica do dicionário Huffman ---
  // Libera árvore e dicionário anteriores (se houver)
  if (huffmanTree) {
    freeHuffmanTree(huffmanTree);
    huffmanCodes.clear();
  }
  // Constrói a árvore com base no payload atual
  huffmanTree = buildHuffmanTree(payload);
  buildHuffmanCodes(huffmanTree);

  // Serializa o dicionário em um header (formato: cada par 'ch:code' separado por ;)
  String header = "";
  for (auto &p : huffmanCodes) {
    header += p.first;
    header += ":";
    header += p.second;
    header += ";";
  }
  // Remove o último ponto e vírgula
  if (header.endsWith(";")) {
    header.remove(header.length()-1);
  }

  // Comprimi o payload usando o dicionário dinâmico
  String compressed = huffmanCompress(payload);

  // Junta header e payload compactado usando "||" como delimitador
  String finalPayload = header + "||" + compressed;
  Serial.println("[DEBUG] Payload final (header||dados compactados): " + finalPayload);

  bool ok = client.publish(mqtt_topic.c_str(), finalPayload.c_str());
  Serial.printf("[DEBUG] Publish em %s %s\n", mqtt_topic.c_str(), ok ? "SUCESSO" : "FALHA");
}

// Callback para mensagens MQTT recebidas
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("[DEBUG] Mensagem recebida em [%s], tamanho %u\n", topic, length);
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  Serial.println("[DEBUG] Mensagem bruta: " + msg);

  // Se a mensagem contiver o delimitador "||", trata como mensagem compactada com header
  int delimiterIndex = msg.indexOf("||");
  if (delimiterIndex != -1) {
    String header = msg.substring(0, delimiterIndex);
    String compressed = msg.substring(delimiterIndex + 2);

    // Reconstrói o dicionário a partir do header
    std::map<char, String> dynamicCodes;
    int start = 0;
    while (start < header.length()) {
      int colonIndex = header.indexOf(':', start);
      int semicolonIndex = header.indexOf(';', start);
      if (colonIndex == -1) break;
      // Se não encontrar ponto e vírgula, pega até o final
      if (semicolonIndex == -1) {
        semicolonIndex = header.length();
      }
      char key = header.charAt(start);
      String code = header.substring(colonIndex + 1, semicolonIndex);
      dynamicCodes[key] = code;
      start = semicolonIndex + 1;
    }
    // Constrói a árvore a partir do dicionário dinâmico
    HuffmanNode* dynamicTree = buildHuffmanTreeFromCodes(dynamicCodes);
    String decompressed = huffmanDecompress(compressed, dynamicTree);
    Serial.println("[DEBUG] Mensagem descompactada: " + decompressed);
    freeHuffmanTree(dynamicTree);

    // Caso a mensagem descompactada seja de controle, processa o comando
    if (decompressed.startsWith("change_topic")) {
      int a = decompressed.indexOf(';');
      int b = decompressed.indexOf(';', a + 1);
      centro    = decompressed.substring(a+1, b);
      container = decompressed.substring(b+1);
      mqtt_topic = "/" + centro + "/" + container;
      Serial.println("[DEBUG] Tópico alterado para: " + mqtt_topic);
    }
  }
  else {
    // Se não houver delimitador, trata como mensagem não compactada
    Serial.println("[DEBUG] Mensagem sem compactação: " + msg);
  }
}
