// Estrutura de dados principal
let dados = {
    turmas: [],
    disciplinas: [],
    professores: [],
    salas: [],
    nextId: {
        turma: 1,
        disciplina: 101,
        professor: 201,
        sala: 501
    }
};

// Constantes do sistema
const DIAS = ['Segunda', 'Terça', 'Quarta', 'Quinta', 'Sexta'];
const HORARIOS = ['7:30-8:15', '8:15-9:00', '9:00-9:45', '10:05-10:50', '10:50-11:35', '11:35-12:20'];
const TURNOS = {
    manha: { nome: 'Manhã', horarios: [0, 1, 2, 3, 4, 5] },
    tarde: { nome: 'Tarde', horarios: [6, 7, 8, 9, 10, 11] },
    noite: { nome: 'Noite', horarios: [12, 13, 14, 15, 16, 17] }
};

// Sistema de notificações melhorado
const Notificacao = {
    container: null,

    init() {
        this.container = document.createElement('div');
        this.container.className = 'notification-container';
        document.body.appendChild(this.container);
    },

    mostrar(mensagem, tipo = 'info', duracao = 3000) {
        const notif = document.createElement('div');
        notif.className = `notification ${tipo}`;
        notif.innerHTML = `
            <span class="notification-icon">${this.getIcon(tipo)}</span>
            <span class="notification-message">${mensagem}</span>
            <button class="notification-close" onclick="this.parentElement.remove()">×</button>
        `;

        this.container.appendChild(notif);

        // Animação de entrada
        setTimeout(() => notif.classList.add('show'), 10);

        // Auto remover
        if (duracao > 0) {
            setTimeout(() => {
                notif.classList.remove('show');
                setTimeout(() => notif.remove(), 300);
            }, duracao);
        }
    },

    getIcon(tipo) {
        const icons = {
            success: '✓',
            error: '✗',
            warning: '⚠',
            info: 'ℹ'
        };
        return icons[tipo] || icons.info;
    }
};

// ========== FUNÇÕES DE PERSISTÊNCIA ==========

function carregarDados() {
    try {
        const savedData = localStorage.getItem('gradeDados');
        if (savedData) {
            const parsed = JSON.parse(savedData);
            // Validar estrutura dos dados
            if (parsed && typeof parsed === 'object') {
                dados = { ...dados, ...parsed };
            }
        }
    } catch (e) {
        console.error('Erro ao carregar dados:', e);
        Notificacao.mostrar('Erro ao carregar dados salvos. Iniciando com dados vazios.', 'error');
    }

    atualizarTodasTabelas();
    atualizarResumo();
}

function salvarDados() {
    try {
        localStorage.setItem('gradeDados', JSON.stringify(dados));
        // Salvar backup com timestamp
        const backup = {
            dados: dados,
            timestamp: new Date().toISOString()
        };
        localStorage.setItem(`gradeDados_backup_${Date.now()}`, JSON.stringify(backup));

        // Limpar backups antigos (manter apenas os 5 mais recentes)
        limparBackupsAntigos();
    } catch (e) {
        console.error('Erro ao salvar dados:', e);
        Notificacao.mostrar('Erro ao salvar dados', 'error');
    }
}

function limparBackupsAntigos() {
    const backups = [];
    for (let i = 0; i < localStorage.length; i++) {
        const key = localStorage.key(i);
        if (key && key.startsWith('gradeDados_backup_')) {
            backups.push(key);
        }
    }

    backups.sort().reverse();
    backups.slice(5).forEach(key => localStorage.removeItem(key));
}

function importarDados() {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.json';

    input.onchange = (e) => {
        const file = e.target.files[0];
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (event) => {
            try {
                const importedData = JSON.parse(event.target.result);

                // Validar estrutura
                if (!importedData.turmas || !importedData.disciplinas ||
                    !importedData.professores || !importedData.salas) {
                    throw new Error('Formato de arquivo inválido');
                }

                // Confirmar importação
                if (confirm('Isso substituirá todos os dados atuais. Deseja continuar?')) {
                    dados = importedData;
                    salvarDados();
                    atualizarTodasTabelas();
                    atualizarResumo();
                    Notificacao.mostrar('Dados importados com sucesso!', 'success');
                }
            } catch (error) {
                Notificacao.mostrar('Erro ao importar arquivo: ' + error.message, 'error');
            }
        };

        reader.readAsText(file);
    };

    input.click();
}

// ========== FUNÇÕES DE NAVEGAÇÃO ==========

function showTab(tabName) {
    // Esconder todas as abas
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.style.display = 'none';
        tab.classList.remove('active');
    });

    // Remover classe active de todos os botões
    document.querySelectorAll('.nav-tab').forEach(tab => {
        tab.classList.remove('active');
    });

    // Mostrar aba selecionada
    const selectedTab = document.getElementById(tabName + '-tab');
    if (selectedTab) {
        selectedTab.style.display = 'block';
        selectedTab.classList.add('active');
    }

    // Adicionar classe active ao botão
    const activeButton = document.querySelector(`[onclick*="showTab('${tabName}')"]`);
    if (activeButton) {
        activeButton.classList.add('active');
    }

    // Executar ações específicas de cada aba
    switch (tabName) {
        case 'turmas':
            // Focar no campo de entrada
            setTimeout(() => document.getElementById('turmaNome')?.focus(), 100);
            break;
        case 'disciplinas':
            atualizarCargaHorariaForm();
            break;
        case 'professores':
            atualizarDisciplinasSelect();
            criarGridDisponibilidade();
            break;
        case 'salas':
            // Atualizar estatísticas de salas
            atualizarEstatisticasSalas();
            break;
        case 'resumo':
            atualizarResumo();
            validarDados();
            break;
    }
}

// ========== FUNÇÕES PARA TURMAS ==========

function adicionarTurma() {
    const nome = document.getElementById('turmaNome').value.trim();
    const turno = document.getElementById('turmaTurno').value;

    // Validações
    if (!nome) {
        Notificacao.mostrar('Por favor, preencha o nome da turma', 'error');
        document.getElementById('turmaNome').focus();
        return;
    }

    if (nome.length < 3) {
        Notificacao.mostrar('O nome da turma deve ter pelo menos 3 caracteres', 'error');
        return;
    }

    if (dados.turmas.some(t => t.nome.toLowerCase() === nome.toLowerCase())) {
        Notificacao.mostrar('Já existe uma turma com este nome', 'error');
        return;
    }

    // Criar nova turma
    const turma = {
        id: dados.nextId.turma++,
        nome: nome,
        turno: turno,
        criadoEm: new Date().toISOString()
    };

    dados.turmas.push(turma);
    salvarDados();

    // Limpar formulário
    document.getElementById('turmaNome').value = '';
    document.getElementById('turmaNome').focus();

    // Atualizar interface
    atualizarTabelaTurmas();
    Notificacao.mostrar(`Turma "${nome}" adicionada com sucesso!`, 'success');

    // Adicionar animação à nova linha
    setTimeout(() => {
        const novaLinha = document.querySelector(`#turmasTable tr[data-id="${turma.id}"]`);
        if (novaLinha) {
            novaLinha.classList.add('highlight-new');
            setTimeout(() => novaLinha.classList.remove('highlight-new'), 2000);
        }
    }, 100);
}

function atualizarTabelaTurmas() {
    const tbody = document.querySelector('#turmasTable tbody');
    tbody.innerHTML = '';

    // Ordenar turmas por nome
    const turmasOrdenadas = [...dados.turmas].sort((a, b) =>
        a.nome.localeCompare(b.nome, 'pt-BR', { numeric: true })
    );

    turmasOrdenadas.forEach(turma => {
        const tr = document.createElement('tr');
        tr.dataset.id = turma.id;

        // Calcular estatísticas da turma
        const totalAulas = calcularTotalAulasTurma(turma.id);
        const statusClass = totalAulas === 30 ? 'status-ok' : totalAulas > 30 ? 'status-warning' : '';

        tr.innerHTML = `
            <td>${turma.id}</td>
            <td class="turma-nome">${escapeHtml(turma.nome)}</td>
            <td>
                <span class="badge badge-${turma.turno}">
                    ${TURNOS[turma.turno].nome}
                </span>
            </td>
            <td>
                <span class="aulas-count ${statusClass}">
                    ${totalAulas}/30 aulas
                </span>
            </td>
            <td class="action-buttons">
                <button class="btn btn-secondary" onclick="editarTurma(${turma.id})" title="Editar">
                    <span class="btn-icon">✏️</span> Editar
                </button>
                <button class="btn btn-danger" onclick="excluirTurma(${turma.id})" title="Excluir">
                    <span class="btn-icon">🗑️</span> Excluir
                </button>
            </td>`;
        tbody.appendChild(tr);
    });

    // Atualizar contador
    atualizarContador('turmas', turmasOrdenadas.length);
}

function calcularTotalAulasTurma(turmaId) {
    let total = 0;
    dados.disciplinas.forEach(disc => {
        if (disc.cargaHoraria[turmaId]) {
            total += disc.cargaHoraria[turmaId];
        }
    });
    return total;
}

function editarTurma(id) {
    const turma = dados.turmas.find(t => t.id === id);
    if (!turma) return;

    // Criar modal de edição
    const modal = criarModal({
        titulo: 'Editar Turma',
        conteudo: `
            <div class="form-group">
                <label for="editTurmaNome">Nome da Turma</label>
                <input type="text" id="editTurmaNome" value="${escapeHtml(turma.nome)}" class="form-control">
            </div>
            <div class="form-group">
                <label for="editTurmaTurno">Turno</label>
                <select id="editTurmaTurno" class="form-control">
                    ${Object.entries(TURNOS).map(([key, value]) =>
                        `<option value="${key}" ${turma.turno === key ? 'selected' : ''}>
                            ${value.nome}
                        </option>`
                    ).join('')}
                </select>
            </div>
        `,
        botoes: [
            {
                texto: 'Cancelar',
                classe: 'btn-secondary',
                acao: () => modal.fechar()
            },
            {
                texto: 'Salvar',
                classe: 'btn-primary',
                acao: () => {
                    const novoNome = document.getElementById('editTurmaNome').value.trim();
                    const novoTurno = document.getElementById('editTurmaTurno').value;

                    if (!novoNome) {
                        Notificacao.mostrar('O nome não pode estar vazio', 'error');
                        return;
                    }

                    if (dados.turmas.some(t => t.id !== id && t.nome.toLowerCase() === novoNome.toLowerCase())) {
                        Notificacao.mostrar('Já existe uma turma com este nome', 'error');
                        return;
                    }

                    turma.nome = novoNome;
                    turma.turno = novoTurno;
                    turma.modificadoEm = new Date().toISOString();

                    salvarDados();
                    atualizarTabelaTurmas();
                    modal.fechar();
                    Notificacao.mostrar('Turma atualizada com sucesso!', 'success');
                }
            }
        ]
    });

    modal.abrir();
    setTimeout(() => document.getElementById('editTurmaNome').select(), 100);
}

function excluirTurma(id) {
    const turma = dados.turmas.find(t => t.id === id);
    if (!turma) return;

    // Verificar dependências
    const disciplinasAfetadas = dados.disciplinas.filter(d => d.cargaHoraria[id] > 0).length;
    const professoresAfetados = contarProfessoresAfetados(id);

    let mensagemAviso = `Tem certeza que deseja excluir a turma "${turma.nome}"?`;

    if (disciplinasAfetadas > 0 || professoresAfetados > 0) {
        mensagemAviso += '\n\nIsso afetará:';
        if (disciplinasAfetadas > 0) {
            mensagemAviso += `\n- ${disciplinasAfetadas} disciplina(s)`;
        }
        if (professoresAfetados > 0) {
            mensagemAviso += `\n- ${professoresAfetados} professor(es)`;
        }
    }

    const modal = criarModal({
        titulo: '⚠️ Confirmar Exclusão',
        conteudo: `
            <p>${mensagemAviso.replace(/\n/g, '<br>')}</p>
            <p class="text-danger mt-3"><strong>Esta ação não pode ser desfeita!</strong></p>
        `,
        botoes: [
            {
                texto: 'Cancelar',
                classe: 'btn-secondary',
                acao: () => modal.fechar()
            },
            {
                texto: 'Excluir',
                classe: 'btn-danger',
                acao: () => {
                    // Remover turma
                    dados.turmas = dados.turmas.filter(t => t.id !== id);

                    // Limpar referências
                    dados.disciplinas.forEach(d => delete d.cargaHoraria[id]);

                    salvarDados();
                    atualizarTabelaTurmas();
                    modal.fechar();
                    Notificacao.mostrar(`Turma "${turma.nome}" excluída!`, 'success');
                }
            }
        ]
    });

    modal.abrir();
}

