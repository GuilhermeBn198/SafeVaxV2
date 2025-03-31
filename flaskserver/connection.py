import psycopg2
from psycopg2.extras import RealDictCursor
import os
from dotenv import load_dotenv

# Carrega as variáveis de ambiente do arquivo .env
load_dotenv()

DB_HOST = os.getenv("DB_HOST", "localhost")
DB_NAME = os.getenv("DB_NAME", "seu_banco")
DB_USER = os.getenv("DB_USER", "seu_usuario")
DB_PASS = os.getenv("DB_PASS", "sua_senha")
DB_PORT = os.getenv("DB_PORT", "5432")

# Função para obter conexão com o banco de dados
def get_db_connection():
    return psycopg2.connect(
        host=DB_HOST,
        database=DB_NAME,
        user=DB_USER,
        password=DB_PASS,
        port=DB_PORT
    )

# Função para criar tabelas no banco de dados
def criar_tabelas():
    conn = get_db_connection()
    cur = conn.cursor()

    # Tabela de containers
    cur.execute("""
        CREATE TABLE IF NOT EXISTS containers (
            id SERIAL PRIMARY KEY,
            container_id VARCHAR(50) UNIQUE NOT NULL,
            nome VARCHAR(100),
            localizacao VARCHAR(100),
            data_criacao TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    """)

    # Tabela de monitoramento
    cur.execute("""
        CREATE TABLE IF NOT EXISTS dados_monitoramento (
            id SERIAL PRIMARY KEY,
            container_id VARCHAR(50) NOT NULL,
            temperatura REAL,
            umidade REAL,
            estado_porta VARCHAR(20),
            usuario VARCHAR(50),
            motor_state VARCHAR(20),
            alarm_state VARCHAR(20),
            data_registro TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            CONSTRAINT fk_container
                FOREIGN KEY (container_id)
                REFERENCES containers(container_id)
        )
    """)

    conn.commit()
    cur.close()
    conn.close()

# Criar tabelas ao iniciar o sistema
criar_tabelas()
