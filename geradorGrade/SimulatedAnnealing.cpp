#include "SimulatedAnnealing.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <chrono>

SimulatedAnnealing::SimulatedAnnealing(
    std::vector<Aula> solucaoInicial,
    std::vector<Professor> professores,
    std::vector<Disciplina> disciplinas,
    std::vector<Turma> turmas,
    std::vector<Sala> salas,
    std::set<std::tuple<int, int, int>> disponibilidadeProfessores,
    std::map<int, int> turmaSalaMap,
    int numIteracoes,
    double temperaturaInicial,
    double taxaResfriamento)
    : solucaoAtual(solucaoInicial), melhorSolucao(solucaoInicial),
      professores(professores), disciplinas(disciplinas),
      turmas(turmas), salas(salas),
      disponibilidadeProfessores(disponibilidadeProfessores),
      turmaSalaMap(turmaSalaMap),
      numIteracoes(numIteracoes),
      temperaturaInicial(temperaturaInicial),
      taxaResfriamento(taxaResfriamento),
      temperaturaAtual(temperaturaInicial),
      iteracoesSemMelhoria(0),
      movimentosAceitos(0),
      movimentosRejeitados(0),
      gen(std::chrono::steady_clock::now().time_since_epoch().count()),
      dis(0.0, 1.0)
{
    custoAtual = calcularCusto(solucaoAtual);
    melhorCusto = custoAtual;

    std::cout << "\n=== INICIANDO SIMULATED ANNEALING ===" << std::endl;
    std::cout << "Temperatura inicial: " << temperaturaInicial << std::endl;
    std::cout << "Taxa de resfriamento: " << taxaResfriamento << std::endl;
    std::cout << "Numero de iteracoes: " << numIteracoes << std::endl;
    std::cout << "Custo inicial: " << custoAtual << std::endl;
}