function contarProfessoresAfetados(turmaId) {
    // Contar quantos professores seriam afetados pela exclusão da turma
    let count = 0;

    dados.professores.forEach(prof => {
        const disciplina = dados.disciplinas.find(d => d.id === prof.disciplinaId);
        if (disciplina && disciplina.cargaHoraria[turmaId] > 0) {
            // Este professor dá aula para esta turma
            count++;
        }
    });

    return count;
}

// Adicionar também uma função para analisar impacto completo
function analisarImpactoExclusaoTurma(turmaId) {
    const impacto = {
        professoresAfetados: [],
        disciplinasAfetadas: [],
        totalAulasRemovidas: 0
    };

    // Analisar disciplinas afetadas
    dados.disciplinas.forEach(disc => {
        if (disc.cargaHoraria[turmaId] > 0) {
            impacto.disciplinasAfetadas.push({
                disciplina: disc,
                aulasRemovidas: disc.cargaHoraria[turmaId]
            });
            impacto.totalAulasRemovidas += disc.cargaHoraria[turmaId];

            // Encontrar professor desta disciplina
            const professor = dados.professores.find(p => p.disciplinaId === disc.id);
            if (professor && !impacto.professoresAfetados.some(p => p.id === professor.id)) {
                impacto.professoresAfetados.push(professor);
            }
        }
    });

    return impacto;
}

function atualizarAnaliseDetalhada(analise) {
    let analiseHtml = '<div class="analise-detalhada">';

    // Resumo geral
    analiseHtml += `
        <div class="analise-section">
            <h4>📊 Resumo Geral</h4>
            <p>Total de aulas previstas: <strong>${analise.totalAulasPrevistas}</strong></p>
            <p>Média por turma: <strong>${(analise.totalAulasPrevistas / dados.turmas.length).toFixed(1)}</strong> aulas</p>
        </div>
    `;

    // Problemas identificados
    if (analise.turmasSemAulas.length > 0 ||
        analise.disciplinasSemProfessor.length > 0 ||
        analise.professoresComCargaInsuficiente.length > 0 ||
        analise.salasInsuficientes) {

        analiseHtml += '<div class="analise-section problemas">';
        analiseHtml += '<h4>⚠️ Problemas Identificados</h4>';

        if (analise.turmasSemAulas.length > 0) {
            analiseHtml += '<div class="problema-item">';
            analiseHtml += '<strong>Turmas sem aulas:</strong><ul>';
            analise.turmasSemAulas.forEach(t => {
                analiseHtml += `<li>${escapeHtml(t.nome)}</li>`;
            });
            analiseHtml += '</ul></div>';
        }

        if (analise.disciplinasSemProfessor.length > 0) {
            analiseHtml += '<div class="problema-item">';
            analiseHtml += '<strong>Disciplinas sem professor:</strong><ul>';
            analise.disciplinasSemProfessor.forEach(d => {
                analiseHtml += `<li>${escapeHtml(d.nome)}</li>`;
            });
            analiseHtml += '</ul></div>';
        }

        if (analise.professoresComCargaInsuficiente.length > 0) {
            analiseHtml += '<div class="problema-item">';
            analiseHtml += '<strong>Professores com disponibilidade insuficiente:</strong><ul>';
            analise.professoresComCargaInsuficiente.forEach(p => {
                analiseHtml += `<li>${escapeHtml(p.professor.nome)} - ${escapeHtml(p.disciplina.nome)}:
                    ${p.disponivel}h disponível de ${p.necessario}h necessário</li>`;
            });
            analiseHtml += '</ul></div>';
        }

        if (analise.salasInsuficientes) {
            analiseHtml += '<div class="problema-item">';
            analiseHtml += `<strong>Salas insuficientes:</strong>
                ${dados.turmas.length} turmas mas apenas ${dados.salas.filter(s => !s.compartilhada).length} salas exclusivas`;
            analiseHtml += '</div>';
        }

        analiseHtml += '</div>';
    } else {
        analiseHtml += `
            <div class="analise-section sucesso">
                <h4>✅ Tudo OK!</h4>
                <p>Todos os dados parecem estar corretos e prontos para gerar a grade horária.</p>
            </div>
        `;
    }

    analiseHtml += '</div>';

    // Atualizar o container
    let container = document.getElementById('analiseContainer');
    if (!container) {
        container = document.createElement('div');
        container.id = 'analiseContainer';
        document.querySelector('.summary-cards').after(container);
    }
    container.innerHTML = analiseHtml;
}

function validarDados() {
    const results = document.getElementById('validationResults');
    const analise = analisarDados();
    let errors = [];
    let warnings = [];

    // Verificações críticas (erros)
    if (dados.turmas.length === 0) errors.push("Nenhuma turma cadastrada");
    if (dados.disciplinas.length === 0) errors.push("Nenhuma disciplina cadastrada");
    if (dados.professores.length === 0) errors.push("Nenhum professor cadastrado");
    if (dados.salas.length === 0) errors.push("Nenhuma sala cadastrada");

    // Verificações importantes (avisos)
    if (analise.salasInsuficientes) {
        warnings.push(`Há ${dados.turmas.length} turmas, mas apenas ${dados.salas.filter(s => !s.compartilhada).length} salas exclusivas`);
    }

    analise.disciplinasSemProfessor.forEach(d => {
        errors.push(`A disciplina "${d.nome}" não tem professor atribuído`);
    });

    analise.professoresComCargaInsuficiente.forEach(p => {
        warnings.push(`Professor "${p.professor.nome}": disponibilidade insuficiente (${p.disponivel}h disponível, ${p.necessario}h necessário)`);
    });

    // Exibir resultados
    let html = '';

    if (errors.length > 0) {
        html += `
            <div class="message error">
                <strong>❌ Erros Críticos (devem ser corrigidos):</strong>
                <ul>${errors.map(e => `<li>${e}</li>`).join('')}</ul>
            </div>
        `;
    }

    if (warnings.length > 0) {
        html += `
            <div class="message warning">
                <strong>⚠️ Avisos (podem dificultar a geração):</strong>
                <ul>${warnings.map(w => `<li>${w}</li>`).join('')}</ul>
            </div>
        `;
    }

    if (errors.length === 0 && warnings.length === 0) {
        html = '<div class="message success">✅ Dados válidos e prontos para exportação!</div>';
    }

    results.innerHTML = html;
    return errors.length === 0;
}

function exportarDados() {
    if (!validarDados()) {
        Notificacao.mostrar("Corrija os erros críticos antes de exportar", "error");
        return;
    }

    // Associar turmas a salas
    const salasNormais = dados.salas.filter(s => !s.compartilhada);
    const turmaSalaMap = {};

    dados.turmas.forEach((turma, index) => {
        if (index < salasNormais.length) {
            turmaSalaMap[turma.id] = salasNormais[index].id;
        }
    });

    const exportData = {
        metadata: {
            exportadoEm: new Date().toISOString(),
            versao: "2.0"
        },
        turmas: dados.turmas.map(t => ({
            id: t.id,
            nome: t.nome,
            turno: t.turno
        })),
        disciplinas: dados.disciplinas.map(d => ({
            id: d.id,
            nome: d.nome,
            aulasPorTurma: d.cargaHoraria
        })),
        professores: dados.professores.map(p => ({
            id: p.id,
            nome: p.nome,
            idDisciplina: p.disciplinaId,
            disponibilidade: p.disponibilidade
        })),
        salas: dados.salas.map(s => ({
            id: s.id,
            nome: s.nome,
            tipo: s.tipo,
            compartilhada: s.compartilhada,
            capacidade: s.capacidade
        })),
        associacoes: {
            turmaSala: turmaSalaMap
        }
    };

    // Gerar arquivo
    const dataStr = JSON.stringify(exportData, null, 2);
    const dataBlob = new Blob([dataStr], { type: 'application/json' });
    const url = URL.createObjectURL(dataBlob);
    const link = document.createElement('a');
    link.href = url;
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
    link.download = `grade_horaria_dados_${timestamp}.json`;
    link.click();
    URL.revokeObjectURL(url);

    Notificacao.mostrar("Dados exportados com sucesso!", "success");
    function gerarGradeDiretamente() {
    // Primeiro validar os dados
    if (!validarDados()) {
        Notificacao.mostrar("Corrija os erros críticos antes de gerar a grade", "error");
        return;
    }

    // Preparar dados para exportação
    const salasNormais = dados.salas.filter(s => !s.compartilhada);
    const turmaSalaMap = {};

    dados.turmas.forEach((turma, index) => {
        if (index < salasNormais.length) {
            turmaSalaMap[turma.id] = salasNormais[index].id;
        }
    });

    const exportData = {
        metadata: {
            exportadoEm: new Date().toISOString(),
            versao: "2.0",
            origem: "cadastro_direto"
        },
        turmas: dados.turmas.map(t => ({
            id: t.id,
            nome: t.nome,
            turno: t.turno
        })),
        disciplinas: dados.disciplinas.map(d => ({
            id: d.id,
            nome: d.nome,
            aulasPorTurma: d.cargaHoraria
        })),
        professores: dados.professores.map(p => ({
            id: p.id,
            nome: p.nome,
            idDisciplina: p.disciplinaId,
            disponibilidade: p.disponibilidade
        })),
        salas: dados.salas.map(s => ({
            id: s.id,
            nome: s.nome,
            tipo: s.tipo,
            compartilhada: s.compartilhada,
            capacidade: s.capacidade
        })),
        associacoes: {
            turmaSala: turmaSalaMap
        }
    };

    // Salvar no sessionStorage para a próxima página
    sessionStorage.setItem('dadosGradeTemp', JSON.stringify(exportData));

    // Mostrar modal de confirmação
    const modal = criarModal({
        titulo: '🚀 Gerar Grade Horária',
        conteudo: `
            <div style="text-align: center;">
                <div style="font-size: 48px; margin-bottom: 20px;">⚙️</div>
                <h4>Seus dados estão prontos!</h4>
                <p>Você será redirecionado para o gerador de grade horária.</p>
                <br>
                <div style="background: #e3f2fd; padding: 15px; border-radius: 8px; margin: 15px 0;">
                    <strong>Dados que serão processados:</strong><br>
                    ✓ ${dados.turmas.length} turmas<br>
                    ✓ ${dados.disciplinas.length} disciplinas<br>
                    ✓ ${dados.professores.length} professores<br>
                    ✓ ${dados.salas.length} salas
                </div>
                <p style="color: #666; font-size: 0.9em;">
                    Seus dados foram salvos automaticamente e estarão disponíveis na próxima tela.
                </p>
            </div>
        `,
        botoes: [
            {
                texto: 'Cancelar',
                classe: 'btn-secondary',
                acao: () => modal.fechar()
            },
            {
                texto: '🚀 Continuar para Geração',
                classe: 'btn-success',
                acao: () => {
                    modal.fechar();
                    // Salvar também no localStorage como backup
                    localStorage.setItem('ultimosDadosCadastrados', JSON.stringify(dados));

                    // Redirecionar para o sistema integrado
                    window.location.href = '../sistemaIntegrado.html';
                }
            }
        ]
    });

    modal.abrir();
}

// Adicionar atalho Ctrl+G para gerar grade
document.addEventListener('keydown', (e) => {
    if (e.ctrlKey && e.key === 'g') {
        e.preventDefault();
        gerarGradeDiretamente();
    }
});
}

