#!/bin/bash

echo "🔨 Compilando WebAssembly..."

# Criar diretório de saída se não existir
mkdir -p docs

# Compilar C++ para WebAssembly
emcc -O3 \
    src/geradorArquivos/GeradorWeb.cpp \
    src/geradorArquivos/GeradorHorario.cpp \
    src/geradorArquivos/SimulatedAnnealing.cpp \
    -s WASM=1 \
    -s EXPORTED_FUNCTIONS='["_malloc", "_free"]' \
    -s EXPORTED_RUNTIME_METHODS='["ccall", "cwrap", "UTF8ToString"]' \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME='GeradorModule' \
    -lembind \
    -o docs/gerador.js

echo "✅ Compilação concluída!"
echo "📁 Arquivo gerado: docs/gerador.js"