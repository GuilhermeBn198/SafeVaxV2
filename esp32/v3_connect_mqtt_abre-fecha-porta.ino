#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>

// Configurações do sensor DHT11
#define DHT_PIN 4          // GPIO onde o sensor DHT11 está conectado
#define DHT_TYPE DHT11     // Define o tipo do sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Configurações do sensor ultrassônico HC-SR04
#define TRIG_PIN 5  // GPIO do pino TRIG
#define ECHO_PIN 18 // GPIO do pino ECHO

// Configuração do LED indicador
#define LED_PIN 2 // GPIO do pino do LED

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

// Variáveis auxiliares
unsigned long lastMsg = 0;
const int intervalo = 5000;
const float distancia_limite = 10.0; // Distância limite em cm para a porta
unsigned long portaAbertaDesde = 0;  // Momento em que a porta foi detectada aberta
bool ledEstado = false;              // Estado atual do LED

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT); // Configura o LED como saída

  // Conecta à rede Wi-Fi
  setup_wifi();

  // Configura o cliente MQTT
  espClient.setInsecure(); // Permite conexões TLS sem validar certificados
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback); // Adiciona callback para mensagens recebidas
  
  dht.begin();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Envia dados a cada intervalo definido
  unsigned long now = millis();
  if (now - lastMsg > intervalo) {
    lastMsg = now;

    // Envia dados do sensor
    enviarDados();
  }

  // Atualiza o estado do LED
  gerenciarLED();
}

// Função para conectar ao Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println("Conectando à rede Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi conectado!");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Função para reconectar ao MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.println("Tentando reconectar ao HiveMQ...");
    if (client.connect("ESP32_Client", mqtt_username, mqtt_password)) {
      Serial.println("Conectado ao HiveMQ!");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(". Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}

// Callback para processar mensagens recebidas
void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Mensagem recebida em [");
  Serial.print(topic);
  Serial.print("]: ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
  }
  Serial.println();
}

// Função para enviar dados de temperatura, umidade e estado da porta
void enviarDados() {
  // Leitura do DHT11
  float temperatura = dht.readTemperature();
  float umidade = dht.readHumidity();

  // Verifica leituras válidas
  if (isnan(temperatura) || isnan(umidade)) {
    Serial.println("Erro ao ler o sensor DHT11.");
    return;
  }

  // Verifica o estado da porta
  float distancia = medirDistancia();
  String estado_porta = (distancia <= distancia_limite) ? "Fechada" : "Aberta";

  if (estado_porta == "Aberta") {
    if (portaAbertaDesde == 0) {
      portaAbertaDesde = millis();
    }
  } else {
    portaAbertaDesde = 0;
    digitalWrite(LED_PIN, LOW); // Desliga o LED quando a porta é fechada
  }

  // Monta a mensagem JSON
  String mensagem = "{";
  mensagem += "\"temperatura\": " + String(temperatura) + ", ";
  mensagem += "\"umidade\": " + String(umidade) + ", ";
  mensagem += "\"estado_porta\": \"" + estado_porta + "\"}";

  // Envia ao HiveMQ
  if (client.publish(mqtt_topic, mensagem.c_str())) {
    Serial.println("Dados enviados: " + mensagem);
  } else {
    Serial.println("Falha ao enviar dados.");
  }
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

// Função para medir distância com HC-SR04
float medirDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  long duracao = pulseIn(ECHO_PIN, HIGH);
  return (duracao * 0.034) / 2.0;
}