function exportarCodigo() {
    if (!validarDados()) {
        Notificacao.mostrar("Corrija os erros críticos antes de exportar", "error");
        return;
    }

    let cppCode = `// ==================================================
// Dados gerados automaticamente pelo Sistema de Cadastro
// Data: ${new Date().toLocaleString('pt-BR')}
// ==================================================

void setupDadosExemplo(
    std::vector<Professor>& profs,
    std::vector<Disciplina>& discs,
    std::vector<Turma>& turmas,
    std::vector<Sala>& salas,
    std::vector<RequisicaoAlocacao>& reqs,
    std::set<std::tuple<int, int, int>>& disponibilidade,
    std::map<int, int>& turmaSalaMap
) {
    // Limpar dados existentes
    profs.clear();
    discs.clear();
    turmas.clear();
    salas.clear();
    reqs.clear();
    disponibilidade.clear();
    turmaSalaMap.clear();

    // === TURMAS ===
`;

    // Turmas
    dados.turmas.forEach(t => {
        cppCode += `    turmas.push_back({${t.id}, "${t.nome}"});\n`;
    });

    cppCode += `\n    // === DISCIPLINAS ===\n`;

    // Disciplinas
    dados.disciplinas.forEach(d => {
        cppCode += `    {\n`;
        cppCode += `        Disciplina disc;\n`;
        cppCode += `        disc.id = ${d.id};\n`;
        cppCode += `        disc.nome = "${d.nome}";\n`;

        Object.entries(d.cargaHoraria).forEach(([turmaId, carga]) => {
            if (carga > 0) {
                cppCode += `        disc.aulasPorTurma[${turmaId}] = ${carga};\n`;
            }
        });

        cppCode += `        discs.push_back(disc);\n`;
        cppCode += `    }\n`;
    });

    cppCode += `\n    // === PROFESSORES ===\n`;

    // Professores
    dados.professores.forEach(p => {
        cppCode += `    profs.push_back({${p.id}, "${p.nome}"});\n`;
    });

    cppCode += `\n    // === DISPONIBILIDADE DOS PROFESSORES ===\n`;

    // Disponibilidade
    dados.professores.forEach(p => {
        if (p.disponibilidade.length > 0) {
            cppCode += `    // ${p.nome}\n`;
            p.disponibilidade.forEach(d => {
                cppCode += `    disponibilidade.insert({${p.id}, ${d.dia}, ${d.horario}});\n`;
            });
        }
    });

    cppCode += `\n    // === SALAS ===\n`;

    // Salas
    dados.salas.forEach(s => {
        cppCode += `    salas.push_back({${s.id}, "${s.nome}", ${s.compartilhada ? 'true' : 'false'}});\n`;
    });

    cppCode += `\n    // === ASSOCIAÇÃO TURMA-SALA ===\n`;

    // Mapeamento Turma-Sala
    const salasNormais = dados.salas.filter(s => !s.compartilhada);
    dados.turmas.forEach((turma, index) => {
        if (index < salasNormais.length) {
            cppCode += `    turmaSalaMap[${turma.id}] = ${salasNormais[index].id};\n`;
        }
    });

    cppCode += `\n    // === REQUISIÇÕES DE ALOCAÇÃO ===\n`;
    cppCode += `    // Gerar requisições baseadas nas disciplinas e professores\n`;
    cppCode += `    for (const auto& disc : discs) {\n`;
    cppCode += `        // Encontrar professor para esta disciplina\n`;
    cppCode += `        int idProfessor = -1;\n`;
    cppCode += `        // (Implementar lógica de busca do professor)\n`;
    cppCode += `        \n`;
    cppCode += `        for (const auto& [idTurma, qtdAulas] : disc.aulasPorTurma) {\n`;
    cppCode += `            for (int i = 0; i < qtdAulas; i++) {\n`;
    cppCode += `                reqs.push_back({idTurma, disc.id, idProfessor, 0.0, false});\n`;
    cppCode += `            }\n`;
    cppCode += `        }\n`;
    cppCode += `    }\n`;
    cppCode += `}\n`;

    // Mostrar código em modal
    const modal = criarModal({
        titulo: '📄 Código C++ Gerado',
        conteudo: `
            <div class="code-export">
                <div class="code-actions">
                    <button class="btn btn-sm btn-primary" onclick="copiarCodigo()">
                        📋 Copiar Código
                    </button>
                    <button class="btn btn-sm btn-secondary" onclick="baixarCodigo()">
                        💾 Baixar Arquivo
                    </button>
                </div>
                <textarea id="codigoGerado" class="code-textarea" readonly>${escapeHtml(cppCode)}</textarea>
            </div>
        `,
        botoes: [
            {
                texto: 'Fechar',
                classe: 'btn-primary',
                acao: () => modal.fechar()
            }
        ],
        largura: '80%'
    });

    modal.abrir();

    // Funções auxiliares para o modal
    window.copiarCodigo = function() {
        const textarea = document.getElementById('codigoGerado');
        navigator.clipboard.writeText(textarea.value)
            .then(() => {
                Notificacao.mostrar('Código copiado!', 'success');
            })
            .catch(() => {
                Notificacao.mostrar('Falha ao copiar o código', 'error');
            });
    };

    window.baixarCodigo = function() {
        const blob = new Blob([cppCode], { type: 'text/plain' });
        const url = URL.createObjectURL(blob);
        const link = document.createElement('a');
        link.href = url;
        link.download = 'dados_grade_horaria.cpp';
        link.click();
        URL.revokeObjectURL(url);
    };
}

// ========== FUNÇÕES UTILITÁRIAS ==========

function escapeHtml(text) {
    const map = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#039;'
    };
    return text.replace(/[&<>"']/g, m => map[m]);
}

function atualizarContador(tipo, quantidade) {
    const badge = document.querySelector(`[onclick*="showTab('${tipo}')"] .tab-badge`);
    if (!badge) {
        const button = document.querySelector(`[onclick*="showTab('${tipo}')"]`);
        if (button) {
            const newBadge = document.createElement('span');
            newBadge.className = 'tab-badge';
            newBadge.textContent = quantidade;
            button.appendChild(newBadge);
        }
    } else {
        badge.textContent = quantidade;
    }
}

function criarModal(opcoes) {
    const { titulo, conteudo, botoes, largura = '600px' } = opcoes;

    // Criar elementos do modal
    const overlay = document.createElement('div');
    overlay.className = 'modal-overlay';

    const modal = document.createElement('div');
    modal.className = 'modal-container';
    modal.style.maxWidth = largura;

    const header = document.createElement('div');
    header.className = 'modal-header';
    header.innerHTML = `
        <h3>${titulo}</h3>
        <button class="modal-close" onclick="this.closest('.modal-overlay').remove()">×</button>
    `;

    const body = document.createElement('div');
    body.className = 'modal-body';
    body.innerHTML = conteudo;

    const footer = document.createElement('div');
    footer.className = 'modal-footer';

    botoes.forEach(botao => {
        const btn = document.createElement('button');
        btn.className = `btn ${botao.classe}`;
        btn.textContent = botao.texto;
        btn.onclick = botao.acao;
        footer.appendChild(btn);
    });

    modal.appendChild(header);
    modal.appendChild(body);
    modal.appendChild(footer);
    overlay.appendChild(modal);

    return {
        abrir: () => {
            document.body.appendChild(overlay);
            setTimeout(() => overlay.classList.add('show'), 10);
        },
        fechar: () => {
            overlay.classList.remove('show');
            setTimeout(() => overlay.remove(), 300);
        }
    };
}

function atualizarTodasTabelas() {
    atualizarTabelaTurmas();
    atualizarTabelaDisciplinas();
    atualizarTabelaProfessores();
    atualizarTabelaSalas();
}

// ========== INICIALIZAÇÃO ==========

document.addEventListener('DOMContentLoaded', () => {
    // Inicializar sistema de notificações
    Notificacao.init();

    // Carregar dados salvos
    carregarDados();

    // Configurar navegação
    document.querySelectorAll('.nav-tab').forEach(button => {
        button.addEventListener('click', (e) => {
            e.preventDefault();
            const tabName = button.getAttribute('onclick').match(/'([^']+)'/)[1];
            showTab(tabName);
        });
    });

    // Adicionar listeners para teclas de atalho
    document.addEventListener('keydown', (e) => {
        // Ctrl+S para salvar/exportar
        if (e.ctrlKey && e.key === 's') {
            e.preventDefault();
            exportarDados();
        }

        // Esc para fechar modais
        if (e.key === 'Escape') {
            const modal = document.querySelector('.modal-overlay');
            if (modal) modal.remove();
        }
    });

    // Adicionar campos extras no HTML (email e telefone para professores)
    const professorForm = document.querySelector('#professores-tab .form-grid');
    if (professorForm) {
        const emailGroup = document.createElement('div');
        emailGroup.className = 'form-group';
        emailGroup.innerHTML = `
            <label for="professorEmail">Email (opcional)</label>
            <input type="email" id="professorEmail" placeholder="professor@escola.com">
        `;

        const telefoneGroup = document.createElement('div');
        telefoneGroup.className = 'form-group';
        telefoneGroup.innerHTML = `
            <label for="professorTelefone">Telefone (opcional)</label>
            <input type="tel" id="professorTelefone" placeholder="(11) 99999-9999">
        `;

        // Inserir após o campo de disciplina
        const disciplinaGroup = professorForm.querySelector('.form-group:nth-child(2)');
        if (disciplinaGroup) {
            disciplinaGroup.after(emailGroup);
            emailGroup.after(telefoneGroup);
        }
    }

    // Adicionar campo de capacidade para salas
    const salaForm = document.querySelector('#salas-tab .form-grid');
    if (salaForm) {
        const capacidadeGroup = document.createElement('div');
        capacidadeGroup.className = 'form-group';
        capacidadeGroup.innerHTML = `
            <label for="salaCapacidade">Capacidade</label>
            <input type="number" id="salaCapacidade" min="1" value="30" placeholder="30">
        `;

        // Inserir após o tipo
        const tipoGroup = salaForm.querySelector('.form-group:nth-child(2)');
        if (tipoGroup) {
            tipoGroup.after(capacidadeGroup);
        }
    }

    // Adicionar botão de importação no resumo
    const exportSection = document.querySelector('.export-section');
    if (exportSection) {
        const importBtn = document.createElement('button');
        importBtn.className = 'btn btn-info';
        importBtn.innerHTML = '📂 Importar Dados';
        importBtn.onclick = importarDados;
        importBtn.style.marginLeft = '10px';

        const btnContainer = exportSection.querySelector('div');
        if (btnContainer) {
            btnContainer.appendChild(importBtn);
        }
    }

    // Mostrar primeira aba
    showTab('turmas');

    // Log de inicialização
    console.log('Sistema de Cadastro de Grade Horária v2.0 inicializado');
});

// ========== FUNÇÕES PARA DISCIPLINAS ==========

function atualizarCargaHorariaForm() {
    const container = document.getElementById('cargaHorariaContainer');

    if (dados.turmas.length === 0) {
        container.innerHTML = `
            <div class="empty-state">
                <p>📚 Nenhuma turma cadastrada</p>
                <small>Cadastre turmas primeiro para definir a carga horária</small>
            </div>
        `;
        return;
    }

    const grid = document.createElement('div');
    grid.className = 'form-grid carga-horaria-grid';

    // Ordenar turmas
    const turmasOrdenadas = [...dados.turmas].sort((a, b) =>
        a.nome.localeCompare(b.nome, 'pt-BR', { numeric: true })
    );

    turmasOrdenadas.forEach(turma => {
        const div = document.createElement('div');
        div.className = 'form-group';
        div.innerHTML = `
            <label for="carga_${turma.id}">
                ${escapeHtml(turma.nome)}
                <span class="turno-badge">${TURNOS[turma.turno].nome}</span>
            </label>
            <div class="input-group">
                <input type="number"
                       id="carga_${turma.id}"
                       min="0"
                       max="10"
                       value="0"
                       class="form-control carga-input">
                <span class="input-addon">aulas</span>
            </div>
        `;
        grid.appendChild(div);
    });

    // Adicionar total
    const totalDiv = document.createElement('div');
    totalDiv.className = 'carga-total';
    totalDiv.innerHTML = `
        <strong>Total de aulas:</strong>
        <span id="totalCargaHoraria">0</span>
    `;

    container.innerHTML = '';
    container.appendChild(grid);
    container.appendChild(totalDiv);

    // Adicionar listeners para atualizar total
    container.querySelectorAll('.carga-input').forEach(input => {
        input.addEventListener('input', atualizarTotalCargaHoraria);
    });
}

function atualizarTotalCargaHoraria() {
    let total = 0;
    document.querySelectorAll('.carga-input').forEach(input => {
        total += parseInt(input.value) || 0;
    });
    document.getElementById('totalCargaHoraria').textContent = total;
}

function adicionarDisciplina() {
    const nome = document.getElementById('disciplinaNome').value.trim();

    if (!nome) {
        Notificacao.mostrar('Preencha o nome da disciplina', 'error');
        document.getElementById('disciplinaNome').focus();
        return;
    }

    if (dados.disciplinas.some(d => d.nome.toLowerCase() === nome.toLowerCase())) {
        Notificacao.mostrar('Disciplina já existe', 'error');
        return;
    }

    const disciplina = {
        id: dados.nextId.disciplina++,
        nome: nome,
        cargaHoraria: {},
        criadoEm: new Date().toISOString()
    };

    let totalCarga = 0;
    dados.turmas.forEach(turma => {
        const carga = parseInt(document.getElementById(`carga_${turma.id}`)?.value) || 0;
        if (carga > 0) {
            disciplina.cargaHoraria[turma.id] = carga;
            totalCarga += carga;
        }
    });

    if (totalCarga === 0) {
        Notificacao.mostrar('Defina a carga horária para pelo menos uma turma', 'error');
        return;
    }

    dados.disciplinas.push(disciplina);
    salvarDados();

    // Limpar formulário
    document.getElementById('disciplinaNome').value = '';
    atualizarCargaHorariaForm();
    atualizarTabelaDisciplinas();

    Notificacao.mostrar(`Disciplina "${nome}" adicionada com ${totalCarga} aulas!`, 'success');
}

