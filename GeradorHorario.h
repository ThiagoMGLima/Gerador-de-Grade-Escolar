#pragma once
#include "Estruturas.h"
#include <vector>
#include <stack>
#include <set>
#include <tuple>
#include <map>

class GeradorHorario {
public:
    GeradorHorario(
        std::vector<Professor> profs,
        std::vector<Disciplina> disc,
        std::vector<Turma> turmas,
        std::vector<Sala> salas,
        std::vector<RequisicaoAlocacao> reqs,
        std::set<std::tuple<int, int, int>> disponibilidade,
        std::map<int, int> disponibilidadeTotalProf
    );
    bool gerarHorario();
    void reset();
    void imprimirHorario();
    std::vector<Aula> getGradeHoraria();


private:
    // Dados de entrada e de estado
    std::vector<Professor> professores;
    std::vector<Disciplina> disciplinas;
    std::vector<Turma> turmas;
    std::vector<Sala> salas;
    std::vector<RequisicaoAlocacao> requisicoes;
    std::vector<Aula> gradeHoraria;

    // Mapas para tradução e consulta
    std::map<int, std::string> mapaNomesProfessores;
    std::map<int, std::string> mapaNomesDisciplinas;
    std::map<int, std::string> mapaNomesTurmas;
    std::map<int, std::string> mapaNomesSalas;
    std::map<int, Disciplina> mapaDisciplinas;

    // Estruturas de controle do algoritmo
    int encurralamento_i = 0;
    std::stack<std::vector<Aula>> historicoDeAlocacoes;
    std::set<std::tuple<int, int, int>> disponibilidadeProfessores;
    std::map<int, int> disponibilidadeTotalProfessores;

    // Funções auxiliares privadas
    bool horarioCompleto();
    RequisicaoAlocacao selecionarCandidatoAleatorio();
    bool tentarAlocarRequisicao(const RequisicaoAlocacao& req);
    bool verificarDisponibilidade(int idTurma, int idProfessor, int idSala, Slot slot);
    void desfazerUltimasAlocacoes(int n);
};