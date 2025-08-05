#!/bin/bash

echo "Compilando para WebAssembly..."

source ./WebAssembly/emsdk/emsdk_env.sh

# Verificar se o Emscripten está configurado
if ! command -v emcc &> /dev/null; then
    echo "Erro: emcc não encontrado. Execute: source ./emsdk/emsdk_env.sh"
    exit 1
fi

# Criar diretório de saída se não existir
mkdir -p web/sistemaCadastro

# Compilar
emcc -O2 \
    src/geradorGrade/GeradorWeb.cpp \
    src/geradorGrade/GeradorHorario.cpp \
    src/geradorGrade/SimulatedAnnealing.cpp \
    -I./src/geradorGrade \
    -s WASM=1 \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME='GeradorModule' \
    -s ENVIRONMENT='web' \
    -s SINGLE_FILE=1 \
    --bind \
    -std=c++17 \
    -o web/gerador.js

if [ $? -eq 0 ]; then
    echo "✓ Compilação concluída com sucesso!"
    echo "Arquivo gerado: web/gerador.js"

    echo ""
    echo "testar o sistema"
    python3 -m http.server 8000
    xdg-open "http://localhost:8000/index.html" >/dev/null 2>&1 &

else
    echo "✗ Erro na compilação"
    exit 1
fi