void SimulatedAnnealing::executar() {
    auto start = std::chrono::high_resolution_clock::now();

    for (int iter = 0; iter < numIteracoes; iter++) {
        // Gera uma solução vizinha
        std::vector<Aula> vizinho = gerarVizinho(solucaoAtual);

        // Verifica se a solução é viável
        if (!verificarViabilidade(vizinho)) {
            movimentosRejeitados++;
            continue;
        }

        // Calcula o custo da nova solução
        double custoVizinho = calcularCusto(vizinho);
        double deltaCusto = custoVizinho - custoAtual;

        // Decide se aceita o movimento
        if (aceitarMovimento(deltaCusto)) {
            solucaoAtual = vizinho;
            custoAtual = custoVizinho;
            movimentosAceitos++;

            // Atualiza a melhor solução encontrada
            if (custoAtual < melhorCusto) {
                melhorSolucao = solucaoAtual;
                melhorCusto = custoAtual;
                iteracoesSemMelhoria = 0;

                std::cout << "Iteracao " << iter << ": Novo melhor custo = "
                          << melhorCusto << " (T=" << temperaturaAtual << ")" << std::endl;
            } else {
                iteracoesSemMelhoria++;
            }
        } else {
            movimentosRejeitados++;
            iteracoesSemMelhoria++;
        }

        // Resfria a temperatura a cada N iterações
        if ((iter + 1) % 100 == 0) {
            temperaturaAtual *= taxaResfriamento;
        }

        // Mostra progresso
        if ((iter + 1) % 1000 == 0) {
            std::cout << "Progresso: " << (iter + 1) << "/" << numIteracoes
                      << " (T=" << std::fixed << std::setprecision(2) << temperaturaAtual
                      << ", Aceitos=" << movimentosAceitos
                      << ", Rejeitados=" << movimentosRejeitados << ")" << std::endl;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "\n=== SIMULATED ANNEALING CONCLUIDO ===" << std::endl;
    std::cout << "Tempo de execucao: " << duration.count() << " ms" << std::endl;
    std::cout << "Custo inicial: " << calcularCusto(melhorSolucao) << std::endl;
    std::cout << "Custo final: " << melhorCusto << std::endl;
    std::cout << "Melhoria: " << std::fixed << std::setprecision(2)
              << ((calcularCusto(melhorSolucao) - melhorCusto) / calcularCusto(melhorSolucao) * 100)
              << "%" << std::endl;
}

double SimulatedAnnealing::calcularCusto(const std::vector<Aula>& solucao) {
    // Função de custo baseada nas penalidades do modelo
    double custo = 0.0;

    // Pesos para cada penalidade (ajustáveis)
    const double peso1 = 1.0; // Distribuição por dia
    const double peso2 = 2.0; // Aulas consecutivas
    const double peso3 = 3.0; // Janelas
    const double peso4 = 0.5; // Preferências

    custo += peso1 * calcularPenalidade1(solucao);
    custo += peso2 * calcularPenalidade2(solucao);
    custo += peso3 * calcularPenalidade3(solucao);
    custo += peso4 * calcularPenalidade4(solucao);

    return custo;
}

double SimulatedAnnealing::calcularPenalidade1(const std::vector<Aula>& solucao) const {
    // Penaliza distribuição desigual de aulas por dia para cada turma
    double penalidade = 0.0;

    for (const auto& turma : turmas) {
        std::map<int, int> aulasPorDia;
        int totalAulas = 0;

        for (const auto& aula : solucao) {
            if (aula.idTurma == turma.id) {
                aulasPorDia[aula.slot.dia]++;
                totalAulas++;
            }
        }

        if (totalAulas == 0) continue;

        double mediaIdeal = totalAulas / 5.0;

        for (int dia = 0; dia < 5; dia++) {
            double desvio = std::abs(aulasPorDia[dia] - mediaIdeal);
            penalidade += desvio * desvio; // Penalidade quadrática
        }
    }

    return penalidade;
}

double SimulatedAnnealing::calcularPenalidade2(const std::vector<Aula>& solucao) const {
    // Bonifica aulas consecutivas da mesma disciplina
    double bonus = 0.0;

    for (const auto& turma : turmas) {
        for (int dia = 0; dia < 5; dia++) {
            std::vector<std::pair<int, int>> aulasNoDia; // (hora, idDisciplina)

            for (const auto& aula : solucao) {
                if (aula.idTurma == turma.id && aula.slot.dia == dia) {
                    aulasNoDia.push_back({aula.slot.hora, aula.idDisciplina});
                }
            }

            std::sort(aulasNoDia.begin(), aulasNoDia.end());

            // Verifica aulas consecutivas
            for (size_t i = 1; i < aulasNoDia.size(); i++) {
                if (aulasNoDia[i].first == aulasNoDia[i-1].first + 1 &&
                    aulasNoDia[i].second == aulasNoDia[i-1].second) {
                    bonus += 10.0; // Bônus por aulas consecutivas
                }
            }
        }
    }

    return -bonus; // Retorna negativo porque é um bônus
}

double SimulatedAnnealing::calcularPenalidade3(const std::vector<Aula>& solucao) const {
    // Penaliza janelas (horários vagos) no horário dos professores
    double penalidade = 0.0;

    for (const auto& prof : professores) {
        for (int dia = 0; dia < 5; dia++) {
            std::vector<int> horasOcupadas;

            for (const auto& aula : solucao) {
                if (aula.idProfessor == prof.id && aula.slot.dia == dia) {
                    horasOcupadas.push_back(aula.slot.hora);
                }
            }

            if (horasOcupadas.size() > 1) {
                std::sort(horasOcupadas.begin(), horasOcupadas.end());

                // Conta janelas
                for (size_t i = 1; i < horasOcupadas.size(); i++) {
                    int janela = horasOcupadas[i] - horasOcupadas[i-1] - 1;
                    if (janela > 0) {
                        penalidade += janela * 5.0; // Penaliza cada hora vaga
                    }
                }
            }
        }
    }

    return penalidade;
}

double SimulatedAnnealing::calcularPenalidade4(const std::vector<Aula>& solucao) const {
    // Penaliza aulas em horários extremos (primeiro e último)
    double penalidade = 0.0;

    for (const auto& aula : solucao) {
        if (aula.slot.hora == 0 || aula.slot.hora == 5) {
            penalidade += 2.0; // Penaliza levemente horários extremos
        }
    }

    return penalidade;
}

std::vector<Aula> SimulatedAnnealing::gerarVizinho(const std::vector<Aula>& solucao) {
    // Escolhe aleatoriamente um tipo de movimento
    std::uniform_int_distribution<> tipoMov(0, 2);
    int tipo = tipoMov(gen);

    switch (tipo) {
        case 0:
            return trocarHorario(solucao);
        case 1:
            return trocarDia(solucao);
        case 2:
            return trocarSlot(solucao);
        default:
            return solucao;
    }
}

std::vector<Aula> SimulatedAnnealing::trocarHorario(const std::vector<Aula>& solucao) {
    std::vector<Aula> nova = solucao;

    if (nova.size() < 2) return nova;

    // Seleciona duas aulas aleatórias do mesmo dia e turma
    std::uniform_int_distribution<> dist(0, nova.size() - 1);

    int tentativas = 0;
    while (tentativas < 100) {
        int i = dist(gen);
        int j = dist(gen);

        if (i != j &&
            nova[i].idTurma == nova[j].idTurma &&
            nova[i].slot.dia == nova[j].slot.dia) {

            // Troca os horários
            std::swap(nova[i].slot.hora, nova[j].slot.hora);
            return nova;
        }
        tentativas++;
    }

    return nova;
}

std::vector<Aula> SimulatedAnnealing::trocarDia(const std::vector<Aula>& solucao) {
    std::vector<Aula> nova = solucao;

    if (nova.empty()) return nova;

    std::uniform_int_distribution<> distAula(0, nova.size() - 1);
    std::uniform_int_distribution<> distDia(0, 4);

    int idx = distAula(gen);
    int novoDia = distDia(gen);

    // Tenta mudar o dia mantendo o mesmo horário
    nova[idx].slot.dia = novoDia;

    return nova;
}

std::vector<Aula> SimulatedAnnealing::trocarSlot(const std::vector<Aula>& solucao) {
    std::vector<Aula> nova = solucao;

    if (nova.empty()) return nova;

    std::uniform_int_distribution<> distAula(0, nova.size() - 1);
    std::uniform_int_distribution<> distDia(0, 4);
    std::uniform_int_distribution<> distHora(0, 5);

    int idx = distAula(gen);

    // Muda para um slot completamente diferente
    nova[idx].slot.dia = distDia(gen);
    nova[idx].slot.hora = distHora(gen);

    return nova;
}

bool SimulatedAnnealing::verificarViabilidade(const std::vector<Aula>& solucao) {
    // Verifica restrições hard

    for (size_t i = 0; i < solucao.size(); i++) {
        const auto& aula1 = solucao[i];

        // Verifica disponibilidade do professor
        if (disponibilidadeProfessores.find({aula1.idProfessor, aula1.slot.dia, aula1.slot.hora}) ==
            disponibilidadeProfessores.end()) {
            return false;
        }

        // Verifica conflitos
        for (size_t j = i + 1; j < solucao.size(); j++) {
            const auto& aula2 = solucao[j];

            if (aula1.slot.dia == aula2.slot.dia && aula1.slot.hora == aula2.slot.hora) {
                // Mesmo horário
                if (aula1.idProfessor == aula2.idProfessor) return false; // Professor em 2 lugares
                if (aula1.idTurma == aula2.idTurma) return false; // Turma em 2 aulas
                if (aula1.idSala == aula2.idSala) return false; // Sala com 2 turmas
            }
        }
    }

    return true;
}

bool SimulatedAnnealing::aceitarMovimento(double deltaCusto) {
    if (deltaCusto < 0) {
        return true; // Sempre aceita melhorias
    }

    // Probabilidade de aceitar pioras (equação de Boltzmann)
    double probabilidade = std::exp(-deltaCusto / temperaturaAtual);
    return dis(gen) < probabilidade;
}

void SimulatedAnnealing::mostrarEstatisticas() const {
    std::cout << "\n=== ESTATISTICAS DO SIMULATED ANNEALING ===" << std::endl;
    std::cout << "Movimentos aceitos: " << movimentosAceitos << std::endl;
    std::cout << "Movimentos rejeitados: " << movimentosRejeitados << std::endl;
    std::cout << "Taxa de aceitacao: " << std::fixed << std::setprecision(2)
              << (movimentosAceitos * 100.0 / (movimentosAceitos + movimentosRejeitados)) << "%" << std::endl;
    std::cout << "Iteracoes sem melhoria: " << iteracoesSemMelhoria << std::endl;
    std::cout << "Temperatura final: " << temperaturaAtual << std::endl;

    // Decompõe o custo final
    std::cout << "\nDecomposicao do custo final:" << std::endl;
    std::cout << "  Penalidade 1 (distribuicao): " << calcularPenalidade1(melhorSolucao) << std::endl;
    std::cout << "  Penalidade 2 (consecutivas): " << calcularPenalidade2(melhorSolucao) << std::endl;
    std::cout << "  Penalidade 3 (janelas): " << calcularPenalidade3(melhorSolucao) << std::endl;
    std::cout << "  Penalidade 4 (horarios): " << calcularPenalidade4(melhorSolucao) << std::endl;
}