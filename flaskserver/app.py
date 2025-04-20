# app.py
from flask import Flask, request, jsonify
import psycopg2.extras
from db import (
    criar_unidade, listar_unidades,
    criar_container, listar_containers,
    inserir_monitoramento, buscar_monitoramento,
    listar_todos_dados
)
from mqtt_client import iniciar_mqtt
# Importando o decodificador Huffman final
from huffman_decoder import huffman_decompress

app = Flask(__name__)
# Inicia conexão MQTT em background
print("Iniciando aplicação Flask")
iniciar_mqtt()
print("Cliente MQTT iniciado com sucesso")

# Rotas Unidades
@app.route('/api/unidades', methods=['POST'])
def rota_criar_unidade():
    data = request.get_json()
    try:
        unidade = criar_unidade(data['unit_id'], data.get('nome'), data.get('localizacao'))
        return jsonify({'status': 'sucesso', 'unidade': unidade}), 201
    except Exception as e:
        return jsonify({'status': 'erro', 'message': str(e)}), 400

@app.route('/api/unidades', methods=['GET'])
def rota_listar_unidades():
    return jsonify(listar_unidades()), 200

# Rotas Containers
@app.route('/api/containers', methods=['POST'])
def rota_criar_container():
    data = request.get_json()
    try:
        container = criar_container(
            data['unit_id'], data['container_id'],
            data.get('nome'), data.get('localizacao')
        )
        return jsonify({'status': 'sucesso', 'container': container}), 201
    except Exception as e:
        return jsonify({'status': 'erro', 'message': str(e)}), 400

@app.route('/api/containers', methods=['GET'])
def rota_listar_containers():
    return jsonify(listar_containers()), 200

# Rotas Monitoramento via HTTP (opcionais)
@app.route('/api/data', methods=['POST'])
def rota_inserir_dados():
    data = request.get_json()
    topic = data.get('topic')
    parts = topic.strip('/').split('/')
    try:
        inserir_monitoramento(parts[0], parts[1], **data)
        return jsonify({'status': 'sucesso'}), 201
    except Exception as e:
        return jsonify({'status': 'erro', 'message': str(e)}), 400

@app.route('/api/data/<unit_id>/<container_id>', methods=['GET'])
def rota_buscar_dados(unit_id, container_id):
    return jsonify(buscar_monitoramento(unit_id, container_id)), 200

@app.route('/api/all_data', methods=['GET'])
def rota_todos_dados():
    return jsonify(listar_todos_dados()), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
