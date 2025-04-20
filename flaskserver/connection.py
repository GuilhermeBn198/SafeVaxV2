import psycopg2
from psycopg2.extras import RealDictCursor
import os
from dotenv import load_dotenv

# Carrega variáveis de ambiente
load_dotenv()

DB_HOST = os.getenv("DB_HOST", "localhost")
DB_NAME = os.getenv("DB_NAME", "seu_banco")
DB_USER = os.getenv("DB_USER", "seu_usuario")
DB_PASS = os.getenv("DB_PASS", "sua_senha")
DB_PORT = os.getenv("DB_PORT", "5432")

# Conexão com o banco
def get_db_connection():
    return psycopg2.connect(
        host=DB_HOST,
        database=DB_NAME,
        user=DB_USER,
        password=DB_PASS,
        port=DB_PORT
    )