function atualizarTabelaDisciplinas() {
    const tbody = document.querySelector('#disciplinasTable tbody');
    tbody.innerHTML = '';

    // Ordenar disciplinas
    const disciplinasOrdenadas = [...dados.disciplinas].sort((a, b) =>
        a.nome.localeCompare(b.nome, 'pt-BR')
    );

    disciplinasOrdenadas.forEach(disc => {
        const tr = document.createElement('tr');
        tr.dataset.id = disc.id;

        // Preparar detalhes da carga horária
        const cargaDetalhes = Object.entries(disc.cargaHoraria)
            .filter(([_, carga]) => carga > 0)
            .map(([turmaId, carga]) => {
                const turma = dados.turmas.find(t => t.id == turmaId);
                return turma ? `<span class="carga-item">${escapeHtml(turma.nome)}: ${carga}h</span>` : '';
            })
            .filter(Boolean)
            .join('');

        const totalCarga = Object.values(disc.cargaHoraria).reduce((a, b) => a + b, 0);

        // Verificar se tem professor
        const temProfessor = dados.professores.some(p => p.disciplinaId === disc.id);
        const statusProfessor = temProfessor ?
            '<span class="status-indicator status-ok" title="Professor atribuído">✓</span>' :
            '<span class="status-indicator status-warning" title="Sem professor">⚠</span>';

        tr.innerHTML = `
            <td>${disc.id}</td>
            <td class="disciplina-nome">
                ${escapeHtml(disc.nome)}
                ${statusProfessor}
            </td>
            <td>
                <div class="carga-horaria-details">
                    ${cargaDetalhes || '<span class="text-muted">Nenhuma</span>'}
                </div>
            </td>
            <td>
                <span class="badge badge-info">${totalCarga}h total</span>
            </td>
            <td class="action-buttons">
                <button class="btn btn-secondary" onclick="editarDisciplina(${disc.id})">
                    <span class="btn-icon">✏️</span> Editar
                </button>
                <button class="btn btn-danger" onclick="excluirDisciplina(${disc.id})">
                    <span class="btn-icon">🗑️</span> Excluir
                </button>
            </td>`;
        tbody.appendChild(tr);
    });

    atualizarContador('disciplinas', disciplinasOrdenadas.length);
}

function editarDisciplina(id) {
    const disciplina = dados.disciplinas.find(d => d.id === id);
    if (!disciplina) return;

    // Criar formulário de edição
    let cargaFormHtml = '';
    if (dados.turmas.length > 0) {
        cargaFormHtml = '<h4>Carga Horária por Turma</h4><div class="form-grid">';

        dados.turmas.forEach(turma => {
            const cargaAtual = disciplina.cargaHoraria[turma.id] || 0;
            cargaFormHtml += `
                <div class="form-group">
                    <label for="editCarga_${turma.id}">${escapeHtml(turma.nome)}</label>
                    <input type="number"
                           id="editCarga_${turma.id}"
                           min="0"
                           max="10"
                           value="${cargaAtual}"
                           class="form-control">
                </div>
            `;
        });
        cargaFormHtml += '</div>';
    }

    const modal = criarModal({
        titulo: 'Editar Disciplina',
        conteudo: `
            <div class="form-group">
                <label for="editDisciplinaNome">Nome da Disciplina</label>
                <input type="text" id="editDisciplinaNome" value="${escapeHtml(disciplina.nome)}" class="form-control">
            </div>
            ${cargaFormHtml}
        `,
        botoes: [
            {
                texto: 'Cancelar',
                classe: 'btn-secondary',
                acao: () => modal.fechar()
            },
            {
                texto: 'Salvar',
                classe: 'btn-primary',
                acao: () => {
                    const novoNome = document.getElementById('editDisciplinaNome').value.trim();

                    if (!novoNome) {
                        Notificacao.mostrar('O nome não pode estar vazio', 'error');
                        return;
                    }

                    if (dados.disciplinas.some(d => d.id !== id && d.nome.toLowerCase() === novoNome.toLowerCase())) {
                        Notificacao.mostrar('Já existe uma disciplina com este nome', 'error');
                        return;
                    }

                    // Atualizar dados
                    disciplina.nome = novoNome;
                    disciplina.cargaHoraria = {};

                    let totalCarga = 0;
                    dados.turmas.forEach(turma => {
                        const carga = parseInt(document.getElementById(`editCarga_${turma.id}`)?.value) || 0;
                        if (carga > 0) {
                            disciplina.cargaHoraria[turma.id] = carga;
                            totalCarga += carga;
                        }
                    });

                    if (totalCarga === 0) {
                        Notificacao.mostrar('A disciplina deve ter pelo menos 1 hora de carga', 'error');
                        return;
                    }

                    disciplina.modificadoEm = new Date().toISOString();

                    salvarDados();
                    atualizarTabelaDisciplinas();
                    modal.fechar();
                    Notificacao.mostrar('Disciplina atualizada com sucesso!', 'success');
                }
            }
        ]
    });

    modal.abrir();
}

function excluirDisciplina(id) {
    const disciplina = dados.disciplinas.find(d => d.id === id);
    if (!disciplina) return;

    // Verificar dependências
    const professoresAfetados = dados.professores.filter(p => p.disciplinaId === id);

    let mensagemAviso = `Tem certeza que deseja excluir a disciplina "${disciplina.nome}"?`;

    if (professoresAfetados.length > 0) {
        mensagemAviso += `\n\nProfessores que serão desvinculados:\n`;
        professoresAfetados.forEach(p => {
            mensagemAviso += `- ${p.nome}\n`;
        });
    }

    const modal = criarModal({
        titulo: '⚠️ Confirmar Exclusão',
        conteudo: `<p>${mensagemAviso.replace(/\n/g, '<br>')}</p>`,
        botoes: [
            {
                texto: 'Cancelar',
                classe: 'btn-secondary',
                acao: () => modal.fechar()
            },
            {
                texto: 'Excluir',
                classe: 'btn-danger',
                acao: () => {
                    // Remover disciplina
                    dados.disciplinas = dados.disciplinas.filter(d => d.id !== id);

                    // Desvincular professores
                    professoresAfetados.forEach(p => {
                        p.disciplinaId = null;
                    });

                    salvarDados();
                    atualizarTabelaDisciplinas();
                    modal.fechar();
                    Notificacao.mostrar(`Disciplina "${disciplina.nome}" excluída!`, 'success');
                }
            }
        ]
    });

    modal.abrir();
}

// ========== FUNÇÕES PARA PROFESSORES ==========

function atualizarDisciplinasSelect() {
    const select = document.getElementById('professorDisciplina');
    select.innerHTML = '<option value="">Selecione uma disciplina</option>';

    // 1. Agrupar disciplinas por status
    const disciplinasComProfessor = [];
    const disciplinasSemProfessor = [];

    dados.disciplinas.forEach(disc => {
        const jaTemProfessor = dados.professores.some(p => p.disciplinaId === disc.id);
        if (jaTemProfessor) {
            disciplinasComProfessor.push(disc);
        } else {
            disciplinasSemProfessor.push(disc);
        }
    });
    // 2. Adicionar as disciplinas SEM professor (disponíveis) ao select
    if (disciplinasSemProfessor.length > 0) {
        const optgroup1 = document.createElement('optgroup');
        optgroup1.label = 'Disciplinas disponíveis'; // É bom adicionar um label

        disciplinasSemProfessor.sort((a, b) => a.nome.localeCompare(b.nome, 'pt-BR'));
        disciplinasSemProfessor.forEach(disc => {
            const option = document.createElement('option');
            option.value = disc.id;
            option.textContent = disc.nome;
            optgroup1.appendChild(option);
        });
        select.appendChild(optgroup1);
    }

    // 3. Adicionar as disciplinas COM professor (desabilitadas)
    if (disciplinasComProfessor.length > 0) {
        const optgroup2 = document.createElement('optgroup');
        optgroup2.label = 'Disciplinas já atribuídas';

        disciplinasComProfessor.sort((a, b) => a.nome.localeCompare(b.nome, 'pt-BR'));
        disciplinasComProfessor.forEach(disc => {
            const professor = dados.professores.find(p => p.disciplinaId === disc.id);
            const option = document.createElement('option');
            option.value = disc.id;
            option.textContent = `${disc.nome} (${professor.nome})`;
            option.disabled = true;
            optgroup2.appendChild(option);
        });
        select.appendChild(optgroup2);
    }
}

function criarGridDisponibilidade() {
    const grid = document.getElementById('availabilityGrid');
    grid.innerHTML = '';

    // Cabeçalho vazio
    const emptyHeader = document.createElement('div');
    emptyHeader.className = 'grid-corner';
    grid.appendChild(emptyHeader);

    // Cabeçalhos de horários
    HORARIOS.forEach(h => {
        const header = document.createElement('div');
        header.className = 'availability-header horario-header';
        header.textContent = h;
        grid.appendChild(header);
    });

    // Linhas por dia
    DIAS.forEach((dia, diaIndex) => {
        // Cabeçalho do dia
        const dayHeader = document.createElement('div');
        dayHeader.className = 'availability-header dia-header';
        dayHeader.innerHTML = `
            <span>${dia}</span>
            <button class="select-all-day" onclick="selecionarTodoDia(${diaIndex})" title="Selecionar todo o dia">
                ✓
            </button>
        `;
        grid.appendChild(dayHeader);

        // Células de horário
        HORARIOS.forEach((_, horarioIndex) => {
            const cell = document.createElement('div');
            cell.className = 'availability-cell';
            cell.dataset.dia = diaIndex;
            cell.dataset.horario = horarioIndex;
            cell.onclick = () => toggleCelula(cell);
            cell.innerHTML = '<span class="check-icon">✓</span>';
            grid.appendChild(cell);
        });
    });

    // Adicionar controles rápidos
    const controls = document.createElement('div');
    controls.className = 'grid-controls';
    controls.innerHTML = `
        <button class="btn btn-sm" onclick="selecionarTodos()">Selecionar Todos</button>
        <button class="btn btn-sm" onclick="limparSelecao()">Limpar Seleção</button>
        <button class="btn btn-sm" onclick="inverterSelecao()">Inverter Seleção</button>
    `;
    grid.parentElement.appendChild(controls);
}

function toggleCelula(cell) {
    cell.classList.toggle('selected');
    atualizarContagemDisponibilidade();
}

function selecionarTodoDia(diaIndex) {
    const cells = document.querySelectorAll(`[data-dia="${diaIndex}"]`);
    const todasSelecionadas = Array.from(cells).every(c => c.classList.contains('selected'));

    cells.forEach(cell => {
        if (todasSelecionadas) {
            cell.classList.remove('selected');
        } else {
            cell.classList.add('selected');
        }
    });

    atualizarContagemDisponibilidade();
}

function selecionarTodos() {
    document.querySelectorAll('.availability-cell').forEach(cell => {
        cell.classList.add('selected');
    });
    atualizarContagemDisponibilidade();
}

function limparSelecao() {
    document.querySelectorAll('.availability-cell').forEach(cell => {
        cell.classList.remove('selected');
    });
    atualizarContagemDisponibilidade();
}

function inverterSelecao() {
    document.querySelectorAll('.availability-cell').forEach(cell => {
        cell.classList.toggle('selected');
    });
    atualizarContagemDisponibilidade();
}

function atualizarContagemDisponibilidade() {
    const total = document.querySelectorAll('.availability-cell.selected').length;
    const disciplinaId = parseInt(document.getElementById('professorDisciplina').value);

    if (disciplinaId) {
        const disciplina = dados.disciplinas.find(d => d.id === disciplinaId);
        if (disciplina) {
            const cargaTotal = Object.values(disciplina.cargaHoraria).reduce((a, b) => a + b, 0);
            const status = total >= cargaTotal ? 'ok' : 'warning';

            // Mostrar status
            let statusDiv = document.getElementById('disponibilidadeStatus');
            if (!statusDiv) {
                statusDiv = document.createElement('div');
                statusDiv.id = 'disponibilidadeStatus';
                document.querySelector('.grid-controls').appendChild(statusDiv);
            }

            statusDiv.className = `disponibilidade-status ${status}`;
            statusDiv.innerHTML = `
                <span>Horários selecionados: ${total}</span>
                <span>Carga da disciplina: ${cargaTotal}h</span>
                ${total < cargaTotal ? '<span class="warning">⚠️ Disponibilidade insuficiente</span>' : ''}
            `;
        }
    }
}

