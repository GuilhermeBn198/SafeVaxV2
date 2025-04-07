#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <time.h>
#include <Adafruit_PN532.h>
#include <LiquidCrystal.h>

// --------------------------------------------------------------------------------
// Definições de pinos e constantes
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

// Configuração do LCD 16x4 (utilizando interface paralela)
// Defina os pinos conforme disponíveis no ESP32:
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
float THRESHOLD_TEMP = 9.0;       // Temperatura limiar para alerta (exemplo)
unsigned long tempAboveStart = 0; // Tempo em que a temperatura ficou acima do limiar

// Intervalos de tempo (em milissegundos)
const unsigned long PERIODIC_SEND_INTERVAL = 60000;    // Envio periódico a cada 1 minuto
const unsigned long GREEN_BLINK_INTERVAL   = 5000;     // LED verde pisca a cada 5 segundos
const unsigned long RED_BLINK_INTERVAL     = 1000;     // LED vermelho pisca a cada 1 segundo
const unsigned long UNAUTH_WINDOW_BEFORE   = 60000;     // 1 minuto antes do evento
const unsigned long UNAUTH_WINDOW_AFTER    = 15000;     // 15 segundos depois
const unsigned long TEMP_ALERT_DURATION    = 10000;     // 10 segundos de temperatura alta

// Limite de distância para considerar a porta "fechada"
const float distancia_limite = 10.0;

// MQTT: Tópicos – usamos variáveis para facilitar alteração
String centro    = "centroDeVacinaXYZ";
String container = "containerXYZ";
String mqtt_topic = "/" + centro + "/" + container;  // Ex: /centroDeVacinaXYZ/containerXYZ

// Tópico de controle para receber comandos remotos
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
unsigned long lastRFIDTime = 0;  // Última leitura de RFID (milissegundos)

// Histórico de temperaturas (últimos 10 valores do sensor interno)
#define TEMP_HISTORY_SIZE 10
float tempHistory[TEMP_HISTORY_SIZE] = {0};
int tempIndex = 0;
bool avgAlertActive = false; // Indica se o alerta de média está ativo

// --------------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // Configura os LEDs
  pinMode(LED_OK_PIN, OUTPUT);
  pinMode(LED_ALERT_PIN, OUTPUT);

  // Configura os pinos do sensor ultrassônico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Inicializa os sensores DHT
  dht_in.begin();
  dht_ext.begin();

  // Inicializa o módulo PN532 (RFID)
  Serial.println("Inicializando PN532...");
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (versiondata) {
    nfc.SAMConfig();
    Serial.println("PN532 inicializado com sucesso.");
  } else {
    Serial.println("Módulo PN532 não detectado.");
  }

  // Inicializa o display LCD
  lcd.begin(16, 4);
  lcd.clear();
  lcd.print("Iniciando...");

  // Conexão Wi-Fi
  setup_wifi();

  // Configura o MQTT
  espClient.setInsecure(); // Desabilita verificação de certificado para MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// --------------------------------------------------------------------------------
// LOOP
// --------------------------------------------------------------------------------
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Atualiza o histórico de temperaturas e verifica a condição de média
  atualizarTempHistory();

  // Verifica o RFID
  verificarRFID();

  // Atualiza o estado da porta usando o sensor ultrassônico
  atualizarDoorState();

  // Checa condições de eventos (como alta temperatura)
  checarEventos();

  // Atualiza o display LCD com as informações atuais
  atualizarLCD();

  // Envia dados periodicamente a cada 1 minuto
  unsigned long now = millis();
  if (now - lastPeriodicSend >= PERIODIC_SEND_INTERVAL) {
    lastPeriodicSend = now;
    enviarDados("periodic");
  }

  // Pisca LED verde a cada 5 segundos se não houver alerta
  if (!alarmState && (now - lastGreenBlink >= GREEN_BLINK_INTERVAL)) {
    lastGreenBlink = now;
    blinkLED(LED_OK_PIN, 100);
  }

  // Se houver alerta, pisca LED vermelho a cada 1 segundo
  if (alarmState && (now - lastRedBlink >= RED_BLINK_INTERVAL)) {
    lastRedBlink = now;
    blinkLED(LED_ALERT_PIN, 100);
  }

  // Atualiza os LEDs conforme o estado global
  atualizarLEDs();
}

// --------------------------------------------------------------------------------
// FUNÇÕES AUXILIARES
// --------------------------------------------------------------------------------

void setup_wifi() {
  delay(10);
  Serial.println("Conectando ao Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.println("Reconectando ao HiveMQ...");
    if (client.connect("ESP32_Client", mqtt_username, mqtt_password)) {
      Serial.println("Conectado ao HiveMQ!");
      // Inscreve-se no tópico de controle
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

// Atualiza o histórico de temperaturas (sensor interno) e verifica a condição de média
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

// Verifica o RFID e atualiza o usuário e a última leitura
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
        // Se a porta estiver aberta, envia imediatamente
        if (doorOpen) {
          enviarDados("rfid_door");
        }
      }
    }
  }
}

