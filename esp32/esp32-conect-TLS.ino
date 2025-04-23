
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

// --- Huffman Estático ---
const String HUFFMAN_DICT_VERSION = "v2";
const std::map<char, String> huffmanCodes = {
  {' ', String("110100")},
  {'!', String("1111111101")},
  {'"', String("000")},
  {'#', String("111111101")},
  {'$', String("11111111001")},
  {'%', String("111111011")},
  {'&', String("111111010")},
  {'(', String("111111111")},
  {')', String("1111111000")},
  {'*', String("111111110")},
  {'+', String("11111110")},
  {',', String("0011")},
  {'-', String("11111101")},
  {'.', String("01101")},
  {'/', String("11111111")},
  {'0', String("00011")},
  {'1', String("00100")},
  {'2', String("00101")},
  {'3', String("00110")},
  {'4', String("00111")},
  {'5', String("01000")},
  {'6', String("01001")},
  {'7', String("01010")},
  {'8', String("01011")},
  {'9', String("01100")},
  {':', String("0010")},
  {';', String("1111111011")},
  {'<', String("1111111001")},
  {'=', String("111111001")},
  {'>', String("1111111010")},
  {'?', String("1111111100")},
  {'@', String("111111100")},
  {'A', String("11111")},
  {'B', String("111100")},
  {'C', String("11101")},
  {'D', String("00010")},
  {'E', String("01110")},
  {'F', String("11110")},
  {'G', String("111101")},
  {'H', String("111110")},
  {'I', String("11001")},
  {'J', String("111111")},
  {'K', String("11000")},
  {'L', String("00000")},
  {'M', String("110011")},
  {'N', String("1111000")},
  {'O', String("10111")},
  {'P', String("1111001")},
  {'Q', String("1111010")},
  {'R', String("00001")},
  {'S', String("1111011")},
  {'T', String("110001")},
  {'U', String("110010")},
  {'V', String("1111100")},
  {'W', String("1111101")},
  {'X', String("1111110")},
  {'Y', String("1111111")},
  {'Z', String("11111000")},
  {'[', String("11111011")},
  {'\\', String("111111000")},
  {']', String("11111100")},
  {'^', String("11111111000")},
  {'_', String("11100")},
  {'a', String("0100")},
  {'b', String("10101")},
  {'c', String("01111")},
  {'d', String("1100")},
  {'e', String("0101")},
  {'f', String("110101")},
  {'g', String("10011")},
  {'h', String("10001")},
  {'i', String("0110")},
  {'j', String("110110")},
  {'k', String("10110")},
  {'l', String("10100")},
  {'m', String("1010")},
  {'n', String("10000")},
  {'o', String("0111")},
  {'p', String("11010")},
  {'q', String("110111")},
  {'r', String("1000")},
  {'s', String("11011")},
  {'t', String("1001")},
  {'u', String("1011")},
  {'v', String("10010")},
  {'w', String("111000")},
  {'x', String("111001")},
  {'y', String("111010")},
  {'z', String("111011")},
  {'{', String("11111001")},
  {'|', String("1111111110")},
  {'}', String("11111010")},
  {'~', String("1111111111")},
  {'$', String("11111111001")}
};

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
// FUNÇÕES HUFFMAN
// --------------------------------------------------------------------------------
String huffmanCompress(const String &data) {
  String compressed;
  for (char c : data) {
    auto it = huffmanCodes.find(c);
    if (it != huffmanCodes.end()) {
      compressed += it->second;
    } else {
      compressed += "110101"; // Caractere desconhecido (espaço como fallback)
    }
  }
  return compressed;
}

String binaryToBase64(const String &binary) {
  const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  String result;
  
  // Cria uma cópia modificável do input
  String paddedBinary = binary;
  Serial.printf("\n[DEBUG] TAMANHO DO CODIGO BINARIO: %s\n", String(paddedBinary.length()).c_str());
  int len = paddedBinary.length();
  
  // Preencher com zeros se necessário
  while (len % 6 != 0) {
    paddedBinary += '0';
    len++;
  }

  int i = 0;
  while (i < len) {
    int val = 0;
    for (int j = 0; j < 6; j++) {
      val <<= 1;
      if (paddedBinary[i+j] == '1') val |= 1;
    }
    i += 6;
    
    result += base64_chars[val];
  }

  // // Ajustar padding ANTIGO
  // int pad = (6 - (binary.length() % 6)) % 6;
  // for (int p = 0; p < pad/2; p++) {
  //   result += '=';
  // }

  // Ajustar padding NOVO
  while (result.length() % 4 != 0) {
    result += '=';
  }
  // Serial.printf("\n[DEBUG] O PAD DO CODIGO EM BASE64: %s\n", String(pad).c_str());
  Serial.printf("\n[DEBUG] TAMANHO DO CODIGO EM BASE64 %s\n", String(result.length()).c_str());
  Serial.printf("\n[DEBUG] CODIGO EM BASE64 %s\n", result.c_str());
  return result;
}


