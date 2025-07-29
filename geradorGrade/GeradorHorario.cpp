#include "GeradorHorario.h"
#include <algorithm>
#include <random>
#include <limits>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>

#include "json.hpp"

using json = nlohmann::json;

// Construtor melhorado
GeradorHorario::GeradorHorario(
    std::vector<Professor> profs, std::vector<Disciplina> disc,
    std::vector<Turma> turmas, std::vector<Sala> salas,
    std::vector<RequisicaoAlocacao> reqs,
    std::set<std::tuple<int, int, int>> disponibilidade,
    std::map<int, int> disponibilidadeTotalProf,
    std::map<int, int> turmaSalaMapping,
    ConfiguracaoGerador config)
    : professores(profs), disciplinas(disc), turmas(turmas), salas(salas),
      requisicoes(reqs), disponibilidadeProfessores(disponibilidade),
      disponibilidadeTotalProfessores(disponibilidadeTotalProf),
      turmaSalaMap(turmaSalaMapping), configuracao(config)
{
    // Inicializa mapas de nomes para facilitar debug e relatórios
    for (const auto& p : professores) {
        mapaNomesProfessores[p.id] = p.nome;
    }
    for (const auto& d : disciplinas) {
        mapaNomesDisciplinas[d.id] = d.nome;
        mapaDisciplinas[d.id] = d;
    }
    for (const auto& t : turmas) {
        mapaNomesTurmas[t.id] = t.nome;
    }
    for (const auto& s : salas) {
        mapaNomesSalas[s.id] = s.nome;
    }

    log("GeradorHorario inicializado com " + std::to_string(requisicoes.size()) + " requisições");
}

void GeradorHorario::reset() {
    gradeHoraria.clear();
    cacheDisponibilidade.clear();
    log("Grade horária resetada");
}

// Logging melhorado
void GeradorHorario::log(const std::string& mensagem, bool forcarExibicao) const {
    if (configuracao.verboso || forcarExibicao) {
        std::cout << "[INFO] " << mensagem << std::endl;
    }
}

void GeradorHorario::logErro(const std::string& mensagem) const {
    std::cerr << "[ERRO] " << mensagem << std::endl;
}

void GeradorHorario::logAviso(const std::string& mensagem) const {
    std::cout << "[AVISO] " << mensagem << std::endl;
}

// Cálculo de criticidade melhorado
float GeradorHorario::calcularCriticidade(int idProfessor, int aulasNecessarias) {
    int disponibilidade = disponibilidadeTotalProfessores.at(idProfessor);

    // Considera também as aulas já alocadas
    int aulasJaAlocadas = 0;
    for (const auto& aula : gradeHoraria) {
        if (aula.idProfessor == idProfessor) {
            aulasJaAlocadas++;
        }
    }

    int disponibilidadeRestante = disponibilidade - aulasJaAlocadas;
    if (disponibilidadeRestante <= 0) return 999.0f; // Criticidade máxima

    return (float)aulasNecessarias / disponibilidadeRestante;
}

