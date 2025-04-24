#!/bin/sh
set -e

# espera o Postgres ficar disponível
while ! pg_isready -h "$DB_HOST" -p "$DB_PORT" > /dev/null 2>&1; do
  sleep 1
done
echo "✅ Postgres disponível"

# migra/cria tabelas
python init_db.py

# inicia o Flask (sem Xvfb!)
exec flask run --host=0.0.0.0 --port=5000
