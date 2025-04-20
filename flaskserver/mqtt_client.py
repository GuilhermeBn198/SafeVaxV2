# mqtt_client.py
import os
import json
import paho.mqtt.client as mqtt
from db import inserir_monitoramento
# Importando o decodificador Huffman final
from huffman_decoder import huffman_decompress

# Variáveis de ambiente para MQTT
MQTT_BROKER = os.getenv('MQTT_BROKER')
MQTT_PORT = int(os.getenv('MQTT_PORT', 8883))
MQTT_USER = os.getenv('MQTT_USER')
MQTT_PASS = os.getenv('MQTT_PASSWORD')
TOPIC = '/+/+'

def on_connect(client, userdata, flags, rc):
    client.subscribe(TOPIC)
    print(f"Conectado ao MQTT em {MQTT_BROKER}, tópico {TOPIC}")


def on_message(client, userdata, msg):
    print(f"Mensagem recebida do tópico {msg.topic}: {msg.payload}")
    try:
        payload_raw = msg.payload.decode('utf-8')
        print(f"Payload decodificado: '{payload_raw}'")
        payload = json.loads(payload_raw)
        
        # Verifica se a mensagem contém dados compactados com Huffman
        if 'huffman' in payload and 'dict' in payload:
            # Usa o decodificador Huffman final com o comprimento esperado, se disponível
            expected_length = payload.get('length')
            json_str = huffman_decompress(payload['huffman'], payload['dict'], expected_length)
            print(f"JSON descompactado: '{json_str}'")
            data = json.loads(json_str)
        else:
            data = payload
            
        # Extrai unit_id e container_id do tópico
        unit_id, container_id = msg.topic.strip('/').split('/')
        
        # Insere os dados no banco de dados
        inserir_monitoramento(unit_id, container_id, **data)
        print(f"Inserido: {unit_id}/{container_id} → {data}")
    except Exception as e:
        print("Erro no processamento MQTT:", e)


def iniciar_mqtt():
    client = mqtt.Client()
    client.enable_logger()
    # Configura autenticação
    if MQTT_USER and MQTT_PASS:
        client.username_pw_set(MQTT_USER, MQTT_PASS)
        print("Autenticação MQTT configurada com sucesso")
        
    client.on_connect = on_connect
    client.on_message = on_message
    client.tls_set()  # Usa TLS com as configurações padrão
    print(f"Conectando a {MQTT_BROKER}:{MQTT_PORT}...")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
