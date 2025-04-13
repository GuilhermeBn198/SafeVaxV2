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
#include <AESLib.h>
#include <Base64.h> // Certifique-se de ter uma biblioteca de Base64 instalada

// ======== AES‑CBC IMPLEMENTATION ========
static const uint8_t AES_KEY[32] = {
    0x60, 0x3d, 0xeb, 0x10, 0x15, 0xca, 0x71, 0xbe,
    0x2b, 0x73, 0xae, 0xf0, 0x85, 0x7d, 0x77, 0x81,
    0x1f, 0x35, 0x2c, 0x07, 0x3b, 0x61, 0x08, 0xd7,
    0x2d, 0x98, 0x10, 0xa3, 0x09, 0x14, 0xdf, 0xf4};
AESLib aesLib;
void generateIV(uint8_t *iv)
{
  for (int i = 0; i < 16; i++)
    iv[i] = random(0, 256);
}
String aes_encrypt(const String &plain)
{
  int len = plain.length();
  int padded = ((len + 15) / 16) * 16;
  uint8_t iv[16];
  generateIV(iv);
  uint8_t cipher[padded];
  aesLib.encryptCBC((uint8_t *)plain.c_str(), len, cipher, AES_KEY, 256, iv);
  uint8_t msg[16 + padded];
  memcpy(msg, iv, 16);
  memcpy(msg + 16, cipher, padded);
  int b64len = base64_enc_len(16 + padded);
  char b64[b64len + 1];
  base64_encode(b64, (char *)msg, 16 + padded);
  return String(b64);
}
String aes_decrypt(const String &b64)
{
  int bl = b64.length();
  int dl = base64_dec_len(b64.c_str(), bl);
  uint8_t buf[dl];
  base64_decode((char *)buf, b64.c_str(), bl);
  uint8_t iv[16];
  memcpy(iv, buf, 16);
  int clen = dl - 16;
  uint8_t plain[clen + 1];
  aesLib.decryptCBC(buf + 16, clen, plain, AES_KEY, 256, iv);
  plain[clen] = '\0'; // Assegura a terminação
  return String((char *)plain);
}

// ----- IMPLEMENTAÇÃO DO HUFFMAN -----
// Estrutura de nó da árvore Huffman
struct HuffmanNode
{
  char ch;
  int freq;
  HuffmanNode *left, *right;
  HuffmanNode(char _ch, int _freq) : ch(_ch), freq(_freq), left(NULL), right(NULL) {}
};

// Função de comparação para a fila de prioridade
struct CompareNode
{
  bool operator()(HuffmanNode *const &n1, HuffmanNode *const &n2)
  {
    return n1->freq > n2->freq;
  }
};

HuffmanNode *huffmanTree = NULL;
std::map<char, String> huffmanCodes;

// Constrói os códigos a partir da árvore Huffman
void buildHuffmanCodes(HuffmanNode *root, String code = "")
{
  if (!root)
    return;
  if (!root->left && !root->right)
  {
    huffmanCodes[root->ch] = code;
  }
  buildHuffmanCodes(root->left, code + "0");
  buildHuffmanCodes(root->right, code + "1");
}

// Constrói a árvore Huffman com base na frequência dos caracteres
HuffmanNode *buildHuffmanTree(const String &data)
{
  std::map<char, int> freq;
  for (char c : data)
    freq[c]++;
  std::priority_queue<HuffmanNode *, std::vector<HuffmanNode *>, CompareNode> pq;
  for (auto &p : freq)
  {
    pq.push(new HuffmanNode(p.first, p.second));
  }
  while (pq.size() > 1)
  {
    HuffmanNode *l = pq.top();
    pq.pop();
    HuffmanNode *r = pq.top();
    pq.pop();
    HuffmanNode *m = new HuffmanNode('\0', l->freq + r->freq);
    m->left = l;
    m->right = r;
    pq.push(m);
  }
  return pq.top();
}

// Comprime a string de dados usando os códigos gerados
String huffmanCompress(const String &data)
{
  String out;
  for (char c : data)
  {
    out += huffmanCodes[c];
  }
  return out;
}

// Descomprime a string de bits usando a árvore Huffman
String huffmanDecompress(const String &bits, HuffmanNode *root)
{
  String out;
  HuffmanNode *curr = root;
  for (char b : bits)
  {
    curr = (b == '0') ? curr->left : curr->right;
    if (!curr->left && !curr->right)
    {
      out += curr->ch;
      curr = root;
    }
  }
  return out;
}

// Libera a memória ocupada pela árvore Huffman
void freeHuffmanTree(HuffmanNode *node)
{
  if (!node)
    return;
  freeHuffmanTree(node->left);
  freeHuffmanTree(node->right);
  delete node;
}

