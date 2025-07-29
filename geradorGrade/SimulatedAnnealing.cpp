#include "SimulatedAnnealing.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <fstream>
#include <numeric>
#include <sstream>

// Construtor
SimulatedAnnealing::SimulatedAnnealing(
    std::vector<Aula> solucaoInicial,
    std::vector<Professor> professores,
    std::vector<Disciplina> disciplinas,
    std::vector<Turma> turmas,
    std::vector<Sala> salas,
    std::set<std::tuple<int, int, int>> disponibilidadeProfessores,
    std::map<int, int> turmaSalaMap,
    ConfiguracaoSA config)
    : solucaoAtual(solucaoInicial), melhorSolucao(solucaoInicial),
      professores(professores), disciplinas(disciplinas),
      turmas(turmas), salas(salas),
      disponibilidadeProfessores(disponibilidadeProfessores),
      turmaSalaMap(turmaSalaMap),
      config(config),
      temperaturaAtual(config.temperaturaInicial),
      executando(false),
      gen(std::chrono::steady_clock::now().time_since_epoch().count()),
      dis(0.0, 1.0),
      disMovimento(0, 6) // 7 tipos de movimento
{
    // Calcular custo inicial
    estatisticas.custoInicial = calcularCusto(solucaoAtual);
    estatisticas.custoFinal = estatisticas.custoInicial;
    melhorCusto = estatisticas.custoInicial;

    log("Simulated Annealing inicializado");
    log("Custo inicial: " + std::to_string(estatisticas.custoInicial));
    log("Temperatura inicial: " + std::to_string(config.temperaturaInicial));
}

// Logging
void SimulatedAnnealing::log(const std::string& mensagem) const {
    if (config.verboso) {
        std::cout << "[SA] " << mensagem << std::endl;
    }
}

// Formatar tempo
std::string SimulatedAnnealing::formatarTempo(double segundos) const {
    if (segundos < 60) {
        return std::to_string(static_cast<int>(segundos)) + "s";
    } else if (segundos < 3600) {
        int minutos = static_cast<int>(segundos / 60);
        int segs = static_cast<int>(segundos) % 60;
        return std::to_string(minutos) + "m " + std::to_string(segs) + "s";
    } else {
        int horas = static_cast<int>(segundos / 3600);
        int minutos = static_cast<int>((segundos - horas * 3600) / 60);
        return std::to_string(horas) + "h " + std::to_string(minutos) + "m";
    }
}

// Execução principal
void SimulatedAnnealing::executar() {
    executarComCallback(nullptr);
}