// Atualiza o estado da porta usando o sensor ultrassônico
void atualizarDoorState() {
  float distancia = medirDistancia();
  bool newDoorOpen = (distancia > distancia_limite);
  if (newDoorOpen != doorOpen) {
    doorOpen = newDoorOpen;
    if (doorOpen) {
      doorOpenTime = millis();
      Serial.println("Porta aberta (ultrassônico).");
      // Se não houve identificação de RFID recentemente, envia alerta
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

// Checa condições de temperatura para disparar alerta
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

// Atualiza os LEDs conforme o estado do sistema
void atualizarLEDs() {
  if (alarmState) {
    digitalWrite(LED_ALERT_PIN, HIGH);
    digitalWrite(LED_OK_PIN, LOW);
  } else {
    digitalWrite(LED_ALERT_PIN, LOW);
    digitalWrite(LED_OK_PIN, HIGH);
  }
}

// Atualiza o display LCD com informações de temperatura interna, externa e estado da porta
void atualizarLCD() {
  // Lê temperaturas dos sensores
  float tempInterna = dht_in.readTemperature();
  float tempExterna = dht_ext.readTemperature();
  
  // Lê umidade externa (pode-se incluir umidade interna se desejado)
  float umidadeExterna = dht_ext.readHumidity();
  
  // Atualiza o LCD
  lcd.clear();
  // Linha 1: Exibe temperaturas
  lcd.setCursor(0, 0);
  lcd.print("Int:");
  lcd.print(tempInterna, 1);
  lcd.print("C Ext:");
  lcd.print(tempExterna, 1);
  lcd.print("C");
  // Linha 2: Exibe status da porta
  lcd.setCursor(0, 1);
  lcd.print("Porta:");
  lcd.print(doorOpen ? "Aberta" : "Fechada");
  // Linha 3: Exibe umidade externa
  lcd.setCursor(0, 2);
  lcd.print("Umid Ext:");
  lcd.print(umidadeExterna, 1);
  lcd.print("%");
  // Linha 4: Pode ser usada para status ou outro dado
  lcd.setCursor(0, 3);
  lcd.print("Status:");
  lcd.print(alarmState ? "ALERT" : "OK");
}

// Função para enviar dados via MQTT, incluindo o motivo do envio e novos dados externos
void enviarDados(String motivo) {
  // Leitura dos sensores internos
  float temperatura = dht_in.readTemperature();
  float umidade     = dht_in.readHumidity();
  
  // Leitura do sensor ultrassônico para estado da porta
  float distancia   = medirDistancia();
  String estado_porta = (distancia <= distancia_limite) ? "Fechada" : "Aberta";
  
  // Leitura dos sensores externos
  float temperatura_ext = dht_ext.readTemperature();
  float umidade_ext     = dht_ext.readHumidity();
  
  // Monta o payload JSON com os dados
  String mensagem = "{";
  mensagem += "\"temperatura_interna\": " + String(temperatura, 2) + ",";
  mensagem += "\"umidade_interna\": " + String(umidade, 2) + ",";
  mensagem += "\"temperatura_externa\": " + String(temperatura_ext, 2) + ",";
  mensagem += "\"umidade_externa\": " + String(umidade_ext, 2) + ",";
  mensagem += "\"estado_porta\": \"" + estado_porta + "\",";
  mensagem += "\"usuario\": \"" + usuarioAtual + "\",";
  mensagem += "\"alarm_state\": \"" + String(alarmState ? "ALERT" : "OK") + "\",";
  mensagem += "\"motivo\": \"" + motivo + "\"";
  mensagem += "}";
  
  if (client.publish(mqtt_topic.c_str(), mensagem.c_str())) {
    Serial.println("Dados enviados via MQTT: " + mensagem);
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

// Callback para mensagens MQTT recebidas
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Mensagem recebida em [" + String(topic) + "]: " + message);
  
  // Processa comandos do tópico de controle, ex.: alteração do tópico MQTT
  if (String(topic) == control_topic) {
    // Formato esperado: "change_topic;novoCentro;novoContainer"
    if (message.indexOf("change_topic") >= 0) {
      int firstSep = message.indexOf(";");
      int secondSep = message.indexOf(";", firstSep + 1);
      if (firstSep > 0 && secondSep > firstSep) {
        String novoCentro = message.substring(firstSep + 1, secondSep);
        String novoContainer = message.substring(secondSep + 1);
        centro = novoCentro;
        container = novoContainer;
        mqtt_topic = "/" + centro + "/" + container;
        Serial.println("Novo MQTT topic: " + mqtt_topic);
      }
    }
  }
}