// Função auxiliar para reconstruir uma árvore Huffman a partir de um dicionário serializado
HuffmanNode *buildHuffmanTreeFromCodes(std::map<char, String> &codes)
{
  HuffmanNode *root = new HuffmanNode('\0', 0);
  for (auto &entry : codes)
  {
    char key = entry.first;
    String code = entry.second;
    HuffmanNode *current = root;
    for (int i = 0; i < code.length(); i++)
    {
      char bit = code.charAt(i);
      if (bit == '0')
      {
        if (!current->left)
        {
          current->left = new HuffmanNode('\0', 0);
        }
        current = current->left;
      }
      else
      {
        if (!current->right)
        {
          current->right = new HuffmanNode('\0', 0);
        }
        current = current->right;
      }
    }
    current->ch = key;
  }
  return root;
}

// --------------------------------------------------------------------------------
// Definições de pinos e constantes do projeto
// --------------------------------------------------------------------------------

// Sensor DHT interno (container)
const int DHT_PIN_IN = 4;
const int DHT_TYPE = DHT11;
DHT dht_in(DHT_PIN_IN, DHT_TYPE);

// Sensor DHT externo (sala)
const int DHT_PIN_EXT = 18;
DHT dht_ext(DHT_PIN_EXT, DHT_TYPE);

// Sensor ultrassônico
const int TRIG_PIN = 5;
const int ECHO_PIN = 23;

// LEDs de status
const int LED_OK_PIN = 2;     // LED Verde (status normal)
const int LED_ALERT_PIN = 15; // LED Vermelho (alerta)

// Módulo PN532 (RFID)
const int SDA_PIN = 21;
const int SCL_PIN = 22;
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Configuração do LCD 20x4
const int LCD_RS = 26;
const int LCD_E = 27;
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
const unsigned long GREEN_BLINK_INTERVAL = 5000;
const unsigned long RED_BLINK_INTERVAL = 1000;
const unsigned long UNAUTH_WINDOW_BEFORE = 60000;
const unsigned long TEMP_ALERT_DURATION = 10000;

// Distância para considerar porta fechada
const float distancia_limite = 7.0;

// MQTT: tópicos dinâmicos
String centro = "centroDeVacinaXYZ";
String container = "containerXYZ";
String mqtt_topic = "/" + centro + "/" + container;

// Tópico de controle para comandos
const char *control_topic = "esp32/control";

// Wi‑Fi e MQTT
const char *ssid = "Starlink_CIT";
const char *password = "Ufrr@2024Cit";
const char *mqtt_server = "07356c1b41e34d65a6152a202151c24d.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char *mqtt_username = "hivemq.webclient.1744472320531";
const char *mqtt_password = "QOwsih0.7!tV%L6gD1B>";

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

// --------------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------------
void setup()
{
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
  if (version)
  {
    nfc.SAMConfig();
    Serial.println("[DEBUG] PN532 OK.");
  }
  else
  {
    Serial.println("[ERROR] PN532 não detectado!");
  }

  // LCD
  lcd.begin(20, 4);
  lcd.clear();
  lcd.print("Iniciando...");
  Serial.println("[DEBUG] LCD inicializado.");

  // Wi‑Fi
  setup_wifi();

  // MQTT
  espClient.setInsecure();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println("[DEBUG] MQTT configurado.");

  Serial.println("[DEBUG] Setup concluído!!!");
}

