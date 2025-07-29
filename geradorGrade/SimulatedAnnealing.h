#pragma once
#include "Estruturas.h"
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <random>
#include <functional>
#include <memory>

// Estrutura para configurações do Simulated Annealing
struct ConfiguracaoSA {
    // Parâmetros básicos
    int numIteracoes;
    double temperaturaInicial;
    double taxaResfriamento;
    double temperaturaMinima;

    // Pesos para função de custo
    double pesoDistribuicao;      // Penalidade por distribuição desigual
    double pesoConsecutivas;      // Bônus por aulas consecutivas
    double pesoJanelas;          // Penalidade por janelas
    double pesoHorariosExtremos; // Penalidade por horários extremos
    double pesoPreferencias;     // Peso das preferências

    // Estratégias
    bool usarReaquecimento;      // Reaquece quando estagnar
    bool usarMemoriaTabu;        // Evita movimentos recentes
    int tamanhoListaTabu;        // Tamanho da lista tabu

    // Controle
    bool verboso;                // Exibir progresso detalhado
    int frequenciaRelatorio;     // A cada quantas iterações mostrar status

    // Construtor com valores padrão
    ConfiguracaoSA()
        : numIteracoes(10000),
          temperaturaInicial(100.0),
          taxaResfriamento(0.95),
          temperaturaMinima(0.01),
          pesoDistribuicao(2.0),
          pesoConsecutivas(3.0),
          pesoJanelas(4.0),
          pesoHorariosExtremos(1.0),
          pesoPreferencias(1.5),
          usarReaquecimento(true),
          usarMemoriaTabu(true),
          tamanhoListaTabu(50),
          verboso(false),
          frequenciaRelatorio(1000) {}
};

// Estrutura para estatísticas do SA
struct EstatisticasSA {
    // Contadores
    int movimentosAceitos;
    int movimentosRejeitados;
    int movimentosMelhoria;
    int movimentosPiora;
    int reaquecimentos;

    // Custos
    double custoInicial;
    double custoFinal;
    double melhorCusto;
    int iteracaoMelhorCusto;

    // Histórico
    std::vector<double> historicoCusto;
    std::vector<double> historicoTemperatura;
    std::vector<double> historicoTaxaAceitacao;

    // Tempo
    double tempoExecucao; // em segundos

    EstatisticasSA()
        : movimentosAceitos(0), movimentosRejeitados(0),
          movimentosMelhoria(0), movimentosPiora(0),
          reaquecimentos(0), custoInicial(0), custoFinal(0),
          melhorCusto(0), iteracaoMelhorCusto(0), tempoExecucao(0) {}

    double getTaxaAceitacao() const {
        int total = movimentosAceitos + movimentosRejeitados;
        return total > 0 ? (double)movimentosAceitos / total * 100 : 0;
    }

    double getPercentualMelhoria() const {
        return custoInicial > 0 ?
            (custoInicial - custoFinal) / custoInicial * 100 : 0;
    }
};

// Tipos de movimentos possíveis
enum class TipoMovimento {
    TROCAR_HORARIO,      // Troca horário mantendo dia
    TROCAR_DIA,          // Troca dia mantendo horário
    TROCAR_SLOT,         // Troca dia e horário
    TROCAR_AULAS,        // Troca duas aulas de lugar
    MOVER_BLOCO,         // Move bloco de aulas consecutivas
    OTIMIZAR_PROFESSOR,  // Otimiza horário de um professor
    OTIMIZAR_TURMA       // Otimiza horário de uma turma
};

// Movimento realizado (para lista tabu)
struct Movimento {
    TipoMovimento tipo;
    std::vector<int> parametros; // IDs envolvidos
    size_t hash;

    Movimento(TipoMovimento t) : tipo(t) {}

    void calcularHash() {
        hash = std::hash<int>()(static_cast<int>(tipo));
        for (int p : parametros) {
            hash ^= std::hash<int>()(p) << 1;
        }
    }

    bool operator==(const Movimento& outro) const {
        return hash == outro.hash;
    }
};

class SimulatedAnnealing {
public:
    // Construtor
    SimulatedAnnealing(
        std::vector<Aula> solucaoInicial,
        std::vector<Professor> professores,
        std::vector<Disciplina> disciplinas,
        std::vector<Turma> turmas,
        std::vector<Sala> salas,
        std::set<std::tuple<int, int, int>> disponibilidadeProfessores,
        std::map<int, int> turmaSalaMap,
        ConfiguracaoSA config = ConfiguracaoSA()
    );

