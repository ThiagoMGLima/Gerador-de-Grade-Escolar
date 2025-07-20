#include "GeradorHorario.h"
#include <algorithm>
#include <random>
#include <limits>
#include <iomanip>
#include <iostream>

GeradorHorario::GeradorHorario(
    std::vector<Professor> profs, std::vector<Disciplina> disc,
    std::vector<Turma> turmas, std::vector<Sala> salas,
    std::vector<RequisicaoAlocacao> reqs,
    std::set<std::tuple<int, int, int>> disponibilidade,
    std::map<int, int> disponibilidadeTotalProf,
    std::map<int, int> turmaSalaMapping)  // NOVO parâmetro
    : professores(profs), disciplinas(disc), turmas(turmas), salas(salas),
      requisicoes(reqs), disponibilidadeProfessores(disponibilidade),
      disponibilidadeTotalProfessores(disponibilidadeTotalProf),
      turmaSalaMap(turmaSalaMapping)  // NOVO: inicializa o mapa
{
    // Inicializa mapas de nomes para facilitar debug
    for (const auto& p : professores) { mapaNomesProfessores[p.id] = p.nome; }
    for (const auto& d : disciplinas) { mapaNomesDisciplinas[d.id] = d.nome; mapaDisciplinas[d.id] = d; }
    for (const auto& t : turmas) { mapaNomesTurmas[t.id] = t.nome; }
    for (const auto& s : salas) { mapaNomesSalas[s.id] = s.nome; }
}

void GeradorHorario::reset() {
    gradeHoraria.clear();
}

float GeradorHorario::calcularCriticidade(int idProfessor, int aulasNecessarias) {
    int disponibilidade = disponibilidadeTotalProfessores.at(idProfessor);
    return (float)aulasNecessarias / disponibilidade;
}