// --------------------------------------------------------------------------------
// LOOP
// --------------------------------------------------------------------------------
void loop()
{
  if (!client.connected())
  {
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
  if (now - lastPeriodicSend >= PERIODIC_SEND_INTERVAL)
  {
    lastPeriodicSend = now;
    Serial.println("[DEBUG] Envio periódico acionado.");
    enviarDados("periodic");
  }

  // LEDs de status
  if (!alarmState && now - lastGreenBlink >= GREEN_BLINK_INTERVAL)
  {
    lastGreenBlink = now;
    blinkLED(LED_OK_PIN, 100);
  }
  if (alarmState && now - lastRedBlink >= RED_BLINK_INTERVAL)
  {
    lastRedBlink = now;
    blinkLED(LED_ALERT_PIN, 100);
    Serial.println("[DEBUG] Blink vermelho.");
  }
}

// --------------------------------------------------------------------------------
// FUNÇÕES AUXILIARES
// --------------------------------------------------------------------------------

static void setup_wifi()
{
  Serial.print("[DEBUG] Conectando ao Wi‑Fi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[DEBUG] Wi‑Fi conectado: %s\n", WiFi.localIP().toString().c_str());
}

static void reconnect_mqtt()
{
  while (!client.connected())
  {
    Serial.println("[DEBUG] Tentando conexão MQTT...");
    if (client.connect("ESP32_Client", mqtt_username, mqtt_password))
    {
      Serial.println("[DEBUG] Conectado ao MQTT.");
      client.subscribe(control_topic);
      Serial.printf("[DEBUG] Inscrito em: %s\n", control_topic);
    }
    else
    {
      Serial.printf("[ERROR] Falha MQTT, rc=%d. Tentando em 5s\n", client.state());
      delay(5000);
    }
  }
}

static void blinkLED(uint8_t pin, int duration)
{
  digitalWrite(pin, HIGH);
  delay(duration);
  digitalWrite(pin, LOW);
}

static float medirDistancia()
{
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long dur = pulseIn(ECHO_PIN, HIGH);
  return (dur * 0.034) / 2.0;
}

void atualizarTempHistory()
{
  float t = dht_in.readTemperature();
  tempHistory[tempIndex] = t;
  tempIndex = (tempIndex + 1) % TEMP_HISTORY_SIZE;
  if (TEMP_HISTORY_SIZE >= 10)
  {
    float s5 = t, s10 = t;
    for (int i = 1; i < 5; i++)
      s5 += tempHistory[(tempIndex - i + TEMP_HISTORY_SIZE) % TEMP_HISTORY_SIZE];
    for (int i = 1; i < 10; i++)
      s10 += tempHistory[(tempIndex - i + TEMP_HISTORY_SIZE) % TEMP_HISTORY_SIZE];
    if (s5 / 5.0 > s10 / 10.0 && !avgAlertActive)
    {
      avgAlertActive = true;
      Serial.println("[DEBUG] Alerta média ativado.");
      enviarDados("avg_temp_alert");
    }
    else if (s5 / 5.0 <= s10 / 10.0 && avgAlertActive)
    {
      avgAlertActive = false;
      Serial.println("[DEBUG] Média estabilizada.");
      enviarDados("avg_temp_stable");
    }
  }
}

void atualizarLCD()
{
  float ti = dht_in.readTemperature();
  float te = dht_ext.readTemperature();
  float he = dht_ext.readHumidity();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("Int:%.1fC Ext:%.1fC", ti, te);
  lcd.setCursor(0, 1);
  lcd.printf("Porta:%s", doorOpen ? "Aberta" : "Fechada");
  lcd.setCursor(0, 2);
  lcd.printf("Umid Ext:%.1f%%", he);
  lcd.setCursor(0, 3);
  lcd.printf("Stat:%s", alarmState ? "ALERT" : "OK");
}

void verificarRFID()
{
  uint8_t success = nfc.inListPassiveTarget();
  if (success)
  {
    uint8_t uid[7], len;
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &len))
    {
      String u;
      for (int i = 0; i < len; i++)
      {
        u += String(uid[i], HEX);
      }
      if (u != usuarioAtual)
      {
        usuarioAtual = u;
        lastRFIDTime = millis();
        Serial.printf("[DEBUG] Novo RFID: %s\n", usuarioAtual.c_str());
        if (doorOpen)
        {
          Serial.println("[DEBUG] Porta aberta com RFID, enviando rfid_door.");
          enviarDados("rfid_door");
        }
      }
    }
  }
}

void atualizarDoorState()
{
  float dist = medirDistancia();
  bool nowOpen = dist > distancia_limite;
  if (nowOpen != doorOpen)
  {
    doorOpen = nowOpen;
    if (doorOpen)
    {
      Serial.println("[DEBUG] Porta abriu.");
      if (millis() - lastRFIDTime > UNAUTH_WINDOW_BEFORE)
      {
        Serial.println("[DEBUG] Porta não autorizada, enviando unauthorized_door.");
        alarmState = true;
        enviarDados("unauthorized_door");
      }
    }
    else
    {
      Serial.println("[DEBUG] Porta fechou.");
      alarmState = false;
    }
  }
}

void checarEventos()
{
  float t = dht_in.readTemperature();
  if (t > THRESHOLD_TEMP)
  {
    if (tempAboveStart == 0)
    {
      tempAboveStart = millis();
      Serial.println("[DEBUG] Temperatura acima do limiar, iniciando contador.");
    }
    else if (millis() - tempAboveStart >= TEMP_ALERT_DURATION)
    {
      Serial.println("[DEBUG] Temperatura alta por 10s, enviando temp_high.");
      alarmState = true;
      enviarDados("temp_high");
    }
  }
  else
  {
    tempAboveStart = 0;
  }
}

