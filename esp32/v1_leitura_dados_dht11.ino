#include <DHT.h>

// Definições do sensor
#define DHT_PIN 4       // Define o pino do ESP32 onde o DHT11 está conectado
#define DHT_TYPE DHT11  // Define o tipo do sensor (DHT11)

// Inicializa o sensor DHT11 com escopo global
DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  // Inicia a comunicação entre o ESP-32 e o computador com uma taxa de 115200 bits por segundo
  Serial.begin(115200);
  dht.begin(); // Inicializa o sensor DHT11
  Serial.println("Sensor DHT11 funcionando!");
}

void loop() {
  // Aguarda um intervalo de 2 segundos entre leituras
  delay(2000);

  // Lê a umidade e a temperatura
  float umidade = dht.readHumidity();
  float temperatura = dht.readTemperature();

  // Verifica se houve erro na leitura
  if (isnan(umidade) || isnan(temperatura)) {
    Serial.println("Erro ao ler o sensor DHTT11!");
    return;
  }

  // Exibe os dados da leitura no Serial Monitor
  Serial.println("Umidade: " + String(umidade) + "%, Temperatura: " + String(temperatura) + "°C");
}
