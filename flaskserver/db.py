from connection import get_db_connection
import psycopg2.extras

# Unidades de Saúde

def criar_unidade(unit_id, nome=None, localizacao=None):
    conn = get_db_connection()
    cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute(
        "INSERT INTO unidade_de_saude (unit_id, nome, localizacao) VALUES (%s, %s, %s) RETURNING *",
        (unit_id, nome, localizacao)
    )
    unidade = cur.fetchone()
    conn.commit()
    cur.close()
    conn.close()
    return unidade


def listar_unidades():
    conn = get_db_connection()
    cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute("SELECT * FROM unidade_de_saude ORDER BY data_criacao DESC")
    unidades = cur.fetchall()
    cur.close()
    conn.close()
    return unidades

# Containers

def criar_container(unit_id, container_id, nome=None, localizacao=None):
    conn = get_db_connection()
    cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute(
        "SELECT 1 FROM unidade_de_saude WHERE unit_id = %s", (unit_id,)
    )
    if not cur.fetchone():
        raise ValueError("Unidade não cadastrada.")
    cur.execute(
        "INSERT INTO containers (unit_id, container_id, nome, localizacao) VALUES (%s, %s, %s, %s) RETURNING *",
        (unit_id, container_id, nome, localizacao)
    )
    container = cur.fetchone()
    conn.commit()
    cur.close()
    conn.close()
    return container


def listar_containers():
    conn = get_db_connection()
    cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute("SELECT * FROM containers ORDER BY data_criacao DESC")
    containers = cur.fetchall()
    cur.close()
    conn.close()
    return containers

# Dados de Monitoramento

def inserir_monitoramento(unit_id, container_id, **dados):
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute(
        "SELECT 1 FROM containers WHERE unit_id = %s AND container_id = %s",
        (unit_id, container_id)
    )
    if not cur.fetchone():
        raise ValueError("Container não cadastrado para esta unidade.")
    cur.execute(
        """
        INSERT INTO dados_monitoramento (
            unit_id, container_id,
            temperatura_interna, umidade_interna,
            temperatura_externa, umidade_externa,
            estado_porta, usuario, alarm_state, motivo
        ) VALUES (%(unit_id)s, %(container_id)s,
                  %(temperatura_interna)s, %(umidade_interna)s,
                  %(temperatura_externa)s, %(umidade_externa)s,
                  %(estado_porta)s, %(usuario)s, %(alarm_state)s, %(motivo)s)
        """,
        {**dados, 'unit_id': unit_id, 'container_id': container_id}
    )
    conn.commit()
    cur.close()
    conn.close()


def buscar_monitoramento(unit_id, container_id):
    conn = get_db_connection()
    cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute(
        "SELECT * FROM dados_monitoramento WHERE unit_id = %s AND container_id = %s ORDER BY data_registro DESC",
        (unit_id, container_id)
    )
    registros = cur.fetchall()
    cur.close()
    conn.close()
    return registros


def listar_todos_dados():
    conn = get_db_connection()
    cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute(
        """
        SELECT dm.*, c.nome AS nome_container, c.localizacao AS local_container,
               u.nome AS nome_unidade, u.localizacao AS local_unidade
        FROM dados_monitoramento dm
        JOIN containers c ON dm.unit_id = c.unit_id AND dm.container_id = c.container_id
        JOIN unidade_de_saude u ON dm.unit_id = u.unit_id
        ORDER BY dm.unit_id, dm.container_id, dm.data_registro DESC
        """
    )
    registros = cur.fetchall()
    cur.close()
    conn.close()
    return registros
