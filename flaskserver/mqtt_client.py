import os
import json
import paho.mqtt.client as mqtt
from db import inserir_monitoramento

MQTT_BROKER = os.getenv('MQTT_BROKER', 'localhost')
MQTT_PORT = int(os.getenv('MQTT_PORT', 1883))
TOPIC = '/+/+'  # Assina tópicos no formato /unit_id/container_id


def on_connect(client, userdata, flags, rc):
    client.subscribe(TOPIC)
    print(f"Conectado ao MQTT, subscribe em {TOPIC}")


def on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload)
        topic = msg.topic.strip('/').split('/')
        unit_id, container_id = topic
        inserir_monitoramento(unit_id, container_id, **payload)
        print(f"Dados inseridos de {unit_id}/{container_id}")
    except Exception as e:
        print("Erro no processamento MQTT:", e)


def iniciar_mqtt():
    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
