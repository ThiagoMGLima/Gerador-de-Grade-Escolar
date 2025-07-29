#pragma once
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <chrono>
#include <algorithm>
#include <functional>

// ==============================================================================
// ENUMS - Melhor type safety e legibilidade
// ==============================================================================

enum class TipoSala {
    NORMAL = 0,
    LABORATORIO = 1,
    QUADRA = 2,
    BIBLIOTECA = 3,
    AUDITORIO = 4,
    SALA_INFORMATICA = 5
};

enum class Turno {
    MANHA = 0,
    TARDE = 1,
    NOITE = 2,
    INTEGRAL = 3
};

enum class DiaSemana {
    SEGUNDA = 0,
    TERCA = 1,
    QUARTA = 2,
    QUINTA = 3,
    SEXTA = 4,
    SABADO = 5
};

enum class TipoRestricao {
    PROFESSOR_INDISPONIVEL,
    TURMA_OCUPADA,
    SALA_INADEQUADA,
    HORARIO_BLOQUEADO,
    PERSONALIZADA
};

enum class TipoPreferencia {
    AULAS_CONSECUTIVAS,
    EVITAR_JANELAS,
    DISTRIBUICAO_UNIFORME,
    HORARIOS_PREFERENCIAIS,
    SALAS_ESPECIFICAS
};

// ==============================================================================
// ESTRUTURAS PRINCIPAIS
// ==============================================================================

struct Professor {
    int id;
    std::string nome;
    std::string email;
    std::string telefone;
    int cargaHorariaMaxima;
    std::set<int> disciplinasHabilitadas; // IDs das disciplinas que pode lecionar

    // Construtor padrão
    Professor() : id(0), nome(""), email(""), telefone(""), cargaHorariaMaxima(40) {}

    // Construtor completo
    Professor(int _id, const std::string& _nome, const std::string& _email = "",
              const std::string& _telefone = "", int _cargaMax = 40)
        : id(_id), nome(_nome), email(_email), telefone(_telefone),
          cargaHorariaMaxima(_cargaMax) {}

    // Operador de comparação para uso em containers
    bool operator<(const Professor& other) const {
        return id < other.id;
    }

    bool operator==(const Professor& other) const {
        return id == other.id;
    }

    // Verifica se pode lecionar uma disciplina
    bool podeLecionar(int idDisciplina) const {
        return disciplinasHabilitadas.empty() ||
               disciplinasHabilitadas.count(idDisciplina) > 0;
    }

    // Serialização para debug
    std::string toString() const {
        return "Professor[id=" + std::to_string(id) + ", nome=" + nome + "]";
    }
};

struct Disciplina {
    int id;
    std::string nome;
    std::string codigo;
    // Mapeia um ID de turma para a quantidade de aulas semanais
    std::map<int, int> aulasPorTurma;
    // Mapeia um ID de turma para a quantidade de aulas em local compartilhado
    std::map<int, int> aulasCompartilhadasPorTurma;
    // Preferências de horário (opcional)
    std::set<int> horariosPreferidos;
    // Tipo de sala necessária
    TipoSala tipoSalaPreferida;
    // Se precisa de aulas geminadas (2 seguidas)
    bool requerAulasGeminadas;

    Disciplina() : id(0), nome(""), codigo(""),
                   tipoSalaPreferida(TipoSala::NORMAL),
                   requerAulasGeminadas(false) {}

    Disciplina(int _id, const std::string& _nome, const std::string& _codigo = "")
        : id(_id), nome(_nome), codigo(_codigo),
          tipoSalaPreferida(TipoSala::NORMAL),
          requerAulasGeminadas(false) {}

    // Calcula carga horária total
    int getCargaHorariaTotal() const {
        int total = 0;
        for (const auto& [turmaId, qtd] : aulasPorTurma) {
            total += qtd;
        }
        return total;
    }

    // Verifica se tem aulas para uma turma
    bool temAulasPara(int idTurma) const {
        return aulasPorTurma.count(idTurma) > 0 && aulasPorTurma.at(idTurma) > 0;
    }

    // Obtém quantidade de aulas para uma turma
    int getAulasPara(int idTurma) const {
        auto it = aulasPorTurma.find(idTurma);
        return (it != aulasPorTurma.end()) ? it->second : 0;
    }
};

struct Turma {
    int id;
    std::string nome;
    std::string serie;
    Turno turno;
    int numeroAlunos;
    int salaFixa; // ID da sala fixa (se houver)
    std::set<int> disciplinasCursadas; // IDs das disciplinas