void SimulatedAnnealing::executarComCallback(std::function<void(int, double, double)> callback) {
    auto inicio = std::chrono::high_resolution_clock::now();
    executando = true;

    std::cout << "\n=== INICIANDO SIMULATED ANNEALING ===" << std::endl;
    std::cout << "Configuração:" << std::endl;
    std::cout << "  Iterações: " << config.numIteracoes << std::endl;
    std::cout << "  Temperatura inicial: " << config.temperaturaInicial << std::endl;
    std::cout << "  Taxa de resfriamento: " << config.taxaResfriamento << std::endl;
    std::cout << "  Usar reaquecimento: " << (config.usarReaquecimento ? "Sim" : "Não") << std::endl;
    std::cout << "  Usar memória tabu: " << (config.usarMemoriaTabu ? "Sim" : "Não") << std::endl;

    int iteracoesSemMelhoria = 0;
    int iteracoesDesdeRelatorio = 0;

    for (int iter = 0; iter < config.numIteracoes && executando; iter++) {
        // Gerar movimento
        Movimento movimento = selecionarMovimento();

        // Verificar se não é tabu
        if (config.usarMemoriaTabu && movimentoTabu(movimento)) {
            estatisticas.movimentosRejeitados++;
            continue;
        }

        // Gerar solução vizinha
        std::vector<Aula> vizinho = gerarVizinho(solucaoAtual);

        // Verificar viabilidade rápida
        if (!verificarViabilidadeRapida(vizinho, movimento)) {
            estatisticas.movimentosRejeitados++;
            continue;
        }

        // Calcular custo
        double custoVizinho = calcularCusto(vizinho);
        double deltaCusto = custoVizinho - estatisticas.custoFinal;

        // Decidir se aceita
        if (aceitarMovimento(deltaCusto)) {
            solucaoAtual = vizinho;
            estatisticas.custoFinal = custoVizinho;
            estatisticas.movimentosAceitos++;

            if (deltaCusto < 0) {
                estatisticas.movimentosMelhoria++;
            } else {
                estatisticas.movimentosPiora++;
            }

            // Adicionar à lista tabu
            if (config.usarMemoriaTabu) {
                adicionarTabu(movimento);
            }

            // Atualizar melhor solução
            if (custoVizinho < melhorCusto) {
                melhorSolucao = solucaoAtual;
                melhorCusto = custoVizinho;
                estatisticas.iteracaoMelhorCusto = iter;
                iteracoesSemMelhoria = 0;

                log("Iteração " + std::to_string(iter) +
                    ": Novo melhor custo = " + std::to_string(melhorCusto) +
                    " (T=" + std::to_string(temperaturaAtual) + ")");
            } else {
                iteracoesSemMelhoria++;
            }
        } else {
            estatisticas.movimentosRejeitados++;
            iteracoesSemMelhoria++;
        }

        // Atualizar temperatura
        if ((iter + 1) % 100 == 0) {
            atualizarTemperatura(iter);
        }

        // Verificar reaquecimento
        if (config.usarReaquecimento && criterioReaquecimento(iteracoesSemMelhoria)) {
            reaquecerTemperatura();
            iteracoesSemMelhoria = 0;
            estatisticas.reaquecimentos++;
        }

        // Registrar estatísticas
        if ((iter + 1) % 100 == 0) {
            registrarEstatistica(iter);
        }

        // Callback de progresso
        if (callback && (iter + 1) % 10 == 0) {
            callback(iter + 1, temperaturaAtual, estatisticas.custoFinal);
        }

        // Mostrar progresso
        iteracoesDesdeRelatorio++;
        if (iteracoesDesdeRelatorio >= config.frequenciaRelatorio) {
            double taxaAceitacao = estatisticas.getTaxaAceitacao();
            std::cout << "Progresso: " << std::setw(6) << (iter + 1)
                      << "/" << config.numIteracoes
                      << " | T=" << std::fixed << std::setprecision(2) << std::setw(7) << temperaturaAtual
                      << " | Custo=" << std::setw(10) << estatisticas.custoFinal
                      << " | Melhor=" << std::setw(10) << melhorCusto
                      << " | Taxa=" << std::setw(5) << taxaAceitacao << "%"
                      << std::endl;
            iteracoesDesdeRelatorio = 0;
        }
    }

    auto fim = std::chrono::high_resolution_clock::now();
    auto duracao = std::chrono::duration_cast<std::chrono::milliseconds>(fim - inicio);
    estatisticas.tempoExecucao = duracao.count() / 1000.0;

    // Aplicar busca local final para refinar
    if (estatisticas.custoFinal < estatisticas.custoInicial * 1.5) { // Só se a solução não for muito ruim
        log("Aplicando busca local final...");
        melhorSolucao = buscaLocal2opt(melhorSolucao);
        melhorSolucao = buscaLocalJanelas(melhorSolucao);
        melhorCusto = calcularCusto(melhorSolucao);
    }

    std::cout << "\n=== SIMULATED ANNEALING CONCLUÍDO ===" << std::endl;
    std::cout << "Tempo de execução: " << formatarTempo(estatisticas.tempoExecucao) << std::endl;
    std::cout << "Custo inicial: " << estatisticas.custoInicial << std::endl;
    std::cout << "Custo final: " << melhorCusto << std::endl;
    std::cout << "Melhoria: " << std::fixed << std::setprecision(2)
              << estatisticas.getPercentualMelhoria() << "%" << std::endl;
}

// Cálculo de custo total
double SimulatedAnnealing::calcularCusto(const std::vector<Aula>& solucao) {
    // Verificar cache
    size_t hashSolucao = 0;
    for (const auto& aula : solucao) {
        hashSolucao ^= aula.getHash();
    }

    auto it = cacheCusto.find(hashSolucao);
    if (it != cacheCusto.end()) {
        return it->second;
    }

    // Calcular custo
    double custo = 0.0;

    custo += config.pesoDistribuicao * calcularPenalidade1(solucao);
    custo += config.pesoConsecutivas * calcularPenalidade2(solucao);
    custo += config.pesoJanelas * calcularPenalidade3(solucao);
    custo += config.pesoHorariosExtremos * calcularPenalidade4(solucao);
    custo += config.pesoPreferencias * calcularPenalidade5(solucao);

    // Armazenar no cache
    if (cacheCusto.size() > 1000) {
        cacheCusto.clear(); // Limpar cache se ficar muito grande
    }
    cacheCusto[hashSolucao] = custo;

    return custo;
}

// Penalidade 1: Distribuição desigual de aulas por dia
double SimulatedAnnealing::calcularPenalidade1(const std::vector<Aula>& solucao) const {
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

        // Calcular média ideal e desvio
        double mediaIdeal = totalAulas / 5.0;
        double variancia = 0.0;

        for (int dia = 0; dia < 5; dia++) {
            double desvio = aulasPorDia[dia] - mediaIdeal;
            variancia += desvio * desvio;
        }

        penalidade += std::sqrt(variancia / 5.0);

        // Penalizar dias muito carregados
        for (const auto& [dia, qtd] : aulasPorDia) {
            if (qtd > 7) { // Mais de 7 aulas em um dia é excessivo
                penalidade += (qtd - 7) * 10;
            }
            if (qtd == 0) { // Dia sem aulas também não é ideal
                penalidade += 5;
            }
        }
    }

    return penalidade;
}

