#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <map>

// Estruturas de dados baseadas nos conjuntos do documento
struct Professor {
    int id;
    std::string nome;
};

struct Disciplina {
    int id;
    std::string nome;
    // Mapeia um ID de turma para a quantidade de aulas semanais
    std::map<int, int> aulasPorTurma;
    // Mapeia um ID de turma para a quantidade de aulas em local compartilhado
    std::map<int, int> aulasCompartilhadasPorTurma;
};

struct Turma {
    int id;
    std::string nome;
};

struct Sala {
    int id;
    std::string nome;
    bool compartilhada; // Parâmetro SR_r
};

struct Slot {
    int dia; // 0=Seg, 1=Ter, ...
    int hora; // 0=1º horário, 1=2º horário, ...
};

// Representa uma aula alocada na grade final
struct Aula {
    int idProfessor;
    int idDisciplina;
    int idTurma;
    int idSala;
    Slot slot;
};

// Representa a necessidade de alocar uma disciplina para uma turma por um professor
struct RequisicaoAlocacao {
    int idTurma;
    int idDisciplina;
    int idProfessor;
    double custoPreferencia; // Custo combinado (alfa * PST + beta * PTS)
    bool alocada = false;
};