function adicionarProfessor() {
    const nome = document.getElementById('professorNome').value.trim();
    const disciplinaId = parseInt(document.getElementById('professorDisciplina').value);
    const email = document.getElementById('professorEmail')?.value.trim() || '';
    const telefone = document.getElementById('professorTelefone')?.value.trim() || '';

    // Validações
    if (!nome) {
        Notificacao.mostrar('Preencha o nome do professor', 'error');
        document.getElementById('professorNome').focus();
        return;
    }

    if (!disciplinaId) {
        Notificacao.mostrar('Selecione uma disciplina', 'error');
        return;
    }

    if (dados.professores.some(p => p.nome.toLowerCase() === nome.toLowerCase())) {
        Notificacao.mostrar('Professor já cadastrado', 'error');
        return;
    }

    // Coletar disponibilidade
    const disponibilidade = [];
    document.querySelectorAll('.availability-cell.selected').forEach(cell => {
        disponibilidade.push({
            dia: parseInt(cell.dataset.dia),
            horario: parseInt(cell.dataset.horario)
        });
    });

    if (disponibilidade.length === 0) {
        Notificacao.mostrar('Selecione pelo menos um horário de disponibilidade', 'error');
        return;
    }

    // Verificar carga horária
    const disciplina = dados.disciplinas.find(d => d.id === disciplinaId);
    const totalCarga = Object.values(disciplina.cargaHoraria).reduce((a, b) => a + b, 0);

    if (disponibilidade.length < totalCarga) {
        const confirmar = confirm(
            `⚠️ Atenção!\n\n` +
            `A disponibilidade do professor (${disponibilidade.length}h) é menor que a carga horária da disciplina (${totalCarga}h).\n\n` +
            `Isso pode dificultar a geração da grade horária.\n\n` +
            `Deseja continuar mesmo assim?`
        );
        if (!confirmar) return;
    }

    // Criar professor
    const professor = {
        id: dados.nextId.professor++,
        nome: nome,
        email: email,
        telefone: telefone,
        disciplinaId: disciplinaId,
        disponibilidade: disponibilidade,
        criadoEm: new Date().toISOString()
    };

    dados.professores.push(professor);
    salvarDados();

    // Limpar formulário
    document.getElementById('professorNome').value = '';
    document.getElementById('professorEmail').value = '';
    document.getElementById('professorTelefone').value = '';
    document.getElementById('professorDisciplina').value = '';
    limparSelecao();

    // Atualizar interface
    atualizarTabelaProfessores();
    atualizarDisciplinasSelect();

    Notificacao.mostrar(`Professor "${nome}" adicionado com sucesso!`, 'success');
}

function atualizarTabelaProfessores() {
    const tbody = document.querySelector('#professoresTable tbody');
    tbody.innerHTML = '';

    // Ordenar professores
    const professoresOrdenados = [...dados.professores].sort((a, b) =>
        a.nome.localeCompare(b.nome, 'pt-BR')
    );

    professoresOrdenados.forEach(prof => {
        const tr = document.createElement('tr');
        tr.dataset.id = prof.id;

        const disciplina = dados.disciplinas.find(d => d.id === prof.disciplinaId);
        const cargaTotal = disciplina ?
            Object.values(disciplina.cargaHoraria).reduce((a, b) => a + b, 0) : 0;
        const disp = prof.disponibilidade.length;

        // Calcular status
        let statusClass = 'status-ok';
        let statusIcon = '✓';
        let statusText = 'OK';

        if (disp < cargaTotal) {
            statusClass = 'status-warning';
            statusIcon = '⚠️';
            statusText = 'Insuficiente';
        } else if (disp === cargaTotal) {
            statusClass = 'status-info';
            statusIcon = '📌';
            statusText = 'Justo';
        }

        tr.innerHTML = `
            <td>${prof.id}</td>
            <td class="professor-nome">
                ${escapeHtml(prof.nome)}
                ${prof.email ? `<br><small class="text-muted">${escapeHtml(prof.email)}</small>` : ''}
            </td>
            <td>${disciplina ? escapeHtml(disciplina.nome) : '<span class="text-muted">N/A</span>'}</td>
            <td>
                <div class="disponibilidade-info ${statusClass}">
                    <span class="status-icon">${statusIcon}</span>
                    <span>${disp}/${cargaTotal}h</span>
                    <span class="status-text">${statusText}</span>
                </div>
            </td>
            <td class="action-buttons">
                <button class="btn btn-secondary" onclick="visualizarDisponibilidade(${prof.id})" title="Ver horários">
                    <span class="btn-icon">📅</span> Ver
                </button>
                <button class="btn btn-secondary" onclick="editarProfessor(${prof.id})">
                    <span class="btn-icon">✏️</span> Editar
                </button>
                <button class="btn btn-danger" onclick="excluirProfessor(${prof.id})">
                    <span class="btn-icon">🗑️</span> Excluir
                </button>
            </td>`;
        tbody.appendChild(tr);
    });

    atualizarContador('professores', professoresOrdenados.length);
}

function visualizarDisponibilidade(id) {
    const professor = dados.professores.find(p => p.id === id);
    if (!professor) return;

    // Criar grid visual
    let gridHtml = '<div class="disponibilidade-visual">';
    gridHtml += '<table class="table-disponibilidade">';

    // Cabeçalho
    gridHtml += '<tr><th></th>';
    HORARIOS.forEach(h => {
        gridHtml += `<th>${h}</th>`;
    });
    gridHtml += '</tr>';

    // Dias
    DIAS.forEach((dia, diaIdx) => {
        gridHtml += `<tr><th>${dia}</th>`;
        HORARIOS.forEach((_, horaIdx) => {
            const disponivel = professor.disponibilidade.some(
                d => d.dia === diaIdx && d.horario === horaIdx
            );
            gridHtml += `<td class="${disponivel ? 'disponivel' : 'indisponivel'}">
                ${disponivel ? '✓' : '✗'}
            </td>`;
        });
        gridHtml += '</tr>';
    });

    gridHtml += '</table></div>';

    const modal = criarModal({
        titulo: `Disponibilidade: ${professor.nome}`,
        conteudo: gridHtml,
        botoes: [
            {
                texto: 'Fechar',
                classe: 'btn-primary',
                acao: () => modal.fechar()
            }
        ]
    });

    modal.abrir();
}

function editarProfessor(id) {
    const professor = dados.professores.find(p => p.id === id);
    if (!professor) return;

    // Criar formulário de edição
    const modal = criarModal({
        titulo: 'Editar Professor',
        conteudo: `
            <div class="form-group">
                <label for="editProfessorNome">Nome</label>
                <input type="text" id="editProfessorNome" value="${escapeHtml(professor.nome)}" class="form-control">
            </div>
            <div class="form-group">
                <label for="editProfessorEmail">Email</label>
                <input type="email" id="editProfessorEmail" value="${escapeHtml(professor.email || '')}" class="form-control">
            </div>
            <div class="form-group">
                <label for="editProfessorTelefone">Telefone</label>
                <input type="text" id="editProfessorTelefone" value="${escapeHtml(professor.telefone || '')}" class="form-control">
            </div>
            <div class="form-group">
                <label>Disciplina</label>
                <p class="form-control-static">${escapeHtml(dados.disciplinas.find(d => d.id === professor.disciplinaId)?.nome || 'N/A')}</p>
                <small class="text-muted">Para mudar a disciplina, exclua e recadastre o professor</small>
            </div>
        `,
        botoes: [
            {
                texto: 'Cancelar',
                classe: 'btn-secondary',
                acao: () => modal.fechar()
            },
            {
                texto: 'Salvar',
                classe: 'btn-primary',
                acao: () => {
                    const novoNome = document.getElementById('editProfessorNome').value.trim();
                    const novoEmail = document.getElementById('editProfessorEmail').value.trim();
                    const novoTelefone = document.getElementById('editProfessorTelefone').value.trim();

                    if (!novoNome) {
                        Notificacao.mostrar('O nome não pode estar vazio', 'error');
                        return;
                    }

                    if (dados.professores.some(p => p.id !== id && p.nome.toLowerCase() === novoNome.toLowerCase())) {
                        Notificacao.mostrar('Já existe um professor com este nome', 'error');
                        return;
                    }

                    professor.nome = novoNome;
                    professor.email = novoEmail;
                    professor.telefone = novoTelefone;
                    professor.modificadoEm = new Date().toISOString();

                    salvarDados();
                    atualizarTabelaProfessores();
                    modal.fechar();
                    Notificacao.mostrar('Professor atualizado com sucesso!', 'success');
                }
            }
        ]
    });

    modal.abrir();
}

function excluirProfessor(id) {
    const professor = dados.professores.find(p => p.id === id);
    if (!professor) return;

    const disciplina = dados.disciplinas.find(d => d.id === professor.disciplinaId);

    const modal = criarModal({
        titulo: '⚠️ Confirmar Exclusão',
        conteudo: `
            <p>Tem certeza que deseja excluir o professor "<strong>${escapeHtml(professor.nome)}</strong>"?</p>
            ${disciplina ? `<p>A disciplina "${escapeHtml(disciplina.nome)}" ficará sem professor.</p>` : ''}
            <p class="text-danger mt-3"><strong>Esta ação não pode ser desfeita!</strong></p>
        `,
        botoes: [
            {
                texto: 'Cancelar',
                classe: 'btn-secondary',
                acao: () => modal.fechar()
            },
            {
                texto: 'Excluir',
                classe: 'btn-danger',
                acao: () => {
                    dados.professores = dados.professores.filter(p => p.id !== id);
                    salvarDados();
                    atualizarTabelaProfessores();
                    atualizarDisciplinasSelect();
                    modal.fechar();
                    Notificacao.mostrar(`Professor "${professor.nome}" excluído!`, 'success');
                }
            }
        ]
    });

    modal.abrir();
}

// ========== FUNÇÕES PARA SALAS ==========

function adicionarSala() {
    const nome = document.getElementById('salaNome').value.trim();
    const tipo = document.getElementById('salaTipo').value;
    const compartilhada = document.getElementById('salaCompartilhada').checked;
    const capacidade = parseInt(document.getElementById('salaCapacidade')?.value) || 30;

    if (!nome) {
        Notificacao.mostrar('Preencha o nome da sala', 'error');
        document.getElementById('salaNome').focus();
        return;
    }

    if (dados.salas.some(s => s.nome.toLowerCase() === nome.toLowerCase())) {
        Notificacao.mostrar('Sala já existe', 'error');
        return;
    }

    const sala = {
        id: dados.nextId.sala++,
        nome: nome,
        tipo: tipo,
        compartilhada: compartilhada,
        capacidade: capacidade,
        criadoEm: new Date().toISOString()
    };

    dados.salas.push(sala);
    salvarDados();

    // Limpar formulário
    document.getElementById('salaNome').value = '';
    document.getElementById('salaCompartilhada').checked = false;

    atualizarTabelaSalas();
    atualizarEstatisticasSalas();

    Notificacao.mostrar(`Sala "${nome}" adicionada com sucesso!`, 'success');
}

function atualizarTabelaSalas() {
    const tbody = document.querySelector('#salasTable tbody');
    tbody.innerHTML = '';

    // Ordenar salas
    const salasOrdenadas = [...dados.salas].sort((a, b) => {
        // Salas normais primeiro, depois compartilhadas
        if (a.compartilhada !== b.compartilhada) {
            return a.compartilhada ? 1 : -1;
        }
        return a.nome.localeCompare(b.nome, 'pt-BR', { numeric: true });
    });

    salasOrdenadas.forEach(sala => {
        const tr = document.createElement('tr');
        tr.dataset.id = sala.id;

        const tipoIcon = {
            normal: '🏫',
            laboratorio: '🔬',
            quadra: '⚽',
            biblioteca: '📚'
        }[sala.tipo] || '🏫';

        tr.innerHTML = `
            <td>${sala.id}</td>
            <td class="sala-nome">
                <span class="tipo-icon">${tipoIcon}</span>
                ${escapeHtml(sala.nome)}
            </td>
            <td>
                <span class="badge badge-${sala.tipo}">
                    ${sala.tipo.charAt(0).toUpperCase() + sala.tipo.slice(1)}
                </span>
            </td>
            <td>
                ${sala.compartilhada ?
                    '<span class="badge badge-warning">Compartilhada</span>' :
                    '<span class="badge badge-success">Exclusiva</span>'}
            </td>
            <td>
                <span class="capacidade">${sala.capacidade || 30} alunos</span>
            </td>
            <td class="action-buttons">
                <button class="btn btn-secondary" onclick="editarSala(${sala.id})">
                    <span class="btn-icon">✏️</span> Editar
                </button>
                <button class="btn btn-danger" onclick="excluirSala(${sala.id})">
                    <span class="btn-icon">🗑️</span> Excluir
                </button>
            </td>`;
        tbody.appendChild(tr);
    });

    atualizarContador('salas', salasOrdenadas.length);
}

