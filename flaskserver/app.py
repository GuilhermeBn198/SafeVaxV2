import os, hmac, hashlib
from flask import Flask, request,abort,  jsonify # type: ignore
from db import (
    listar_unidades, listar_containers,
    buscar_monitoramento, listar_todos_dados
)
from mqtt_client_hibrido import iniciar_mqtt

VERIFY_TOKEN    = os.getenv("WHATSAPP_VERIFY_TOKEN")
APP_SECRET      = os.getenv("WHATSAPP_APP_SECRET")

app = Flask(__name__)
print("Aplicação Flask iniciada")
iniciar_mqtt()
print("MQTT ativo")


@app.route("/webhook", methods=["GET"])
def verify_webhook():
    mode      = request.args.get("hub.mode")
    token     = request.args.get("hub.verify_token")
    challenge = request.args.get("hub.challenge")

    # Deve validar exatamente “subscribe” e token igual ao configurado
    if mode == "subscribe" and token == VERIFY_TOKEN:
        return challenge, 200
    else:
        return "Verification failed", 403

@app.route("/webhook", methods=["POST"])
def receive_webhook():
    # 1) valida assinatura
    # verify_signature(request)

    # 2) parse JSON
    data = request.get_json()
    # formato genérico: object + entry[]
    if data.get("object") != "whatsapp_business_account":
        return "ignored", 200

    # 3) iterar sobre cada change
    for entry in data.get("entry", []):
        for change in entry.get("changes", []):
            field = change.get("field")
            value = change.get("value", {})

            # 4a) mensagens recebidas
            if field == "messages":
                for msg in value.get("messages", []):
                    from_number = msg["from"]
                    body       = msg.get("text",{}).get("body")
                    # chamar sua lógica: ex: enviar alerta, salvar, etc.
                    print(f"Incoming message from {from_number}: {body}")

            # 4b) status de mensagem (delivered, read, etc.)
            if field == "message_status":
                for status in value.get("statuses", []):
                    msg_id = status.get("id")
                    status_name = status.get("status")
                    print(f"Message {msg_id} status changed to {status_name}")

    # 5) ACK rápido
    return "EVENT_RECEIVED", 200


@app.route('/api/unidades', methods=['GET'])
def get_unidades(): return jsonify(listar_unidades()), 200

@app.route('/api/containers', methods=['GET'])
def get_containers(): return jsonify(listar_containers()), 200

@app.route('/api/data/<unit_id>/<container_id>', methods=['GET'])
def get_data(unit_id, container_id): return jsonify(buscar_monitoramento(unit_id, container_id)), 200

@app.route('/api/all_data', methods=['GET'])
def get_all(): return jsonify(listar_todos_dados()), 200

if __name__ == '__main__': app.run(host='0.0.0.0', port=5000, debug=True)