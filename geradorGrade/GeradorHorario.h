#pragma once
#include "Estruturas.h"
#include <vector>
#include <set>
#include <tuple>
#include <map>
#include <chrono>
#include <string>
#include <memory>
#include <functional>

// Enums para melhor legibilidade
enum class StatusAlocacao {
    SUCESSO,
    FALHA_PROFESSOR_INDISPONIVEL,
    FALHA_TURMA_OCUPADA,
    FALHA_SALA_OCUPADA,
    FALHA_SEM_SLOTS_DISPONIVEIS
};

// Estrutura para estatísticas detalhadas
struct EstatisticasGrade {
    int totalAulas;
    int aulasAlocadas;
    std::map<int, int> aulasPorTurma;
    std::map<int, int> aulasPorProfessor;
    std::map<int, int> aulasPorDia;
    std::map<int, float> ocupacaoSalas;
    int janelasHorario;
    int conflitos;
};

// Estrutura para configurações do gerador
struct ConfiguracaoGerador {
    bool priorizarMinimoJanelas = true;
    bool distribuirAulasUniformemente = true;
    bool evitarAulasExtremos = true;
    int maxTentativasPorRequisicao = 100;
    bool verboso = false;
};

class GeradorHorario {
public:
    // Construtor melhorado com configurações
    GeradorHorario(
        std::vector<Professor> profs,
        std::vector<Disciplina> disc,
        std::vector<Turma> turmas,
        std::vector<Sala> salas,
        std::vector<RequisicaoAlocacao> reqs,
        std::set<std::tuple<int, int, int>> disponibilidade,
        std::map<int, int> disponibilidadeTotalProf,
        std::map<int, int> turmaSalaMapping,
        ConfiguracaoGerador config = ConfiguracaoGerador()
    );

    // Métodos principais
    bool gerarHorario();
    void reset();
    void imprimirHorario();
    void mostrarEstatisticasGrade();

    // Métodos de acesso
    std::vector<Aula> getGradeHoraria() const { return gradeHoraria; }
    void setGradeHoraria(const std::vector<Aula>& novaGrade) { gradeHoraria = novaGrade; }
    EstatisticasGrade obterEstatisticasDetalhadas() const;

    // Exportação melhorada
    void exportarJSON(const std::string& nomeArquivo) const;
    void exportarCSV(const std::string& nomeArquivo) const;
    void exportarHTML(const std::string& nomeArquivo) const;

    // Validação
    bool validarGradeCompleta() const;
    std::vector<std::string> obterProblemasGrade() const;

    // Callbacks para progresso
    void setCallbackProgresso(std::function<void(int, int)> callback) {
        callbackProgresso = callback;
    }

private:
    // Dados de entrada
    std::vector<Professor> professores;
    std::vector<Disciplina> disciplinas;
    std::vector<Turma> turmas;
    std::vector<Sala> salas;
    std::vector<RequisicaoAlocacao> requisicoes;
    std::vector<Aula> gradeHoraria;

    // Configurações
    ConfiguracaoGerador configuracao;

    // Mapas para tradução e consulta
    std::map<int, std::string> mapaNomesProfessores;
    std::map<int, std::string> mapaNomesDisciplinas;
    std::map<int, std::string> mapaNomesTurmas;
    std::map<int, std::string> mapaNomesSalas;
    std::map<int, Disciplina> mapaDisciplinas;
    std::map<int, int> turmaSalaMap;

    // Estruturas de controle
    std::set<std::tuple<int, int, int>> disponibilidadeProfessores;
    std::map<int, int> disponibilidadeTotalProfessores;

    // Cache para otimização
    mutable std::map<std::tuple<int, int, int>, bool> cacheDisponibilidade;

    // Callback para progresso
    std::function<void(int, int)> callbackProgresso;

    // Métodos privados principais
    StatusAlocacao tentarAlocarRequisicao(const RequisicaoAlocacao& req);
    bool verificarDisponibilidade(int idTurma, int idProfessor, int idSala, Slot slot);

    // Métodos de análise e otimização
    void analisarCargaDeTrabalho(const std::vector<RequisicaoAlocacao>& requisicoes);
    float calcularCriticidade(int idProfessor, int aulasNecessarias);
    std::vector<Slot> obterSlotsOrdenados(const RequisicaoAlocacao& req);
    int calcularPontuacaoSlot(const RequisicaoAlocacao& req, const Slot& slot);

    // Métodos de validação interna
    bool validarConsistenciaInterna() const;
    bool verificarConflitosProfessor() const;
    bool verificarConflitosTurma() const;
    bool verificarConflitosSala() const;

    // Métodos auxiliares
    int contarJanelasHorario(int idProfessor) const;
    int contarAulasConsecutivas(int idTurma, int idDisciplina) const;
    std::map<int, std::vector<int>> obterDistribuicaoSemanal(int idTurma) const;

    // Logging
    void log(const std::string& mensagem, bool forcarExibicao = false) const;
    void logErro(const std::string& mensagem) const;
    void logAviso(const std::string& mensagem) const;
};