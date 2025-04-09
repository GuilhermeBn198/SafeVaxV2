void setup_wifi() {
    Serial.print("[DEBUG] Conectando ao Wi-Fi ");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.printf("\n[DEBUG] Wi-Fi conectado: %s\n", WiFi.localIP().toString().c_str());
  }
  
  void reconnect() {
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

  void blinkLED(uint8_t pin, int duration) {
    digitalWrite(pin, HIGH);
    delay(duration);
    digitalWrite(pin, LOW);
  }

  void atualizarLEDs() {
    digitalWrite(LED_ALERT_PIN, alarmState);
    digitalWrite(LED_OK_PIN, !alarmState);
  }
  
  float medirDistancia() {
    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    long dur = pulseIn(ECHO_PIN, HIGH);
    return (dur * 0.034) / 2.0;
  }