function atualizarEstatisticasSalas() {
    const salasNormais = dados.salas.filter(s => !s.compartilhada).length;
    const salasCompartilhadas = dados.salas.filter(s => s.compartilhada).length;
    const totalTurmas = dados.turmas.length;

    let statsDiv = document.getElementById('estatisticasSalas');
    if (!statsDiv) {
        statsDiv = document.createElement('div');
        statsDiv.id = 'estatisticasSalas';
        statsDiv.className = 'stats-salas';
        const container = document.querySelector('#salas-tab .form-section:first-child');
        container.appendChild(statsDiv);
    }

    let statusHtml = '';
    if (salasNormais < totalTurmas) {
        statusHtml = `
            <div class="alert alert-warning">
                <strong>⚠️ Atenção:</strong>
                Você tem ${totalTurmas} turma(s) mas apenas ${salasNormais} sala(s) exclusiva(s).
                Cada turma precisa de pelo menos uma sala exclusiva.
            </div>
        `;
    }

    statsDiv.innerHTML = `
        ${statusHtml}
        <div class="stats-grid">
            <div class="stat-item">
                <span class="stat-label">Salas Exclusivas:</span>
                <span class="stat-value">${salasNormais}</span>
            </div>
            <div class="stat-item">
                <span class="stat-label">Salas Compartilhadas:</span>
                <span class="stat-value">${salasCompartilhadas}</span>
            </div>
            <div class="stat-item">
                <span class="stat-label">Total de Turmas:</span>
                <span class="stat-value">${totalTurmas}</span>
            </div>
        </div>
    `;
}

function editarSala(id) {
    const sala = dados.salas.find(s => s.id === id);
    if (!sala) return;

    const modal = criarModal({
        titulo: 'Editar Sala',
        conteudo: `
            <div class="form-group">
                <label for="editSalaNome">Nome da Sala</label>
                <input type="text" id="editSalaNome" value="${escapeHtml(sala.nome)}" class="form-control">
            </div>
            <div class="form-group">
                <label for="editSalaTipo">Tipo</label>
                <select id="editSalaTipo" class="form-control">
                    <option value="normal" ${sala.tipo === 'normal' ? 'selected' : ''}>Sala Normal</option>
                    <option value="laboratorio" ${sala.tipo === 'laboratorio' ? 'selected' : ''}>Laboratório</option>
                    <option value="quadra" ${sala.tipo === 'quadra' ? 'selected' : ''}>Quadra</option>
                    <option value="biblioteca" ${sala.tipo === 'biblioteca' ? 'selected' : ''}>Biblioteca</option>
                </select>
            </div>
            <div class="form-group">
                <label for="editSalaCapacidade">Capacidade</label>
                <input type="number" id="editSalaCapacidade" value="${sala.capacidade || 30}" min="1" class="form-control">
            </div>
            <div class="form-group">
                <label>
                    <input type="checkbox" id="editSalaCompartilhada" ${sala.compartilhada ? 'checked' : ''}>
                    Sala Compartilhada
                </label>
            </div>
        `,
        botoes: [
            {
                texto: 'Cancelar',
                classe: 'btn-secondary',
                acao: () => modal.fechar()
            },
            {
                texto: 'Salvar',
                classe: 'btn-primary',
                acao: () => {
                    const novoNome = document.getElementById('editSalaNome').value.trim();
                    const novoTipo = document.getElementById('editSalaTipo').value;
                    const novaCapacidade = parseInt(document.getElementById('editSalaCapacidade').value) || 30;
                    const novaCompartilhada = document.getElementById('editSalaCompartilhada').checked;

                    if (!novoNome) {
                        Notificacao.mostrar('O nome não pode estar vazio', 'error');
                        return;
                    }

                    if (dados.salas.some(s => s.id !== id && s.nome.toLowerCase() === novoNome.toLowerCase())) {
                        Notificacao.mostrar('Já existe uma sala com este nome', 'error');
                        return;
                    }

                    sala.nome = novoNome;
                    sala.tipo = novoTipo;
                    sala.capacidade = novaCapacidade;
                    sala.compartilhada = novaCompartilhada;
                    sala.modificadoEm = new Date().toISOString();

                    salvarDados();
                    atualizarTabelaSalas();
                    atualizarEstatisticasSalas();
                    modal.fechar();
                    Notificacao.mostrar('Sala atualizada com sucesso!', 'success');
                }
            }
        ]
    });

    modal.abrir();
}

function excluirSala(id) {
    const sala = dados.salas.find(s => s.id === id);
    if (!sala) return;

    const modal = criarModal({
        titulo: '⚠️ Confirmar Exclusão',
        conteudo: `
            <p>Tem certeza que deseja excluir a sala "<strong>${escapeHtml(sala.nome)}</strong>"?</p>
            <p class="text-danger mt-3"><strong>Esta ação não pode ser desfeita!</strong></p>
        `,
        botoes: [
            {
                texto: 'Cancelar',
                classe: 'btn-secondary',
                acao: () => modal.fechar()
            },
            {
                texto: 'Excluir',
                classe: 'btn-danger',
                acao: () => {
                    dados.salas = dados.salas.filter(s => s.id !== id);
                    salvarDados();
                    atualizarTabelaSalas();
                    atualizarEstatisticasSalas();
                    modal.fechar();
                    Notificacao.mostrar(`Sala "${sala.nome}" excluída!`, 'success');
                }
            }
        ]
    });

    modal.abrir();
}

// ========== FUNÇÕES DO RESUMO E EXPORTAÇÃO ==========

function atualizarResumo() {
    document.getElementById('totalTurmas').textContent = dados.turmas.length;
    document.getElementById('totalDisciplinas').textContent = dados.disciplinas.length;
    document.getElementById('totalProfessores').textContent = dados.professores.length;
    document.getElementById('totalSalas').textContent = dados.salas.length;

    // Análise detalhada
    const analise = analisarDados();
    atualizarAnaliseDetalhada(analise);
}

function analisarDados() {
    const analise = {
        turmasSemAulas: [],
        disciplinasSemProfessor: [],
        professoresComCargaInsuficiente: [],
        salasInsuficientes: false,
        totalAulasPrevistas: 0,
        distribuicaoAulasPorDia: {}
    };

    // Analisar turmas
    dados.turmas.forEach(turma => {
        const totalAulas = calcularTotalAulasTurma(turma.id);
        if (totalAulas === 0) {
            analise.turmasSemAulas.push(turma);
        }
        analise.totalAulasPrevistas += totalAulas;
    });

    // Analisar disciplinas
    dados.disciplinas.forEach(disc => {
        if (!dados.professores.some(p => p.disciplinaId === disc.id)) {
            analise.disciplinasSemProfessor.push(disc);
        }
    });

    // Analisar professores
dados.professores.forEach(prof => {
    const disc = dados.disciplinas.find(d => d.id === prof.disciplinaId);
    if (disc) {
        const cargaTotal = Object.values(disc.cargaHoraria).reduce((a, b) => a + b, 0);
        if (prof.disponibilidade.length < cargaTotal) {
            analise.professoresComCargaInsuficiente.push({
                professor: prof,
                disciplina: disc,
                disponivel: prof.disponibilidade.length,
                necessario: cargaTotal
            });
        }
    }
});

// Analisar salas
const salasNormais = dados.salas.filter(s => !s.compartilhada).length;
analise.salasInsuficientes = salasNormais < dados.turmas.length;

return analise;
}

function atualizarAnaliseDetalhada(analise) {
    let analiseHtml = '<div class="analise-detalhada">';

    // Resumo geral
    analiseHtml += `
        <div class="analise-section">
            <h4>📊 Resumo Geral</h4>
            <p>Total de aulas previstas: <strong>${analise.totalAulasPrevistas}</strong></p>
            <p>Média por turma: <strong>${(analise.totalAulasPrevistas / dados.turmas.length).toFixed(1)}</strong> aulas</p>
        </div>
    `;

    // Problemas identificados
    if (analise.turmasSemAulas.length > 0 ||
        analise.disciplinasSemProfessor.length > 0 ||
        analise.professoresComCargaInsuficiente.length > 0 ||
        analise.salasInsuficientes) {

        analiseHtml += '<div class="analise-section problemas">';
        analiseHtml += '<h4>⚠️ Problemas Identificados</h4>';

        if (analise.turmasSemAulas.length > 0) {
            analiseHtml += '<div class="problema-item">';
            analiseHtml += '<strong>Turmas sem aulas:</strong><ul>';
            analise.turmasSemAulas.forEach(t => {
                analiseHtml += `<li>${escapeHtml(t.nome)}</li>`;
            });
            analiseHtml += '</ul></div>';
        }

        if (analise.disciplinasSemProfessor.length > 0) {
            analiseHtml += '<div class="problema-item">';
            analiseHtml += '<strong>Disciplinas sem professor:</strong><ul>';
            analise.disciplinasSemProfessor.forEach(d => {
                analiseHtml += `<li>${escapeHtml(d.nome)}</li>`;
            });
            analiseHtml += '</ul></div>';
        }

        if (analise.professoresComCargaInsuficiente.length > 0) {
            analiseHtml += '<div class="problema-item">';
            analiseHtml += '<strong>Professores com disponibilidade insuficiente:</strong><ul>';
            analise.professoresComCargaInsuficiente.forEach(p => {
                analiseHtml += `<li>${escapeHtml(p.professor.nome)} - ${escapeHtml(p.disciplina.nome)}:
                    ${p.disponivel}h disponível de ${p.necessario}h necessário</li>`;
            });
            analiseHtml += '</ul></div>';
        }

        if (analise.salasInsuficientes) {
            analiseHtml += '<div class="problema-item">';
            analiseHtml += `<strong>Salas insuficientes:</strong>
                ${dados.turmas.length} turmas mas apenas ${dados.salas.filter(s => !s.compartilhada).length} salas exclusivas`;
            analiseHtml += '</div>';
        }

        analiseHtml += '</div>';
    } else {
        analiseHtml += `
            <div class="analise-section sucesso">
                <h4>✅ Tudo OK!</h4>
                <p>Todos os dados parecem estar corretos e prontos para gerar a grade horária.</p>
            </div>
        `;
    }

    analiseHtml += '</div>';

    // Atualizar o container
    let container = document.getElementById('analiseContainer');
    if (!container) {
        container = document.createElement('div');
        container.id = 'analiseContainer';
        document.querySelector('.summary-cards').after(container);
    }
    container.innerHTML = analiseHtml;
}

function validarDados() {
    const results = document.getElementById('validationResults');
    const analise = analisarDados();
    let errors = [];
    let warnings = [];

    // Verificações críticas (erros)
    if (dados.turmas.length === 0) errors.push("Nenhuma turma cadastrada");
    if (dados.disciplinas.length === 0) errors.push("Nenhuma disciplina cadastrada");
    if (dados.professores.length === 0) errors.push("Nenhum professor cadastrado");
    if (dados.salas.length === 0) errors.push("Nenhuma sala cadastrada");

    // Verificações importantes (avisos)
    if (analise.salasInsuficientes) {
        warnings.push(`Há ${dados.turmas.length} turmas, mas apenas ${dados.salas.filter(s => !s.compartilhada).length} salas exclusivas`);
    }

    analise.disciplinasSemProfessor.forEach(d => {
        errors.push(`A disciplina "${d.nome}" não tem professor atribuído`);
    });

    analise.professoresComCargaInsuficiente.forEach(p => {
        warnings.push(`Professor "${p.professor.nome}": disponibilidade insuficiente (${p.disponivel}h disponível, ${p.necessario}h necessário)`);
    });

    // Exibir resultados
    let html = '';

    if (errors.length > 0) {
        html += `
            <div class="message error">
                <strong>❌ Erros Críticos (devem ser corrigidos):</strong>
                <ul>${errors.map(e => `<li>${e}</li>`).join('')}</ul>
            </div>
        `;
    }

    if (warnings.length > 0) {
        html += `
            <div class="message warning">
                <strong>⚠️ Avisos (podem dificultar a geração):</strong>
                <ul>${warnings.map(w => `<li>${w}</li>`).join('')}</ul>
            </div>
        `;
    }

    if (errors.length === 0 && warnings.length === 0) {
        html = '<div class="message success">✅ Dados válidos e prontos para exportação!</div>';
    }

    results.innerHTML = html;
    return errors.length === 0;
}