// Penalidade 2: Falta de aulas consecutivas (na verdade é bônus)
double SimulatedAnnealing::calcularPenalidade2(const std::vector<Aula>& solucao) const {
    double bonus = 0.0;

    // Analisar por turma e dia
    for (const auto& turma : turmas) {
        for (int dia = 0; dia < 5; dia++) {
            std::map<int, std::vector<int>> aulasPorDisciplina;

            for (const auto& aula : solucao) {
                if (aula.idTurma == turma.id && aula.slot.dia == dia) {
                    aulasPorDisciplina[aula.idDisciplina].push_back(aula.slot.hora);
                }
            }

            // Verificar aulas consecutivas por disciplina
            for (auto& [discId, horarios] : aulasPorDisciplina) {
                if (horarios.size() < 2) continue;

                std::sort(horarios.begin(), horarios.end());

                // Contar sequências consecutivas
                int sequenciaAtual = 1;
                for (size_t i = 1; i < horarios.size(); i++) {
                    if (horarios[i] == horarios[i-1] + 1) {
                        sequenciaAtual++;
                    } else {
                        // Bonificar sequências (quanto maior, melhor)
                        if (sequenciaAtual >= 2) {
                            bonus += sequenciaAtual * sequenciaAtual * 5;
                        }
                        sequenciaAtual = 1;
                    }
                }
                // Verificar última sequência
                if (sequenciaAtual >= 2) {
                    bonus += sequenciaAtual * sequenciaAtual * 5;
                }
            }
        }
    }

    return -bonus; // Retorna negativo porque é bônus
}

// Penalidade 3: Janelas de horário dos professores
double SimulatedAnnealing::calcularPenalidade3(const std::vector<Aula>& solucao) const {
    double penalidade = 0.0;

    auto janelasMap = obterJanelasPorProfessor(solucao);

    for (const auto& [profId, janelas] : janelasMap) {
        int totalJanelas = std::accumulate(janelas.begin(), janelas.end(), 0);

        // Penalidade progressiva
        if (totalJanelas <= 2) {
            penalidade += totalJanelas * 5;
        } else if (totalJanelas <= 5) {
            penalidade += 10 + (totalJanelas - 2) * 10;
        } else {
            penalidade += 40 + (totalJanelas - 5) * 20;
        }

        // Penalizar janelas longas
        for (int janela : janelas) {
            if (janela >= 3) {
                penalidade += janela * janela * 2;
            }
        }
    }

    return penalidade;
}

// Penalidade 4: Horários extremos
double SimulatedAnnealing::calcularPenalidade4(const std::vector<Aula>& solucao) const {
    double penalidade = 0.0;

    for (const auto& aula : solucao) {
        if (aula.slot.isHorarioExtremo()) {
            penalidade += 3;

            // Penalizar mais se for disciplina importante
            auto it = std::find_if(disciplinas.begin(), disciplinas.end(),
                [&](const Disciplina& d) { return d.id == aula.idDisciplina; });

            if (it != disciplinas.end() && it->getCargaHorariaTotal() >= 20) {
                penalidade += 2; // Disciplinas com muitas aulas devem evitar extremos
            }
        }

        // Penalizar aulas após intervalo longo (após 10:05)
        if (aula.slot.hora >= 3) {
            penalidade += 1;
        }
    }

    return penalidade;
}

// Penalidade 5: Preferências não atendidas
double SimulatedAnnealing::calcularPenalidade5(const std::vector<Aula>& solucao) const {
    double penalidade = 0.0;

    // Verificar preferências de horário das disciplinas
    for (const auto& aula : solucao) {
        auto it = std::find_if(disciplinas.begin(), disciplinas.end(),
            [&](const Disciplina& d) { return d.id == aula.idDisciplina; });

        if (it != disciplinas.end() && !it->horariosPreferidos.empty()) {
            if (it->horariosPreferidos.find(aula.slot.hora) == it->horariosPreferidos.end()) {
                penalidade += 5;
            }
        }
    }

    // Verificar turno das turmas
    for (const auto& aula : solucao) {
        auto turma = std::find_if(turmas.begin(), turmas.end(),
            [&](const Turma& t) { return t.id == aula.idTurma; });

        if (turma != turmas.end()) {
            // Penalizar aulas fora do turno preferido
            if (turma->turno == Turno::MANHA && aula.slot.hora >= 4) {
                penalidade += 10;
            } else if (turma->turno == Turno::TARDE && aula.slot.hora < 2) {
                penalidade += 10;
            }
        }
    }

    return penalidade;
}

// Obter janelas por professor
std::map<int, std::vector<int>> SimulatedAnnealing::obterJanelasPorProfessor(
    const std::vector<Aula>& solucao) const {

    std::map<int, std::vector<int>> janelas;

    for (const auto& prof : professores) {
        janelas[prof.id] = std::vector<int>();

        for (int dia = 0; dia < 5; dia++) {
            std::vector<int> horariosNoDia;

            for (const auto& aula : solucao) {
                if (aula.idProfessor == prof.id && aula.slot.dia == dia) {
                    horariosNoDia.push_back(aula.slot.hora);
                }
            }

            if (horariosNoDia.size() > 1) {
                std::sort(horariosNoDia.begin(), horariosNoDia.end());

                for (size_t i = 1; i < horariosNoDia.size(); i++) {
                    int janela = horariosNoDia[i] - horariosNoDia[i-1] - 1;
                    if (janela > 0) {
                        janelas[prof.id].push_back(janela);
                    }
                }
            }
        }
    }

    return janelas;
}