    // Métodos principais
    void executar();
    void executarComCallback(std::function<void(int, double, double)> callback);
    void pararExecucao() { executando = false; }

    // Acesso aos resultados
    std::vector<Aula> getSolucaoFinal() const { return melhorSolucao; }
    double getCustoFinal() const { return melhorCusto; }
    EstatisticasSA getEstatisticas() const { return estatisticas; }

    // Análise
    void mostrarEstatisticas() const;
    void exportarHistorico(const std::string& arquivo) const;
    std::map<std::string, double> analisarQualidadeSolucao() const;

    // Configuração dinâmica
    void ajustarPesos(double dist, double consec, double jan, double ext);
    void setConfiguracao(const ConfiguracaoSA& novaConfig) { config = novaConfig; }

private:
    // Dados do problema
    std::vector<Aula> solucaoAtual;
    std::vector<Aula> melhorSolucao;
    std::vector<Professor> professores;
    std::vector<Disciplina> disciplinas;
    std::vector<Turma> turmas;
    std::vector<Sala> salas;
    std::set<std::tuple<int, int, int>> disponibilidadeProfessores;
    std::map<int, int> turmaSalaMap;

    // Configuração e estado
    ConfiguracaoSA config;
    EstatisticasSA estatisticas;
    double temperaturaAtual;
    bool executando;

    // Lista tabu
    std::deque<Movimento> listaTabu;

    // Gerador de números aleatórios
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
    std::uniform_int_distribution<> disMovimento;

    // Cache para otimização
    mutable std::map<size_t, double> cacheCusto;
    mutable std::map<std::pair<int, int>, std::vector<int>> cacheHorariosProfessor;

    // Métodos de custo
    double calcularCusto(const std::vector<Aula>& solucao);
    double calcularCustoIncremental(const std::vector<Aula>& solucao,
                                   const Movimento& movimento);

    // Componentes do custo
    double calcularPenalidade1(const std::vector<Aula>& solucao) const; // Distribuição
    double calcularPenalidade2(const std::vector<Aula>& solucao) const; // Consecutivas
    double calcularPenalidade3(const std::vector<Aula>& solucao) const; // Janelas
    double calcularPenalidade4(const std::vector<Aula>& solucao) const; // Extremos
    double calcularPenalidade5(const std::vector<Aula>& solucao) const; // Preferências

    // Análise detalhada
    std::map<int, std::vector<int>> obterJanelasPorProfessor(const std::vector<Aula>& solucao) const;
    std::map<int, double> obterDistribuicaoPorTurma(const std::vector<Aula>& solucao) const;
    int contarAulasConsecutivasTotal(const std::vector<Aula>& solucao) const;

    // Geração de vizinhos
    std::vector<Aula> gerarVizinho(const std::vector<Aula>& solucao);
    Movimento selecionarMovimento();
    bool movimentoTabu(const Movimento& mov) const;
    void adicionarTabu(const Movimento& mov);

    // Tipos de movimento específicos
    std::vector<Aula> trocarHorario(const std::vector<Aula>& solucao);
    std::vector<Aula> trocarDia(const std::vector<Aula>& solucao);
    std::vector<Aula> trocarSlot(const std::vector<Aula>& solucao);
    std::vector<Aula> trocarAulas(const std::vector<Aula>& solucao);
    std::vector<Aula> moverBloco(const std::vector<Aula>& solucao);
    std::vector<Aula> otimizarProfessor(const std::vector<Aula>& solucao);
    std::vector<Aula> otimizarTurma(const std::vector<Aula>& solucao);

    // Validação
    bool verificarViabilidade(const std::vector<Aula>& solucao);
    bool verificarViabilidadeRapida(const std::vector<Aula>& solucao,
                                   const Movimento& movimento);
    ResultadoValidacao validarSolucaoCompleta(const std::vector<Aula>& solucao);

    // Controle de temperatura
    bool aceitarMovimento(double deltaCusto);
    void atualizarTemperatura(int iteracao);
    bool criterioReaquecimento(int iteracoesSemMelhoria);
    void reaquecerTemperatura();

    // Utilidades
    void limparCaches();
    void registrarEstatistica(int iteracao);
    void log(const std::string& mensagem) const;
    std::string formatarTempo(double segundos) const;

    // Busca local (melhoria adicional)
    std::vector<Aula> buscaLocal2opt(const std::vector<Aula>& solucao);
    std::vector<Aula> buscaLocalJanelas(const std::vector<Aula>& solucao);
};