    Turma() : id(0), nome(""), serie(""), turno(Turno::MANHA),
              numeroAlunos(30), salaFixa(-1) {}

    Turma(int _id, const std::string& _nome, const std::string& _serie = "",
          Turno _turno = Turno::MANHA, int _numAlunos = 30)
        : id(_id), nome(_nome), serie(_serie), turno(_turno),
          numeroAlunos(_numAlunos), salaFixa(-1) {}

    // Verifica se está no turno da manhã
    bool isManha() const { return turno == Turno::MANHA; }
    bool isTarde() const { return turno == Turno::TARDE; }
    bool isNoite() const { return turno == Turno::NOITE; }
    bool isIntegral() const { return turno == Turno::INTEGRAL; }

    // Verifica se cursa uma disciplina
    bool cursaDisciplina(int idDisciplina) const {
        return disciplinasCursadas.empty() ||
               disciplinasCursadas.count(idDisciplina) > 0;
    }
};

struct Sala {
    int id;
    std::string nome;
    std::string bloco;
    bool compartilhada;
    TipoSala tipo;
    int capacidade;
    std::set<int> recursosDisponiveis; // IDs de recursos (projetor, lab, etc)
    std::map<int, bool> disponibilidadePorSlot; // Disponibilidade específica

    Sala() : id(0), nome(""), bloco(""), compartilhada(false),
             tipo(TipoSala::NORMAL), capacidade(40) {}

    Sala(int _id, const std::string& _nome, bool _compartilhada,
         TipoSala _tipo = TipoSala::NORMAL, int _capacidade = 40,
         const std::string& _bloco = "")
        : id(_id), nome(_nome), bloco(_bloco), compartilhada(_compartilhada),
          tipo(_tipo), capacidade(_capacidade) {}

    // Verifica se a sala comporta uma turma
    bool comportaTurma(int numeroAlunos) const {
        return capacidade >= numeroAlunos;
    }

    // Verifica se tem um recurso específico
    bool temRecurso(int idRecurso) const {
        return recursosDisponiveis.count(idRecurso) > 0;
    }

    // Verifica se é adequada para uma disciplina
    bool adequadaPara(const Disciplina& disc) const {
        return tipo == disc.tipoSalaPreferida ||
               tipo == TipoSala::NORMAL;
    }

    // Verifica disponibilidade em um slot específico
    bool disponivelEm(int slotId) const {
        auto it = disponibilidadePorSlot.find(slotId);
        return it == disponibilidadePorSlot.end() || it->second;
    }
};

struct Slot {
    int dia;  // 0=Seg, 1=Ter, ...
    int hora; // 0=1º horário, 1=2º horário, ...

    Slot() : dia(0), hora(0) {}
    Slot(int _dia, int _hora) : dia(_dia), hora(_hora) {}

    // Operadores para uso em containers
    bool operator<(const Slot& other) const {
        if (dia != other.dia) return dia < other.dia;
        return hora < other.hora;
    }

    bool operator==(const Slot& other) const {
        return dia == other.dia && hora == other.hora;
    }

    bool operator!=(const Slot& other) const {
        return !(*this == other);
    }

    // Verifica se é horário extremo (primeiro ou último)
    bool isHorarioExtremo() const {
        return hora == 0 || hora == 5;
    }

    // Verifica se é adjacente a outro slot
    bool isAdjacente(const Slot& other) const {
        return dia == other.dia && std::abs(hora - other.hora) == 1;
    }

    // Verifica se é anterior a outro slot
    bool isAntes(const Slot& other) const {
        if (dia < other.dia) return true;
        if (dia > other.dia) return false;
        return hora < other.hora;
    }

    // Calcula distância em slots
    int distancia(const Slot& other) const {
        if (dia == other.dia) {
            return std::abs(hora - other.hora);
        }
        return std::abs(dia - other.dia) * 6 + std::abs(hora - other.hora);
    }

    // Obtém ID único do slot (0-29 para 5 dias x 6 horários)
    int getId() const {
        return dia * 6 + hora;
    }

    // Cria slot a partir de ID
    static Slot fromId(int id) {
        return Slot(id / 6, id % 6);
    }

    // Converte para string para debug
    std::string toString() const {
        const std::vector<std::string> dias = {"Seg", "Ter", "Qua", "Qui", "Sex"};
        const std::vector<std::string> horas = {"7:30", "8:15", "9:00", "10:05", "10:50", "11:35"};

        if (dia >= 0 && dia < 5 && hora >= 0 && hora < 6) {
            return dias[dia] + " " + horas[hora];
        }
        return "Slot inválido";
    }
};