// Função que envia os dados pelo MQTT com payload compactado via Huffman e encriptado com AES
void enviarDados(String motivo)
{
  Serial.printf("[DEBUG] Montando payload para motivo: %s\n", motivo.c_str());
  float ti = dht_in.readTemperature();
  float hi = dht_in.readHumidity();
  float te = dht_ext.readTemperature();
  float he = dht_ext.readHumidity();
  String portState = (medirDistancia() <= distancia_limite) ? "Fechada" : "Aberta";

  String payload = "{";
  payload += "\"temperatura_interna\":" + String(ti, 2) + ",";
  payload += "\"umidade_interna\":" + String(hi, 2) + ",";
  payload += "\"temperatura_externa\":" + String(te, 2) + ",";
  payload += "\"umidade_externa\":" + String(he, 2) + ",";
  payload += "\"estado_porta\":\"" + portState + "\",";
  payload += "\"usuario\":\"" + usuarioAtual + "\",";
  payload += "\"alarm_state\":\"" + String(alarmState ? "ALERT" : "OK") + "\",";
  payload += "\"motivo\":\"" + motivo + "\"}";
  Serial.println("[DEBUG] Payload JSON: " + payload);

  // --- Construção dinâmica do dicionário Huffman ---
  if (huffmanTree)
  {
    freeHuffmanTree(huffmanTree);
    huffmanCodes.clear();
  }
  huffmanTree = buildHuffmanTree(payload);
  buildHuffmanCodes(huffmanTree);

  // Serializa o dicionário em um header (formato: cada par 'ch:code' separado por ;)
  String header = "";
  for (auto &p : huffmanCodes)
  {
    header += p.first;
    header += ":";
    header += p.second;
    header += ";";
  }
  if (header.endsWith(";"))
  {
    header.remove(header.length() - 1);
  }

  // Comprime o payload usando o dicionário dinâmico
  String compressed = huffmanCompress(payload);

  // Junta header e payload compactado usando "||" como delimitador
  String finalPayload = header + "||" + compressed;
  Serial.println("[DEBUG] Payload final (header||dados compactados): " + finalPayload);

  // Encripta o payload final com AES
  String encryptedPayload = aes_encrypt(finalPayload);
  Serial.println("[DEBUG] Payload encriptado: " + encryptedPayload);

  bool ok = client.publish(mqtt_topic.c_str(), encryptedPayload.c_str());
  Serial.printf("[DEBUG] Publish em %s %s\n", mqtt_topic.c_str(), ok ? "SUCESSO" : "FALHA");
}

// Callback para mensagens MQTT recebidas (desencripta e descompacta)
void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.printf("[DEBUG] Mensagem recebida em [%s], tamanho %u\n", topic, length);
  String msg;
  for (unsigned int i = 0; i < length; i++)
  {
    msg += (char)payload[i];
  }
  Serial.println("[DEBUG] Mensagem bruta (encriptada): " + msg);

  // Decripta a mensagem AES
  String decrypted = aes_decrypt(msg);
  Serial.println("[DEBUG] Mensagem decriptada: " + decrypted);

  // Se a mensagem contiver o delimitador "||", trata como mensagem compactada com header
  int delimiterIndex = decrypted.indexOf("||");
  if (delimiterIndex != -1)
  {
    String header = decrypted.substring(0, delimiterIndex);
    String compressed = decrypted.substring(delimiterIndex + 2);

    // Reconstrói o dicionário a partir do header
    std::map<char, String> dynamicCodes;
    int start = 0;
    while (start < header.length())
    {
      int colonIndex = header.indexOf(':', start);
      int semicolonIndex = header.indexOf(';', start);
      if (colonIndex == -1)
        break;
      if (semicolonIndex == -1)
        semicolonIndex = header.length();
      char key = header.charAt(start);
      String code = header.substring(colonIndex + 1, semicolonIndex);
      dynamicCodes[key] = code;
      start = semicolonIndex + 1;
    }
    // Constrói a árvore a partir do dicionário dinâmico
    HuffmanNode *dynamicTree = buildHuffmanTreeFromCodes(dynamicCodes);
    String decompressed = huffmanDecompress(compressed, dynamicTree);
    Serial.println("[DEBUG] Mensagem descompactada: " + decompressed);
    freeHuffmanTree(dynamicTree);

    // Caso a mensagem descompactada seja de controle, processa o comando
    if (decompressed.startsWith("change_topic"))
    {
      int a = decompressed.indexOf(';');
      int b = decompressed.indexOf(';', a + 1);
      centro = decompressed.substring(a + 1, b);
      container = decompressed.substring(b + 1);
      mqtt_topic = "/" + centro + "/" + container;
      Serial.println("[DEBUG] Tópico alterado para: " + mqtt_topic);
    }
  }
  else
  {
    Serial.println("[DEBUG] Mensagem sem compactação: " + decrypted);
  }
}