// Seleção de movimento
Movimento SimulatedAnnealing::selecionarMovimento() {
    // Selecionar tipo de movimento com probabilidades ajustadas
    int tipo = disMovimento(gen);

    // Ajustar probabilidades baseado na temperatura
    if (temperaturaAtual < config.temperaturaInicial * 0.3) {
        // Em temperaturas baixas, preferir movimentos locais
        if (tipo > 3) tipo = gen() % 4;
    }

    Movimento movimento(static_cast<TipoMovimento>(tipo));

    // Adicionar parâmetros específicos do movimento
    switch (movimento.tipo) {
        case TipoMovimento::TROCAR_HORARIO:
        case TipoMovimento::TROCAR_DIA:
        case TipoMovimento::TROCAR_SLOT:
            movimento.parametros.push_back(gen() % solucaoAtual.size());
            break;

        case TipoMovimento::TROCAR_AULAS:
            movimento.parametros.push_back(gen() % solucaoAtual.size());
            movimento.parametros.push_back(gen() % solucaoAtual.size());
            break;

        case TipoMovimento::MOVER_BLOCO:
            movimento.parametros.push_back(gen() % turmas.size());
            movimento.parametros.push_back(gen() % disciplinas.size());
            break;

        case TipoMovimento::OTIMIZAR_PROFESSOR:
            movimento.parametros.push_back(gen() % professores.size());
            break;

        case TipoMovimento::OTIMIZAR_TURMA:
            movimento.parametros.push_back(gen() % turmas.size());
            break;
    }

    movimento.calcularHash();
    return movimento;
}

// Verificar se movimento é tabu
bool SimulatedAnnealing::movimentoTabu(const Movimento& mov) const {
    return std::find(listaTabu.begin(), listaTabu.end(), mov) != listaTabu.end();
}

// Adicionar movimento à lista tabu
void SimulatedAnnealing::adicionarTabu(const Movimento& mov) {
    listaTabu.push_back(mov);
    if (listaTabu.size() > static_cast<size_t>(config.tamanhoListaTabu)) {
        listaTabu.pop_front();
    }
}

// Gerar vizinho
std::vector<Aula> SimulatedAnnealing::gerarVizinho(const std::vector<Aula>& solucao) {
    Movimento movimento = selecionarMovimento();

    switch (movimento.tipo) {
        case TipoMovimento::TROCAR_HORARIO:
            return trocarHorario(solucao);
        case TipoMovimento::TROCAR_DIA:
            return trocarDia(solucao);
        case TipoMovimento::TROCAR_SLOT:
            return trocarSlot(solucao);
        case TipoMovimento::TROCAR_AULAS:
            return trocarAulas(solucao);
        case TipoMovimento::MOVER_BLOCO:
            return moverBloco(solucao);
        case TipoMovimento::OTIMIZAR_PROFESSOR:
            return otimizarProfessor(solucao);
        case TipoMovimento::OTIMIZAR_TURMA:
            return otimizarTurma(solucao);
        default:
            return solucao;
    }
}

// Implementação dos movimentos
std::vector<Aula> SimulatedAnnealing::trocarHorario(const std::vector<Aula>& solucao) {
    if (solucao.empty()) return solucao;

    std::vector<Aula> nova = solucao;
    std::uniform_int_distribution<> distAula(0, nova.size() - 1);
    std::uniform_int_distribution<> distHora(0, 5);

    int idx = distAula(gen);
    int novoHorario = distHora(gen);

    // Tentar mudar apenas o horário
    Slot novoSlot = nova[idx].slot;
    novoSlot.hora = novoHorario;
    nova[idx].slot = novoSlot;

    return nova;
}

std::vector<Aula> SimulatedAnnealing::trocarDia(const std::vector<Aula>& solucao) {
    if (solucao.empty()) return solucao;

    std::vector<Aula> nova = solucao;
    std::uniform_int_distribution<> distAula(0, nova.size() - 1);
    std::uniform_int_distribution<> distDia(0, 4);

    int idx = distAula(gen);
    int novoDia = distDia(gen);

    // Tentar mudar apenas o dia
    Slot novoSlot = nova[idx].slot;
    novoSlot.dia = novoDia;
    nova[idx].slot = novoSlot;

    return nova;
}

std::vector<Aula> SimulatedAnnealing::trocarSlot(const std::vector<Aula>& solucao) {
    if (solucao.empty()) return solucao;

    std::vector<Aula> nova = solucao;
    std::uniform_int_distribution<> distAula(0, nova.size() - 1);
    std::uniform_int_distribution<> distDia(0, 4);
    std::uniform_int_distribution<> distHora(0, 5);

    int idx = distAula(gen);

    // Mudar dia e horário
    Slot novoSlot(distDia(gen), distHora(gen));
    nova[idx].slot = novoSlot;

    return nova;
}

std::vector<Aula> SimulatedAnnealing::trocarAulas(const std::vector<Aula>& solucao) {
    if (solucao.size() < 2) return solucao;

    std::vector<Aula> nova = solucao;
    std::uniform_int_distribution<> distAula(0, nova.size() - 1);

    int idx1 = distAula(gen);
    int idx2 = distAula(gen);

    if (idx1 != idx2) {
        // Trocar os slots das duas aulas
        std::swap(nova[idx1].slot, nova[idx2].slot);
    }

    return nova;
}

