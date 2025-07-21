#pragma once
#include "Estruturas.h"
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <random>

class SimulatedAnnealing {
public:
    SimulatedAnnealing(
        std::vector<Aula> solucaoInicial,
        std::vector<Professor> professores,
        std::vector<Disciplina> disciplinas,
        std::vector<Turma> turmas,
        std::vector<Sala> salas,
        std::set<std::tuple<int, int, int>> disponibilidadeProfessores,
        std::map<int, int> turmaSalaMap,
        int numIteracoes = 10000,
        double temperaturaInicial = 100.0,
        double taxaResfriamento = 0.95
    );

    void executar();
    std::vector<Aula> getSolucaoFinal() const { return melhorSolucao; }
    double getCustoFinal() const { return melhorCusto; }
    void mostrarEstatisticas() const;

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

    // Parâmetros do SA
    int numIteracoes;
    double temperaturaInicial;
    double taxaResfriamento;
    double temperaturaAtual;

    // Estatísticas
    double custoAtual;
    double melhorCusto;
    int iteracoesSemMelhoria;
    int movimentosAceitos;
    int movimentosRejeitados;

    // Gerador de números aleatórios
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;

    // Métodos auxiliares
    double calcularCusto(const std::vector<Aula>& solucao);
    std::vector<Aula> gerarVizinho(const std::vector<Aula>& solucao);
    bool verificarViabilidade(const std::vector<Aula>& solucao);
    bool aceitarMovimento(double deltaCusto);

    // Tipos de movimento para gerar vizinhos
    std::vector<Aula> trocarHorario(const std::vector<Aula>& solucao);
    std::vector<Aula> trocarDia(const std::vector<Aula>& solucao);
    std::vector<Aula> trocarSlot(const std::vector<Aula>& solucao);

    // Componentes do custo
    double calcularPenalidade1(const std::vector<Aula>& solucao) const; // Distribuição de aulas por dia
    double calcularPenalidade2(const std::vector<Aula>& solucao) const; // Aulas consecutivas
    double calcularPenalidade3(const std::vector<Aula>& solucao) const; // Janelas (horários vagos)
    double calcularPenalidade4(const std::vector<Aula>& solucao) const; // Preferências de horário
};