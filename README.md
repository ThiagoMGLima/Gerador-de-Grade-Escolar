Claro, aqui está uma proposta de `README.md` para o seu projeto, elaborada a partir da análise dos arquivos de código e da sua solicitação.

-----

# Gerador de Horários Escolares com Otimização

Este projeto é uma aplicação robusta para a geração e otimização de grades horárias escolares, um problema clássico de otimização combinatória. A solução foi desenvolvida em C++ e utiliza uma abordagem de duas fases: primeiro, a construção de uma grade viável através de uma heurística construtiva e, em seguida, o refinamento dessa grade usando a meta-heurística de *Simulated Annealing* (Têmpera Simulada).

O projeto inclui também um visualizador web interativo para analisar e inspecionar as grades geradas.

A metodologia implementada foi inspirada e baseada nos conceitos apresentados na dissertação de mestrado de Everton Luiz Tives, disponível em: [Geração de Grade de Horários para Instituições de Ensino Utilizando Algoritmos Genéticos e Têmpera Simulada](http://hdl.handle.net/10183/39522).

## 🚀 Funcionalidades

  * **Geração em Duas Fases:** Garante tanto a viabilidade (cumprimento de restrições obrigatórias) quanto a qualidade (otimização de restrições flexíveis) da grade horária.
  * **Manipulação de Restrições Complexas:**
      * **Restrições Rígidas (Hard Constraints):** Conflitos de alocação (mesmo professor, turma ou sala em um mesmo horário) e respeito à disponibilidade pré-definida dos professores.
      * **Restrições Flexíveis (Soft Constraints):** Otimização da grade para minimizar "janelas" na grade dos professores, agrupar aulas da mesma disciplina, e distribuir as aulas de forma equilibrada ao longo da semana.
  * **Exportação de Dados:** As grades geradas (inicial e otimizada) são exportadas para o formato JSON, permitindo fácil integração com outras ferramentas.
  * **Visualizador Web Interativo:** Uma interface front-end (HTML, CSS, JS) permite carregar os arquivos JSON e visualizar as grades de forma clara e organizada.
      * Visualização por Turma, Professor ou Sala.
      * Filtros dinâmicos para focar em um item específico.
      * Legenda de cores por disciplina.
      * Painel com estatísticas de qualidade da grade (distribuição de aulas, carga horária, etc.).

## ⚙️ Metodologia

O processo para a criação da grade horária é dividido em duas fases principais:

### Fase 1: Geração da Solução Inicial (Heurística Construtiva)

Nesta fase, o objetivo é gerar uma grade horária **viável**, ou seja, que atenda a todas as restrições rígidas. O algoritmo, implementado na classe `GeradorHorario`, funciona da seguinte maneira:

1.  **Análise de Criticidade:** As requisições de alocação de aulas são priorizadas com base em um critério de "criticidade". Este critério considera a razão entre o número de aulas que um professor precisa lecionar e o número total de horários que ele tem disponível. Professores com agendas mais "apertadas" são priorizados.
2.  **Alocação Aleatorizada e Guiada:** As requisições, ordenadas por criticidade, são alocadas em slots de tempo válidos. Uma pitada de aleatoriedade é usada para embaralhar requisições com criticidade semelhante, evitando um viés determinístico e permitindo a geração de diferentes soluções iniciais a cada execução.
3.  **Verificação Contínua:** A cada tentativa de alocação, o sistema verifica se o slot escolhido não viola nenhuma restrição rígida.

O resultado desta fase é o arquivo `grade_horaria.json`, uma solução completa e válida, mas ainda não otimizada em termos de qualidade.

### Fase 2: Otimização da Solução (Simulated Annealing)

Com uma solução viável em mãos, a segunda fase foca em **melhorá-la**. O algoritmo de *Simulated Annealing*, implementado na classe `SimulatedAnnealing`, refina a grade com base em um conjunto de restrições flexíveis (soft constraints).

1.  **Função de Custo:** Uma função de custo avalia a "qualidade" de uma grade. A nota é calculada a partir de um sistema de penalidades. A implementação atual penaliza:
      * **Janelas de Horário:** Espaços vagos entre aulas na grade de um professor.
      * **Má Distribuição de Aulas:** Concentração de aulas de uma turma em poucos dias.
      * **Aulas em Horários Extremos:** Alocação de aulas no primeiro ou último horário do dia.
      * *Bônus* é concedido para aulas consecutivas da mesma disciplina para a mesma turma.
2.  **Geração de Vizinhos:** O algoritmo explora o espaço de soluções gerando "vizinhos", que são pequenas modificações aleatórias na grade atual (ex: trocar o horário de duas aulas).
3.  **Critério de Aceitação:** Movimentos que levam a uma solução de menor custo (melhor qualidade) são sempre aceitos. Movimentos que pioram a solução podem ser aceitos com uma certa probabilidade, que diminui à medida que a "temperatura" do sistema baixa. Isso permite que o algoritmo escape de ótimos locais.

Ao final, o processo gera o arquivo `grade_melhorada.json`, contendo a versão final e otimizada da grade horária.

## 📂 Estrutura do Projeto

```
/
├── backend/
│   ├── Main.cpp                 # Ponto de entrada, configuração dos dados
│   ├── GeradorHorario.h         # Definição da classe de geração
│   ├── GeradorHorario.cpp       # Implementação da geração
│   ├── SimulatedAnnealing.h     # Definição da classe de otimização
│   └── SimulatedAnnealing.cpp   # Implementação da otimização
│
├── frontend/
│   ├── Visualizador.html        # Estrutura da página do visualizador
│   ├── style_visualizador.css   # Estilos da página
│   └── script_visualizador.js   # Lógica do visualizador (carregamento e renderização)
│
└── output/
    ├── grade_horaria.json       # Grade gerada pela Fase 1
    └── grade_melhorada.json     # Grade otimizada pela Fase 2
```

## ▶️ Como Executar

### Pré-requisitos

  * Um compilador C++ moderno (g++, Clang, MSVC).
  * Um navegador de internet para usar o visualizador.

### 1\. Compilar e Executar o Backend (Gerador)

1.  **Navegue até o diretório `backend/`**.
2.  **Compile os arquivos C++**. Exemplo usando g++:
    ```bash
    g++ Main.cpp GeradorHorario.cpp SimulatedAnnealing.cpp -o gerador_horario -std=c++17
    ```
3.  **Execute o programa compilado**:
    ```bash
    ./gerador_horario
    ```
4.  O programa irá executar a **Fase 1** e salvar o resultado em `grade_horaria.json`. Em seguida, ele perguntará se você deseja executar a **Fase 2 (otimização)**. Digite `S` e pressione Enter para continuar.
5.  Ao final da **Fase 2**, o arquivo `grade_melhorada.json` será gerado.

### 2\. Usar o Frontend (Visualizador)

1.  **Abra o arquivo `frontend/Visualizador.html`** em seu navegador de preferência.
2.  Clique no botão **"📁 Carregar Grade (JSON)"**.
3.  Selecione um dos arquivos gerados (`grade_horaria.json` ou `grade_melhorada.json`).
4.  A grade será exibida na tela. Use os controles de "Visualizar" e "Filtrar" para explorar os horários por turma, professor ou sala.

## 🔧 Configuração dos Dados

Atualmente, todos os dados da instituição de ensino (professores, disciplinas, turmas, disponibilidade, etc.) estão definidos diretamente no código, dentro da função `setupDadosExemplo()` no arquivo `Main.cpp`. Para adaptar o projeto para outra realidade, é necessário alterar os dados nesta função.

## 🛠️ Tecnologias Utilizadas

  * **Backend:** C++ (STL)
  * **Frontend:** HTML5, CSS3, JavaScript (vanilla)
  * **Formato de Dados:** JSON