std::vector<Aula> SimulatedAnnealing::moverBloco(const std::vector<Aula>& solucao) {
    if (solucao.empty()) return solucao;

    std::vector<Aula> nova = solucao;

    // Selecionar turma e disciplina aleatórias
    std::uniform_int_distribution<> distTurma(0, turmas.size() - 1);
    std::uniform_int_distribution<> distDisc(0, disciplinas.size() - 1);

    int idTurma = turmas[distTurma(gen)].id;
    int idDisciplina = disciplinas[distDisc(gen)].id;

    // Encontrar aulas da turma/disciplina
    std::vector<size_t> indices;
    for (size_t i = 0; i < nova.size(); i++) {
        if (nova[i].idTurma == idTurma && nova[i].idDisciplina == idDisciplina) {
            indices.push_back(i);
        }
    }

    if (indices.size() >= 2) {
        // Tentar mover para horários consecutivos
        std::uniform_int_distribution<> distDia(0, 4);
        std::uniform_int_distribution<> distHoraInicio(0, 6 - indices.size());

        int novoDia = distDia(gen);
        int horaInicio = distHoraInicio(gen);

        for (size_t i = 0; i < indices.size(); i++) {
            nova[indices[i]].slot.dia = novoDia;
            nova[indices[i]].slot.hora = horaInicio + i;
        }
    }

    return nova;
}

std::vector<Aula> SimulatedAnnealing::otimizarProfessor(const std::vector<Aula>& solucao) {
    if (solucao.empty() || professores.empty()) return solucao;

    std::vector<Aula> nova = solucao;
    std::uniform_int_distribution<> distProf(0, professores.size() - 1);

    int idProfessor = professores[distProf(gen)].id;

    // Coletar aulas do professor
    std::vector<size_t> indicesProf;
    for (size_t i = 0; i < nova.size(); i++) {
        if (nova[i].idProfessor == idProfessor) {
            indicesProf.push_back(i);
        }
    }

    if (indicesProf.size() < 2) return nova;

    // Tentar compactar horário do professor
    std::map<int, std::vector<size_t>> aulasPorDia;
    for (size_t idx : indicesProf) {
        aulasPorDia[nova[idx].slot.dia].push_back(idx);
    }

    // Para cada dia com aulas, tentar compactar
    for (auto& [dia, indices] : aulasPorDia) {
        if (indices.size() < 2) continue;

        // Ordenar por horário
        std::sort(indices.begin(), indices.end(),
            [&](size_t a, size_t b) {
                return nova[a].slot.hora < nova[b].slot.hora;
            });

        // Compactar removendo janelas
        int horaAtual = nova[indices[0]].slot.hora;
        for (size_t i = 1; i < indices.size(); i++) {
            if (nova[indices[i]].slot.hora > horaAtual + 1) {
                nova[indices[i]].slot.hora = horaAtual + 1;
            }
            horaAtual = nova[indices[i]].slot.hora;
        }
    }

    return nova;
}

std::vector<Aula> SimulatedAnnealing::otimizarTurma(const std::vector<Aula>& solucao) {
    if (solucao.empty() || turmas.empty()) return solucao;

    std::vector<Aula> nova = solucao;
    std::uniform_int_distribution<> distTurma(0, turmas.size() - 1);

    int idTurma = turmas[distTurma(gen)].id;

    // Implementação similar à otimização de professor, mas para turma
    // ... (código similar ao otimizarProfessor mas para turma)

    return nova;
}

// Verificação de viabilidade completa
bool SimulatedAnnealing::verificarViabilidade(const std::vector<Aula>& solucao) {
    // Verificar conflitos de professor
    std::map<std::tuple<int, int, int>, int> ocupacaoProfessor;

    for (const auto& aula : solucao) {
        // Verificar disponibilidade do professor
        if (disponibilidadeProfessores.find({aula.idProfessor, aula.slot.dia, aula.slot.hora}) ==
            disponibilidadeProfessores.end()) {
            return false;
        }

        // Verificar conflitos
        auto chave = std::make_tuple(aula.idProfessor, aula.slot.dia, aula.slot.hora);
        ocupacaoProfessor[chave]++;
        if (ocupacaoProfessor[chave] > 1) {
            return false;
        }
    }

    // Verificar conflitos de turma
    std::map<std::tuple<int, int, int>, int> ocupacaoTurma;
    for (const auto& aula : solucao) {
        auto chave = std::make_tuple(aula.idTurma, aula.slot.dia, aula.slot.hora);
        ocupacaoTurma[chave]++;
        if (ocupacaoTurma[chave] > 1) {
            return false;
        }
    }

    // Verificar conflitos de sala
    std::map<std::tuple<int, int, int>, std::vector<int>> ocupacaoSala;
    for (const auto& aula : solucao) {
        auto chave = std::make_tuple(aula.idSala, aula.slot.dia, aula.slot.hora);
        ocupacaoSala[chave].push_back(aula.idTurma);

        // Verificar se é sala compartilhada
        auto sala = std::find_if(salas.begin(), salas.end(),
            [&](const Sala& s) { return s.id == aula.idSala; });

        if (sala != salas.end() && !sala->compartilhada && ocupacaoSala[chave].size() > 1) {
            return false;
        }
    }

    return true;
}

