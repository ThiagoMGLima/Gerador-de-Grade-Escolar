#include "GeradorHorario.h"
#include <algorithm>
#include <random>
#include <limits>
#include <iomanip>

GeradorHorario::GeradorHorario(
    std::vector<Professor> profs, std::vector<Disciplina> disc,
    std::vector<Turma> turmas, std::vector<Sala> salas,
    std::vector<RequisicaoAlocacao> reqs,
    std::set<std::tuple<int, int, int>> disponibilidade,
    std::map<int, int> disponibilidadeTotalProf)
    : professores(profs), disciplinas(disc), turmas(turmas), salas(salas),
      requisicoes(reqs), disponibilidadeProfessores(disponibilidade),
      disponibilidadeTotalProfessores(disponibilidadeTotalProf)
{
    for (const auto& p : professores) { mapaNomesProfessores[p.id] = p.nome; }
    for (const auto& d : disciplinas) { mapaNomesDisciplinas[d.id] = d.nome; mapaDisciplinas[d.id] = d; }
    for (const auto& t : turmas) { mapaNomesTurmas[t.id] = t.nome; }
    for (const auto& s : salas) { mapaNomesSalas[s.id] = s.nome; }
}

void GeradorHorario::reset() {
    gradeHoraria.clear();
}

bool GeradorHorario::gerarHorario() {
    std::vector<RequisicaoAlocacao> requisicoesParaTentar = this->requisicoes;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(requisicoesParaTentar.begin(), requisicoesParaTentar.end(), g);

    std::stable_sort(requisicoesParaTentar.begin(), requisicoesParaTentar.end(),
        [&](const RequisicaoAlocacao& a, const RequisicaoAlocacao& b) {
        return disponibilidadeTotalProfessores.at(a.idProfessor) < disponibilidadeTotalProfessores.at(b.idProfessor);
    });

    for (const auto& req : requisicoesParaTentar) {
        if (!tentarAlocarRequisicao(req)) {
            std::cout << "Falha CRITICA ao alocar " << mapaNomesDisciplinas.at(req.idDisciplina)
                      << " para " << mapaNomesTurmas.at(req.idTurma) << ". Tentativa descartada." << std::endl;
            return false;
        }
    }

    std::cout << "\nTentativa de construcao finalizada. Verificando o resultado..." << std::endl;
    return true; // Retorna sucesso se conseguiu alocar tudo sem falha crítica
}

// Retorna uma cópia da grade para que o main possa inspecioná-la
std::vector<Aula> GeradorHorario::getGradeHoraria() {
    return this->gradeHoraria;
}

bool GeradorHorario::tentarAlocarRequisicao(const RequisicaoAlocacao& req) {
    // Como cada requisição agora é para UMA aula, a lógica é mais simples.
    // Primeiro, coletamos todos os slots possíveis para esta aula.
    std::vector<std::pair<Sala, Slot>> slotsDisponiveis;

    // Itera por todas as salas, dias e horas para encontrar locais válidos
    for (const auto& sala : salas) {
        // Regra simples: por enquanto vamos ignorar a necessidade de salas compartilhadas,
        // pois a lógica original já não a implementava de forma robusta.
        // Se precisar disso, a lógica pode ser adicionada aqui.
        for (int dia = 0; dia < 5; ++dia) {
            for (int hora = 0; hora < 6; ++hora) {
                Slot slotAtual = { dia, hora };
                // A função verificarDisponibilidade checa conflitos de professor, turma e sala.
                if (verificarDisponibilidade(req.idTurma, req.idProfessor, sala.id, slotAtual)) {
                    slotsDisponiveis.push_back({ sala, slotAtual });
                }
            }
        }
    }

    // Se não houver nenhum slot disponível em toda a semana, a alocação falha.
    if (slotsDisponiveis.empty()) {
        return false; // Não foi possível encontrar um lugar para esta aula.
    }

    // --- MUDANÇA DE ESTRATÉGIA ---
    // Em vez de pegar o primeiro slot, escolhemos um aleatoriamente.
    // Isso aumenta a variabilidade entre as tentativas e a chance de encontrar uma solução.
    std::random_device rd;
    std::mt19937 g(rd());
    std::uniform_int_distribution<> dist(0, slotsDisponiveis.size() - 1);
    std::pair<Sala, Slot> slotEscolhido = slotsDisponiveis[dist(g)];

    // Cria a nova aula e a adiciona na grade.
    Aula novaAula = { req.idProfessor, req.idDisciplina, req.idTurma, slotEscolhido.first.id, slotEscolhido.second };
    gradeHoraria.push_back(novaAula);

    return true; // Sucesso na alocação desta aula.
}

bool GeradorHorario::verificarDisponibilidade(int idTurma, int idProfessor, int idSala, Slot slot) {
    if (disponibilidadeProfessores.find({ idProfessor, slot.dia, slot.hora }) == disponibilidadeProfessores.end()) {
        return false;
    }
    for (const auto& aula : gradeHoraria) {
        if (aula.slot.dia == slot.dia && aula.slot.hora == slot.hora) {
            if (aula.idProfessor == idProfessor) return false;
            if (aula.idTurma == idTurma) return false;
            if (aula.idSala == idSala) return false;
        }
    }
    return true;
}
void GeradorHorario::imprimirHorario() {
    std::cout << "\n--- GRADE HORARIA GERADA ---" << std::endl;
    const std::vector<std::string> diasNomes = { "Segunda", "Terca", "Quarta", "Quinta", "Sexta" };
    const std::vector<std::string> horariosNomes = { "7:30-8:15", "8:15-9:00", "9:00-9:45", "10:05-10:50", "10:50-11:35", "11:35-12:20" };

    for (const auto& t : turmas) {
        std::cout << "\n============================== HORARIO: " << t.nome << " ==============================\n";
        std::cout << std::left << std::setw(14) << "Horario";
        for (const auto& dia : diasNomes) {
            std::cout << std::setw(30) << dia;
        }
        std::cout << std::endl;
        std::cout << "----------------------------------------------------------------------------------------------------------------------------------\n";

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
}