function exportarDados() {
    if (!validarDados()) {
        Notificacao.mostrar("Corrija os erros críticos antes de exportar", "error");
        return;
    }

    // Associar turmas a salas
    const salasNormais = dados.salas.filter(s => !s.compartilhada);
    const turmaSalaMap = {};

    dados.turmas.forEach((turma, index) => {
        if (index < salasNormais.length) {
            turmaSalaMap[turma.id] = salasNormais[index].id;
        }
    });

    const exportData = {
        metadata: {
            exportadoEm: new Date().toISOString(),
            versao: "2.0"
        },
        turmas: dados.turmas.map(t => ({
            id: t.id,
            nome: t.nome,
            turno: t.turno
        })),
        disciplinas: dados.disciplinas.map(d => ({
            id: d.id,
            nome: d.nome,
            aulasPorTurma: d.cargaHoraria
        })),
        professores: dados.professores.map(p => ({
            id: p.id,
            nome: p.nome,
            idDisciplina: p.disciplinaId,
            disponibilidade: p.disponibilidade
        })),
        salas: dados.salas.map(s => ({
            id: s.id,
            nome: s.nome,
            tipo: s.tipo,
            compartilhada: s.compartilhada,
            capacidade: s.capacidade
        })),
        associacoes: {
            turmaSala: turmaSalaMap
        }
    };

    // Gerar arquivo
    const dataStr = JSON.stringify(exportData, null, 2);
    const dataBlob = new Blob([dataStr], { type: 'application/json' });
    const url = URL.createObjectURL(dataBlob);
    const link = document.createElement('a');
    link.href = url;
    const timestamp = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
    link.download = `grade_horaria_dados_${timestamp}.json`;
    link.click();
    URL.revokeObjectURL(url);

    Notificacao.mostrar("Dados exportados com sucesso!", "success");
}

function exportarCodigoMelhorado() {
    if (!validarDados()) {
        Notificacao.mostrar("Corrija os erros críticos antes de exportar", "error");
        return;
    }

    let cppCode = `// ==================================================
// Dados gerados automaticamente pelo Sistema de Cadastro
// Data: ${new Date().toLocaleString('pt-BR')}
// ==================================================

void setupDadosExemplo(
    std::vector<Professor>& profs,
    std::vector<Disciplina>& discs,
    std::vector<Turma>& turmas,
    std::vector<Sala>& salas,
    std::vector<RequisicaoAlocacao>& reqs,
    std::set<std::tuple<int, int, int>>& disponibilidade,
    std::map<int, int>& turmaSalaMap
) {
    // Limpar dados existentes
    profs.clear();
    discs.clear();
    turmas.clear();
    salas.clear();
    reqs.clear();
    disponibilidade.clear();
    turmaSalaMap.clear();

    // === TURMAS ===
`;

    // Turmas
    dados.turmas.forEach(t => {
        cppCode += `    turmas.push_back({${t.id}, "${t.nome}", "${t.serie || ""}", `;

        // Mapear turno
        const turnoMap = {
            'manha': 'Turno::MANHA',
            'tarde': 'Turno::TARDE',
            'noite': 'Turno::NOITE'
        };
        cppCode += `${turnoMap[t.turno] || 'Turno::MANHA'}, 30});\n`;
    });

    cppCode += `\n    // === DISCIPLINAS ===\n`;

    // Disciplinas
    dados.disciplinas.forEach(d => {
        cppCode += `    {\n`;
        cppCode += `        Disciplina disc;\n`;
        cppCode += `        disc.id = ${d.id};\n`;
        cppCode += `        disc.nome = "${d.nome}";\n`;
        cppCode += `        disc.codigo = "${d.codigo || d.nome.substring(0, 3).toUpperCase()}";\n`;

        Object.entries(d.cargaHoraria).forEach(([turmaId, carga]) => {
            if (carga > 0) {
                cppCode += `        disc.aulasPorTurma[${turmaId}] = ${carga};\n`;
            }
        });

        cppCode += `        discs.push_back(disc);\n`;
        cppCode += `    }\n`;
    });

    cppCode += `\n    // === PROFESSORES ===\n`;

    // Professores com informações completas
    dados.professores.forEach(p => {
        cppCode += `    profs.push_back({${p.id}, "${p.nome}", `;
        cppCode += `"${p.email || ""}", "${p.telefone || ""}", 40});\n`;

        // Adicionar disciplina habilitada
        if (p.disciplinaId) {
            cppCode += `    profs.back().disciplinasHabilitadas.insert(${p.disciplinaId});\n`;
        }
    });

    cppCode += `\n    // === DISPONIBILIDADE DOS PROFESSORES ===\n`;

    // Disponibilidade
    dados.professores.forEach(p => {
        if (p.disponibilidade.length > 0) {
            cppCode += `    // ${p.nome}\n`;
            p.disponibilidade.forEach(d => {
                cppCode += `    disponibilidade.insert({${p.id}, ${d.dia}, ${d.horario}});\n`;
            });
        }
    });

    cppCode += `\n    // === SALAS ===\n`;

    // Salas com tipo correto
    dados.salas.forEach(s => {
        const tipoMap = {
            'normal': 'TipoSala::NORMAL',
            'laboratorio': 'TipoSala::LABORATORIO',
            'quadra': 'TipoSala::QUADRA',
            'biblioteca': 'TipoSala::BIBLIOTECA'
        };

        cppCode += `    salas.push_back({${s.id}, "${s.nome}", ${s.compartilhada ? 'true' : 'false'}, `;
        cppCode += `${tipoMap[s.tipo] || 'TipoSala::NORMAL'}, ${s.capacidade || 30}});\n`;
    });

    cppCode += `\n    // === ASSOCIAÇÃO TURMA-SALA ===\n`;

    // Mapeamento Turma-Sala
    const salasNormais = dados.salas.filter(s => !s.compartilhada);
    dados.turmas.forEach((turma, index) => {
        if (index < salasNormais.length) {
            cppCode += `    turmaSalaMap[${turma.id}] = ${salasNormais[index].id};\n`;
        }
    });

    cppCode += `\n    // === DISPONIBILIDADE TOTAL DOS PROFESSORES ===\n`;
    cppCode += `    std::map<int, int> disponibilidadeTotalProf;\n`;

    dados.professores.forEach(p => {
        cppCode += `    disponibilidadeTotalProf[${p.id}] = ${p.disponibilidade.length};\n`;
    });

    cppCode += `\n    // === REQUISIÇÕES DE ALOCAÇÃO ===\n`;
    cppCode += `    // Gerar requisições com mapeamento correto professor-disciplina\n`;

    // Criar mapa professor->disciplina
    const profDiscMap = {};
    dados.professores.forEach(p => {
        if (p.disciplinaId) {
            profDiscMap[p.disciplinaId] = p.id;
        }
    });

    // Gerar requisições
    dados.disciplinas.forEach(disc => {
        const profId = profDiscMap[disc.id];
        if (!profId) {
            cppCode += `    // AVISO: Disciplina "${disc.nome}" sem professor atribuído!\n`;
            return;
        }

        Object.entries(disc.cargaHoraria).forEach(([turmaId, qtdAulas]) => {
            if (qtdAulas > 0) {
                cppCode += `    // ${qtdAulas} aula(s) de ${disc.nome} para turma ${turmaId}\n`;
                cppCode += `    for (int i = 0; i < ${qtdAulas}; i++) {\n`;
                cppCode += `        reqs.push_back({${turmaId}, ${disc.id}, ${profId}, 0.0});\n`;
                cppCode += `    }\n`;
            }
        });
    });

    cppCode += `\n    // === CONFIGURAÇÃO DO GERADOR ===\n`;
    cppCode += `    ConfiguracaoGerador config;\n`;
    cppCode += `    config.verboso = true;\n`;
    cppCode += `    config.priorizarMinimoJanelas = true;\n`;
    cppCode += `    config.distribuirAulasUniformemente = true;\n`;
    cppCode += `    config.evitarAulasExtremos = true;\n`;

    cppCode += `\n    // Criar gerador e executar\n`;
    cppCode += `    GeradorHorario gerador(profs, discs, turmas, salas, reqs,\n`;
    cppCode += `                          disponibilidade, disponibilidadeTotalProf, turmaSalaMap, config);\n`;
    cppCode += `}\n`;

    // Adicionar também uma versão standalone main()
    cppCode += `\n// === FUNÇÃO MAIN PARA TESTE DIRETO ===\n`;
    cppCode += `/*\nint main() {\n`;
    cppCode += `    std::vector<Professor> professores;\n`;
    cppCode += `    std::vector<Disciplina> disciplinas;\n`;
    cppCode += `    std::vector<Turma> turmas;\n`;
    cppCode += `    std::vector<Sala> salas;\n`;
    cppCode += `    std::vector<RequisicaoAlocacao> requisicoes;\n`;
    cppCode += `    std::set<std::tuple<int, int, int>> disponibilidade;\n`;
    cppCode += `    std::map<int, int> turmaSalaMap;\n\n`;
    cppCode += `    setupDadosExemplo(professores, disciplinas, turmas, salas,\n`;
    cppCode += `                     requisicoes, disponibilidade, turmaSalaMap);\n\n`;
    cppCode += `    // Executar geração...\n`;
    cppCode += `    return 0;\n`;
    cppCode += `}\n*/`;

    // Mostrar código em modal (usar a função existente)
    mostrarCodigoModal(cppCode);
}

// Função auxiliar para mostrar o modal
function mostrarCodigoModal(cppCode) {
    const modal = criarModal({
        titulo: '📄 Código C++ Gerado',
        conteudo: `
            <div class="code-export">
                <div class="code-actions">
                    <button class="btn btn-sm btn-primary" onclick="copiarCodigo()">
                        📋 Copiar Código
                    </button>
                    <button class="btn btn-sm btn-secondary" onclick="baixarCodigo()">
                        💾 Baixar Arquivo
                    </button>
                </div>
                <textarea id="codigoGerado" class="code-textarea" readonly>${escapeHtml(cppCode)}</textarea>
            </div>
        `,
        botoes: [
            {
                texto: 'Fechar',
                classe: 'btn-primary',
                acao: () => modal.fechar()
            }
        ],
        largura: '80%'
    });

    modal.abrir();

    // Armazenar código para as funções de copiar/baixar
    window.codigoGeradoAtual = cppCode;
}

// Atualizar funções auxiliares
window.copiarCodigo = function() {
    navigator.clipboard.writeText(window.codigoGeradoAtual)
        .then(() => {
            Notificacao.mostrar('Código copiado!', 'success');
        })
        .catch(() => {
            Notificacao.mostrar('Falha ao copiar o código', 'error');
        });
};

window.baixarCodigo = function() {
    const blob = new Blob([window.codigoGeradoAtual], { type: 'text/plain' });
    const url = URL.createObjectURL(blob);
    const link = document.createElement('a');
    link.href = url;
    link.download = 'dados_grade_horaria.cpp';
    link.click();
    URL.revokeObjectURL(url);
};

// ========== FUNÇÕES UTILITÁRIAS ==========

