# mqtt_client_hibrido.py
import os
import json
import ssl
import datetime
import paho.mqtt.client as mqtt  # type: ignore
from db import inserir_monitoramento
from huffman_decoder_dict_novo_hibrido import huffman_decompress
from whatsapp_service import send_whatsapp_message

# Variáveis de ambiente para MQTT e WhatsApp
MQTT_BROKER      = os.getenv('MQTT_BROKER')
MQTT_PORT        = int(os.getenv('MQTT_PORT', 8883))
MQTT_USER        = os.getenv('MQTT_USER')
MQTT_PASS        = os.getenv('MQTT_PASSWORD')
ALERT_RECIPIENT  = os.getenv("ALERT_RECIPIENT")
TOPIC            = '/+/+'

def on_connect(client, userdata, flags, rc):
    client.subscribe(TOPIC)
    print(f"Conectado ao MQTT em {MQTT_BROKER}, tópico {TOPIC}")

def on_message(client, userdata, msg):
    print(f"Mensagem recebida do tópico {msg.topic}: {msg.payload}")
    try:
        payload = json.loads(msg.payload.decode('utf-8'))
        
        # se compactado em Huffman, descomprime
        if 'huffman' in payload and 'dict' in payload:
            expected_length = payload.get('length')
            version = payload.get('dict')
            json_str = huffman_decompress(payload['huffman'], version, expected_length)
            data = json.loads(json_str)
        else:
            data = payload

        unit_id, container_id = msg.topic.strip('/').split('/')
        inserir_monitoramento(unit_id, container_id, **data)
        print(f"Inserido: {unit_id}/{container_id} → {data}")

        # Monta corpo da mensagem WhatsApp conforme motivo
        motivo         = data.get("motivo", "desconhecido")
        ti             = data.get("temperatura_interna", "-")
        te             = data.get("temperatura_externa", "-")
        estado_porta   = data.get("estado_porta", "-")
        usuario        = data.get("usuario", "desconhecido")
        agora          = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")

        if motivo == "unauthorized_door":
            body = (
                f"Aviso de acesso não autorizado.\n\n"
                f"- ID do usuário: desconhecido\n"
                f"- Estado da porta: {estado_porta}\n"
                f"- Horário do evento: {agora}"
            )

        elif motivo == "rfid_door":
            body = (
                f"Aviso de passagem via RFID.\n\n"
                f"- ID do usuário: {usuario}\n"
                f"- Estado da porta: {estado_porta}\n"
                f"- Temperatura interna: {ti}°C\n"
                f"- Temperatura externa: {te}°C"
                f"- Horário do evento: {agora}"
            )

        elif motivo == "temp_high":
            body = (
                f"Aviso de temperatura alta.\n\n"
                f"- Temperatura interna: {ti}°C\n"
                f"- Temperatura externa: {te}°C"
            )

        elif motivo == "avg_temp_alert":
            body = (
                f"Aviso de média de temperatura acima do limite.\n\n"
                f"- Temperatura interna: {ti}°C\n"
                f"- Temperatura externa: {te}°C"
            )

        elif motivo == "avg_temp_stable":
            body = (
                f"Aviso de média de temperatura estável.\n\n"
                f"- Temperatura interna: {ti}°C\n"
                f"- Temperatura externa: {te}°C"
            )

        elif motivo == "periodic":
            body = (
                f"Relatório periódico de condições.\n\n"
                f"- Temperatura interna: {ti}°C\n"
                f"- Temperatura externa: {te}°C\n"
                f"- Estado da porta: {estado_porta}"
            )

        else:
            body = (
                f"Aviso de {motivo}.\n\n"
                f"- Temperatura interna: {ti}°C\n"
                f"- Temperatura externa: {te}°C\n"
                f"- Estado da porta: {estado_porta}\n"
                f"- ID do usuário: {usuario}"
            )

        # Envia pelo WhatsApp Web
        if send_whatsapp_message(body, ALERT_RECIPIENT):
            print("WhatsApp enviado com sucesso")
        else:
            print("Falha ao enviar WhatsApp")

    except Exception as e:
        print("Erro no processamento MQTT:", e)

def iniciar_mqtt():
    client = mqtt.Client()
    client.tls_set(
        ca_certs="/etc/ssl/certs/ca-certificates.crt",
        cert_reqs=ssl.CERT_REQUIRED,
        tls_version=ssl.PROTOCOL_TLS_CLIENT
    )
    client.enable_logger()
    if MQTT_USER and MQTT_PASS:
        client.username_pw_set(MQTT_USER, MQTT_PASS)
        print("Autenticação MQTT configurada com sucesso")
    client.on_connect = on_connect
    client.on_message = on_message
    print(f"Conectando a {MQTT_BROKER}:{MQTT_PORT}...")
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