// Análise de carga de trabalho melhorada
void GeradorHorario::analisarCargaDeTrabalho(const std::vector<RequisicaoAlocacao>& requisicoes) {
    std::map<int, int> aulasPoeProfessor;
    std::map<int, std::set<int>> turmasPorProfessor;

    for (const auto& req : requisicoes) {
        aulasPoeProfessor[req.idProfessor]++;
        turmasPorProfessor[req.idProfessor].insert(req.idTurma);
    }

    std::cout << "\n=== ANÁLISE DETALHADA DE CARGA DE TRABALHO ===" << std::endl;
    std::cout << std::left;

    // Ordena professores por criticidade
    std::vector<std::pair<int, float>> criticidadePorProfessor;
    for (const auto& [idProf, qtdAulas] : aulasPoeProfessor) {
        float criticidade = calcularCriticidade(idProf, qtdAulas);
        criticidadePorProfessor.push_back({idProf, criticidade});
    }

    std::sort(criticidadePorProfessor.begin(), criticidadePorProfessor.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Exibe análise detalhada
    for (const auto& [idProf, criticidade] : criticidadePorProfessor) {
        int qtdAulas = aulasPoeProfessor[idProf];
        int disponibilidade = disponibilidadeTotalProfessores.at(idProf);
        int numTurmas = turmasPorProfessor[idProf].size();

        std::cout << std::setw(25) << mapaNomesProfessores.at(idProf) << ": "
                  << std::setw(3) << qtdAulas << " aulas, "
                  << std::setw(3) << disponibilidade << " slots, "
                  << std::setw(2) << numTurmas << " turmas";

        if (criticidade > 0.8) {
            std::cout << " [CRÍTICO: ";
        } else if (criticidade > 0.6) {
            std::cout << " [ALTO: ";
        } else {
            std::cout << " [OK: ";
        }

        std::cout << std::fixed << std::setprecision(1)
                  << (criticidade * 100) << "%]" << std::endl;
    }

    std::cout << "\nResumo: "
              << requisicoes.size() << " aulas para alocar, "
              << professores.size() << " professores disponíveis" << std::endl;
}

// Obter slots ordenados por qualidade
std::vector<Slot> GeradorHorario::obterSlotsOrdenados(const RequisicaoAlocacao& req) {
    std::vector<std::pair<Slot, int>> slotsComPontuacao;

    auto it = turmaSalaMap.find(req.idTurma);
    if (it == turmaSalaMap.end()) {
        logErro("Turma " + std::to_string(req.idTurma) + " sem sala associada!");
        return {};
    }
    int idSalaDaTurma = it->second;

    // Avaliar todos os slots possíveis
    for (int dia = 0; dia < 5; ++dia) {
        for (int hora = 0; hora < 6; ++hora) {
            Slot slotAtual = { dia, hora };
            if (verificarDisponibilidade(req.idTurma, req.idProfessor, idSalaDaTurma, slotAtual)) {
                int pontuacao = calcularPontuacaoSlot(req, slotAtual);
                slotsComPontuacao.push_back({slotAtual, pontuacao});
            }
        }
    }

    // Ordenar por pontuação (maior = melhor)
    std::sort(slotsComPontuacao.begin(), slotsComPontuacao.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    // Extrair apenas os slots
    std::vector<Slot> slotsOrdenados;
    for (const auto& [slot, pontuacao] : slotsComPontuacao) {
        slotsOrdenados.push_back(slot);
    }

    return slotsOrdenados;
}

// Calcular pontuação de um slot (quanto maior, melhor)
int GeradorHorario::calcularPontuacaoSlot(const RequisicaoAlocacao& req, const Slot& slot) {
    int pontuacao = 100; // Base

    // Penalizar horários extremos
    if (configuracao.evitarAulasExtremos) {
        if (slot.hora == 0 || slot.hora == 5) {
            pontuacao -= 20;
        }
    }

    // Bonificar aulas consecutivas da mesma disciplina
    for (const auto& aula : gradeHoraria) {
        if (aula.idTurma == req.idTurma && aula.idDisciplina == req.idDisciplina) {
            if (aula.slot.dia == slot.dia) {
                if (abs(aula.slot.hora - slot.hora) == 1) {
                    pontuacao += 30; // Aula adjacente
                }
            }
        }
    }

    // Verificar distribuição semanal
    if (configuracao.distribuirAulasUniformemente) {
        int aulasNoDia = 0;
        for (const auto& aula : gradeHoraria) {
            if (aula.idTurma == req.idTurma && aula.slot.dia == slot.dia) {
                aulasNoDia++;
            }
        }
        // Penalizar dias muito carregados
        pontuacao -= aulasNoDia * 5;
    }

    // Verificar janelas do professor
    if (configuracao.priorizarMinimoJanelas) {
        int janelasAntes = contarJanelasHorario(req.idProfessor);

        // Simular adição temporária
        Aula aulaTemp = { req.idProfessor, req.idDisciplina, req.idTurma, 0, slot };
        gradeHoraria.push_back(aulaTemp);
        int janelasDepois = contarJanelasHorario(req.idProfessor);
        gradeHoraria.pop_back();

        if (janelasDepois > janelasAntes) {
            pontuacao -= (janelasDepois - janelasAntes) * 25;
        }
    }

    return pontuacao;
}

// Geração de horário melhorada
bool GeradorHorario::gerarHorario() {
    auto inicio = std::chrono::high_resolution_clock::now();

    std::vector<RequisicaoAlocacao> requisicoesParaTentar = this->requisicoes;

    // Análise inicial
    analisarCargaDeTrabalho(requisicoesParaTentar);

    // Calcula criticidade para cada requisição
    std::map<int, int> aulasPoeProfessor;
    for (const auto& req : requisicoesParaTentar) {
        aulasPoeProfessor[req.idProfessor]++;
    }

    // Ordena por criticidade com estratégia melhorada
    std::sort(requisicoesParaTentar.begin(), requisicoesParaTentar.end(),
        [&](const RequisicaoAlocacao& a, const RequisicaoAlocacao& b) {
            float critA = calcularCriticidade(a.idProfessor, aulasPoeProfessor[a.idProfessor]);
            float critB = calcularCriticidade(b.idProfessor, aulasPoeProfessor[b.idProfessor]);

            // Se criticidade muito diferente, usa ela
            if (std::abs(critA - critB) > 0.1) {
                return critA > critB;
            }

            // Priorizar disciplinas com mais aulas (para facilitar consecutivas)
            auto discA = mapaDisciplinas.find(a.idDisciplina);
            auto discB = mapaDisciplinas.find(b.idDisciplina);
            if (discA != mapaDisciplinas.end() && discB != mapaDisciplinas.end()) {
                int totalA = 0, totalB = 0;
                for (const auto& [turma, qtd] : discA->second.aulasPorTurma) {
                    totalA += qtd;
                }
                for (const auto& [turma, qtd] : discB->second.aulasPorTurma) {
                    totalB += qtd;
                }
                if (totalA != totalB) {
                    return totalA > totalB;
                }
            }

            // Senão, usa disponibilidade absoluta como desempate
            return disponibilidadeTotalProfessores.at(a.idProfessor) <
                   disponibilidadeTotalProfessores.at(b.idProfessor);
        });

    // Adiciona aleatoriedade controlada
    std::random_device rd;
    std::mt19937 g(rd());

    size_t inicio_grupo = 0;
    while (inicio_grupo < requisicoesParaTentar.size()) {
        size_t fim_grupo = inicio_grupo + 1;

        float critInicio = calcularCriticidade(requisicoesParaTentar[inicio_grupo].idProfessor,
                                              aulasPoeProfessor[requisicoesParaTentar[inicio_grupo].idProfessor]);

        while (fim_grupo < requisicoesParaTentar.size()) {
            float critFim = calcularCriticidade(requisicoesParaTentar[fim_grupo].idProfessor,
                                               aulasPoeProfessor[requisicoesParaTentar[fim_grupo].idProfessor]);
            if (std::abs(critInicio - critFim) > 0.1) break;
            fim_grupo++;
        }

        if (fim_grupo - inicio_grupo > 1) {
            std::shuffle(requisicoesParaTentar.begin() + inicio_grupo,
                        requisicoesParaTentar.begin() + fim_grupo, g);
        }

        inicio_grupo = fim_grupo;
    }

    std::cout << "\n=== INICIANDO ALOCAÇÃO INTELIGENTE ===" << std::endl;
    std::cout << "Total de aulas a alocar: " << requisicoesParaTentar.size() << std::endl;

    // Tenta alocar todas as requisições
    int alocadas = 0;
    int falhas = 0;
    std::map<StatusAlocacao, int> contagemStatus;

    for (size_t i = 0; i < requisicoesParaTentar.size(); i++) {
        const auto& req = requisicoesParaTentar[i];
        StatusAlocacao status = tentarAlocarRequisicao(req);

        contagemStatus[status]++;

        if (status == StatusAlocacao::SUCESSO) {
            alocadas++;
            // Callback de progresso
            if (callbackProgresso && i % 10 == 0) {
                callbackProgresso(i + 1, requisicoesParaTentar.size());
            }
            // Mostra progresso
            if (alocadas % 10 == 0) {
                std::cout << "." << std::flush;
            }
        } else {
            falhas++;
            if (configuracao.verboso) {
                std::cout << "\nFalha ao alocar "
                          << mapaNomesDisciplinas.at(req.idDisciplina)
                          << " para " << mapaNomesTurmas.at(req.idTurma)
                          << " com " << mapaNomesProfessores.at(req.idProfessor);

                switch (status) {
                    case StatusAlocacao::FALHA_SEM_SLOTS_DISPONIVEIS:
                        std::cout << " - Sem slots disponíveis";
                        break;
                    case StatusAlocacao::FALHA_PROFESSOR_INDISPONIVEL:
                        std::cout << " - Professor indisponível";
                        break;
                    case StatusAlocacao::FALHA_TURMA_OCUPADA:
                        std::cout << " - Turma ocupada";
                        break;
                    case StatusAlocacao::FALHA_SALA_OCUPADA:
                        std::cout << " - Sala ocupada";
                        break;
                    default:
                        std::cout << " - Erro desconhecido";
                }
                std::cout << std::endl;
            }

            // Se muitas falhas consecutivas, pode ser problema grave
            if (falhas > 10) {
                logErro("Muitas falhas consecutivas. Verificar dados de entrada.");
                return false;
            }
        }
    }

    auto fim = std::chrono::high_resolution_clock::now();
    auto duracao = std::chrono::duration_cast<std::chrono::milliseconds>(fim - inicio);

    std::cout << "\n\n=== RESULTADO DA ALOCAÇÃO ===" << std::endl;
    std::cout << "Aulas alocadas: " << alocadas << "/" << requisicoesParaTentar.size() << std::endl;
    std::cout << "Taxa de sucesso: " << std::fixed << std::setprecision(1)
              << (alocadas * 100.0 / requisicoesParaTentar.size()) << "%" << std::endl;
    std::cout << "Tempo de processamento: " << duracao.count() << "ms" << std::endl;

    // Estatísticas de falhas
    if (!contagemStatus.empty()) {
        std::cout << "\nDetalhamento de status:" << std::endl;
        for (const auto& [status, count] : contagemStatus) {
            if (status != StatusAlocacao::SUCESSO && count > 0) {
                std::cout << "  ";
                switch (status) {
                    case StatusAlocacao::FALHA_SEM_SLOTS_DISPONIVEIS:
                        std::cout << "Sem slots disponíveis: ";
                        break;
                    case StatusAlocacao::FALHA_PROFESSOR_INDISPONIVEL:
                        std::cout << "Professor indisponível: ";
                        break;
                    case StatusAlocacao::FALHA_TURMA_OCUPADA:
                        std::cout << "Turma ocupada: ";
                        break;
                    case StatusAlocacao::FALHA_SALA_OCUPADA:
                        std::cout << "Sala ocupada: ";
                        break;
                    default:
                        std::cout << "Outros: ";
                }
                std::cout << count << std::endl;
            }
        }
    }

    return alocadas == requisicoesParaTentar.size();
}

// Tentativa de alocação melhorada
StatusAlocacao GeradorHorario::tentarAlocarRequisicao(const RequisicaoAlocacao& req) {
    // Obtém a sala específica da turma
    auto it = turmaSalaMap.find(req.idTurma);
    if (it == turmaSalaMap.end()) {
        logErro("Turma " + std::to_string(req.idTurma) + " não tem sala associada!");
        return StatusAlocacao::FALHA_SALA_OCUPADA;
    }
    int idSalaDaTurma = it->second;

    // Busca slots ordenados por qualidade
    std::vector<Slot> slotsOrdenados = obterSlotsOrdenados(req);

    if (slotsOrdenados.empty()) {
        return StatusAlocacao::FALHA_SEM_SLOTS_DISPONIVEIS;
    }

    // Tenta alocar no melhor slot disponível
    for (const auto& slot : slotsOrdenados) {
        // Verificação dupla (cache pode estar desatualizado)
        if (verificarDisponibilidade(req.idTurma, req.idProfessor, idSalaDaTurma, slot)) {
            // Cria a aula
            Aula novaAula = { req.idProfessor, req.idDisciplina, req.idTurma, idSalaDaTurma, slot };
            gradeHoraria.push_back(novaAula);

            // Invalida cache relevante
            cacheDisponibilidade.clear();

            return StatusAlocacao::SUCESSO;
        }
    }

    // Analisa por que falhou
    if (!slotsOrdenados.empty()) {
        // Tinha slots mas nenhum funcionou - provavelmente conflito
        for (const auto& aula : gradeHoraria) {
            if (aula.idProfessor == req.idProfessor) {
                return StatusAlocacao::FALHA_PROFESSOR_INDISPONIVEL;
            }
            if (aula.idTurma == req.idTurma) {
                return StatusAlocacao::FALHA_TURMA_OCUPADA;
            }
        }
        return StatusAlocacao::FALHA_SALA_OCUPADA;
    }

    return StatusAlocacao::FALHA_SEM_SLOTS_DISPONIVEIS;
}

// Verificação de disponibilidade com cache
bool GeradorHorario::verificarDisponibilidade(int idTurma, int idProfessor, int idSala, Slot slot) {
    // Verifica cache primeiro
    auto chaveCache = std::make_tuple(idProfessor, slot.dia, slot.hora);
    auto itCache = cacheDisponibilidade.find(chaveCache);
    if (itCache != cacheDisponibilidade.end() && !itCache->second) {
        return false; // Já sabemos que não está disponível
    }

    // Verifica se o professor está disponível neste horário
    if (disponibilidadeProfessores.find({ idProfessor, slot.dia, slot.hora }) ==
        disponibilidadeProfessores.end()) {
        cacheDisponibilidade[chaveCache] = false;
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

    cacheDisponibilidade[chaveCache] = true;
    return true;
}
// Contar janelas de horário de um professor
int GeradorHorario::contarJanelasHorario(int idProfessor) const {
    int totalJanelas = 0;

    for (int dia = 0; dia < 5; dia++) {
        std::vector<int> horariosNoDia;

        for (const auto& aula : gradeHoraria) {
            if (aula.idProfessor == idProfessor && aula.slot.dia == dia) {
                horariosNoDia.push_back(aula.slot.hora);
            }
        }

        if (horariosNoDia.size() > 1) {
            std::sort(horariosNoDia.begin(), horariosNoDia.end());

            for (size_t i = 1; i < horariosNoDia.size(); i++) {
                int janela = horariosNoDia[i] - horariosNoDia[i-1] - 1;
                if (janela > 0) {
                    totalJanelas += janela;
                }
            }
        }
    }

    return totalJanelas;
}

// Contar aulas consecutivas de uma disciplina
int GeradorHorario::contarAulasConsecutivas(int idTurma, int idDisciplina) const {
    int totalConsecutivas = 0;

    for (int dia = 0; dia < 5; dia++) {
        std::vector<int> horariosNoDia;

        for (const auto& aula : gradeHoraria) {
            if (aula.idTurma == idTurma && aula.idDisciplina == idDisciplina && aula.slot.dia == dia) {
                horariosNoDia.push_back(aula.slot.hora);
            }
        }

        if (horariosNoDia.size() > 1) {
            std::sort(horariosNoDia.begin(), horariosNoDia.end());

            for (size_t i = 1; i < horariosNoDia.size(); i++) {
                if (horariosNoDia[i] - horariosNoDia[i-1] == 1) {
                    totalConsecutivas++;
                }
            }
        }
    }

    return totalConsecutivas;
}

// Obter distribuição semanal de uma turma
std::map<int, std::vector<int>> GeradorHorario::obterDistribuicaoSemanal(int idTurma) const {
    std::map<int, std::vector<int>> distribuicao;

    for (int dia = 0; dia < 5; dia++) {
        distribuicao[dia] = std::vector<int>();
    }

    for (const auto& aula : gradeHoraria) {
        if (aula.idTurma == idTurma) {
            distribuicao[aula.slot.dia].push_back(aula.slot.hora);
        }
    }

    // Ordenar horários de cada dia
    for (auto& [dia, horarios] : distribuicao) {
        std::sort(horarios.begin(), horarios.end());
    }

    return distribuicao;
}

// Obter estatísticas detalhadas
EstatisticasGrade GeradorHorario::obterEstatisticasDetalhadas() const {
    EstatisticasGrade stats;
    stats.totalAulas = requisicoes.size();
    stats.aulasAlocadas = gradeHoraria.size();
    stats.janelasHorario = 0;
    stats.conflitos = 0;

    // Aulas por turma
    for (const auto& aula : gradeHoraria) {
        stats.aulasPorTurma[aula.idTurma]++;
        stats.aulasPorProfessor[aula.idProfessor]++;
        stats.aulasPorDia[aula.slot.dia]++;
    }

    // Janelas de horário total
    for (const auto& prof : professores) {
        stats.janelasHorario += contarJanelasHorario(prof.id);
    }

    // Ocupação de salas
    std::map<int, std::set<std::pair<int, int>>> slotsOcupadosPorSala;
    for (const auto& aula : gradeHoraria) {
        slotsOcupadosPorSala[aula.idSala].insert({aula.slot.dia, aula.slot.hora});
    }

    for (const auto& sala : salas) {
        float ocupacao = slotsOcupadosPorSala[sala.id].size() / 30.0f * 100;
        stats.ocupacaoSalas[sala.id] = ocupacao;
    }

    return stats;
}

// Validação completa da grade
bool GeradorHorario::validarGradeCompleta() const {
    // Verifica se todas as requisições foram atendidas
    std::map<std::tuple<int, int, int>, int> contagemRequisicoes;

    for (const auto& req : requisicoes) {
        contagemRequisicoes[{req.idTurma, req.idDisciplina, req.idProfessor}]++;
    }

    for (const auto& aula : gradeHoraria) {
        auto chave = std::make_tuple(aula.idTurma, aula.idDisciplina, aula.idProfessor);
        if (contagemRequisicoes.find(chave) != contagemRequisicoes.end()) {
            contagemRequisicoes[chave]--;
        }
    }

    for (const auto& [chave, count] : contagemRequisicoes) {
        if (count != 0) {
            return false;
        }
    }

    // Verifica conflitos
    return verificarConflitosProfessor() &&
           verificarConflitosTurma() &&
           verificarConflitosSala();
}

// Verificar conflitos de professor
bool GeradorHorario::verificarConflitosProfessor() const {
    std::map<std::tuple<int, int, int>, int> ocupacaoProfessor;

    for (const auto& aula : gradeHoraria) {
        auto chave = std::make_tuple(aula.idProfessor, aula.slot.dia, aula.slot.hora);
        ocupacaoProfessor[chave]++;

        if (ocupacaoProfessor[chave] > 1) {
            logErro("Professor " + mapaNomesProfessores.at(aula.idProfessor) +
                   " tem conflito no dia " + std::to_string(aula.slot.dia) +
                   " hora " + std::to_string(aula.slot.hora));
            return false;
        }
    }

    return true;
}

// Verificar conflitos de turma
bool GeradorHorario::verificarConflitosTurma() const {
    std::map<std::tuple<int, int, int>, int> ocupacaoTurma;

    for (const auto& aula : gradeHoraria) {
        auto chave = std::make_tuple(aula.idTurma, aula.slot.dia, aula.slot.hora);
        ocupacaoTurma[chave]++;

        if (ocupacaoTurma[chave] > 1) {
            logErro("Turma " + mapaNomesTurmas.at(aula.idTurma) +
                   " tem conflito no dia " + std::to_string(aula.slot.dia) +
                   " hora " + std::to_string(aula.slot.hora));
            return false;
        }
    }

    return true;
}

// Verificar conflitos de sala
bool GeradorHorario::verificarConflitosSala() const {
    std::map<std::tuple<int, int, int>, std::vector<int>> ocupacaoSala;

    for (const auto& aula : gradeHoraria) {
        auto chave = std::make_tuple(aula.idSala, aula.slot.dia, aula.slot.hora);
        ocupacaoSala[chave].push_back(aula.idTurma);

        // Verificar se é sala compartilhada
        bool salaCompartilhada = false;
        for (const auto& sala : salas) {
            if (sala.id == aula.idSala && sala.compartilhada) {
                salaCompartilhada = true;
                break;
            }
        }

        if (!salaCompartilhada && ocupacaoSala[chave].size() > 1) {
            logErro("Sala " + mapaNomesSalas.at(aula.idSala) +
                   " (não compartilhada) tem conflito no dia " + std::to_string(aula.slot.dia) +
                   " hora " + std::to_string(aula.slot.hora));
            return false;
        }
    }

    return true;
}

// Obter lista de problemas da grade
std::vector<std::string> GeradorHorario::obterProblemasGrade() const {
    std::vector<std::string> problemas;

    // Verificar alocação completa
    if (gradeHoraria.size() < requisicoes.size()) {
        problemas.push_back("Nem todas as aulas foram alocadas: " +
                           std::to_string(gradeHoraria.size()) + "/" +
                           std::to_string(requisicoes.size()));
    }

    // Verificar conflitos
    if (!verificarConflitosProfessor()) {
        problemas.push_back("Existem conflitos de horário para professores");
    }

    if (!verificarConflitosTurma()) {
        problemas.push_back("Existem conflitos de horário para turmas");
    }

    if (!verificarConflitosSala()) {
        problemas.push_back("Existem conflitos de uso de salas");
    }

    // Verificar janelas excessivas
    for (const auto& prof : professores) {
        int janelas = contarJanelasHorario(prof.id);
        if (janelas > 5) {
            problemas.push_back("Professor " + prof.nome + " tem " +
                               std::to_string(janelas) + " janelas de horário");
        }
    }

    // Verificar distribuição desigual
    auto stats = obterEstatisticasDetalhadas();
    for (const auto& [dia, qtd] : stats.aulasPorDia) {
        if (qtd > stats.aulasAlocadas / 5 * 1.3) {
            problemas.push_back("Dia " + std::to_string(dia) +
                               " está sobrecarregado com " + std::to_string(qtd) + " aulas");
        }
    }

    return problemas;
}

// Mostrar estatísticas melhoradas
void GeradorHorario::mostrarEstatisticasGrade() {
    auto stats = obterEstatisticasDetalhadas();

    std::cout << "\n=== ESTATÍSTICAS DETALHADAS DA GRADE ===" << std::endl;

    // Taxa de alocação
    std::cout << "\nTaxa de Alocação: " << stats.aulasAlocadas << "/" << stats.totalAulas
              << " (" << std::fixed << std::setprecision(1)
              << (stats.aulasAlocadas * 100.0 / stats.totalAulas) << "%)" << std::endl;

    // Ocupação por turma
    std::cout << "\nOcupação por turma:" << std::endl;
    for (const auto& [idTurma, qtd] : stats.aulasPorTurma) {
        std::cout << "  " << std::setw(15) << mapaNomesTurmas[idTurma]
                  << ": " << std::setw(2) << qtd << "/30 slots ("
                  << std::fixed << std::setprecision(1)
                  << (qtd * 100.0 / 30) << "%)" << std::endl;
    }

    // Distribuição por dia
    const std::vector<std::string> diasNomes = {"Segunda", "Terça", "Quarta", "Quinta", "Sexta"};
    std::cout << "\nDistribuição por dia:" << std::endl;

    int maxAulas = 0;
    for (const auto& [dia, qtd] : stats.aulasPorDia) {
        if (qtd > maxAulas) maxAulas = qtd;
    }

    for (int d = 0; d < 5; d++) {
        int qtd = stats.aulasPorDia[d];
        std::cout << "  " << std::setw(10) << diasNomes[d]
                  << ": " << std::setw(3) << qtd << " aulas ";

        // Gráfico de barras
        int barSize = (qtd * 40) / maxAulas;
        std::cout << "[";
        for (int i = 0; i < barSize; i++) std::cout << "=";
        for (int i = barSize; i < 40; i++) std::cout << " ";
        std::cout << "]" << std::endl;
    }

    // Utilização de professores
    std::cout << "\nUtilização dos professores:" << std::endl;
    std::vector<std::pair<std::string, float>> utilizacaoProfessores;

    for (const auto& [idProf, aulas] : stats.aulasPorProfessor) {
        int disponivel = disponibilidadeTotalProfessores[idProf];
        float utilizacao = (float)aulas / disponivel * 100;
        utilizacaoProfessores.push_back({mapaNomesProfessores[idProf], utilizacao});
    }

    // Ordenar por utilização
    std::sort(utilizacaoProfessores.begin(), utilizacaoProfessores.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    for (const auto& [nome, utilizacao] : utilizacaoProfessores) {
        std::cout << "  " << std::setw(20) << nome
                  << ": " << std::fixed << std::setprecision(1) << std::setw(5)
                  << utilizacao << "% ";

        // Indicador visual
        if (utilizacao > 80) {
            std::cout << "[ALTO]";
        } else if (utilizacao < 40) {
            std::cout << "[BAIXO]";
        }
        std::cout << std::endl;
    }

    // Qualidade da grade
    std::cout << "\nQualidade da Grade:" << std::endl;
    std::cout << "  Janelas de horário totais: " << stats.janelasHorario << std::endl;

    int totalConsecutivas = 0;
    for (const auto& turma : turmas) {
        for (const auto& disc : disciplinas) {
            totalConsecutivas += contarAulasConsecutivas(turma.id, disc.id);
        }
    }
    std::cout << "  Aulas consecutivas: " << totalConsecutivas << std::endl;

    // Problemas encontrados
    auto problemas = obterProblemasGrade();
    if (!problemas.empty()) {
        std::cout << "\n⚠️  Problemas detectados:" << std::endl;
        for (const auto& problema : problemas) {
            std::cout << "  - " << problema << std::endl;
        }
    } else {
        std::cout << "\n✅ Nenhum problema detectado na grade!" << std::endl;
    }
}

// Exportar para JSON melhorado
void GeradorHorario::exportarJSON(const std::string& nomeArquivo) const {
    std::ofstream arquivo(nomeArquivo);

    if (!arquivo.is_open()) {
        logErro("Erro ao criar arquivo JSON: " + nomeArquivo);
        return;
    }

    auto stats = obterEstatisticasDetalhadas();
    json j; // Objeto JSON principal

    // Metadata
    j["metadata"]["versao"] = "2.0";
    j["metadata"]["geradoEm"] = obterDataHoraAtual();
    j["metadata"]["totalAulas"] = stats.totalAulas;
    j["metadata"]["aulasAlocadas"] = stats.aulasAlocadas;
    j["metadata"]["taxaSucesso"] = (stats.totalAulas > 0) ? (stats.aulasAlocadas * 100.0 / stats.totalAulas) : 0.0;
    j["metadata"]["turmas"] = json::array();
    for (const auto& t : turmas) j["metadata"]["turmas"].push_back(t.nome);
    j["metadata"]["dias"] = {"Segunda", "Terça", "Quarta", "Quinta", "Sexta"};
    j["metadata"]["horarios"] = {"7:30-8:15", "8:15-9:00", "9:00-9:45", "10:05-10:50", "10:50-11:35", "11:35-12:20"};

    // Aulas
    j["aulas"] = json::array();
    for (size_t i = 0; i < gradeHoraria.size(); i++) {
        const auto& aula = gradeHoraria[i];
        json j_aula;
        j_aula["id"] = i + 1;
        j_aula["turma"] = mapaNomesTurmas.at(aula.idTurma);
        j_aula["turmaId"] = aula.idTurma;
        j_aula["disciplina"] = mapaNomesDisciplinas.at(aula.idDisciplina);
        j_aula["disciplinaId"] = aula.idDisciplina;
        j_aula["professor"] = mapaNomesProfessores.at(aula.idProfessor);
        j_aula["professorId"] = aula.idProfessor;
        j_aula["sala"] = mapaNomesSalas.at(aula.idSala);
        j_aula["salaId"] = aula.idSala;
        j_aula["dia"] = aula.slot.dia;
        j_aula["diaNome"] = getDiaNome(aula.slot.dia);
        j_aula["hora"] = aula.slot.hora;
        j_aula["horarioInicio"] = getHorarioInicio(aula.slot.hora);
        j_aula["horarioFim"] = getHorarioFim(aula.slot.hora);
        j["aulas"].push_back(j_aula);
    }

    // Estatísticas
    for (const auto& [idTurma, qtd] : stats.aulasPorTurma) {
        j["estatisticas"]["aulasPorTurma"][mapaNomesTurmas.at(idTurma)] = qtd;
    }
    for (const auto& [idProf, qtd] : stats.aulasPorProfessor) {
        j["estatisticas"]["aulasPorProfessor"][mapaNomesProfessores.at(idProf)] = qtd;
    }
    for (int d = 0; d < 5; d++) {
        j["estatisticas"]["aulasPorDia"][getDiaNome(d)] = stats.aulasPorDia.count(d) ? stats.aulasPorDia.at(d) : 0;
    }
    j["estatisticas"]["qualidade"]["janelasHorario"] = stats.janelasHorario;
    j["estatisticas"]["qualidade"]["conflitos"] = stats.conflitos;

    // Escreve o JSON formatado no arquivo (indentação de 2 espaços)
    arquivo << std::setw(2) << j << std::endl;

    arquivo.close();
    log("Grade exportada para: " + nomeArquivo, true);
}
// Funções auxiliares para exportação
std::string GeradorHorario::obterDataHoraAtual() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string GeradorHorario::getDiaNome(int dia) const {
    const std::vector<std::string> dias = {"Segunda", "Terça", "Quarta", "Quinta", "Sexta"};
    return (dia >= 0 && dia < 5) ? dias[dia] : "Inválido";
}

std::string GeradorHorario::getHorarioInicio(int hora) const {
    const std::vector<std::string> horarios = {"7:30", "8:15", "9:00", "10:05", "10:50", "11:35"};
    return (hora >= 0 && hora < 6) ? horarios[hora] : "Inválido";
}

std::string GeradorHorario::getHorarioFim(int hora) const {
    const std::vector<std::string> horarios = {"8:15", "9:00", "9:45", "10:50", "11:35", "12:20"};
    return (hora >= 0 && hora < 6) ? horarios[hora] : "Inválido";
}

// Exportar para CSV
void GeradorHorario::exportarCSV(const std::string& nomeArquivo) const {
    std::ofstream arquivo(nomeArquivo);

    if (!arquivo.is_open()) {
        logErro("Erro ao criar arquivo CSV: " + nomeArquivo);
        return;
    }

    // Cabeçalho
    arquivo << "Turma,Disciplina,Professor,Sala,Dia,Horário\n";

    // Dados
    for (const auto& aula : gradeHoraria) {
        arquivo << mapaNomesTurmas.at(aula.idTurma) << ","
                << mapaNomesDisciplinas.at(aula.idDisciplina) << ","
                << mapaNomesProfessores.at(aula.idProfessor) << ","
                << mapaNomesSalas.at(aula.idSala) << ","
                << getDiaNome(aula.slot.dia) << ","
                << getHorarioInicio(aula.slot.hora) << "-" << getHorarioFim(aula.slot.hora) << "\n";
    }

    arquivo.close();
    log("Grade exportada para CSV: " + nomeArquivo, true);
}

// Imprimir horário melhorado
void GeradorHorario::imprimirHorario() {
    std::cout << "\n=== GRADE HORÁRIA GERADA ===" << std::endl;
    const std::vector<std::string> diasNomes = { "Segunda", "Terça", "Quarta", "Quinta", "Sexta" };
    const std::vector<std::string> horariosNomes = { "7:30-8:15", "8:15-9:00", "9:00-9:45",
                                                    "10:05-10:50", "10:50-11:35", "11:35-12:20" };

    for (const auto& t : turmas) {
        std::cout << "\n" << std::string(80, '=') << std::endl;
        std::cout << "HORÁRIO: " << t.nome << std::endl;
        std::cout << std::string(80, '=') << std::endl;

        std::cout << std::left << std::setw(14) << "Horário";
        for (const auto& dia : diasNomes) {
            std::cout << std::setw(25) << dia;
        }
        std::cout << std::endl;
        std::cout << std::string(139, '-') << std::endl;

        for (int h = 0; h < (int)horariosNomes.size(); ++h) {
            std::cout << std::setw(14) << horariosNomes[h];
            for (int d = 0; d < (int)diasNomes.size(); ++d) {
                bool achou = false;
                for (const auto& aula : gradeHoraria) {
                    if (aula.idTurma == t.id && aula.slot.dia == d && aula.slot.hora == h) {
                        std::string nomeProf = mapaNomesProfessores.at(aula.idProfessor);
                        std::string nomeDisc = mapaNomesDisciplinas.at(aula.idDisciplina);

                        // Truncar nomes longos
                        if (nomeDisc.length() > 12) nomeDisc = nomeDisc.substr(0, 11) + ".";
                        if (nomeProf.length() > 10) nomeProf = nomeProf.substr(0, 9) + ".";

                        std::cout << std::setw(25) << (nomeDisc + " (" + nomeProf + ")");
                        achou = true;
                        break;
                    }
                }
                if (!achou) {
                    std::cout << std::setw(25) << "---";
                }
            }
            std::cout << std::endl;
        }

        // Mini estatística por turma
        int totalAulasTurma = 0;
        std::map<int, int> aulasPorDisciplina;
        for (const auto& aula : gradeHoraria) {
            if (aula.idTurma == t.id) {
                totalAulasTurma++;
                aulasPorDisciplina[aula.idDisciplina]++;
            }
        }

        std::cout << "\nResumo: " << totalAulasTurma << " aulas alocadas" << std::endl;
    }

    // Mostra estatísticas no final
    mostrarEstatisticasGrade();
}