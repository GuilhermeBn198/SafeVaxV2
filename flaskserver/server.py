from flask import Flask, request, jsonify
from connection import get_db_connection  # Importa a função de conexão
import psycopg2.extras

app = Flask(__name__)

# ---------------------------
# Rotas para gerenciamento de containers (CRUD)
# ---------------------------

# Cria um novo container
@app.route('/api/containers', methods=['POST'])
def criar_container():
    if not request.is_json:
        return jsonify({"error": "Formato JSON esperado"}), 400

    dados = request.get_json()
    container_id = dados.get("container_id")
    nome = dados.get("nome")
    localizacao = dados.get("localizacao")
    
    if not container_id:
        return jsonify({"error": "Campo 'container_id' é obrigatório."}), 400

    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
        cur.execute("""
            INSERT INTO containers (container_id, nome, localizacao)
            VALUES (%s, %s, %s)
            RETURNING *
        """, (container_id, nome, localizacao))
        novo_container = cur.fetchone()
        conn.commit()
        cur.close()
        conn.close()
        return jsonify({"status": "sucesso", "container": novo_container}), 201
    except Exception as e:
        return jsonify({"status": "erro", "message": str(e)}), 500

# Consulta todos os containers
@app.route('/api/containers', methods=['GET'])
def get_containers():
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
        cur.execute("SELECT * FROM containers ORDER BY data_criacao DESC")
        containers = cur.fetchall()
        cur.close()
        conn.close()
        return jsonify(containers), 200
    except Exception as e:
        return jsonify({"status": "erro", "message": str(e)}), 500

# ---------------------------
# Rotas para monitoramento dos containers
# ---------------------------

# Insere dados de monitoramento para um container
@app.route('/api/data', methods=['POST'])
def inserir_dados_monitoramento():
    if not request.is_json:
        return jsonify({"error": "Formato JSON esperado"}), 400

    dados = request.get_json()
    container_id = dados.get("container_id")
    if not container_id:
        return jsonify({"error": "Campo 'container_id' é obrigatório."}), 400

    temperatura = dados.get("temperatura")
    umidade = dados.get("umidade")
    estado_porta = dados.get("estado_porta")
    usuario = dados.get("usuario")
    motor_state = dados.get("motor_state")
    alarm_state = dados.get("alarm_state")

    try:
        conn = get_db_connection()
        cur = conn.cursor()
        # Verifica se o container existe
        cur.execute("SELECT * FROM containers WHERE container_id = %s", (container_id,))
        container = cur.fetchone()
        if not container:
            return jsonify({"error": "Container não cadastrado."}), 400
        
        cur.execute("""
            INSERT INTO dados_monitoramento 
                (container_id, temperatura, umidade, estado_porta, usuario, motor_state, alarm_state)
            VALUES (%s, %s, %s, %s, %s, %s, %s)
        """, (container_id, temperatura, umidade, estado_porta, usuario, motor_state, alarm_state))
        conn.commit()
        cur.close()
        conn.close()
        return jsonify({"status": "sucesso", "message": "Dados inseridos com sucesso"}), 201
    except Exception as e:
        return jsonify({"status": "erro", "message": str(e)}), 500

# Consulta os dados de monitoramento de um container específico
@app.route('/api/data/<container_id>', methods=['GET'])
def get_dados_container(container_id):
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
        cur.execute("""
            SELECT * FROM dados_monitoramento
            WHERE container_id = %s
            ORDER BY data_registro DESC
        """, (container_id,))
        registros = cur.fetchall()
        cur.close()
        conn.close()
        return jsonify(registros), 200
    except Exception as e:
        return jsonify({"status": "erro", "message": str(e)}), 500

# Consulta todos os dados de monitoramento de todos os containers (para visão global)
@app.route('/api/all_data', methods=['GET'])
def get_todos_dados():
    try:
        conn = get_db_connection()
        cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
        cur.execute("""
            SELECT dm.*, c.nome, c.localizacao 
            FROM dados_monitoramento dm
            JOIN containers c ON dm.container_id = c.container_id
            ORDER BY c.container_id, dm.data_registro DESC
        """)
        registros = cur.fetchall()
        cur.close()
        conn.close()
        return jsonify(registros), 200
    except Exception as e:
        return jsonify({"status": "erro", "message": str(e)}), 500

if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000, debug=True)
