# 📚 Sistema de Grade Horária Escolar com Otimização

Sistema completo para geração e otimização de grades horárias escolares, desenvolvido em C++ com interface web moderna. Utiliza algoritmos avançados de otimização combinatória e pode ser executado inteiramente no navegador através de WebAssembly.

O projeto implementa uma solução em duas fases: construção inicial via heurística construtiva e refinamento através de *Simulated Annealing*, baseado na dissertação de Camilo José Bornia Poulsen: [Desenvolvimento de um modelo para o School Timetabling Problem baseado na Meta-Heurística Simulated Annealing](http://hdl.handle.net/10183/39522).

## 🚀 Principais Funcionalidades

### Sistema de Cadastro Web
- **Interface Intuitiva:** Sistema completo de cadastro com validação em tempo real
- **Gerenciamento Completo:** Turmas, disciplinas, professores e salas
- **Disponibilidade Visual:** Grid interativo para marcar horários disponíveis dos professores
- **Validação Automática:** Verificação de consistência dos dados antes da exportação
- **Persistência Local:** Dados salvos automaticamente no navegador

### Processamento no Navegador
- **WebAssembly:** Código C++ compilado para rodar diretamente no navegador
- **Sem Servidor:** Todo processamento acontece localmente
- **Performance Nativa:** Velocidade comparável à execução desktop
- **Multiplataforma:** Funciona em qualquer navegador moderno

### Visualizador Interativo
- **Múltiplas Visões:** Por turma, professor ou sala
- **Filtros Dinâmicos:** Foco em elementos específicos
- **Estatísticas em Tempo Real:** Análise de qualidade da grade
- **Exportação:** Download em JSON para uso posterior

## ⚙️ Arquitetura do Sistema

### Fluxo de Trabalho

```mermaid
graph LR
    A[Sistema de Cadastro] --> B[Dados JSON]
    B --> C[Processador C++/WASM]
    C --> D[Grade Otimizada]
    D --> E[Visualizador]
```

### Algoritmos Implementados

#### Fase 1: Heurística Construtiva
- **Análise de Criticidade:** Priorização baseada em disponibilidade
- **Alocação Inteligente:** Consideração de múltiplas restrições
- **Backtracking Parcial:** Correção automática de conflitos

#### Fase 2: Simulated Annealing
- **Função de Custo Multiobjetivo:**
  - Minimização de janelas de horário
  - Distribuição equilibrada de aulas
  - Agrupamento de aulas consecutivas
  - Preferências de horários
- **Movimentos Adaptativos:** 7 tipos diferentes de perturbação
- **Memória Tabu:** Evita ciclos na busca
- **Reaquecimento Automático:** Escape de ótimos locais

## 📂 Estrutura do Projeto

```
projeto/
├── index.html                          # Página inicial do sistema
├── src/
│   └── geradorArquivos/
│       ├── Estruturas.h                # Definições de estruturas de dados
│       ├── GeradorHorario.h/.cpp       # Algoritmo de geração inicial
│       ├── SimulatedAnnealing.h/.cpp   # Algoritmo de otimização
│       ├── GeradorWeb.cpp              # Interface WebAssembly
│       ├── Main.cpp                    # Versão desktop
│       └── json.hpp                    # Biblioteca JSON
├── web/
│   ├── sistemaCadastro/
│   │   ├── sistemaCadastro.html       # Interface de cadastro
│   │   ├── sistemaIntegrado.html      # Sistema completo integrado
│   │   ├── style_cadastro.css         # Estilos do cadastro
│   │   ├── script_cadastro.js         # Lógica do cadastro
│   │   └── gerador.js                 # Código WASM compilado
│   └── visualizador/
│       └── Visualizador.html           # Visualizador de grades
└── build_web.sh                        # Script de compilação
```

## 🚀 Como Usar

### Opção 1: Versão Web (Recomendada)

1. **Instalar Dependências:**
   ```bash
   # Instalar Emscripten (compilador WebAssembly)
   git clone https://github.com/emscripten-core/emsdk.git
   cd emsdk
   ./emsdk install latest
   ./emsdk activate latest
   source ./emsdk_env.sh
   ```

2. **Compilar para WebAssembly:**
   ```bash
   chmod +x build_web.sh
   ./build_web.sh
   ```

3. **Iniciar Servidor Local:**
   ```bash
   python -m http.server 8000
   # Ou com Node.js: npx http-server
   ```

4. **Acessar no Navegador:**
   ```
   http://localhost:8000/
   ```

### Opção 2: Versão Desktop

1. **Compilar:**
   ```bash
   g++ -o gerador src/geradorArquivos/Main.cpp \
        src/geradorArquivos/GeradorHorario.cpp \
        src/geradorArquivos/SimulatedAnnealing.cpp \
        -std=c++17 -O3
   ```

2. **Executar:**
   ```bash
   ./gerador [arquivo_dados.json]
   ```

## 📋 Workflow Completo

### 1. Cadastro de Dados
- Acesse o Sistema de Cadastro
- Configure todas as informações:
  - **Turmas:** Nome e turno
  - **Disciplinas:** Nome e carga horária por turma
  - **Professores:** Nome, disciplina e disponibilidade
  - **Salas:** Nome, tipo e se é compartilhada
- Exporte os dados em JSON

### 2. Geração da Grade
- Acesse o Sistema Integrado
- Carregue o arquivo JSON exportado
- Clique em "Processar" para gerar a grade
- Aguarde o processamento (executa no navegador)

### 3. Visualização e Análise
- Visualize a grade gerada
- Analise as estatísticas de qualidade
- Exporte o resultado final

## 🔧 Configuração e Personalização

### Parâmetros do Simulated Annealing

No arquivo `SimulatedAnnealing.h`, ajuste os parâmetros em `ConfiguracaoSA`:

```cpp
struct ConfiguracaoSA {
    int numIteracoes = 10000;          // Número de iterações
    double temperaturaInicial = 100.0;  // Temperatura inicial
    double taxaResfriamento = 0.95;     // Taxa de resfriamento
<<<<<<< HEAD

=======
    
>>>>>>> 34df94d427da66d14326e4e64d5ea160cda9733c
    // Pesos das penalidades
    double pesoDistribuicao = 2.0;      // Distribuição de aulas
    double pesoConsecutivas = 3.0;      // Aulas consecutivas
    double pesoJanelas = 4.0;           // Janelas de horário
    double pesoHorariosExtremos = 1.0;  // Horários extremos
};
```

### Restrições e Preferências

Modifique as validações em `GeradorHorario.cpp` para adicionar novas restrições específicas da sua instituição.

## 🛠️ Tecnologias Utilizadas

- **Backend:** C++17 (STL, algoritmos modernos)
- **Compilação Web:** Emscripten/WebAssembly
- **Frontend:** HTML5, CSS3, JavaScript ES6+
- **Serialização:** JSON (nlohmann/json)
- **Algoritmos:** Backtracking, Simulated Annealing, Heurísticas

## 📊 Métricas de Qualidade

O sistema avalia a qualidade da grade através de:

- **Taxa de Alocação:** Percentual de aulas alocadas com sucesso
- **Janelas de Horário:** Total de espaços vazios entre aulas
- **Distribuição Semanal:** Equilíbrio de aulas por dia
- **Aulas Consecutivas:** Agrupamento de aulas da mesma disciplina
- **Ocupação de Salas:** Eficiência no uso dos espaços

## 🔍 Solução de Problemas

### Erro: "emcc não encontrado"
```bash
source ~/emsdk/emsdk_env.sh  # Linux/Mac
# ou
C:\emsdk\emsdk_env.bat       # Windows
```

### Navegador não suporta WebAssembly
Use um navegador moderno: Chrome 57+, Firefox 52+, Safari 11+, Edge 16+

### Grade não converge
- Verifique se há professores suficientes
- Confirme disponibilidade adequada
- Reduza restrições muito restritivas

## 📚 Referências

- Poulsen, C. J. B. (2012). [Desenvolvimento de um modelo para o School Timetabling Problem baseado na Meta-Heurística Simulated Annealing](http://hdl.handle.net/10183/39522)
- [Emscripten Documentation](https://emscripten.org/docs/)
- [WebAssembly MDN](https://developer.mozilla.org/en-US/docs/WebAssembly)

## 📄 Licença

Este projeto é distribuído sob a licença MIT. Veja o arquivo LICENSE para mais detalhes.

## 🤝 Contribuições

Contribuições são bem-vindas! Por favor:
1. Fork o projeto
2. Crie uma branch para sua feature (`git checkout -b feature/NovaFuncionalidade`)
3. Commit suas mudanças (`git commit -m 'Adiciona nova funcionalidade'`)
4. Push para a branch (`git push origin feature/NovaFuncionalidade`)
5. Abra um Pull Request

---

Desenvolvido com ❤️ para facilitar a vida de gestores escolares e coordenadores pedagógicos.
