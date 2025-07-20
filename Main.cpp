#include "GeradorHorario.h"
#include <map>
#include <windows.h>

// Função para criar o cenário de exemplo com base nos seus dados
void setupDadosExemplo(
    std::vector<Professor>& profs, std::vector<Disciplina>& discs,
    std::vector<Turma>& turmas, std::vector<Sala>& salas,
    std::vector<RequisicaoAlocacao>& reqs,
    std::set<std::tuple<int, int, int>>& disponibilidade) {

    // --- Mapeamento de Strings para IDs ---
    std::map<std::string, int> mapaIdDias = { {"Segunda", 0}, {"Terça", 1}, {"Quarta", 2}, {"Quinta", 3}, {"Sexta", 4} };
    std::map<std::string, int> mapaIdHorarios = { {"7:30-8:15", 0}, {"8:15-9:00", 1}, {"9:00-9:45", 2},
                                                  {"10:05-10:50", 3}, {"10:50-11:35", 4}, {"11:35-12:20", 5} };

    std::map<std::string, int> mapaIdTurmas;
    std::map<std::string, int> mapaIdDisciplinas;
    std::map<std::string, int> mapaIdProfessores;

    // --- Dados Brutos (como você forneceu) ---
    const std::vector<std::string> turmasNomes = { "6º Ano", "7º Ano", "8º Ano", "9º Ano" };

    // (Estruturas de disciplinas e professores definidas abaixo para clareza)

    // --- Processamento ---

    // ** FIX: idCounter must be declared **
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
        {"Ana Rosa", {"Geografia"}, {{"Segunda", "7:30-8:15"}, {"Segunda", "8:15-9:00"}, {"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Terça", "7:30-8:15"}, {"Terça", "8:15-9:00"}, {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}, {"Quarta", "7:30-8:15"}, {"Quarta", "8:15-9:00"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}, {"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"},{"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"} ,{"Sexta", "7:30-8:15"}, {"Sexta", "8:15-9:00"}, {"Sexta", "9:00-9:45"}, {"Sexta", "10:05-10:50"},{"Sexta", "10:50-11:35"},{"Sexta", "11:35-12:20"}}},
        {"Bianca", {"Espanhol"}, {{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"}}},
        {"Denise", {"Inglês"},{{"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}}},
        {"Camila R.", {"Educ. Socioemocional"}, { {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}}},
        {"Wanderlei", {"Matemática"}, {{"Segunda", "7:30-8:15"}, {"Segunda", "8:15-9:00"}, {"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Terça", "7:30-8:15"}, {"Terça", "8:15-9:00"}, {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}, {"Quarta", "7:30-8:15"}, {"Quarta", "8:15-9:00"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}, {"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"},{"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"} ,{"Sexta", "7:30-8:15"}, {"Sexta", "8:15-9:00"}, {"Sexta", "9:00-9:45"}, {"Sexta", "10:05-10:50"},{"Sexta", "10:50-11:35"},{"Sexta", "11:35-12:20"}}},
        {"Elizangela", {"Prod. Texto"}, {{"Segunda", "7:30-8:15"}, {"Segunda", "8:15-9:00"}, {"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Terça", "7:30-8:15"}, {"Terça", "8:15-9:00"}, {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}, {"Quarta", "7:30-8:15"}, {"Quarta", "8:15-9:00"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}, {"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"},{"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"} ,{"Sexta", "7:30-8:15"}, {"Sexta", "8:15-9:00"}, {"Sexta", "9:00-9:45"}, {"Sexta", "10:05-10:50"},{"Sexta", "10:50-11:35"},{"Sexta", "11:35-12:20"}}},
        {"Jéssica", {"Ciências"}, {{"Segunda", "7:30-8:15"}, {"Segunda", "8:15-9:00"}, {"Segunda", "9:00-9:45"}, {"Segunda", "10:05-10:50"},{"Segunda", "10:50-11:35"},{"Segunda", "11:35-12:20"}, {"Terça", "7:30-8:15"}, {"Terça", "8:15-9:00"}, {"Terça", "9:00-9:45"}, {"Terça", "10:05-10:50"},{"Terça", "10:50-11:35"},{"Terça", "11:35-12:20"}, {"Quarta", "7:30-8:15"}, {"Quarta", "8:15-9:00"}, {"Quarta", "9:00-9:45"}, {"Quarta", "10:05-10:50"},{"Quarta", "10:50-11:35"},{"Quarta", "11:35-12:20"}, {"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"},{"Quinta", "10:50-11:35"},{"Quinta", "11:35-12:20"} ,{"Sexta", "7:30-8:15"}, {"Sexta", "8:15-9:00"}, {"Sexta", "9:00-9:45"}, {"Sexta", "10:05-10:50"},{"Sexta", "10:50-11:35"},{"Sexta", "11:35-12:20"}}},
        {"Kátia", {"Musíca"}, {{"Terça", "11:35-12:20"}, {"Sexta", "10:05-10:50"}, {"Sexta", "10:50-11:35"}, {"Sexta", "11:35-12:20"}}},
        {"Neto", {"Educ. Física"}, {{"Terça", "10:50-11:35"}, {"Terça", "11:35-12:20"}, {"Quinta", "10:50-11:35"}, {"Quinta", "11:35-12:20"}}},
        {"Ronaldo", {"Robótica"}, {{"Quinta", "7:30-8:15"}, {"Quinta", "8:15-9:00"}, {"Quinta", "9:00-9:45"}, {"Quinta", "10:05-10:50"}}},
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

    // 4. Gerar Requisições (** THIS ENTIRE BLOCK IS CORRECTED **)
    for (const auto& discPair : disciplinasData) {
        std::string nomeDisciplina = discPair.first;
        int idDisciplina = mapaIdDisciplinas[nomeDisciplina];

        // Encontrar o professor para esta disciplina
        for (const auto& profData : professoresData) {
            for (const auto& discProf : profData.disciplinas) {
                if (discProf == nomeDisciplina) {
                    int idProfessor = mapaIdProfessores[profData.nome];

                    // Criar requisições para cada turma
                    for (const auto& turmaPair : discPair.second) {
                        int idTurma = mapaIdTurmas[turmaPair.first];
                        int aulasNecessarias = turmaPair.second;

                        // Cria uma requisição para CADA AULA necessária
                        for (int i = 0; i < aulasNecessarias; ++i) {
                            reqs.push_back({ idTurma, idDisciplina, idProfessor, 1.0, false });
                        }
                    }
                    break;
                }
            }
        }
    }

    // 5. Adicionar Salas
    salas = { {501, "Sala 1", false}, {502, "Sala 2", false}, {503, "Sala 3", false},
              {504, "Sala 4", false}, {505, "Quadra", true}, {506, "Lab", true} };
}


int main() {
    // 2. Mudar a página de código do console para UTF-8
    SetConsoleOutputCP(CP_UTF8);
    // Para garantir que o buffer de saída também funcione corretamente com UTF-8
    setvbuf(stdout, nullptr, _IOFBF, 1000);

    std::vector<Professor> professores;
    std::vector<Disciplina> disciplinas;
    std::vector<Turma> turmas;
    std::vector<Sala> salas;
    std::vector<RequisicaoAlocacao> requisicoes;
    std::set<std::tuple<int, int, int>> disponibilidade;

    setupDadosExemplo(professores, disciplinas, turmas, salas, requisicoes, disponibilidade);

    std::map<int, int> disponibilidadeTotalProf;
    for (const auto& p : professores) {
        disponibilidadeTotalProf[p.id] = 0;
    }
    for (const auto& disp : disponibilidade) {
        disponibilidadeTotalProf[std::get<0>(disp)]++;
    }

       GeradorHorario gerador(professores, disciplinas, turmas, salas, requisicoes, disponibilidade, disponibilidadeTotalProf);

    const int MAX_TENTATIVAS = 100000;
    bool sucesso = false;

    for (int tentativa = 1; tentativa <= MAX_TENTATIVAS; ++tentativa) {
        std::cout << "================== TENTATIVA NUMERO " << tentativa << " ==================" << std::endl;

        // Se a construção terminou sem falha crítica, vamos verificar o resultado
        if (gerador.gerarHorario()) {

            std::cout << "Verificando a integridade da grade gerada..." << std::endl;
            std::vector<Aula> gradeResultante = gerador.getGradeHoraria();
            bool gradeValida = true;

            // VERIFICAÇÃO EXTERNA
            for (const auto& req : requisicoes) {
                int aulasRequeridas = 0;
                // Acessa o mapa de disciplinas para pegar a carga horária correta
                for(const auto& disc : disciplinas) {
                    if (disc.id == req.idDisciplina) {
                        aulasRequeridas = disc.aulasPorTurma.at(req.idTurma);
                        break;
                    }
                }

                int aulasAlocadas = 0;
                for (const auto& aula : gradeResultante) {
                    if (aula.idTurma == req.idTurma && aula.idDisciplina == req.idDisciplina) {
                        aulasAlocadas++;
                    }
                }

                if (aulasRequeridas != aulasAlocadas) {
                    gradeValida = false;
                    std::cout << "!!! FALHA NA VERIFICACAO: "
                              << "Disciplina " << req.idDisciplina << " para Turma " << req.idTurma
                              << " | Requerido: " << aulasRequeridas << " Alocado: " << aulasAlocadas << std::endl;
                    break; // Para de verificar na primeira falha
                }
            }

            if (gradeValida) {
                sucesso = true;
                break; // SUCESSO! Sai do laço de tentativas.
            } else {
                 std::cout << "--- ESTADO RESETADO, TENTANDO NOVAMENTE ---\n" << std::endl;
                 gerador.reset(); // Se a verificação falhou, reseta e tenta de novo.
            }

        } else {
             std::cout << "--- ESTADO RESETADO, TENTANDO NOVAMENTE ---\n" << std::endl;
             gerador.reset(); // Se a construção falhou, reseta e tenta de novo.
        }
    }

    if (sucesso) {
        std::cout << "\n\nSOLUCAO ENCONTRADA E VERIFICADA!\n";
        gerador.imprimirHorario();
    } else {
        std::cout << "\n\nNAO FOI POSSIVEL ENCONTRAR UMA SOLUCAO COMPLETA APOS " << MAX_TENTATIVAS << " TENTATIVAS." << std::endl;
    }

    return 0;
}