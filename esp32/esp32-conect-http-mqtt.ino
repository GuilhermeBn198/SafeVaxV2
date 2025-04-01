#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <time.h>
#include <Adafruit_PN532.h>
#include <HTTPClient.h>

// --------------------------------------------------------------------------------
// Definições de pinos e constantes
// --------------------------------------------------------------------------------

// Substitua pelo endereço do seu servidor Flask
const char* serverName = "http://localhost:5000/api/data";

// Configurações do sensor DHT (use DHT11 ou DHT22 conforme seu hardware)
#define DHT_PIN     4
#define DHT_TYPE    DHT11  // Ou DHT22, se for o caso
DHT dht(DHT_PIN, DHT_TYPE);

// Pinos do sensor ultrassônico (se estiver sendo utilizado)
#define TRIG_PIN 5
#define ECHO_PIN 25

// LEDs de status
#define LED_OK_PIN    2   // LED Verde
#define LED_ALERT_PIN 15  // LED Vermelho (exemplo)

// Pinos do Módulo PN532 (RFID)
#define SDA_PIN 21
#define SCL_PIN 22
Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);

// Pinos do motor de passo (exemplo básico)
#define STEP_PIN 18
#define DIR_PIN  19

// Limites de temperatura para detecção de alerta
#define TEMP_MIN 2.0
#define TEMP_MAX 8.0

// Limite de distância para considerar a porta "fechada"
const float distancia_limite = 10.0;

// Intervalo de envio de dados MQTT (em ms)
const int intervalo = 1000; // 1 segundo

// Configurações da rede Wi-Fi
const char* ssid     = "Starlink_CIT";
const char* password = "Ufrr@2024Cit";

// Configurações do HiveMQ (broker MQTT)
const char* mqtt_server   = "cd8839ea5ec5423da3aaa6691e5183a5.s1.eu.hivemq.cloud";
const int   mqtt_port     = 8883;
const char* mqtt_topic    = "esp32/refrigerator";
const char* mqtt_username = "hivemq.webclient.1734636778463";
const char* mqtt_password = "EU<pO3F7x?S%wLk4#5ib";

// Objetos de rede
WiFiClientSecure espClient;
PubSubClient client(espClient);

// Variáveis globais
unsigned long lastMsg     = 0;
String usuarioAtual       = "desconhecido";
bool alarmState           = false;   // Indica se há algum tipo de alerta
bool motorLocked          = true;    // true = travado, false = destravado
bool doorOpen             = false;   // Indica se a porta está aberta ou fechada

// --------------------------------------------------------------------------------
// SETUP
// --------------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  // Pinos de LED
  pinMode(LED_OK_PIN, OUTPUT);
  pinMode(LED_ALERT_PIN, OUTPUT);

  // Motor de passo
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN,  OUTPUT);

  // Sensor ultrassônico
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Inicializa DHT
  dht.begin();

  // Inicializa PN532 (RFID)
  Serial.println("Inicializando PN532...");
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (versiondata) {
    nfc.SAMConfig();
    Serial.println("PN532 inicializado com sucesso.");
  } else {
    Serial.println("Módulo PN532 não detectado.");
  }

  // Conexão Wi-Fi
  setup_wifi();

  // Configura MQTT
  espClient.setInsecure();           // Desabilita verificação de certificado para MQTT
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

// --------------------------------------------------------------------------------
// LOOP
// --------------------------------------------------------------------------------
void loop() {
  // Mantém conexão MQTT
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Verifica RFID continuamente
  verificarRFID();

  // Checa se há condição de alarme (porta aberta, temperatura fora do normal, etc.)
  checarAlarme();

  // Envia dados periodicamente
  unsigned long now = millis();
  if (now - lastMsg >= intervalo) {
    lastMsg = now;
    enviarDados();
  }

  // Atualiza LEDs conforme o estado do sistema
  atualizarLEDs();
}

// --------------------------------------------------------------------------------
// FUNÇÕES PRINCIPAIS
// --------------------------------------------------------------------------------

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
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Reconexão ao MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.println("Reconectando ao HiveMQ...");
    if (client.connect("ESP32_Client", mqtt_username, mqtt_password)) {
      Serial.println("Conectado ao HiveMQ!");
      // Se precisar se inscrever em algum tópico, faça aqui: client.subscribe("topico_resposta");
    } else {
      delay(5000);
    }
  }
}

// Verifica cartão RFID
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
      // Se for um usuário diferente, atualiza
      if (novoUsuario != usuarioAtual) {
        usuarioAtual = novoUsuario;
        Serial.println("Novo usuário identificado: " + usuarioAtual);

        // Exemplo de lógica simples de "autorização" para desbloquear:
        // Se o UID for algum valor conhecido, desbloqueia motor.
        // Aqui vamos desbloquear para qualquer novo usuário (apenas exemplo).
        desbloquearMotor();
      }
    }
  }
}