// Representa uma aula alocada na grade final
struct Aula {
    int idProfessor;
    int idDisciplina;
    int idTurma;
    int idSala;
    Slot slot;

    // Metadados adicionais
    std::chrono::system_clock::time_point criadoEm;
    std::chrono::system_clock::time_point modificadoEm;
    int versao; // Para rastreamento de mudanças
    bool fixa;  // Se é uma aula que não pode ser movida

    Aula() : idProfessor(0), idDisciplina(0), idTurma(0), idSala(0),
             versao(1), fixa(false) {
        criadoEm = std::chrono::system_clock::now();
        modificadoEm = criadoEm;
    }

    Aula(int _idProf, int _idDisc, int _idTurma, int _idSala, const Slot& _slot)
        : idProfessor(_idProf), idDisciplina(_idDisc), idTurma(_idTurma),
          idSala(_idSala), slot(_slot), versao(1), fixa(false) {
        criadoEm = std::chrono::system_clock::now();
        modificadoEm = criadoEm;
    }

    // Verifica se conflita com outra aula
    bool conflitaCom(const Aula& outra) const {
        if (slot != outra.slot) {
            return false; // Horários diferentes, sem conflito
        }

        // Mesmo horário, verifica conflitos
        return (idProfessor == outra.idProfessor) ||  // Mesmo professor
               (idTurma == outra.idTurma) ||           // Mesma turma
               (idSala == outra.idSala);               // Mesma sala (se não compartilhada)
    }

    // Verifica se é do mesmo tipo (mesma disciplina e turma)
    bool mesmoTipo(const Aula& outra) const {
        return idDisciplina == outra.idDisciplina && idTurma == outra.idTurma;
    }

    // Gera hash único para a aula
    size_t getHash() const {
        std::hash<int> hasher;
        size_t h1 = hasher(idProfessor);
        size_t h2 = hasher(idDisciplina);
        size_t h3 = hasher(idTurma);
        size_t h4 = hasher(slot.getId());

        return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
    }

    // Atualiza timestamp de modificação
    void marcarModificado() {
        modificadoEm = std::chrono::system_clock::now();
        versao++;
    }

    // Serialização para debug
    std::string toString() const {
        return "Aula[P:" + std::to_string(idProfessor) +
               ", D:" + std::to_string(idDisciplina) +
               ", T:" + std::to_string(idTurma) +
               ", S:" + std::to_string(idSala) +
               ", " + slot.toString() + "]";
    }
};

// Representa a necessidade de alocar uma disciplina para uma turma por um professor
struct RequisicaoAlocacao {
    int idTurma;
    int idDisciplina;
    int idProfessor;
    double custoPreferencia; // Custo combinado (alfa * PST + beta * PTS)
    bool alocada;
    int tentativas; // Número de tentativas de alocação

    // Prioridade da requisição (maior = mais prioritário)
    enum Prioridade {
        BAIXA = 0,
        NORMAL = 1,
        ALTA = 2,
        CRITICA = 3
    };
    Prioridade prioridade;

    // Restrições específicas desta requisição
    std::set<Slot> slotsProibidos;
    std::set<int> salasProibidas;

    RequisicaoAlocacao()
        : idTurma(0), idDisciplina(0), idProfessor(0),
          custoPreferencia(0.0), alocada(false), tentativas(0),
          prioridade(NORMAL) {}

    RequisicaoAlocacao(int _turma, int _disc, int _prof, double _custo = 0.0)
        : idTurma(_turma), idDisciplina(_disc), idProfessor(_prof),
          custoPreferencia(_custo), alocada(false), tentativas(0),
          prioridade(NORMAL) {}

    // Define prioridade baseada em criticidade
    void calcularPrioridade(float criticidade) {
        if (criticidade > 0.9) prioridade = CRITICA;
        else if (criticidade > 0.7) prioridade = ALTA;
        else if (criticidade > 0.4) prioridade = NORMAL;
        else prioridade = BAIXA;
    }

    // Verifica se um slot é permitido
    bool slotPermitido(const Slot& slot) const {
        return slotsProibidos.count(slot) == 0;
    }

    // Verifica se uma sala é permitida
    bool salaPermitida(int idSala) const {
        return salasProibidas.count(idSala) == 0;
    }

    // Incrementa tentativas e retorna se deve continuar tentando
    bool incrementarTentativas(int maxTentativas = 100) {
        tentativas++;
        return tentativas < maxTentativas;
    }
};

// ==============================================================================
// ESTRUTURAS AUXILIARES
// ==============================================================================

