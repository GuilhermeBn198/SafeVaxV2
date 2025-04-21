#!/bin/sh
# 1) cria/atualiza tabelas
python init_db.py

# 2) depois, inicializa o Flask
exec flask run --host=0.0.0.0