// Verificação de viabilidade rápida (incremental)
bool SimulatedAnnealing::verificarViabilidadeRapida(const std::vector<Aula>& solucao,
                                                   const Movimento& movimento) {
    // Implementação otimizada que verifica apenas as mudanças
    // causadas pelo movimento específico

    switch (movimento.tipo) {
        case TipoMovimento::TROCAR_HORARIO:
        case TipoMovimento::TROCAR_DIA:
        case TipoMovimento::TROCAR_SLOT: {
            if (movimento.parametros.empty()) return false;
            int idx = movimento.parametros[0];
            if (idx >= static_cast<int>(solucao.size())) return false;

            const auto& aula = solucao[idx];

            // Verificar disponibilidade do professor
            if (disponibilidadeProfessores.find({aula.idProfessor, aula.slot.dia, aula.slot.hora}) ==
                disponibilidadeProfessores.end()) {
                return false;
            }

            // Verificar conflitos no novo slot
            for (size_t i = 0; i < solucao.size(); i++) {
                if (i == static_cast<size_t>(idx)) continue;

                const auto& outraAula = solucao[i];
                if (outraAula.slot.dia == aula.slot.dia &&
                    outraAula.slot.hora == aula.slot.hora) {

                    if (outraAula.idProfessor == aula.idProfessor ||
                        outraAula.idTurma == aula.idTurma ||
                        outraAula.idSala == aula.idSala) {
                        return false;
                    }
                }
            }
            break;
        }

        case TipoMovimento::TROCAR_AULAS: {
            if (movimento.parametros.size() < 2) return false;
            // Verificar viabilidade das duas trocas
            // ... implementação específica
            break;
        }

        default:
            // Para movimentos complexos, fazer verificação completa
            return verificarViabilidade(solucao);
    }

    return true;
}

// Validação completa da solução
ResultadoValidacao SimulatedAnnealing::validarSolucaoCompleta(const std::vector<Aula>& solucao) {
    ResultadoValidacao resultado;

    // Verificar conflitos básicos
    if (!verificarViabilidade(solucao)) {
        resultado.adicionarErro("Solução contém conflitos de horário");
    }

    // Verificar completude
    std::map<std::tuple<int, int, int>, int> aulasEsperadas;
    std::map<std::tuple<int, int, int>, int> aulasReais;

    // Contar aulas esperadas
    for (const auto& disc : disciplinas) {
        for (const auto& [turmaId, qtd] : disc.aulasPorTurma) {
            // Assumir que cada disciplina tem um professor
            int profId = 0;
            for (const auto& prof : professores) {
                // Encontrar professor da disciplina
                // ... lógica para mapear professor-disciplina
            }
            aulasEsperadas[{turmaId, disc.id, profId}] = qtd;
        }
    }

    // Contar aulas reais
    for (const auto& aula : solucao) {
        aulasReais[{aula.idTurma, aula.idDisciplina, aula.idProfessor}]++;
    }

    // Comparar
    for (const auto& [chave, qtdEsperada] : aulasEsperadas) {
        int qtdReal = aulasReais[chave];
        if (qtdReal < qtdEsperada) {
            resultado.adicionarAviso("Faltam " + std::to_string(qtdEsperada - qtdReal) +
                                   " aulas para alguma turma/disciplina");
        }
    }

    // Estatísticas
    resultado.estatisticas["totalAulas"] = solucao.size();
    resultado.estatisticas["janelasTotal"] = 0;

    auto janelasMap = obterJanelasPorProfessor(solucao);
    for (const auto& [profId, janelas] : janelasMap) {
        resultado.estatisticas["janelasTotal"] +=
            std::accumulate(janelas.begin(), janelas.end(), 0);
    }

    return resultado;
}

// Aceitar movimento (critério de Metropolis)
bool SimulatedAnnealing::aceitarMovimento(double deltaCusto) {
    if (deltaCusto < 0) {
        return true; // Sempre aceita melhorias
    }

    // Probabilidade de aceitar piora
    double probabilidade = std::exp(-deltaCusto / temperaturaAtual);
    return dis(gen) < probabilidade;
}

// Atualizar temperatura
void SimulatedAnnealing::atualizarTemperatura(int iteracao) {
    // Resfriamento geométrico
    temperaturaAtual *= config.taxaResfriamento;

    // Garantir temperatura mínima
    if (temperaturaAtual < config.temperaturaMinima) {
        temperaturaAtual = config.temperaturaMinima;
    }

    // Resfriamento adaptativo (opcional)
    if (estatisticas.getTaxaAceitacao() < 20 && temperaturaAtual > config.temperaturaMinima) {
        // Se taxa de aceitação muito baixa, resfriar mais devagar
        temperaturaAtual /= config.taxaResfriamento;
        temperaturaAtual *= std::pow(config.taxaResfriamento, 0.5);
    }
}

// Critério de reaquecimento
bool SimulatedAnnealing::criterioReaquecimento(int iteracoesSemMelhoria) {
    // Reaquece se estagnar por muito tempo
    if (iteracoesSemMelhoria > config.numIteracoes / 20) {
        return true;
    }

    // Reaquece se taxa de aceitação muito baixa
    if (estatisticas.getTaxaAceitacao() < 5 &&
        temperaturaAtual < config.temperaturaInicial * 0.1) {
        return true;
    }

    return false;
}

