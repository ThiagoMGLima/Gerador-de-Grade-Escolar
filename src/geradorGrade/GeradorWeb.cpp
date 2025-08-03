#include <emscripten/emscripten.h>

#include <emscripten/bind.h>
#include "GeradorHorario.h"
#include "SimulatedAnnealing.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace emscripten;

// Função exposta para JavaScript
std::string processarGradeHoraria(std::string dadosJSON) {
    try {
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

        // Executar gerador
        ConfiguracaoGerador config;
        config.verboso = false;

        GeradorHorario gerador(professores, disciplinas, turmas, salas, requisicoes,
                              disponibilidade, disponibilidadeTotalProf, turmaSalaMap, config);

        // Tentar gerar grade
        for (int tentativa = 1; tentativa <= 100; tentativa++) {
            if (gerador.gerarHorario()) {
                // Exportar resultado
                json resultado;
                resultado["metadata"]["versao"] = "2.0";
                resultado["metadata"]["geradoEm"] = "WebAssembly";
                resultado["metadata"]["turmas"] = json::array();
                resultado["metadata"]["dias"] = {"Segunda", "Terça", "Quarta", "Quinta", "Sexta"};
                resultado["metadata"]["horarios"] = {"7:30-8:15", "8:15-9:00", "9:00-9:45",
                                                   "10:05-10:50", "10:50-11:35", "11:35-12:20"};

                for (const auto& t : turmas) {
                    resultado["metadata"]["turmas"].push_back(t.nome);
                }

                // Converter grade para JSON
                resultado["aulas"] = json::array();
                auto grade = gerador.getGradeHoraria();

                for (size_t i = 0; i < grade.size(); i++) {
                    const auto& aula = grade[i];
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

                // Adicionar estatísticas
                auto stats = gerador.obterEstatisticasDetalhadas();
                resultado["estatisticas"]["totalAulas"] = stats.aulasAlocadas;

                return resultado.dump();
            }
            gerador.reset();
        }

        // Se falhar
        json erro;
        erro["erro"] = "Não foi possível gerar a grade após 100 tentativas";
        return erro.dump();

    } catch (const std::exception& e) {
        json erro;
        erro["erro"] = std::string("Erro: ") + e.what();
        return erro.dump();
    }
}

// Exportar funções para JavaScript
EMSCRIPTEN_BINDINGS(gerador_module) {
    function("processarGradeHoraria", &processarGradeHoraria);
}