void GeradorHorario::analisarCargaDeTrabalho(const std::vector<RequisicaoAlocacao>& requisicoes) {
    std::map<int, int> aulasPoeProfessor;
    for (const auto& req : requisicoes) {
        aulasPoeProfessor[req.idProfessor]++;
    }

    std::cout << "\n=== ANALISE DE CARGA DE TRABALHO ===" << std::endl;
    std::cout << std::left;

    // Ordena professores por criticidade
    std::vector<std::pair<int, float>> criticidadePorProfessor;
    for (const auto& [idProf, qtdAulas] : aulasPoeProfessor) {
        float criticidade = calcularCriticidade(idProf, qtdAulas);
        criticidadePorProfessor.push_back({idProf, criticidade});
    }

    std::sort(criticidadePorProfessor.begin(), criticidadePorProfessor.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    for (const auto& [idProf, criticidade] : criticidadePorProfessor) {
        int qtdAulas = aulasPoeProfessor[idProf];
        int disponibilidade = disponibilidadeTotalProfessores.at(idProf);

        std::cout << std::setw(20) << mapaNomesProfessores.at(idProf) << ": "
                  << std::setw(2) << qtdAulas << " aulas, "
                  << std::setw(2) << disponibilidade << " slots";

        if (criticidade > 0.8) {
            std::cout << " [CRITICO: ";
        } else if (criticidade > 0.6) {
            std::cout << " [ALTO: ";
        } else {
            std::cout << " [OK: ";
        }

        std::cout << std::fixed << std::setprecision(1)
                  << (criticidade * 100) << "%]" << std::endl;
    }
}

bool GeradorHorario::gerarHorario() {
    std::vector<RequisicaoAlocacao> requisicoesParaTentar = this->requisicoes;

    // Análise inicial da carga de trabalho
    analisarCargaDeTrabalho(requisicoesParaTentar);

    // Calcula criticidade para cada requisição
    std::map<int, int> aulasPoeProfessor;
    for (const auto& req : requisicoesParaTentar) {
        aulasPoeProfessor[req.idProfessor]++;
    }

    // Ordena por criticidade (professores mais críticos primeiro)
    std::sort(requisicoesParaTentar.begin(), requisicoesParaTentar.end(),
        [&](const RequisicaoAlocacao& a, const RequisicaoAlocacao& b) {
            float critA = calcularCriticidade(a.idProfessor, aulasPoeProfessor[a.idProfessor]);
            float critB = calcularCriticidade(b.idProfessor, aulasPoeProfessor[b.idProfessor]);

            // Se criticidade muito diferente, usa ela
            if (std::abs(critA - critB) > 0.1) {
                return critA > critB;
            }

            // Senão, usa disponibilidade absoluta como desempate
            return disponibilidadeTotalProfessores.at(a.idProfessor) <
                   disponibilidadeTotalProfessores.at(b.idProfessor);
        });

    // Adiciona aleatoriedade dentro de grupos com criticidade similar
    std::random_device rd;
    std::mt19937 g(rd());

    size_t inicio = 0;
    while (inicio < requisicoesParaTentar.size()) {
        size_t fim = inicio + 1;

        float critInicio = calcularCriticidade(requisicoesParaTentar[inicio].idProfessor,
                                              aulasPoeProfessor[requisicoesParaTentar[inicio].idProfessor]);

        while (fim < requisicoesParaTentar.size()) {
            float critFim = calcularCriticidade(requisicoesParaTentar[fim].idProfessor,
                                               aulasPoeProfessor[requisicoesParaTentar[fim].idProfessor]);
            if (std::abs(critInicio - critFim) > 0.1) break;
            fim++;
        }

        if (fim - inicio > 1) {
            std::shuffle(requisicoesParaTentar.begin() + inicio,
                        requisicoesParaTentar.begin() + fim, g);
        }

        inicio = fim;
    }

    std::cout << "\n=== INICIANDO ALOCACAO ===" << std::endl;
    std::cout << "Total de aulas a alocar: " << requisicoesParaTentar.size() << std::endl;

    // Tenta alocar todas as requisições
    int alocadas = 0;
    int falhas = 0;

    for (const auto& req : requisicoesParaTentar) {
        if (tentarAlocarRequisicao(req)) {
            alocadas++;
            // Mostra progresso a cada 10 alocações
            if (alocadas % 10 == 0) {
                std::cout << "." << std::flush;
            }
        } else {
            falhas++;
            std::cout << "\nFALHA CRITICA ao alocar "
                      << mapaNomesDisciplinas.at(req.idDisciplina)
                      << " para " << mapaNomesTurmas.at(req.idTurma)
                      << " com " << mapaNomesProfessores.at(req.idProfessor) << std::endl;
            return false;
        }
    }

    std::cout << "\n\nConstrucao finalizada: " << alocadas << " aulas alocadas com sucesso!" << std::endl;
    return true;
}

bool GeradorHorario::tentarAlocarRequisicao(const RequisicaoAlocacao& req) {
    std::vector<Slot> slotsDisponiveis;

    // Obtém a sala específica da turma
    auto it = turmaSalaMap.find(req.idTurma);
    if (it == turmaSalaMap.end()) {
        std::cerr << "ERRO: Turma " << req.idTurma << " nao tem sala associada!" << std::endl;
        return false;
    }
    int idSalaDaTurma = it->second;

    // Busca apenas slots disponíveis na sala da turma
    for (int dia = 0; dia < 5; ++dia) {
        for (int hora = 0; hora < 6; ++hora) {
            Slot slotAtual = { dia, hora };
            if (verificarDisponibilidade(req.idTurma, req.idProfessor, idSalaDaTurma, slotAtual)) {
                slotsDisponiveis.push_back(slotAtual);
            }
        }
    }

    if (slotsDisponiveis.empty()) {
        return false;
    }

    // Escolhe um slot aleatório entre os disponíveis
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<> dist(0, slotsDisponiveis.size() - 1);
    Slot slotEscolhido = slotsDisponiveis[dist(g)];

    // Cria a aula com a sala específica da turma
    Aula novaAula = { req.idProfessor, req.idDisciplina, req.idTurma, idSalaDaTurma, slotEscolhido };
    gradeHoraria.push_back(novaAula);

    return true;
}

bool GeradorHorario::verificarDisponibilidade(int idTurma, int idProfessor, int idSala, Slot slot) {
    // Verifica se o professor está disponível neste horário
    if (disponibilidadeProfessores.find({ idProfessor, slot.dia, slot.hora }) ==
        disponibilidadeProfessores.end()) {
        return false;
    }

    // Verifica conflitos na grade já construída
    for (const auto& aula : gradeHoraria) {
        if (aula.slot.dia == slot.dia && aula.slot.hora == slot.hora) {
            // Professor não pode estar em dois lugares ao mesmo tempo
            if (aula.idProfessor == idProfessor) {
                return false;
            }
            // Turma não pode ter duas aulas ao mesmo tempo
            if (aula.idTurma == idTurma) {
                return false;
            }
            // Sala não pode ser usada por duas turmas ao mesmo tempo
            if (aula.idSala == idSala) {
                return false;
            }
        }
    }

    return true;
}

std::vector<Aula> GeradorHorario::getGradeHoraria() {
    return this->gradeHoraria;
}

void GeradorHorario::mostrarEstatisticasGrade() {
    std::cout << "\n=== ESTATISTICAS DA GRADE FINAL ===" << std::endl;

    // Slots ocupados por turma
    std::map<int, int> slotsOcupadosPorTurma;
    for (const auto& aula : gradeHoraria) {
        slotsOcupadosPorTurma[aula.idTurma]++;
    }

    std::cout << "\nOcupacao por turma:" << std::endl;
    for (const auto& [idTurma, slots] : slotsOcupadosPorTurma) {
        std::cout << "  " << std::setw(10) << mapaNomesTurmas[idTurma]
                  << ": " << std::setw(2) << slots << "/30 slots ("
                  << std::fixed << std::setprecision(1)
                  << (slots * 100.0 / 30) << "%)" << std::endl;
    }

    // Distribuição por dia da semana
    std::map<int, int> aulasPorDia;
    for (const auto& aula : gradeHoraria) {
        aulasPorDia[aula.slot.dia]++;
    }

    const std::vector<std::string> diasNomes = {"Segunda", "Terca", "Quarta", "Quinta", "Sexta"};
    std::cout << "\nDistribuicao por dia:" << std::endl;
    for (int d = 0; d < 5; d++) {
        std::cout << "  " << std::setw(10) << diasNomes[d]
                  << ": " << std::setw(3) << aulasPorDia[d] << " aulas" << std::endl;
    }

    // Utilização de professores
    std::map<int, int> aulasAlocadasPorProfessor;
    for (const auto& aula : gradeHoraria) {
        aulasAlocadasPorProfessor[aula.idProfessor]++;
    }

    std::cout << "\nUtilizacao dos professores:" << std::endl;
    for (const auto& [idProf, aulas] : aulasAlocadasPorProfessor) {
        int disponivel = disponibilidadeTotalProfessores[idProf];
        float utilizacao = (float)aulas / disponivel * 100;
        std::cout << "  " << std::setw(20) << mapaNomesProfessores[idProf]
                  << ": " << std::setw(2) << aulas << "/" << std::setw(2) << disponivel
                  << " (" << std::fixed << std::setprecision(1) << utilizacao << "%)" << std::endl;
    }
}

void GeradorHorario::imprimirHorario() {
    std::cout << "\n=== GRADE HORARIA GERADA ===" << std::endl;
    const std::vector<std::string> diasNomes = { "Segunda", "Terca", "Quarta", "Quinta", "Sexta" };
    const std::vector<std::string> horariosNomes = { "7:30-8:15", "8:15-9:00", "9:00-9:45",
                                                    "10:05-10:50", "10:50-11:35", "11:35-12:20" };

    for (const auto& t : turmas) {
        std::cout << "\n============================== HORARIO: " << t.nome
                  << " ==============================\n";
        std::cout << std::left << std::setw(14) << "Horario";
        for (const auto& dia : diasNomes) {
            std::cout << std::setw(30) << dia;
        }
        std::cout << std::endl;
        std::cout << std::string(164, '-') << std::endl;

        for (int h = 0; h < (int)horariosNomes.size(); ++h) {
            std::cout << std::setw(14) << horariosNomes[h];
            for (int d = 0; d < (int)diasNomes.size(); ++d) {
                bool achou = false;
                for (const auto& aula : gradeHoraria) {
                    if (aula.idTurma == t.id && aula.slot.dia == d && aula.slot.hora == h) {
                        std::string nomeProf = mapaNomesProfessores.at(aula.idProfessor);
                        std::string nomeDisc = mapaNomesDisciplinas.at(aula.idDisciplina);
                        std::cout << std::setw(30) << (nomeDisc + " (" + nomeProf + ")");
                        achou = true;
                        break;
                    }
                }
                if (!achou) {
                    std::cout << std::setw(30) << "---";
                }
            }
            std::cout << std::endl;
        }
    }

    // Mostra estatísticas no final
    mostrarEstatisticasGrade();
}