// Reaquecimento
void SimulatedAnnealing::reaquecerTemperatura() {
    double novaTemperatura = temperaturaAtual * 10;

    // Não exceder temperatura inicial
    if (novaTemperatura > config.temperaturaInicial) {
        novaTemperatura = config.temperaturaInicial * 0.5;
    }

    temperaturaAtual = novaTemperatura;

    log("Reaquecimento aplicado. Nova temperatura: " + std::to_string(temperaturaAtual));

    // Limpar lista tabu para permitir exploração
    listaTabu.clear();
}

// Busca local 2-opt
std::vector<Aula> SimulatedAnnealing::buscaLocal2opt(const std::vector<Aula>& solucao) {
    std::vector<Aula> melhorLocal = solucao;
    double melhorCustoLocal = calcularCusto(melhorLocal);
    bool melhorou = true;

    while (melhorou) {
        melhorou = false;

        // Tentar todas as trocas de pares
        for (size_t i = 0; i < melhorLocal.size() - 1; i++) {
            for (size_t j = i + 1; j < melhorLocal.size(); j++) {
                // Criar solução com troca
                std::vector<Aula> vizinho = melhorLocal;
                std::swap(vizinho[i].slot, vizinho[j].slot);

                // Verificar viabilidade
                if (!verificarViabilidade(vizinho)) continue;

                // Calcular custo
                double custoVizinho = calcularCusto(vizinho);

                if (custoVizinho < melhorCustoLocal) {
                    melhorLocal = vizinho;
                    melhorCustoLocal = custoVizinho;
                    melhorou = true;
                    break;
                }
            }
            if (melhorou) break;
        }
    }

    return melhorLocal;
}

// Busca local focada em reduzir janelas
std::vector<Aula> SimulatedAnnealing::buscaLocalJanelas(const std::vector<Aula>& solucao) {
    std::vector<Aula> melhorLocal = solucao;
    double melhorCustoLocal = calcularCusto(melhorLocal);

    // Para cada professor com janelas
    auto janelasMap = obterJanelasPorProfessor(melhorLocal);

    for (const auto& [profId, janelas] : janelasMap) {
        if (janelas.empty()) continue;

        // Tentar compactar horário do professor
        std::vector<Aula> tentativa = melhorLocal;
        bool modificou = false;

        // Coletar aulas do professor por dia
        std::map<int, std::vector<size_t>> aulasPorDia;
        for (size_t i = 0; i < tentativa.size(); i++) {
            if (tentativa[i].idProfessor == profId) {
                aulasPorDia[tentativa[i].slot.dia].push_back(i);
            }
        }

        // Compactar cada dia
        for (auto& [dia, indices] : aulasPorDia) {
            if (indices.size() < 2) continue;

            // Ordenar por horário
            std::sort(indices.begin(), indices.end(),
                [&](size_t a, size_t b) {
                    return tentativa[a].slot.hora < tentativa[b].slot.hora;
                });

            // Tentar eliminar janelas
            for (size_t i = 1; i < indices.size(); i++) {
                int horaAnterior = tentativa[indices[i-1]].slot.hora;
                int horaAtual = tentativa[indices[i]].slot.hora;

                if (horaAtual > horaAnterior + 1) {
                    // Há janela, tentar mover
                    tentativa[indices[i]].slot.hora = horaAnterior + 1;
                    modificou = true;
                }
            }
        }

        if (modificou && verificarViabilidade(tentativa)) {
            double custoTentativa = calcularCusto(tentativa);
            if (custoTentativa < melhorCustoLocal) {
                melhorLocal = tentativa;
                melhorCustoLocal = custoTentativa;
            }
        }
    }

    return melhorLocal;
}

// Registrar estatísticas
void SimulatedAnnealing::registrarEstatistica(int iteracao) {
    // Registrar no histórico (com amostragem para não usar muita memória)
    if (iteracao % 100 == 0) {
        estatisticas.historicoCusto.push_back(estatisticas.custoFinal);
        estatisticas.historicoTemperatura.push_back(temperaturaAtual);
        estatisticas.historicoTaxaAceitacao.push_back(estatisticas.getTaxaAceitacao());
    }
}