// Estrutura para restrições adicionais
struct Restricao {
    TipoRestricao tipo;
    std::string descricao;
    std::set<int> idsAfetados; // IDs de professores/turmas/salas afetados
    std::set<Slot> slotsAfetados;
    std::function<bool(const Aula&)> funcaoValidacao; // Função customizada

    Restricao(TipoRestricao _tipo, const std::string& _desc)
        : tipo(_tipo), descricao(_desc) {}

    // Verifica se a restrição se aplica a um ID e slot
    bool aplicavelA(int id, const Slot& slot) const {
        return (idsAfetados.empty() || idsAfetados.count(id) > 0) &&
               (slotsAfetados.empty() || slotsAfetados.count(slot) > 0);
    }

    // Valida uma aula contra a restrição
    bool validar(const Aula& aula) const {
        if (funcaoValidacao) {
            return funcaoValidacao(aula);
        }

        switch (tipo) {
            case TipoRestricao::PROFESSOR_INDISPONIVEL:
                return !aplicavelA(aula.idProfessor, aula.slot);

            case TipoRestricao::TURMA_OCUPADA:
                return !aplicavelA(aula.idTurma, aula.slot);

            case TipoRestricao::SALA_INADEQUADA:
                return !aplicavelA(aula.idSala, aula.slot);

            case TipoRestricao::HORARIO_BLOQUEADO:
                return slotsAfetados.count(aula.slot) == 0;

            default:
                return true;
        }
    }
};

// Estrutura para preferências
struct Preferencia {
    TipoPreferencia tipo;
    double peso; // 0.0 a 1.0
    std::map<std::string, std::string> parametros;
    std::function<double(const std::vector<Aula>&)> funcaoAvaliacao;

    Preferencia(TipoPreferencia _tipo, double _peso = 1.0)
        : tipo(_tipo), peso(_peso) {}

    // Adiciona um parâmetro
    void setParametro(const std::string& chave, const std::string& valor) {
        parametros[chave] = valor;
    }

    // Obtém um parâmetro
    std::string getParametro(const std::string& chave, const std::string& padrao = "") const {
        auto it = parametros.find(chave);
        return (it != parametros.end()) ? it->second : padrao;
    }

    // Avalia o custo/bônus desta preferência
    double avaliar(const std::vector<Aula>& grade) const {
        if (funcaoAvaliacao) {
            return peso * funcaoAvaliacao(grade);
        }
        return 0.0;
    }
};

// Estrutura para resultado de validação
struct ResultadoValidacao {
    bool valido;
    std::vector<std::string> erros;
    std::vector<std::string> avisos;
    std::vector<std::string> sugestoes;
    std::map<std::string, int> estatisticas;

    ResultadoValidacao() : valido(true) {}

    void adicionarErro(const std::string& erro) {
        erros.push_back(erro);
        valido = false;
    }

    void adicionarAviso(const std::string& aviso) {
        avisos.push_back(aviso);
    }

    void adicionarSugestao(const std::string& sugestao) {
        sugestoes.push_back(sugestao);
    }

    void adicionarEstatistica(const std::string& chave, int valor) {
        estatisticas[chave] = valor;
    }

    // Mescla com outro resultado
    void mesclar(const ResultadoValidacao& outro) {
        valido = valido && outro.valido;
        erros.insert(erros.end(), outro.erros.begin(), outro.erros.end());
        avisos.insert(avisos.end(), outro.avisos.begin(), outro.avisos.end());
        sugestoes.insert(sugestoes.end(), outro.sugestoes.begin(), outro.sugestoes.end());

        for (const auto& [chave, valor] : outro.estatisticas) {
            estatisticas[chave] += valor;
        }
    }

    void imprimir() const {
        if (valido) {
            std::cout << "✅ Validação bem-sucedida!" << std::endl;
        } else {
            std::cout << "❌ Validação falhou com " << erros.size() << " erro(s)" << std::endl;
            for (const auto& erro : erros) {
                std::cout << "  ERRO: " << erro << std::endl;
            }
        }

        if (!avisos.empty()) {
            std::cout << "⚠️  " << avisos.size() << " aviso(s):" << std::endl;
            for (const auto& aviso : avisos) {
                std::cout << "  AVISO: " << aviso << std::endl;
            }
        }

        if (!sugestoes.empty()) {
            std::cout << "💡 " << sugestoes.size() << " sugestão(ões):" << std::endl;
            for (const auto& sugestao : sugestoes) {
                std::cout << "  SUGESTÃO: " << sugestao << std::endl;
            }
        }

        if (!estatisticas.empty()) {
            std::cout << "📊 Estatísticas:" << std::endl;
            for (const auto& [chave, valor] : estatisticas) {
                std::cout << "  " << chave << ": " << valor << std::endl;
            }
        }
    }
};

