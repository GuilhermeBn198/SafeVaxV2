from connection import get_db_connection

def reset_db():
    conn = get_db_connection()
    cur = conn.cursor()

    # Drop na ordem inversa das dependências
    cur.execute("DROP TABLE IF EXISTS dados_monitoramento CASCADE;")
    cur.execute("DROP TABLE IF EXISTS containers CASCADE;")
    cur.execute("DROP TABLE IF EXISTS unidade_de_saude CASCADE;")

    conn.commit()
    cur.close()
    conn.close()
    print("Tabelas removidas com sucesso!")

def criar_tabelas():
    conn = get_db_connection()
    cur = conn.cursor()

    # 1) Unidade de Saúde
    cur.execute("""
    CREATE TABLE IF NOT EXISTS unidade_de_saude (
        unit_id       VARCHAR(50) PRIMARY KEY,
        nome          VARCHAR(100) NOT NULL
    );
    """)

    # 2) Containers vinculados à unidade
    cur.execute("""
    CREATE TABLE IF NOT EXISTS containers (
        id            SERIAL PRIMARY KEY,
        unit_id       VARCHAR(50) NOT NULL,
        container_id  VARCHAR(50) NOT NULL,
        nome          VARCHAR(100),
        data_criacao  TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        CONSTRAINT fk_unidade FOREIGN KEY (unit_id)
            REFERENCES unidade_de_saude(unit_id),
        CONSTRAINT uq_container_por_unidade UNIQUE(unit_id, container_id)
    );
    """)

    # 3) Dados de monitoramento
    cur.execute("""
    CREATE TABLE IF NOT EXISTS dados_monitoramento (
        id                    SERIAL PRIMARY KEY,
        unit_id               VARCHAR(50) NOT NULL,
        container_id          VARCHAR(50) NOT NULL,
        temperatura_interna   VARCHAR(50),
        umidade_interna       VARCHAR(50),
        temperatura_externa   VARCHAR(50),
        umidade_externa       VARCHAR(50),
        estado_porta          VARCHAR(20),
        usuario               VARCHAR(50),
        alarm_state           VARCHAR(20),
        motivo                VARCHAR(100),
        data_registro         TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
        CONSTRAINT fk_unidade_data FOREIGN KEY (unit_id)
            REFERENCES unidade_de_saude(unit_id),
        CONSTRAINT fk_container_data FOREIGN KEY (unit_id, container_id)
            REFERENCES containers(unit_id, container_id)
    );
    """)

    conn.commit()
    cur.close()
    conn.close()
    print("Tabelas criadas/atualizadas com sucesso!")


if __name__ == "__main__":
    reset_db()
    criar_tabelas()