void testHuffman() {
  String test = "{\"temperatura_interna\":25.50}";
  String compressed = huffmanCompress(test);
  String encoded = binaryToBase64(compressed);
  
  Serial.println("\n[TESTE] Original: " + test);
  Serial.println("[TESTE] Compactado: " + compressed);
  Serial.println("[TESTE] Base64: " + encoded);
  Serial.println("[TESTE] Tamanho original: " + String(test.length()) + " bytes");
  Serial.println("[TESTE] Tamanho compactado: " + String(encoded.length()) + " bytes\n");
}

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
  if (version) { 
    nfc.SAMConfig(); 
    Serial.println("[DEBUG] PN532 OK."); 
  } else { 
    Serial.println("[ERROR] PN532 não detectado!"); 
  }

  lcd.begin(20, 4);
  lcd.clear();
  lcd.print("Iniciando...");
  Serial.println("[DEBUG] LCD inicializado.");

  setup_wifi();

  espClient.setCACert(hivemq_root_ca);
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  Serial.println("[DEBUG] MQTT TLS configurado.");

  testHuffman();
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
    lastGreenBlink = now; 
    blinkLED(LED_OK_PIN, 100);
  }
  if (alarmState && now - lastRedBlink >= RED_BLINK_INTERVAL) {
    lastRedBlink = now; 
    blinkLED(LED_ALERT_PIN, 100);
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
    // Serial.println("[DEBUG] Tentando conexão MQTT...");
    if (client.connect("ESP32_Client", mqtt_username, mqtt_password)) {
      // Serial.println("[DEBUG] Conectado ao MQTT.");
      client.subscribe(control_topic);
      // Serial.printf("[DEBUG] Inscrito em: %s\n", control_topic);
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

void enviarDados(String motivo) {
  Serial.printf("[DEBUG] Montando payload para motivo: %s\n", motivo.c_str());
  
  // Ler dados dos sensores
  float ti = dht_in.readTemperature();
  float hi = dht_in.readHumidity();
  float te = dht_ext.readTemperature();
  float he = dht_ext.readHumidity();
  String portState = (medirDistancia() <= distancia_limite) ? "Fechada" : "Aberta";

  // Construir payload JSON original
  String payload = "{";
  payload += "\"temperatura_interna\":" + String(ti, 2) + ",";
  payload += "\"umidade_interna\":"   + String(hi, 2) + ",";
  payload += "\"temperatura_externa\":" + String(te, 2) + ",";
  payload += "\"umidade_externa\":"   + String(he, 2) + ",";
  payload += "\"estado_porta\":\""     + portState + "\",";
  payload += "\"usuario\":\""          + usuarioAtual + "\",";
  payload += "\"alarm_state\":\""      + String(alarmState ? "ALERT" : "OK") + "\",";
  payload += "\"motivo\":\""           + motivo + "\"}";

  // Compactar e codificar
  String compressed = huffmanCompress(payload);
  String encoded = binaryToBase64(compressed);
  
  // Construir payload final
  String finalPayload = "{";
  finalPayload += "\"huffman\":\"" + encoded + "\",";
  finalPayload += "\"dict\":\"" + HUFFMAN_DICT_VERSION + "\",";
  finalPayload += "\"length\":" + String(compressed.length()) + "}";

  // Publicar
  bool ok = client.publish(mqtt_topic.c_str(), finalPayload.c_str());
  
  // Logs detalhados
  Serial.println("Payload original: " + payload);
  Serial.println("Tamanho original: " + String(payload.length()) + " bytes");
  Serial.println("Payload compactado: " + finalPayload);
  Serial.println("Tamanho compactado: " + String(finalPayload.length()) + " bytes");
  Serial.printf("[DEBUG] Publicação %s\n", ok ? "OK" : "FALHA");
}

// Callback modificado para lidar com possíveis comandos
void callback(char* topic, byte* payload, unsigned int length) {
  String msg;
  for (unsigned int i = 0; i < length; i++) {
    msg += (char)payload[i];
  }
  
  Serial.println("[DEBUG] Mensagem recebida: " + msg);
  
  // Comando para atualizar threshold
  if (msg.startsWith("SET_THRESHOLD:")) {
    float newThreshold = msg.substring(14).toFloat();
    THRESHOLD_TEMP = newThreshold;
    Serial.println("[DEBUG] Novo threshold definido: " + String(THRESHOLD_TEMP));
  }
}