// Mostrar estatísticas finais
void SimulatedAnnealing::mostrarEstatisticas() const {
    std::cout << "\n=== ESTATÍSTICAS DO SIMULATED ANNEALING ===" << std::endl;
    std::cout << "Tempo de execução: " << formatarTempo(estatisticas.tempoExecucao) << std::endl;
    std::cout << "Movimentos aceitos: " << estatisticas.movimentosAceitos << std::endl;
    std::cout << "Movimentos rejeitados: " << estatisticas.movimentosRejeitados << std::endl;
    std::cout << "Taxa de aceitação: " << std::fixed << std::setprecision(2)
              << estatisticas.getTaxaAceitacao() << "%" << std::endl;
    std::cout << "Movimentos de melhoria: " << estatisticas.movimentosMelhoria << std::endl;
    std::cout << "Movimentos de piora aceitos: " << estatisticas.movimentosPiora << std::endl;
    std::cout << "Reaquecimentos: " << estatisticas.reaquecimentos << std::endl;
    std::cout << "Melhor custo encontrado na iteração: " << estatisticas.iteracaoMelhorCusto << std::endl;

    // Decomposição do custo final
    std::cout << "\nDecomposição do custo final (" << melhorCusto << "):" << std::endl;
    std::cout << "  Distribuição: " << std::fixed << std::setprecision(2)
              << config.pesoDistribuicao * calcularPenalidade1(melhorSolucao) << std::endl;
    std::cout << "  Consecutivas: " << config.pesoConsecutivas * calcularPenalidade2(melhorSolucao) << std::endl;
    std::cout << "  Janelas: " << config.pesoJanelas * calcularPenalidade3(melhorSolucao) << std::endl;
    std::cout << "  Horários extremos: " << config.pesoHorariosExtremos * calcularPenalidade4(melhorSolucao) << std::endl;
    std::cout << "  Preferências: " << config.pesoPreferencias * calcularPenalidade5(melhorSolucao) << std::endl;

    // Qualidade da solução
    auto qualidade = analisarQualidadeSolucao();
    std::cout << "\nQualidade da solução:" << std::endl;
    for (const auto& [metrica, valor] : qualidade) {
        std::cout << "  " << metrica << ": " << valor << std::endl;
    }
}

// Analisar qualidade da solução
std::map<std::string, double> SimulatedAnnealing::analisarQualidadeSolucao() const {
    std::map<std::string, double> metricas;

    // Total de janelas
    auto janelasMap = obterJanelasPorProfessor(melhorSolucao);
    int totalJanelas = 0;
    int maxJanelasPorProf = 0;

    for (const auto& [profId, janelas] : janelasMap) {
        int janelasProfessor = std::accumulate(janelas.begin(), janelas.end(), 0);
        totalJanelas += janelasProfessor;
        maxJanelasPorProf = std::max(maxJanelasPorProf, janelasProfessor);
    }

    metricas["Janelas totais"] = totalJanelas;
    metricas["Máx. janelas por professor"] = maxJanelasPorProf;

    // Aulas consecutivas
    metricas["Aulas consecutivas"] = contarAulasConsecutivasTotal(melhorSolucao);

    // Distribuição
    double desvioTotal = 0;
    for (const auto& turma : turmas) {
        std::map<int, int> aulasPorDia;
        int totalAulas = 0;

        for (const auto& aula : melhorSolucao) {
            if (aula.idTurma == turma.id) {
                aulasPorDia[aula.slot.dia]++;
                totalAulas++;
            }
        }

        if (totalAulas > 0) {
            double media = totalAulas / 5.0;
            for (int dia = 0; dia < 5; dia++) {
                desvioTotal += std::abs(aulasPorDia[dia] - media);
            }
        }
    }

    metricas["Desvio de distribuição"] = desvioTotal;

    // Taxa de ocupação
    metricas["Taxa de ocupação"] = (melhorSolucao.size() * 100.0) / (turmas.size() * 30);

    return metricas;
}

// Exportar histórico
void SimulatedAnnealing::exportarHistorico(const std::string& arquivo) const {
    std::ofstream out(arquivo);
    if (!out.is_open()) {
        log("Erro ao criar arquivo de histórico: " + arquivo);
        return;
    }

    out << "Iteracao,Custo,Temperatura,TaxaAceitacao\n";

    for (size_t i = 0; i < estatisticas.historicoCusto.size(); i++) {
        out << (i * 100) << ","
            << estatisticas.historicoCusto[i] << ","
            << estatisticas.historicoTemperatura[i] << ","
            << estatisticas.historicoTaxaAceitacao[i] << "\n";
    }

    out.close();
    log("Histórico exportado para: " + arquivo);
}

// Ajustar pesos dinamicamente
void SimulatedAnnealing::ajustarPesos(double dist, double consec, double jan, double ext) {
    config.pesoDistribuicao = dist;
    config.pesoConsecutivas = consec;
    config.pesoJanelas = jan;
    config.pesoHorariosExtremos = ext;

    // Limpar cache de custo pois os pesos mudaram
    cacheCusto.clear();

    log("Pesos ajustados: Dist=" + std::to_string(dist) +
        ", Consec=" + std::to_string(consec) +
        ", Jan=" + std::to_string(jan) +
        ", Ext=" + std::to_string(ext));
}

// Contar total de aulas consecutivas
int SimulatedAnnealing::contarAulasConsecutivasTotal(const std::vector<Aula>& solucao) const {
    int total = 0;

    for (const auto& turma : turmas) {
        for (int dia = 0; dia < 5; dia++) {
            std::map<int, std::vector<int>> aulasPorDisciplina;

            for (const auto& aula : solucao) {
                if (aula.idTurma == turma.id && aula.slot.dia == dia) {
                    aulasPorDisciplina[aula.idDisciplina].push_back(aula.slot.hora);
                }
            }

            for (auto& [discId, horarios] : aulasPorDisciplina) {
                if (horarios.size() < 2) continue;

                std::sort(horarios.begin(), horarios.end());

                for (size_t i = 1; i < horarios.size(); i++) {
                    if (horarios[i] == horarios[i-1] + 1) {
                        total++;
                    }
                }
            }
        }
    }

    return total;
}

// Limpar caches
void SimulatedAnnealing::limparCaches() {
    cacheCusto.clear();
    cacheHorariosProfessor.clear();
}