function escapeHtml(text) {
    const map = {
        '&': '&amp;',
        '<': '&lt;',
        '>': '&gt;',
        '"': '&quot;',
        "'": '&#039;'
    };
    return text.replace(/[&<>"']/g, m => map[m]);
}

function atualizarContador(tipo, quantidade) {
    const badge = document.querySelector(`[onclick*="showTab('${tipo}')"] .tab-badge`);
    if (!badge) {
        const button = document.querySelector(`[onclick*="showTab('${tipo}')"]`);
        if (button) {
            const newBadge = document.createElement('span');
            newBadge.className = 'tab-badge';
            newBadge.textContent = quantidade;
            button.appendChild(newBadge);
        }
    } else {
        badge.textContent = quantidade;
    }
}

function criarModal(opcoes) {
    const { titulo, conteudo, botoes, largura = '600px' } = opcoes;

    // Criar elementos do modal
    const overlay = document.createElement('div');
    overlay.className = 'modal-overlay';

    const modal = document.createElement('div');
    modal.className = 'modal-container';
    modal.style.maxWidth = largura;

    const header = document.createElement('div');
    header.className = 'modal-header';
    header.innerHTML = `
        <h3>${titulo}</h3>
        <button class="modal-close" onclick="this.closest('.modal-overlay').remove()">×</button>
    `;

    const body = document.createElement('div');
    body.className = 'modal-body';
    body.innerHTML = conteudo;

    const footer = document.createElement('div');
    footer.className = 'modal-footer';

    botoes.forEach(botao => {
        const btn = document.createElement('button');
        btn.className = `btn ${botao.classe}`;
        btn.textContent = botao.texto;
        btn.onclick = botao.acao;
        footer.appendChild(btn);
    });

    modal.appendChild(header);
    modal.appendChild(body);
    modal.appendChild(footer);
    overlay.appendChild(modal);

    return {
        abrir: () => {
            document.body.appendChild(overlay);
            setTimeout(() => overlay.classList.add('show'), 10);
        },
        fechar: () => {
            overlay.classList.remove('show');
            setTimeout(() => overlay.remove(), 300);
        }
    };
}

function atualizarTodasTabelas() {
    atualizarTabelaTurmas();
    atualizarTabelaDisciplinas();
    atualizarTabelaProfessores();
    atualizarTabelaSalas();
}

// ========== INICIALIZAÇÃO ==========

document.addEventListener('DOMContentLoaded', () => {
    // Inicializar sistema de notificações
    Notificacao.init();

    // Carregar dados salvos
    carregarDados();

    // Configurar navegação
    document.querySelectorAll('.nav-tab').forEach(button => {
        button.addEventListener('click', (e) => {
            e.preventDefault();
            const tabName = button.getAttribute('onclick').match(/'([^']+)'/)[1];
            showTab(tabName);
        });
    });

    // Adicionar listeners para teclas de atalho
    document.addEventListener('keydown', (e) => {
        // Ctrl+S para salvar/exportar
        if (e.ctrlKey && e.key === 's') {
            e.preventDefault();
            exportarDados();
        }

        // Esc para fechar modais
        if (e.key === 'Escape') {
            const modal = document.querySelector('.modal-overlay');
            if (modal) modal.remove();
        }
    });

    // Adicionar campos extras no HTML (email e telefone para professores)
    const professorForm = document.querySelector('#professores-tab .form-grid');
    if (professorForm) {
        const emailGroup = document.createElement('div');
        emailGroup.className = 'form-group';
        emailGroup.innerHTML = `
            <label for="professorEmail">Email (opcional)</label>
            <input type="email" id="professorEmail" placeholder="professor@escola.com">
        `;

        const telefoneGroup = document.createElement('div');
        telefoneGroup.className = 'form-group';
        telefoneGroup.innerHTML = `
            <label for="professorTelefone">Telefone (opcional)</label>
            <input type="tel" id="professorTelefone" placeholder="(11) 99999-9999">
        `;

        // Inserir após o campo de disciplina
        const disciplinaGroup = professorForm.querySelector('.form-group:nth-child(2)');
        if (disciplinaGroup) {
            disciplinaGroup.after(emailGroup);
            emailGroup.after(telefoneGroup);
        }
    }

    // Adicionar campo de capacidade para salas
    const salaForm = document.querySelector('#salas-tab .form-grid');
    if (salaForm) {
        const capacidadeGroup = document.createElement('div');
        capacidadeGroup.className = 'form-group';
        capacidadeGroup.innerHTML = `
            <label for="salaCapacidade">Capacidade</label>
            <input type="number" id="salaCapacidade" min="1" value="30" placeholder="30">
        `;

        // Inserir após o tipo
        const tipoGroup = salaForm.querySelector('.form-group:nth-child(2)');
        if (tipoGroup) {
            tipoGroup.after(capacidadeGroup);
        }
    }

    // Adicionar botão de importação no resumo
    const exportSection = document.querySelector('.export-section');
    if (exportSection) {
        const importBtn = document.createElement('button');
        importBtn.className = 'btn btn-info';
        importBtn.innerHTML = '📂 Importar Dados';
        importBtn.onclick = importarDados;
        importBtn.style.marginLeft = '10px';

        const btnContainer = exportSection.querySelector('div');
        if (btnContainer) {
            btnContainer.appendChild(importBtn);
        }
    }

    // Mostrar primeira aba
    showTab('turmas');

    // Log de inicialização
    console.log('Sistema de Cadastro de Grade Horária v2.0 inicializado');
});

// Adicionar suporte para importação de planilhas
function importarPlanilha() {
    const modal = criarModal({
        titulo: '📊 Importar de Planilha',
        conteudo: `
            <div class="import-section">
                <h4>Escolha o tipo de dados para importar:</h4>
                <div class="import-options">
                    <button class="btn btn-outline" onclick="iniciarImportacao('turmas')">
                        🎓 Turmas
                    </button>
                    <button class="btn btn-outline" onclick="iniciarImportacao('disciplinas')">
                        📚 Disciplinas
                    </button>
                    <button class="btn btn-outline" onclick="iniciarImportacao('professores')">
                        👥 Professores
                    </button>
                    <button class="btn btn-outline" onclick="iniciarImportacao('salas')">
                        🏫 Salas
                    </button>
                </div>

                <div class="import-help">
                    <h5>Formato esperado:</h5>
                    <p><strong>Turmas:</strong> Nome | Turno</p>
                    <p><strong>Disciplinas:</strong> Nome | Turma1:Carga1 | Turma2:Carga2...</p>
                    <p><strong>Professores:</strong> Nome | Email | Telefone | Disciplina | Disponibilidade</p>
                    <p><strong>Salas:</strong> Nome | Tipo | Compartilhada | Capacidade</p>
                </div>
            </div>
        `,
        botoes: [{
            texto: 'Fechar',
            classe: 'btn-secondary',
            acao: () => modal.fechar()
        }]
    });
    modal.abrir();
}

function iniciarImportacao(tipo) {
    const input = document.createElement('input');
    input.type = 'file';
    input.accept = '.csv,.xlsx,.xls';

    input.onchange = (e) => {
        const file = e.target.files[0];
        if (!file) return;

        const reader = new FileReader();
        reader.onload = (event) => {
            processarArquivoImportado(event.target.result, file.name, tipo);
        };

        if (file.name.endsWith('.csv')) {
            reader.readAsText(file);
        } else {
            reader.readAsBinaryString(file);
        }
    };

    input.click();
}

function processarArquivoImportado(conteudo, nomeArquivo, tipo) {
    try {
        let dados = [];

        if (nomeArquivo.endsWith('.csv')) {
            dados = processarCSV(conteudo);
        } else {
            // Para Excel, seria necessário uma biblioteca externa como SheetJS
            Notificacao.mostrar('Para importar Excel, adicione a biblioteca SheetJS', 'warning');
            return;
        }

        importarDadosPorTipo(dados, tipo);

    } catch (error) {
        Notificacao.mostrar('Erro ao processar arquivo: ' + error.message, 'error');
    }
}

function processarCSV(conteudo) {
    const linhas = conteudo.split('\n');
    const dados = [];

    // Pular cabeçalho se existir
    const startIndex = linhas[0].includes('Nome') ? 1 : 0;

    for (let i = startIndex; i < linhas.length; i++) {
        const linha = linhas[i].trim();
        if (linha) {
            const colunas = linha.split(/[,;|\t]/).map(col => col.trim());
            dados.push(colunas);
        }
    }

    return dados;
}

function importarDadosPorTipo(dadosArray, tipo) {
    let importados = 0;
    let erros = [];

    switch (tipo) {
        case 'turmas':
            dadosArray.forEach((linha, index) => {
                if (linha.length >= 2) {
                    const nome = linha[0];
                    const turno = linha[1].toLowerCase();

                    if (!dados.turmas.some(t => t.nome === nome)) {
                        dados.turmas.push({
                            id: dados.nextId.turma++,
                            nome: nome,
                            turno: ['manha', 'tarde', 'noite'].includes(turno) ? turno : 'manha',
                            criadoEm: new Date().toISOString()
                        });
                        importados++;
                    }
                } else {
                    erros.push(`Linha ${index + 1}: formato inválido`);
                }
            });
            break;

        case 'disciplinas':
            dadosArray.forEach((linha, index) => {
                if (linha.length >= 1) {
                    const nome = linha[0];
                    const cargaHoraria = {};

                    // Processar carga horária (formato: "Turma:Carga")
                    for (let i = 1; i < linha.length; i++) {
                        const [turmaNome, carga] = linha[i].split(':');
                        const turma = dados.turmas.find(t => t.nome === turmaNome.trim());
                        if (turma && carga) {
                            cargaHoraria[turma.id] = parseInt(carga) || 0;
                        }
                    }

                    if (!dados.disciplinas.some(d => d.nome === nome)) {
                        dados.disciplinas.push({
                            id: dados.nextId.disciplina++,
                            nome: nome,
                            cargaHoraria: cargaHoraria,
                            criadoEm: new Date().toISOString()
                        });
                        importados++;
                    }
                } else {
                    erros.push(`Linha ${index + 1}: formato inválido`);
                }
            });
            break;

        case 'salas':
            dadosArray.forEach((linha, index) => {
                if (linha.length >= 1) {
                    const nome = linha[0];
                    const tipo = linha[1] || 'normal';
                    const compartilhada = linha[2] === 'sim' || linha[2] === 'true';
                    const capacidade = parseInt(linha[3]) || 30;

                    if (!dados.salas.some(s => s.nome === nome)) {
                        dados.salas.push({
                            id: dados.nextId.sala++,
                            nome: nome,
                            tipo: tipo,
                            compartilhada: compartilhada,
                            capacidade: capacidade,
                            criadoEm: new Date().toISOString()
                        });
                        importados++;
                    }
                } else {
                    erros.push(`Linha ${index + 1}: formato inválido`);
                }
            });
            break;
    }

    salvarDados();
    atualizarTodasTabelas();

    // Mostrar resultado
    let mensagem = `Importados: ${importados} ${tipo}`;
    if (erros.length > 0) {
        mensagem += `\nErros: ${erros.length}`;
    }

    Notificacao.mostrar(mensagem, importados > 0 ? 'success' : 'warning');
}

// Adicionar validações extras antes de exportar
function validarDadosCompleto() {
    const problemas = [];

    // 1. Verificar se todas as turmas têm sala
    const salasNormais = dados.salas.filter(s => !s.compartilhada).length;
    if (salasNormais < dados.turmas.length) {
        problemas.push({
            tipo: 'erro',
            mensagem: `Existem ${dados.turmas.length} turmas mas apenas ${salasNormais} salas exclusivas`
        });
    }

    // 2. Verificar carga horária total por turma
    dados.turmas.forEach(turma => {
        const totalAulas = calcularTotalAulasTurma(turma.id);
        if (totalAulas > 30) {
            problemas.push({
                tipo: 'aviso',
                mensagem: `Turma "${turma.nome}" tem ${totalAulas} aulas (máximo recomendado: 30)`
            });
        }
    });

    // 3. Verificar disponibilidade x carga dos professores
    dados.professores.forEach(prof => {
        const disc = dados.disciplinas.find(d => d.id === prof.disciplinaId);
        if (disc) {
            const cargaTotal = Object.values(disc.cargaHoraria).reduce((a, b) => a + b, 0);
            const disponibilidade = prof.disponibilidade.length;

            if (disponibilidade < cargaTotal) {
                problemas.push({
                    tipo: 'erro',
                    mensagem: `Professor "${prof.nome}": ${disponibilidade}h disponível mas precisa de ${cargaTotal}h`
                });
            }
        }
    });

    // 4. Verificar conflitos potenciais
    const horariosOcupados = {};
    dados.professores.forEach(prof => {
        prof.disponibilidade.forEach(slot => {
            const chave = `${slot.dia}-${slot.horario}`;
            if (!horariosOcupados[chave]) {
                horariosOcupados[chave] = [];
            }
            horariosOcupados[chave].push(prof.nome);
        });
    });

    // Verificar se há professores demais no mesmo horário
    Object.entries(horariosOcupados).forEach(([horario, professores]) => {
        if (professores.length > dados.salas.length) {
            problemas.push({
                tipo: 'aviso',
                mensagem: `Horário ${horario}: ${professores.length} professores disponíveis mas apenas ${dados.salas.length} salas`
            });
        }
    });

    return problemas;
}

// Adicionar botão de importação na interface
function adicionarBotaoImportacao() {
    const headerActions = document.querySelector('.header-actions');
    if (headerActions) {
        const btnImport = document.createElement('button');
        btnImport.className = 'btn btn-sm btn-outline';
        btnImport.innerHTML = '📊 Importar Planilha';
        btnImport.onclick = importarPlanilha;
        headerActions.appendChild(btnImport);
    }
}