// Estrutura para recurso de sala
struct Recurso {
    int id;
    std::string nome;
    std::string tipo;
    bool movel; // Se pode ser movido entre salas

    Recurso() : id(0), nome(""), tipo(""), movel(false) {}

    Recurso(int _id, const std::string& _nome, const std::string& _tipo = "", bool _movel = false)
        : id(_id), nome(_nome), tipo(_tipo), movel(_movel) {}
};

// ==============================================================================
// NAMESPACE COM FUNÇÕES UTILITÁRIAS
// ==============================================================================

namespace Util {
    // Converte enum para string
    inline std::string turnoToString(Turno turno) {
        switch (turno) {
            case Turno::MANHA: return "Manhã";
            case Turno::TARDE: return "Tarde";
            case Turno::NOITE: return "Noite";
            case Turno::INTEGRAL: return "Integral";
            default: return "Desconhecido";
        }
    }

    inline std::string tipoSalaToString(TipoSala tipo) {
        switch (tipo) {
            case TipoSala::NORMAL: return "Normal";
            case TipoSala::LABORATORIO: return "Laboratório";
            case TipoSala::QUADRA: return "Quadra";
            case TipoSala::BIBLIOTECA: return "Biblioteca";
            case TipoSala::AUDITORIO: return "Auditório";
            case TipoSala::SALA_INFORMATICA: return "Sala de Informática";
            default: return "Desconhecido";
        }
    }

    inline std::string diaToString(int dia) {
        const std::vector<std::string> dias = {"Segunda", "Terça", "Quarta", "Quinta", "Sexta", "Sábado"};
        return (dia >= 0 && dia < static_cast<int>(dias.size())) ? dias[dia] : "Inválido";
    }

    inline std::string horaToString(int hora) {
        const std::vector<std::string> horas = {
            "7:30-8:15", "8:15-9:00", "9:00-9:45",
            "10:05-10:50", "10:50-11:35", "11:35-12:20"
        };
        return (hora >= 0 && hora < static_cast<int>(horas.size())) ? horas[hora] : "Inválido";
    }

    inline std::string slotToString(const Slot& slot) {
        return diaToString(slot.dia) + " " + horaToString(slot.hora);
    }

    // Parsers
    inline Turno stringToTurno(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower == "manha" || lower == "manhã") return Turno::MANHA;
        if (lower == "tarde") return Turno::TARDE;
        if (lower == "noite") return Turno::NOITE;
        if (lower == "integral") return Turno::INTEGRAL;

        return Turno::MANHA; // Padrão
    }

    inline TipoSala stringToTipoSala(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

        if (lower == "laboratorio" || lower == "laboratório") return TipoSala::LABORATORIO;
        if (lower == "quadra") return TipoSala::QUADRA;
        if (lower == "biblioteca") return TipoSala::BIBLIOTECA;
        if (lower == "auditorio" || lower == "auditório") return TipoSala::AUDITORIO;
        if (lower == "informatica" || lower == "informática") return TipoSala::SALA_INFORMATICA;

        return TipoSala::NORMAL; // Padrão
    }

    // Validações
    inline bool isDiaValido(int dia) {
        return dia >= 0 && dia <= 5;
    }

    inline bool isHoraValida(int hora) {
        return hora >= 0 && hora <= 5;
    }

    inline bool isSlotValido(const Slot& slot) {
        return isDiaValido(slot.dia) && isHoraValida(slot.hora);
    }

    // Helpers para horários
    inline bool isHorarioManha(int hora) {
        return hora >= 0 && hora <= 2;
    }

    inline bool isHorarioTarde(int hora) {
        return hora >= 3 && hora <= 5;
    }

    inline bool isHorarioNoite(int hora) {
        return hora >= 6; // Se houver horário noturno
    }

    // Formatação
    inline std::string formatarDuracao(int minutos) {
        if (minutos < 60) {
            return std::to_string(minutos) + " min";
        } else {
            int horas = minutos / 60;
            int mins = minutos % 60;
            if (mins == 0) {
                return std::to_string(horas) + "h";
            } else {
                return std::to_string(horas) + "h " + std::to_string(mins) + "min";
            }
        }
    }

    // Calcula duração entre dois slots
    inline int calcularDuracao(const Slot& inicio, const Slot& fim) {
        if (inicio.dia != fim.dia) {
            return -1; // Slots em dias diferentes
        }
        return (fim.hora - inicio.hora) * 45;
    }
}