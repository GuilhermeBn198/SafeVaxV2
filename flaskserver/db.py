from connection import get_db_connection
import psycopg2.extras # type: ignore

# Garante criação automática de unidade e container

def ensure_unidade(unit_id):
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute("SELECT 1 FROM unidade_de_saude WHERE unit_id = %s", (unit_id,))
    if not cur.fetchone():
        cur.execute(
            "INSERT INTO unidade_de_saude (unit_id, nome) VALUES (%s, %s)",
            (unit_id, unit_id)
        )
        conn.commit()
    cur.close()
    conn.close()


def ensure_container(unit_id, container_id):
    ensure_unidade(unit_id)
    conn = get_db_connection()
    cur = conn.cursor()
    cur.execute(
        "SELECT 1 FROM containers WHERE unit_id = %s AND container_id = %s",
        (unit_id, container_id)
    )
    if not cur.fetchone():
        cur.execute(
            "INSERT INTO containers (unit_id, container_id, nome) VALUES (%s, %s, %s)",
            (unit_id, container_id, container_id)
        )
        conn.commit()
    cur.close()
    conn.close()

# Funções existentes (podem ser usadas para rotas REST)

def criar_unidade(unit_id, nome=None):
    conn = get_db_connection()
    cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute(
        "INSERT INTO unidade_de_saude (unit_id, nome) VALUES (%s, %s) RETURNING *",
        (unit_id, nome)
    )
    unidade = cur.fetchone()
    conn.commit()
    cur.close(); conn.close()
    return unidade


def criar_container(unit_id, container_id, nome=None):
    conn = get_db_connection()
    cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute(
        "SELECT 1 FROM unidade_de_saude WHERE unit_id = %s", (unit_id,)
    )
    if not cur.fetchone():
        raise ValueError("Unidade não cadastrada.")
    cur.execute(
        "INSERT INTO containers (unit_id, container_id, nome) VALUES (%s, %s, %s) RETURNING *",
        (unit_id, container_id, nome)
    )
    container = cur.fetchone()
    conn.commit(); cur.close(); conn.close()
    return container

# Dados de Monitoramento com criação automática

def inserir_monitoramento(unit_id, container_id, **dados):
    # Garante que unidade e container existam
    ensure_container(unit_id, container_id)

    conn = get_db_connection()
    cur = conn.cursor()
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
    conn.commit(); cur.close(); conn.close()

# Outras consultas

def listar_unidades():
    conn = get_db_connection(); cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute("SELECT unit_id, nome, data_criacao FROM unidade_de_saude ORDER BY data_criacao DESC")
    data = cur.fetchall(); cur.close(); conn.close(); return data


def listar_containers():
    conn = get_db_connection(); cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute("SELECT id, unit_id, container_id, nome, data_criacao FROM containers ORDER BY data_criacao DESC")
    data = cur.fetchall(); cur.close(); conn.close(); return data


def buscar_monitoramento(unit_id, container_id):
    conn = get_db_connection(); cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute(
        "SELECT * FROM dados_monitoramento WHERE unit_id=%s AND container_id=%s ORDER BY data_registro DESC",
        (unit_id, container_id)
    )
    data = cur.fetchall(); cur.close(); conn.close(); return data


def listar_todos_dados():
    conn = get_db_connection(); cur = conn.cursor(cursor_factory=psycopg2.extras.RealDictCursor)
    cur.execute(
        """
        SELECT dm.*, c.nome AS nome_container, u.nome AS nome_unidade
        FROM dados_monitoramento dm
        JOIN containers c ON dm.unit_id=c.unit_id AND dm.container_id=c.container_id
        JOIN unidade_de_saude u ON dm.unit_id=u.unit_id
        ORDER BY dm.data_registro DESC
        """
    )
    data = cur.fetchall(); cur.close(); conn.close(); return data