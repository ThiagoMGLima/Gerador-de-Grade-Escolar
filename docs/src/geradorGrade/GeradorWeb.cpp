#include <emscripten/emscripten.h>
#include <emscripten/bind.h>
#include "GeradorHorario.h"
#include "SimulatedAnnealing.h"
#include "json.hpp"
#include <sstream>

using json = nlohmann::json;
using namespace emscripten;

// Função para reportar progresso ao JavaScript
EM_JS(void, reportProgress, (const char* mensagem, int percentual), {
    if (Module.onProgress) {
        Module.onProgress(UTF8ToString(mensagem), percentual);
    }
});

// Função exposta para JavaScript
std::string processarGradeHoraria(std::string dadosJSON, bool aplicarOtimizacao = true) {
    try {
        reportProgress("Iniciando processamento...", 0);

        json entrada = json::parse(dadosJSON);

        // Variáveis para armazenar dados
        std::vector<Professor> professores;
        std::vector<Disciplina> disciplinas;
        std::vector<Turma> turmas;
        std::vector<Sala> salas;
        std::vector<RequisicaoAlocacao> requisicoes;
        std::set<std::tuple<int, int, int>> disponibilidade;
        std::map<int, int> turmaSalaMap;
        std::map<int, int> disponibilidadeTotalProf;

        reportProgress("Carregando dados...", 10);

        // Carregar turmas
        for (const auto& t : entrada["turmas"]) {
            Turma turma;
            turma.id = t["id"].get<int>();
            turma.nome = t["nome"].get<std::string>();

            std::string turnoStr = t["turno"].get<std::string>();
            if (turnoStr == "manha") turma.turno = Turno::MANHA;
            else if (turnoStr == "tarde") turma.turno = Turno::TARDE;
            else if (turnoStr == "noite") turma.turno = Turno::NOITE;

            turmas.push_back(turma);
        }

        // Carregar disciplinas
        for (const auto& d : entrada["disciplinas"]) {
            Disciplina disc;
            disc.id = d["id"].get<int>();
            disc.nome = d["nome"].get<std::string>();

            for (const auto& [turmaId, carga] : d["aulasPorTurma"].items()) {
                disc.aulasPorTurma[std::stoi(turmaId)] = carga.get<int>();
            }

            disciplinas.push_back(disc);
        }

        // Carregar professores
        for (const auto& p : entrada["professores"]) {
            Professor prof;
            prof.id = p["id"].get<int>();
            prof.nome = p["nome"].get<std::string>();

            int disciplinaId = p["idDisciplina"].get<int>();
            prof.disciplinasHabilitadas.insert(disciplinaId);

            professores.push_back(prof);
            disponibilidadeTotalProf[prof.id] = 0;

            for (const auto& disp : p["disponibilidade"]) {
                int dia = disp["dia"].get<int>();
                int horario = disp["horario"].get<int>();
                disponibilidade.insert({prof.id, dia, horario});
                disponibilidadeTotalProf[prof.id]++;
            }
        }

        // Carregar salas
        for (const auto& s : entrada["salas"]) {
            Sala sala;
            sala.id = s["id"].get<int>();
            sala.nome = s["nome"].get<std::string>();
            sala.compartilhada = s["compartilhada"].get<bool>();

            std::string tipoStr = s["tipo"].get<std::string>();
            if (tipoStr == "laboratorio") sala.tipo = TipoSala::LABORATORIO;
            else if (tipoStr == "quadra") sala.tipo = TipoSala::QUADRA;
            else if (tipoStr == "biblioteca") sala.tipo = TipoSala::BIBLIOTECA;
            else sala.tipo = TipoSala::NORMAL;

            salas.push_back(sala);
        }

        // Carregar associações
        if (entrada.contains("associacoes")) {
            for (const auto& [turmaId, salaId] : entrada["associacoes"]["turmaSala"].items()) {
                turmaSalaMap[std::stoi(turmaId)] = salaId.get<int>();
            }
        }

        // Gerar requisições
        for (const auto& disc : disciplinas) {
            int idProfessor = -1;
            for (const auto& prof : professores) {
                if (prof.disciplinasHabilitadas.count(disc.id) > 0) {
                    idProfessor = prof.id;
                    break;
                }
            }

            if (idProfessor != -1) {
                for (const auto& [idTurma, qtdAulas] : disc.aulasPorTurma) {
                    for (int i = 0; i < qtdAulas; i++) {
                        requisicoes.push_back({idTurma, disc.id, idProfessor});
                    }
                }
            }
        }

        reportProgress("Iniciando Fase 1: Geração inicial...", 20);

        // FASE 1: Executar gerador
        ConfiguracaoGerador config;
        config.verboso = false;

        GeradorHorario gerador(professores, disciplinas, turmas, salas, requisicoes,
                              disponibilidade, disponibilidadeTotalProf, turmaSalaMap, config);

        // Tentar gerar grade inicial
        bool sucessoFase1 = false;
        std::vector<Aula> gradeInicial;

        for (int tentativa = 1; tentativa <= 10000; tentativa++) {
            if (tentativa % 10 == 0) {
                std::stringstream msg;
                msg << "Fase 1: Tentativa " << tentativa << " de 100...";
                reportProgress(msg.str().c_str(), 20 + (tentativa / 100.0 * 30));
            }

            if (gerador.gerarHorario()) {
                gradeInicial = gerador.getGradeHoraria();

                // Verificar integridade
                std::map<std::pair<int, int>, int> aulasAlocadas;
                std::map<std::pair<int, int>, int> aulasRequeridas;

                for (const auto& disc : disciplinas) {
                    for (const auto& [idTurma, qtd] : disc.aulasPorTurma) {
                        aulasRequeridas[{idTurma, disc.id}] = qtd;
                    }
                }

                for (const auto& aula : gradeInicial) {
                    aulasAlocadas[{aula.idTurma, aula.idDisciplina}]++;
                }

                bool valida = true;
                for (const auto& [chave, req] : aulasRequeridas) {
                    if (aulasAlocadas[chave] != req) {
                        valida = false;
                        break;
                    }
                }

                if (valida) {
                    sucessoFase1 = true;
                    break;
                }
            }
            gerador.reset();
        }

        if (!sucessoFase1) {
            json erro;
            erro["erro"] = "Não foi possível gerar uma grade inicial válida após 10000 tentativas";
            return erro.dump();
        }

        reportProgress("Fase 1 concluída! Grade inicial gerada.", 50);

        // FASE 2: Simulated Annealing (se habilitado)
        std::vector<Aula> gradeFinal = gradeInicial;
        json estatisticasOtimizacao;

        if (aplicarOtimizacao) {
            reportProgress("Iniciando Fase 2: Otimização com Simulated Annealing...", 55);

            ConfiguracaoSA configSA;
            configSA.numIteracoes = 50000; // Menos iterações para WebAssembly
            configSA.temperaturaInicial = 100.0;
            configSA.taxaResfriamento = 0.95;
            configSA.verboso = false;

            SimulatedAnnealing sa(
                gradeInicial,
                professores,
                disciplinas,
                turmas,
                salas,
                disponibilidade,
                turmaSalaMap,
                configSA
            );

            // Callback para progresso
            sa.executarComCallback([](int iteracao, double temperatura, double custo) {
                if (iteracao % 100 == 0) {
                    std::stringstream msg;
                    msg << "Fase 2: Otimizando... (iteração " << iteracao << ")";
                    // Estimar progresso baseado em 5000 iterações
                    int progresso = 55 + (iteracao * 40 / 5000);
                    if (progresso > 95) progresso = 95;
                    reportProgress(msg.str().c_str(), progresso);
                }
            });

            gradeFinal = sa.getSolucaoFinal();

            // Coletar estatísticas
            auto stats = sa.getEstatisticas();
            estatisticasOtimizacao["custoInicial"] = stats.custoInicial;
            estatisticasOtimizacao["custoFinal"] = stats.custoFinal;
            estatisticasOtimizacao["melhoria"] = stats.getPercentualMelhoria();
            estatisticasOtimizacao["iteracoes"] = configSA.numIteracoes;
            estatisticasOtimizacao["movimentosAceitos"] = stats.movimentosAceitos;
            estatisticasOtimizacao["movimentosRejeitados"] = stats.movimentosRejeitados;
            estatisticasOtimizacao["taxaAceitacao"] = stats.getTaxaAceitacao();
            reportProgress("Fase 2 concluída! Grade otimizada.", 95);
        }

        // Preparar resultado final
        reportProgress("Preparando resultado...", 98);

        json resultado;
        resultado["metadata"]["versao"] = "2.0";
        resultado["metadata"]["geradoEm"] = "WebAssembly";
        resultado["metadata"]["turmas"] = json::array();
        resultado["metadata"]["dias"] = {"Segunda", "Terça", "Quarta", "Quinta", "Sexta"};
        resultado["metadata"]["horarios"] = {"7:30-8:15", "8:15-9:00", "9:00-9:45",
                                           "10:05-10:50", "10:50-11:35", "11:35-12:20"};
        resultado["metadata"]["otimizado"] = aplicarOtimizacao;

        if (aplicarOtimizacao) {
            resultado["metadata"]["otimizacao"] = estatisticasOtimizacao;
        }

        for (const auto& t : turmas) {
            resultado["metadata"]["turmas"].push_back(t.nome);
        }

        // Converter grade para JSON
        resultado["aulas"] = json::array();

        for (size_t i = 0; i < gradeFinal.size(); i++) {
            const auto& aula = gradeFinal[i];
            json j_aula;

            // Encontrar nomes
            std::string nomeTurma = "";
            std::string nomeDisc = "";
            std::string nomeProf = "";
            std::string nomeSala = "";

            for (const auto& t : turmas) {
                if (t.id == aula.idTurma) nomeTurma = t.nome;
            }
            for (const auto& d : disciplinas) {
                if (d.id == aula.idDisciplina) nomeDisc = d.nome;
            }
            for (const auto& p : professores) {
                if (p.id == aula.idProfessor) nomeProf = p.nome;
            }
            for (const auto& s : salas) {
                if (s.id == aula.idSala) nomeSala = s.nome;
            }

            j_aula["id"] = i + 1;
            j_aula["turma"] = nomeTurma;
            j_aula["turmaId"] = aula.idTurma;
            j_aula["disciplina"] = nomeDisc;
            j_aula["disciplinaId"] = aula.idDisciplina;
            j_aula["professor"] = nomeProf;
            j_aula["professorId"] = aula.idProfessor;
            j_aula["sala"] = nomeSala;
            j_aula["salaId"] = aula.idSala;
            j_aula["dia"] = aula.slot.dia;
            j_aula["hora"] = aula.slot.hora;

            resultado["aulas"].push_back(j_aula);
        }

        // Adicionar estatísticas gerais
        resultado["estatisticas"]["totalAulas"] = gradeFinal.size();
        resultado["estatisticas"]["fase1"]["tentativas"] = 1; // Simplificado
        resultado["estatisticas"]["fase1"]["sucesso"] = true;

        reportProgress("Processamento concluído!", 100);

        return resultado.dump();

    } catch (const std::exception& e) {
        json erro;
        erro["erro"] = std::string("Erro: ") + e.what();
        return erro.dump();
    }
}

// Versão simplificada sem otimização
std::string processarGradeRapida(std::string dadosJSON) {
    return processarGradeHoraria(dadosJSON, false);
}

// Exportar funções para JavaScript
EMSCRIPTEN_BINDINGS(gerador_module) {
    function("processarGradeHoraria", &processarGradeHoraria);
    function("processarGradeRapida", &processarGradeRapida);
}