#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

// Configurações do sensor DHT11
#define DHT_PIN 4          // GPIO onde o sensor DHT11 está conectado
#define DHT_TYPE DHT11     // Define o tipo do sensor
DHT dht(DHT_PIN, DHT_TYPE);

// Configurações da rede Wi-Fi
const char* ssid = "CIT_Alunos";          // Nome da rede Wi-Fi
const char* password = "alunos@2024";     // Senha da rede Wi-Fi

// Configurações do HiveMQ (broker MQTT)
const char* mqtt_server = "cd8839ea5ec5423da3aaa6691e5183a5.s1.eu.hivemq.cloud";
const int mqtt_port = 1883;
const char* mqtt_topic = "esp32/dht11/temperatura";

// Inicialização do cliente Wi-Fi e MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Variáveis auxiliares
unsigned long lastMsg = 0;  // Timestamp da última mensagem enviada
const int intervalo = 5000; // Intervalo de envio (em milissegundos)

void setup() {
  Serial.begin(115200);
  setup_wifi();        // Conecta-se à rede Wi-Fi
  client.setServer(mqtt_server, mqtt_port); // Configura o broker MQTT
  dht.begin();         // Inicializa o sensor DHT11
}

void loop() {
  if (!client.connected()) {
    reconnect(); // Reconecta ao broker MQTT, caso desconectado
  }
  client.loop(); // Mantém a conexão MQTT ativa

  // Envia a temperatura a cada "intervalo" definido
  unsigned long now = millis();
  if (now - lastMsg > intervalo) {
    lastMsg = now;

    // Lê temperatura e umidade
    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();

    // Verifica se a leitura foi bem-sucedida
    if (isnan(temperatura) || isnan(umidade)) {
      Serial.println("Erro ao ler o sensor DHT11!");
      return;
    }

    // Exibe os valores lidos
    Serial.print("Temperatura: ");
    Serial.print(temperatura);
    Serial.println("°C");

    Serial.print("Umidade: ");
    Serial.print(umidade);
    Serial.println("%");

    // Monta a mensagem para o HiveMQ
    String mensagem = "Temperatura: " + String(temperatura) + "°C, Umidade: " + String(umidade) + "%";

    // Envia ao HiveMQ
    if (client.publish(mqtt_topic, mensagem.c_str())) {
      Serial.println("Mensagem enviada ao HiveMQ com sucesso!");
    } else {
      Serial.println("Falha ao enviar a mensagem ao HiveMQ.");
    }
  }
}

// Conecta-se à rede Wi-Fi
void setup_wifi() {
  delay(10);
  Serial.println("Conectando à rede Wi-Fi...");
  Serial.println(ssid);

  WiFi.begin(ssid, password); // Inicia a conexão

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Wi-Fi conectado!");
  Serial.println("Endereço IP: ");
  Serial.println(WiFi.localIP());
}

// Reconecta ao HiveMQ caso a conexão MQTT seja perdida
void reconnect() {
  while (!client.connected()) {
    Serial.println("Tentando reconectar ao HiveMQ...");
    if (client.connect("ESP32_Client")) {
      Serial.println("Conectado ao HiveMQ!");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(client.state());
      Serial.println(" Tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}