// Checa as condições para acionar ou não o alarme
void checarAlarme() {
  float temperatura = dht.readTemperature();
  float umidade     = dht.readHumidity();

  // Lê a distância para saber se a porta está aberta ou fechada
  float distancia = medirDistancia();
  doorOpen = (distancia > distancia_limite);

  // Condições de alerta
  bool tempForaFaixa = (temperatura < TEMP_MIN || temperatura > TEMP_MAX);

  if (tempForaFaixa || doorOpen) {
    alarmState = true;
  } else {
    alarmState = false;
  }
}

// Mede distância com sensor ultrassônico
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duracao = pulseIn(ECHO_PIN, HIGH);
  float distancia = (duracao * 0.034) / 2.0; // cm
  return distancia;
}

// Atualiza os LEDs com base no estado do sistema
void atualizarLEDs() {
  if (alarmState) {
    // Se tem alarme, LED vermelho aceso, LED verde apagado
    digitalWrite(LED_ALERT_PIN, HIGH);
    digitalWrite(LED_OK_PIN, LOW);
  } else {
    // Se não tem alarme, LED verde aceso, LED vermelho apagado
    digitalWrite(LED_ALERT_PIN, LOW);
    digitalWrite(LED_OK_PIN, HIGH);
  }
}

// Função para enviar os dados via HTTP POST
void enviarDadosHTTP(String mensagem) {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin(serverName);    // Inicia conexão com o servidor
    http.addHeader("Content-Type", "application/json");  // Define o cabeçalho da requisição

    int httpResponseCode = http.POST(mensagem);  // Envia o POST com a mensagem JSON
    
    if(httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("HTTP Response code: " + String(httpResponseCode));
      Serial.println("Response: " + response);
    } else {
      Serial.println("Erro ao enviar POST: " + String(httpResponseCode));
    }
    http.end();  // Fecha a conexão
  } else {
    Serial.println("WiFi desconectado");
  }
}

// Exemplo de chamada dessa função no loop ou após montar o payload JSON:
void enviarDados() {
  float temperatura = dht.readTemperature();
  float umidade     = dht.readHumidity();
  float distancia   = medirDistancia();
  String estado_porta = (distancia <= distancia_limite) ? "Fechada" : "Aberta";

  // Monta a mensagem JSON
  String mensagem = "{";
  mensagem += "\"temperatura\": " + String(temperatura) + ",";
  mensagem += "\"umidade\": " + String(umidade) + ",";
  mensagem += "\"estado_porta\": \"" + estado_porta + "\",";
  mensagem += "\"usuario\": \"" + usuarioAtual + "\",";
  mensagem += "\"motor_state\": \"" + String(motorLocked ? "locked" : "unlocked") + "\",";
  mensagem += "\"alarm_state\": \"" + String(alarmState ? "ALERT" : "OK") + "\"";
  mensagem += "}";

  // Envia via MQTT (já existente) e também via HTTP
  if (client.publish(mqtt_topic, mensagem.c_str())) {
    Serial.println("Dados enviados via MQTT: " + mensagem);
  } else {
    Serial.println("Falha no envio via MQTT.");
  }
  
  // Envia os mesmos dados via HTTP POST para o servidor Flask
  enviarDadosHTTP(mensagem);
} 

// Callback para mensagens recebidas do MQTT
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensagem recebida em [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  // Exemplo: se quiser interpretar comandos de bloqueio/desbloqueio
  // via MQTT, você pode fazer aqui. Ex:
  // if (strstr((char*)payload, "unlock")) { desbloquearMotor(); }
  // if (strstr((char*)payload, "lock"))   { bloquearMotor(); }
}

// --------------------------------------------------------------------------------
// FUNÇÕES DE CONTROLE DO MOTOR DE PASSO (FERROLHO)
// --------------------------------------------------------------------------------

// Exemplo simples de movimento do motor para desbloquear
void desbloquearMotor() {
  if (!motorLocked) return; // Se já está destravado, não faz nada
  Serial.println("Desbloqueando motor...");
  digitalWrite(DIR_PIN, HIGH); // Define direção de rotação

  // Exemplo: 200 passos para destravar
  for (int i = 0; i < 200; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(500);
  }
  motorLocked = false;
}

// Exemplo simples de movimento do motor para bloquear
void bloquearMotor() {
  if (motorLocked) return; // Se já está travado, não faz nada
  Serial.println("Bloqueando motor...");
  digitalWrite(DIR_PIN, LOW); // Define direção oposta

  // Exemplo: 200 passos para travar
  for (int i = 0; i < 200; i++) {
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(500);
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(500);
  }
  motorLocked = true;
}
