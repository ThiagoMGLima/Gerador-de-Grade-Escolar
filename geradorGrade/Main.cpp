#include "GeradorHorario.h"
#include "SimulatedAnnealing.h"
#include <map>
// #include <windows.h>
#include <chrono>

// Função para criar o cenário de exemplo com base nos seus dados
void setupDadosExemplo(
    std::vector<Professor>& profs, std::vector<Disciplina>& discs,
    std::vector<Turma>& turmas, std::vector<Sala>& salas,
    std::vector<RequisicaoAlocacao>& reqs,
    std::set<std::tuple<int, int, int>>& disponibilidade,
    std::map<int, int>& turmaSalaMap) {

    // --- Mapeamento de Strings para IDs ---
    std::map<std::string, int> mapaIdDias = { {"Segunda", 0}, {"Terça", 1}, {"Quarta", 2}, {"Quinta", 3}, {"Sexta", 4} };
    std::map<std::string, int> mapaIdHorarios = { {"7:30-8:15", 0}, {"8:15-9:00", 1}, {"9:00-9:45", 2},
                                                  {"10:05-10:50", 3}, {"10:50-11:35", 4}, {"11:35-12:20", 5} };

    std::map<std::string, int> mapaIdTurmas;
    std::map<std::string, int> mapaIdDisciplinas;
    std::map<std::string, int> mapaIdProfessores;

    // --- Dados Brutos (como você forneceu) ---
    const std::vector<std::string> turmasNomes = { "6º Ano", "7º Ano", "8º Ano", "9º Ano" };

    // --- Processamento ---

    int idCounter = 1;

    // 1. Processar Turmas
    for (const auto& nome : turmasNomes) {
        turmas.push_back({ idCounter, nome });
        mapaIdTurmas[nome] = idCounter++;
    }

    // 2. Processar Disciplinas
    using CargaHorariaMap = std::map<std::string, int>;
    std::map<std::string, CargaHorariaMap> disciplinasData = {
        {"Ling. Port.", {{"6º Ano", 5}, {"7º Ano", 5}, {"8º Ano", 5}, {"9º Ano", 5}}},
        {"Prod. Texto", {{"6º Ano", 2}, {"7º Ano", 2}, {"8º Ano", 2}, {"9º Ano", 2}}},
        {"História", {{"6º Ano", 3}, {"7º Ano", 3}, {"8º Ano", 3}, {"9º Ano", 3}}},
        {"Geografia", {{"6º Ano", 3}, {"7º Ano", 3}, {"8º Ano", 3}, {"9º Ano", 3}}},
        {"Matemática", {{"6º Ano", 6}, {"7º Ano", 6}, {"8º Ano", 6}, {"9º Ano", 6}}},
        {"Ciências", {{"6º Ano", 3}, {"7º Ano", 3}, {"8º Ano", 3}, {"9º Ano", 3}}},
        {"Artes", {{"6º Ano", 1}, {"7º Ano", 1}, {"8º Ano", 1}, {"9º Ano", 1}}},
        {"Educ. Física", {{"6º Ano", 1}, {"7º Ano", 1}, {"8º Ano", 1}, {"9º Ano", 1}}},
        {"Musíca", {{"6º Ano", 1}, {"7º Ano", 1}, {"8º Ano", 1}, {"9º Ano", 1}}},
        {"Educ. Socioemocional", {{"6º Ano", 1}, {"7º Ano", 1}, {"8º Ano", 1}, {"9º Ano", 1}}},
        {"Inglês", {{"6º Ano", 2}, {"7º Ano", 2}, {"8º Ano", 2}, {"9º Ano", 2}}},
        {"Robótica", {{"6º Ano", 1}, {"7º Ano", 1}, {"8º Ano", 1}, {"9º Ano", 1}}},
        {"Espanhol", {{"6º Ano", 1}, {"7º Ano", 1}, {"8º Ano", 1}, {"9º Ano", 1}}}
    };

    idCounter = 101;
    for (const auto& pair : disciplinasData) {
        Disciplina novaDisc;
        novaDisc.id = idCounter;
        novaDisc.nome = pair.first;
        for (const auto& cargaPair : pair.second) {
            novaDisc.aulasPorTurma[mapaIdTurmas[cargaPair.first]] = cargaPair.second;
        }
        discs.push_back(novaDisc);
        mapaIdDisciplinas[novaDisc.nome] = idCounter++;
    }

    // 3. Processar Professores e Disponibilidade
    struct ProfData { std::string nome; std::vector<std::string> disciplinas; std::vector<std::pair<std::string, std::string>> disp; };
    std::vector<ProfData> professoresData = {
        {"Adilson", {"História"}, {{"Quarta", "7:30-8:15"}, {"Quarta", "8:15-9:00"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}, {"Sexta", "7:30-8:15"}, {"Sexta", "8:15-9:00"}, {"Sexta", "9:00-9:45"}, {"Sexta", "10:05-10:50"},{"Sexta", "10:50-11:35"},{"Sexta", "11:35-12:20"}}},
        {"Alexandra", {"Artes"}, {{"Segunda", "7:30-8:15"}, {"Segunda", "8:15-9:00"}, {"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"}}},
        {"Ana Rosa", {"Geografia"}, {{"Segunda", "7:30-8:15"}, {"Segunda", "8:15-9:00"}, {"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Terça", "7:30-8:15"}, {"Terça", "8:15-9:00"}, {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}, {"Quarta", "7:30-8:15"}, {"Quarta", "8:15-9:00"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}, {"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"},{"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"}}},
        {"Bianca", {"Espanhol"}, {{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"}}},
        {"Denise", {"Inglês"},{{"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}}},
        {"Camila R.", {"Educ. Socioemocional"}, { {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}}},
        {"Wanderlei", {"Matemática"}, {{"Segunda", "7:30-8:15"}, {"Segunda", "8:15-9:00"}, {"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Terça", "7:30-8:15"}, {"Terça", "8:15-9:00"}, {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}, {"Quarta", "7:30-8:15"}, {"Quarta", "8:15-9:00"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}, {"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"},{"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"} ,{"Sexta", "7:30-8:15"}, {"Sexta", "8:15-9:00"}, {"Sexta", "9:00-9:45"}, {"Sexta", "10:05-10:50"},{"Sexta", "10:50-11:35"},{"Sexta", "11:35-12:20"}}},
        {"Elizangela", {"Prod. Texto"}, {{"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"},{"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"} ,{"Sexta", "7:30-8:15"}, {"Sexta", "8:15-9:00"}, {"Sexta", "9:00-9:45"}, {"Sexta", "10:05-10:50"},{"Sexta", "10:50-11:35"},{"Sexta", "11:35-12:20"}}},
        {"Jéssica", {"Ciências"}, {{"Segunda", "7:30-8:15"}, {"Segunda", "8:15-9:00"}, {"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Terça", "7:30-8:15"}, {"Terça", "8:15-9:00"}, {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}, {"Quarta", "7:30-8:15"}, {"Quarta", "8:15-9:00"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}, {"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"},{"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"} ,{"Sexta", "7:30-8:15"}, {"Sexta", "8:15-9:00"}, {"Sexta", "9:00-9:45"}, {"Sexta", "10:05-10:50"},{"Sexta", "10:50-11:35"},{"Sexta", "11:35-12:20"}}},
        {"Kátia", {"Musíca"}, {{"Terça", "11:35-12:20"}, {"Sexta", "10:05-10:50"}, {"Sexta", "10:50-11:35"}, {"Sexta", "11:35-12:20"}}},
        {"Neto", {"Educ. Física"}, {{"Terça", "10:50-11:35"}, {"Terça", "11:35-12:20"}, {"Quinta", "10:50-11:35"}, {"Quinta", "11:35-12:20"}}},
        {"Ronaldo", {"Robótica"}, {{"Segunda", "7:30-8:15"}, {"Segunda", "8:15-9:00"}, {"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Terça", "7:30-8:15"}, {"Terça", "8:15-9:00"}, {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}, {"Quarta", "7:30-8:15"}, {"Quarta", "8:15-9:00"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}, {"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"},{"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"} ,{"Sexta", "7:30-8:15"}, {"Sexta", "8:15-9:00"}, {"Sexta", "9:00-9:45"}, {"Sexta", "10:05-10:50"},{"Sexta", "10:50-11:35"},{"Sexta", "11:35-12:20"}}},
        {"Selma", {"Ling. Port."}, {{"Segunda", "7:30-8:15"}, {"Segunda", "8:15-9:00"}, {"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Terça", "7:30-8:15"}, {"Terça", "8:15-9:00"}, {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}, {"Quarta", "7:30-8:15"}, {"Quarta", "8:15-9:00"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}, {"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"},{"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"} ,{"Sexta", "7:30-8:15"}, {"Sexta", "8:15-9:00"}, {"Sexta", "9:00-9:45"}, {"Sexta", "10:05-10:50"},{"Sexta", "10:50-11:35"},{"Sexta", "11:35-12:20"}}}
    };

    idCounter = 201;
    for (const auto& pData : professoresData) {
        int profId = idCounter++;
        profs.push_back({ profId, pData.nome });
        mapaIdProfessores[pData.nome] = profId;
        for (const auto& dispPair : pData.disp) {
            disponibilidade.insert({ profId, mapaIdDias[dispPair.first], mapaIdHorarios[dispPair.second] });
        }
    }

    // 4. Adicionar Salas
    salas = { {501, "Sala 6º Ano", false}, {502, "Sala 7º Ano", false},
              {503, "Sala 8º Ano", false}, {504, "Sala 9º Ano", false},
              {505, "Quadra", true}, {506, "Lab", true} };

    // Associar cada turma à sua sala específica
    turmaSalaMap[mapaIdTurmas["6º Ano"]] = 501;
    turmaSalaMap[mapaIdTurmas["7º Ano"]] = 502;
    turmaSalaMap[mapaIdTurmas["8º Ano"]] = 503;
    turmaSalaMap[mapaIdTurmas["9º Ano"]] = 504;

    // 5. Gerar Requisições
    for (const auto& discPair : disciplinasData) {
        std::string nomeDisciplina = discPair.first;
        int idDisciplina = mapaIdDisciplinas[nomeDisciplina];

        // Encontrar o professor para esta disciplina
        int idProfessor = -1;
        for (const auto& profData : professoresData) {
            if (!profData.disciplinas.empty() && profData.disciplinas[0] == nomeDisciplina) {
                idProfessor = mapaIdProfessores[profData.nome];
                break;
            }
        }

        if (idProfessor == -1) {
            std::cerr << "ERRO: Nao foi encontrado professor para a disciplina " << nomeDisciplina << std::endl;
            continue;
        }

        // Criar requisições para cada turma
        for (const auto& turmaPair : discPair.second) {
            int idTurma = mapaIdTurmas[turmaPair.first];
            int aulasNecessarias = turmaPair.second;

            // Cria uma requisição para CADA AULA necessária
            for (int i = 0; i < aulasNecessarias; ++i) {
                reqs.push_back({ idTurma, idDisciplina, idProfessor });
            }
        }
    }


    // Mostra resumo dos dados carregados
    std::cout << "\n=== DADOS CARREGADOS ===" << std::endl;
    std::cout << "Turmas: " << turmas.size() << std::endl;
    std::cout << "Disciplinas: " << discs.size() << std::endl;
    std::cout << "Professores: " << profs.size() << std::endl;
    std::cout << "Salas: " << salas.size() << std::endl;
    std::cout << "Total de aulas a alocar: " << reqs.size() << std::endl;
}


int main() {
    // Mudar a página de código do console para UTF-8
    // SetConsoleOutputCP(CP_UTF8);
    // setvbuf(stdout, nullptr, _IOFBF, 1000);

    std::vector<Professor> professores;
    std::vector<Disciplina> disciplinas;
    std::vector<Turma> turmas;
    std::vector<Sala> salas;
    std::vector<RequisicaoAlocacao> requisicoes;
    std::set<std::tuple<int, int, int>> disponibilidade;
    std::map<int, int> turmaSalaMap;

    setupDadosExemplo(professores, disciplinas, turmas, salas, requisicoes, disponibilidade, turmaSalaMap);

    // Calcula disponibilidade total de cada professor
    std::map<int, int> disponibilidadeTotalProf;
    for (const auto& p : professores) {
        disponibilidadeTotalProf[p.id] = 0;
    }
    for (const auto& disp : disponibilidade) {
        disponibilidadeTotalProf[std::get<0>(disp)]++;
    }

    // FASE 1: Construção Inicial
    GeradorHorario gerador(professores, disciplinas, turmas, salas, requisicoes,
                          disponibilidade, disponibilidadeTotalProf, turmaSalaMap);

    const int MAX_TENTATIVAS = 100000;
    bool sucesso = false;
    int tentativasRealizadas = 0;

    auto start = std::chrono::high_resolution_clock::now();

    for (int tentativa = 1; tentativa <= MAX_TENTATIVAS; ++tentativa) {
        tentativasRealizadas = tentativa;
        std::cout << "\n================== TENTATIVA NUMERO " << tentativa << " ==================" << std::endl;

        if (gerador.gerarHorario()) {
            std::cout << "\nVerificando a integridade da grade gerada..." << std::endl;
            std::vector<Aula> gradeResultante = gerador.getGradeHoraria();

            // VERIFICAÇÃO EXTERNA
            std::map<std::pair<int, int>, int> aulasAlocadasPorTurmaDisciplina;
            std::map<std::pair<int, int>, int> aulasRequeridasPorTurmaDisciplina;

            for (const auto& disc : disciplinas) {
                for (const auto& [idTurma, qtdAulas] : disc.aulasPorTurma) {
                    aulasRequeridasPorTurmaDisciplina[{idTurma, disc.id}] = qtdAulas;
                }
            }

            for (const auto& aula : gradeResultante) {
                aulasAlocadasPorTurmaDisciplina[{aula.idTurma, aula.idDisciplina}]++;
            }

            bool gradeValida = true;
            for (const auto& [chave, aulasRequeridas] : aulasRequeridasPorTurmaDisciplina) {
                int idTurma = chave.first;
                int idDisciplina = chave.second;
                int aulasAlocadas = aulasAlocadasPorTurmaDisciplina[chave];

                if (aulasRequeridas != aulasAlocadas) {
                    gradeValida = false;
                    std::string nomeTurma = "???";
                    std::string nomeDisciplina = "???";

                    for (const auto& t : turmas) {
                        if (t.id == idTurma) {
                            nomeTurma = t.nome;
                            break;
                        }
                    }

                    for (const auto& d : disciplinas) {
                        if (d.id == idDisciplina) {
                            nomeDisciplina = d.nome;
                            break;
                        }
                    }

                    std::cout << "!!! FALHA NA VERIFICACAO: "
                              << "Disciplina '" << nomeDisciplina << "' para Turma '" << nomeTurma
                              << "' | Requerido: " << aulasRequeridas << " Alocado: " << aulasAlocadas << std::endl;
                }
            }

            if (gradeValida) {
                sucesso = true;
                break;
            } else {
                std::cout << "--- ESTADO RESETADO, TENTANDO NOVAMENTE ---\n" << std::endl;
                gerador.reset();
            }

        } else {
            std::cout << "--- ESTADO RESETADO, TENTANDO NOVAMENTE ---\n" << std::endl;
            gerador.reset();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    if (sucesso) {
        std::cout << "\n\n========================================" << std::endl;
        std::cout << "    FASE 1 CONCLUIDA COM SUCESSO!" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Tentativas realizadas: " << tentativasRealizadas << std::endl;
        std::cout << "Tempo total: " << duration.count() << " ms" << std::endl;
        std::cout << "========================================\n" << std::endl;

        gerador.imprimirHorario();
        gerador.exportarJSON("grade_horaria.json");

        // FASE 2: Simulated Annealing
        char executarFase2;
        std::cout << "\n\nDeseja executar a Fase 2 (melhoramento com Simulated Annealing)? (S/N): ";
        std::cin >> executarFase2;

        if (executarFase2 == 'S' || executarFase2 == 's') {
            std::cout << "\n\n========================================" << std::endl;
            std::cout << "      INICIANDO FASE 2: MELHORAMENTO" << std::endl;
            std::cout << "========================================" << std::endl;

            /// Criar a configuração primeiro
            ConfiguracaoSA configSA;
            configSA.numIteracoes = 10000;
            configSA.temperaturaInicial = 100.0;
            configSA.taxaResfriamento = 0.95;
            configSA.verboso = true; // Habilita logs detalhados

            // Passar a struct para o construtor
            SimulatedAnnealing sa(
                gerador.getGradeHoraria(),
                professores,
                disciplinas,
                turmas,
                salas,
                disponibilidade,
                turmaSalaMap,
                configSA // Passa o objeto de configuração
            );

    sa.executar();
            sa.mostrarEstatisticas();

            // Atualiza o gerador com a solução melhorada
            gerador.setGradeHoraria(sa.getSolucaoFinal());

            std::cout << "\n=== GRADE HORARIA FINAL (APOS MELHORAMENTO) ===" << std::endl;
            gerador.imprimirHorario();
            gerador.exportarJSON("grade_melhorada.json");
        }

    } else {
        std::cout << "\n\n========================================" << std::endl;
        std::cout << "           FALHA NA GERACAO" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Nao foi possivel encontrar uma solucao completa apos "
                  << MAX_TENTATIVAS << " tentativas." << std::endl;
        std::cout << "Tempo total: " << duration.count() << " ms" << std::endl;
        std::cout << "========================================" << std::endl;
    }

    return 0;
}