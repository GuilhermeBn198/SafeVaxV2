from flask import Flask, request, jsonify # type: ignore
from db import (
    listar_unidades, listar_containers,
    buscar_monitoramento, listar_todos_dados
)
from mqtt_client_hibrido import iniciar_mqtt

app = Flask(__name__)
print("Aplicação Flask iniciada")
iniciar_mqtt()
print("MQTT ativo")

@app.route('/api/unidades', methods=['GET'])
def get_unidades(): return jsonify(listar_unidades()), 200

@app.route('/api/containers', methods=['GET'])
def get_containers(): return jsonify(listar_containers()), 200

@app.route('/api/data/<unit_id>/<container_id>', methods=['GET'])
def get_data(unit_id, container_id): return jsonify(buscar_monitoramento(unit_id, container_id)), 200

@app.route('/api/all_data', methods=['GET'])
def get_all(): return jsonify(listar_todos_dados()), 200

if __name__ == '__main__': app.run(host='0.0.0.0', port=5000, debug=True)