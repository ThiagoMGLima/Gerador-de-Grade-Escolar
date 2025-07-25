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
const dias = ['Segunda', 'Terça', 'Quarta', 'Quinta', 'Sexta'];
const horarios = ['7:30-8:15', '8:15-9:00', '9:00-9:45', '10:05-10:50', '10:50-11:35', '11:35-12:20'];

// ========== FUNÇÕES DE PERSISTÊNCIA ==========

function carregarDados() {
    const savedData = localStorage.getItem('gradeDados');
    if (savedData) {
        try {
            dados = JSON.parse(savedData);
        } catch (e) {
            console.error('Erro ao carregar dados:', e);
            mostrarMensagem('Erro ao carregar dados salvos', 'error');
        }
    }
    atualizarTodasTabelas();
    atualizarResumo();
}

function salvarDados() {
    try {
        localStorage.setItem('gradeDados', JSON.stringify(dados));
    } catch (e) {
        console.error('Erro ao salvar dados:', e);
        mostrarMensagem('Erro ao salvar dados', 'error');
    }
}

// ========== FUNÇÕES DE NAVEGAÇÃO ==========

function showTab(tabName) {
    // Ocultar todas as abas
    document.querySelectorAll('.tab-content').forEach(tab => {
        tab.style.display = 'none';
    });

    // Remover classe active de todas as abas
    document.querySelectorAll('.nav-tab').forEach(tab => {
        tab.classList.remove('active');
    });

    // Mostrar aba selecionada
    const selectedTab = document.getElementById(tabName + '-tab');
    if (selectedTab) {
        selectedTab.style.display = 'block';
    }

    // Adicionar classe active ao botão clicado
    if (event && event.target) {
        event.target.classList.add('active');
    }

    // Executar ações específicas de cada aba
    switch(tabName) {
        case 'disciplinas':
            atualizarCargaHorariaForm();
            break;
        case 'professores':
            atualizarDisciplinasSelect();
            criarGridDisponibilidade();
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

    if (!nome) {
        mostrarMensagem('Por favor, preencha o nome da turma', 'error');
        return;
    }

    // Verificar se já existe turma com mesmo nome
    if (dados.turmas.some(t => t.nome.toLowerCase() === nome.toLowerCase())) {
        mostrarMensagem('Já existe uma turma com este nome', 'error');
        return;
    }

    const turma = {
        id: dados.nextId.turma++,
        nome: nome,
        turno: turno
    };

    dados.turmas.push(turma);
    salvarDados();

    // Limpar formulário
    document.getElementById('turmaNome').value = '';
    document.getElementById('turmaTurno').selectedIndex = 0;

    atualizarTabelaTurmas();
    mostrarMensagem('Turma adicionada com sucesso!', 'success');
}

function atualizarTabelaTurmas() {
    const tbody = document.querySelector('#turmasTable tbody');
    if (!tbody) return;

    tbody.innerHTML = '';

    dados.turmas.forEach(turma => {
        const tr = document.createElement('tr');
        tr.innerHTML = `
            <td>${turma.id}</td>
            <td>${turma.nome}</td>
            <td>${turma.turno.charAt(0).toUpperCase() + turma.turno.slice(1)}</td>
            <td class="action-buttons">
                <button class="btn btn-secondary" onclick="editarTurma(${turma.id})">Editar</button>
                <button class="btn btn-danger" onclick="excluirTurma(${turma.id})">Excluir</button>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function editarTurma(id) {
    const turma = dados.turmas.find(t => t.id === id);
    if (!turma) return;

    const novoNome = prompt('Digite o novo nome da turma:', turma.nome);
    if (novoNome && novoNome.trim()) {
        // Verificar duplicação
        if (dados.turmas.some(t => t.id !== id && t.nome.toLowerCase() === novoNome.trim().toLowerCase())) {
            mostrarMensagem('Já existe uma turma com este nome', 'error');
            return;
        }

        turma.nome = novoNome.trim();
        salvarDados();
        atualizarTabelaTurmas();
        mostrarMensagem('Turma atualizada com sucesso!', 'success');
    }
}

function excluirTurma(id) {
    const turma = dados.turmas.find(t => t.id === id);
    if (!turma) return;

    // Verificar se há disciplinas com carga horária para esta turma
    const temCargaHoraria = dados.disciplinas.some(d => d.cargaHoraria[id] > 0);

    if (temCargaHoraria) {
        if (!confirm(`A turma "${turma.nome}" possui carga horária definida em algumas disciplinas. Deseja realmente excluir?`)) {
            return;
        }
    } else if (!confirm(`Tem certeza que deseja excluir a turma "${turma.nome}"?`)) {
        return;
    }

    // Remover turma
    dados.turmas = dados.turmas.filter(t => t.id !== id);

    // Remover carga horária relacionada
    dados.disciplinas.forEach(disc => {
        delete disc.cargaHoraria[id];
    });

    salvarDados();
    atualizarTabelaTurmas();
    mostrarMensagem('Turma excluída com sucesso!', 'success');
}

// ========== FUNÇÕES PARA DISCIPLINAS ==========

function atualizarCargaHorariaForm() {
    const container = document.getElementById('cargaHorariaContainer');
    if (!container) return;

    container.innerHTML = '';

    if (dados.turmas.length === 0) {
        container.innerHTML = '<p style="color: #666;">Nenhuma turma cadastrada. Cadastre turmas primeiro.</p>';
        return;
    }

    const grid = document.createElement('div');
    grid.className = 'form-grid';

    dados.turmas.forEach(turma => {
        const div = document.createElement('div');
        div.className = 'form-group';
        div.innerHTML = `
            <label for="carga_${turma.id}">${turma.nome}</label>
            <input type="number" id="carga_${turma.id}" min="0" max="10" value="0" placeholder="0">
        `;
        grid.appendChild(div);
    });

    container.appendChild(grid);
}

function adicionarDisciplina() {
    const nome = document.getElementById('disciplinaNome').value.trim();

    if (!nome) {
        mostrarMensagem('Por favor, preencha o nome da disciplina', 'error');
        return;
    }

    // Verificar duplicação
    if (dados.disciplinas.some(d => d.nome.toLowerCase() === nome.toLowerCase())) {
        mostrarMensagem('Já existe uma disciplina com este nome', 'error');
        return;
    }

    const disciplina = {
        id: dados.nextId.disciplina++,
        nome: nome,
        cargaHoraria: {}
    };

    // Coletar carga horária por turma
    let totalCarga = 0;
    dados.turmas.forEach(turma => {
        const input = document.getElementById(`carga_${turma.id}`);
        if (input) {
            const carga = parseInt(input.value) || 0;
            if (carga > 0) {
                disciplina.cargaHoraria[turma.id] = carga;
                totalCarga += carga;
            }
        }
    });

    if (totalCarga === 0) {
        mostrarMensagem('Por favor, defina a carga horária para pelo menos uma turma', 'error');
        return;
    }

    dados.disciplinas.push(disciplina);
    salvarDados();

    // Limpar formulário
    document.getElementById('disciplinaNome').value = '';
    atualizarCargaHorariaForm();
    atualizarTabelaDisciplinas();
    mostrarMensagem('Disciplina adicionada com sucesso!', 'success');
}

function atualizarTabelaDisciplinas() {
    const tbody = document.querySelector('#disciplinasTable tbody');
    if (!tbody) return;

    tbody.innerHTML = '';

    dados.disciplinas.forEach(disc => {
        const tr = document.createElement('tr');

        // Formatar carga horária
        const cargaArray = [];
        let totalCarga = 0;

        Object.entries(disc.cargaHoraria).forEach(([turmaId, carga]) => {
            const turma = dados.turmas.find(t => t.id == turmaId);
            if (turma && carga > 0) {
                cargaArray.push(`${turma.nome}: ${carga}h`);
                totalCarga += carga;
            }
        });

        const cargaStr = cargaArray.length > 0 ?
            `${cargaArray.join(', ')} (Total: ${totalCarga}h)` :
            'Não definida';

        tr.innerHTML = `
            <td>${disc.id}</td>
            <td>${disc.nome}</td>
            <td>${cargaStr}</td>
            <td class="action-buttons">
                <button class="btn btn-secondary" onclick="editarDisciplina(${disc.id})">Editar</button>
                <button class="btn btn-danger" onclick="excluirDisciplina(${disc.id})">Excluir</button>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function editarDisciplina(id) {
    const disciplina = dados.disciplinas.find(d => d.id === id);
    if (!disciplina) return;

    const novoNome = prompt('Digite o novo nome da disciplina:', disciplina.nome);
    if (novoNome && novoNome.trim()) {
        // Verificar duplicação
        if (dados.disciplinas.some(d => d.id !== id && d.nome.toLowerCase() === novoNome.trim().toLowerCase())) {
            mostrarMensagem('Já existe uma disciplina com este nome', 'error');
            return;
        }

        disciplina.nome = novoNome.trim();
        salvarDados();
        atualizarTabelaDisciplinas();
        mostrarMensagem('Disciplina atualizada com sucesso!', 'success');
    }
}

function excluirDisciplina(id) {
    const disciplina = dados.disciplinas.find(d => d.id === id);
    if (!disciplina) return;

    // Verificar se há professor atribuído
    const professor = dados.professores.find(p => p.disciplinaId === id);

    if (professor) {
        if (!confirm(`A disciplina "${disciplina.nome}" está atribuída ao professor "${professor.nome}". Deseja realmente excluir?`)) {
            return;
        }
    } else if (!confirm(`Tem certeza que deseja excluir a disciplina "${disciplina.nome}"?`)) {
        return;
    }

    dados.disciplinas = dados.disciplinas.filter(d => d.id !== id);
    salvarDados();
    atualizarTabelaDisciplinas();
    mostrarMensagem('Disciplina excluída com sucesso!', 'success');
}

// ========== FUNÇÕES PARA PROFESSORES ==========

function atualizarDisciplinasSelect() {
    const select = document.getElementById('professorDisciplina');
    if (!select) return;

    select.innerHTML = '<option value="">Selecione uma disciplina</option>';

    // Listar apenas disciplinas sem professor atribuído
    dados.disciplinas.forEach(disc => {
        const jaTemProfessor = dados.professores.some(p => p.disciplinaId === disc.id);
        const option = document.createElement('option');
        option.value = disc.id;
        option.textContent = disc.nome + (jaTemProfessor ? ' (já tem professor)' : '');
        if (jaTemProfessor) {
            option.style.color = '#999';
        }
        select.appendChild(option);
    });
}

function criarGridDisponibilidade() {
    const grid = document.getElementById('availabilityGrid');
    if (!grid) return;

    grid.innerHTML = '';

    // Cabeçalho vazio
    const emptyHeader = document.createElement('div');
    emptyHeader.className = 'availability-header';
    grid.appendChild(emptyHeader);

    // Cabeçalhos de horários
    horarios.forEach(horario => {
        const header = document.createElement('div');
        header.className = 'availability-header';
        header.textContent = horario;
        grid.appendChild(header);
    });

    // Linhas por dia
    dias.forEach((dia, diaIndex) => {
        // Nome do dia
        const dayHeader = document.createElement('div');
        dayHeader.className = 'availability-header';
        dayHeader.textContent = dia;
        grid.appendChild(dayHeader);

        // Células de disponibilidade
        horarios.forEach((horario, horarioIndex) => {
            const cell = document.createElement('div');
            cell.className = 'availability-cell';
            cell.dataset.dia = diaIndex;
            cell.dataset.horario = horarioIndex;
            cell.title = `${dia} - ${horario}`;
            cell.onclick = function() {
                this.classList.toggle('selected');
            };
            grid.appendChild(cell);
        });
    });
}

function adicionarProfessor() {
    const nome = document.getElementById('professorNome').value.trim();
    const disciplinaId = document.getElementById('professorDisciplina').value;

    if (!nome) {
        mostrarMensagem('Por favor, preencha o nome do professor', 'error');
        return;
    }

    if (!disciplinaId) {
        mostrarMensagem('Por favor, selecione uma disciplina', 'error');
        return;
    }

    // Verificar se já existe professor com mesmo nome
    if (dados.professores.some(p => p.nome.toLowerCase() === nome.toLowerCase())) {
        mostrarMensagem('Já existe um professor com este nome', 'error');
        return;
    }

    // Verificar se a disciplina já tem professor
    if (dados.professores.some(p => p.disciplinaId === parseInt(disciplinaId))) {
        mostrarMensagem('Esta disciplina já possui um professor atribuído', 'error');
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
        mostrarMensagem('Por favor, selecione pelo menos um horário disponível', 'error');
        return;
    }

    // Verificar se a disponibilidade é suficiente para a carga horária
    const disciplina = dados.disciplinas.find(d => d.id === parseInt(disciplinaId));
    if (disciplina) {
        const totalCarga = Object.values(disciplina.cargaHoraria).reduce((a, b) => a + b, 0);
        if (disponibilidade.length < totalCarga) {
            mostrarMensagem(`Atenção: O professor tem ${disponibilidade.length} horários disponíveis, mas a disciplina requer ${totalCarga} aulas`, 'warning');
        }
    }

    const professor = {
        id: dados.nextId.professor++,
        nome: nome,
        disciplinaId: parseInt(disciplinaId),
        disponibilidade: disponibilidade
    };

    dados.professores.push(professor);
    salvarDados();

    // Limpar formulário
    document.getElementById('professorNome').value = '';
    document.getElementById('professorDisciplina').value = '';
    criarGridDisponibilidade();
    atualizarTabelaProfessores();
    atualizarDisciplinasSelect();
    mostrarMensagem('Professor adicionado com sucesso!', 'success');
}

function atualizarTabelaProfessores() {
    const tbody = document.querySelector('#professoresTable tbody');
    if (!tbody) return;

    tbody.innerHTML = '';

    dados.professores.forEach(prof => {
        const tr = document.createElement('tr');

        const disciplina = dados.disciplinas.find(d => d.id === prof.disciplinaId);
        const totalHorarios = prof.disponibilidade.length;

        // Calcular carga horária total da disciplina
        let cargaTotal = 0;
        if (disciplina) {
            cargaTotal = Object.values(disciplina.cargaHoraria).reduce((a, b) => a + b, 0);
        }

        const statusHorarios = totalHorarios >= cargaTotal ?
            `${totalHorarios} horários ✓` :
            `${totalHorarios} horários ⚠️ (Necessário: ${cargaTotal})`;

        tr.innerHTML = `
            <td>${prof.id}</td>
            <td>${prof.nome}</td>
            <td>${disciplina ? disciplina.nome : 'N/A'}</td>
            <td>${statusHorarios}</td>
            <td class="action-buttons">
                <button class="btn btn-secondary" onclick="editarProfessor(${prof.id})">Editar</button>
                <button class="btn btn-danger" onclick="excluirProfessor(${prof.id})">Excluir</button>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function editarProfessor(id) {
    const professor = dados.professores.find(p => p.id === id);
    if (!professor) return;

    const novoNome = prompt('Digite o novo nome do professor:', professor.nome);
    if (novoNome && novoNome.trim()) {
        // Verificar duplicação
        if (dados.professores.some(p => p.id !== id && p.nome.toLowerCase() === novoNome.trim().toLowerCase())) {
            mostrarMensagem('Já existe um professor com este nome', 'error');
            return;
        }

        professor.nome = novoNome.trim();
        salvarDados();
        atualizarTabelaProfessores();
        mostrarMensagem('Professor atualizado com sucesso!', 'success');
    }
}

function excluirProfessor(id) {
    const professor = dados.professores.find(p => p.id === id);
    if (!professor) return;

    if (confirm(`Tem certeza que deseja excluir o professor "${professor.nome}"?`)) {
        dados.professores = dados.professores.filter(p => p.id !== id);
        salvarDados();
        atualizarTabelaProfessores();
        atualizarDisciplinasSelect();
        mostrarMensagem('Professor excluído com sucesso!', 'success');
    }
}

// ========== FUNÇÕES PARA SALAS ==========

function adicionarSala() {
    const nome = document.getElementById('salaNome').value.trim();
    const tipo = document.getElementById('salaTipo').value;
    const compartilhada = document.getElementById('salaCompartilhada').checked;

    if (!nome) {
        mostrarMensagem('Por favor, preencha o nome da sala', 'error');
        return;
    }

    // Verificar duplicação
    if (dados.salas.some(s => s.nome.toLowerCase() === nome.toLowerCase())) {
        mostrarMensagem('Já existe uma sala com este nome', 'error');
        return;
    }

    const sala = {
        id: dados.nextId.sala++,
        nome: nome,
        tipo: tipo,
        compartilhada: compartilhada
    };

    dados.salas.push(sala);
    salvarDados();

    // Limpar formulário
    document.getElementById('salaNome').value = '';
    document.getElementById('salaTipo').selectedIndex = 0;
    document.getElementById('salaCompartilhada').checked = false;

    atualizarTabelaSalas();
    mostrarMensagem('Sala adicionada com sucesso!', 'success');
}

function atualizarTabelaSalas() {
    const tbody = document.querySelector('#salasTable tbody');
    if (!tbody) return;

    tbody.innerHTML = '';

    dados.salas.forEach(sala => {
        const tr = document.createElement('tr');

        const tipoFormatado = {
            'normal': 'Sala Normal',
            'laboratorio': 'Laboratório',
            'quadra': 'Quadra',
            'biblioteca': 'Biblioteca'
        }[sala.tipo] || sala.tipo;

        tr.innerHTML = `
            <td>${sala.id}</td>
            <td>${sala.nome}</td>
            <td>${tipoFormatado}</td>
            <td>${sala.compartilhada ? 'Sim' : 'Não'}</td>
            <td class="action-buttons">
                <button class="btn btn-secondary" onclick="editarSala(${sala.id})">Editar</button>
                <button class="btn btn-danger" onclick="excluirSala(${sala.id})">Excluir</button>
            </td>
        `;
        tbody.appendChild(tr);
    });
}

function editarSala(id) {
    const sala = dados.salas.find(s => s.id === id);
    if (!sala) return;

    const novoNome = prompt('Digite o novo nome da sala:', sala.nome);
    if (novoNome && novoNome.trim()) {
        // Verificar duplicação
        if (dados.salas.some(s => s.id !== id && s.nome.toLowerCase() === novoNome.trim().toLowerCase())) {
            mostrarMensagem('Já existe uma sala com este nome', 'error');
            return;
        }

        sala.nome = novoNome.trim();
        salvarDados();
        atualizarTabelaSalas();
        mostrarMensagem('Sala atualizada com sucesso!', 'success');
    }
}

function excluirSala(id) {
    const sala = dados.salas.find(s => s.id === id);
    if (!sala) return;

    if (confirm(`Tem certeza que deseja excluir a sala "${sala.nome}"?`)) {
        dados.salas = dados.salas.filter(s => s.id !== id);
        salvarDados();
        atualizarTabelaSalas();
        mostrarMensagem('Sala excluída com sucesso!', 'success');
    }
}

// ========== FUNÇÕES DO RESUMO ==========

function atualizarResumo() {
    document.getElementById('totalTurmas').textContent = dados.turmas.length;
    document.getElementById('totalDisciplinas').textContent = dados.disciplinas.length;
    document.getElementById('totalProfessores').textContent = dados.professores.length;
    document.getElementById('totalSalas').textContent = dados.salas.length;
}

function validarDados() {
    const results = document.getElementById('validationResults');
    if (!results) return;

    results.innerHTML = '';

    const warnings = [];
    const errors = [];
    const infos = [];

    // Validações básicas
    if (dados.turmas.length === 0) {
        errors.push('Nenhuma turma cadastrada');
    }

    if (dados.disciplinas.length === 0) {
        errors.push('Nenhuma disciplina cadastrada');
    }

    if (dados.professores.length === 0) {
        errors.push('Nenhum professor cadastrado');
    }

    if (dados.salas.length === 0) {
        errors.push('Nenhuma sala cadastrada');
    }

    // Validar número de salas vs turmas
    const salasNormais = dados.salas.filter(s => !s.compartilhada).length;
    if (salasNormais < dados.turmas.length) {
        warnings.push(`Há ${dados.turmas.length} turmas mas apenas ${salasNormais} salas não compartilhadas`);
    }

    // Validar professores sem disciplina válida
    dados.professores.forEach(prof => {
        const disc = dados.disciplinas.find(d => d.id === prof.disciplinaId);
        if (!disc) {
            errors.push(`Professor ${prof.nome} está atribuído a uma disciplina inexistente`);
        }
    });

    // Validar disciplinas sem professor
    dados.disciplinas.forEach(disc => {
        const prof = dados.professores.find(p => p.disciplinaId === disc.id);
        if (!prof) {
            warnings.push(`Disciplina "${disc.nome}" não tem professor atribuído`);
        }
    });

    // Validar carga horária vs disponibilidade
    dados.disciplinas.forEach(disc => {
        const prof = dados.professores.find(p => p.disciplinaId === disc.id);
        if (prof) {
            const totalCarga = Object.values(disc.cargaHoraria).reduce((a, b) => a + b, 0);
            const disponibilidade = prof.disponibilidade.length;

            if (totalCarga > disponibilidade) {
                errors.push(`Professor ${prof.nome}: necessita ${totalCarga} horários mas tem apenas ${disponibilidade} disponíveis`);
            } else if (disponibilidade > totalCarga * 1.5) {
                infos.push(`Professor ${prof.nome}: tem ${disponibilidade} horários disponíveis para ${totalCarga} aulas`);
            }
        }
    });

    // Exibir resultados
    if (errors.length > 0) {
        results.innerHTML += `
            <div class="message error">
                <strong>❌ Erros encontrados (${errors.length}):</strong>
                <ul>${errors.map(e => `<li>${e}</li>`).join('')}</ul>
            </div>
        `;
    }

    if (warnings.length > 0) {
        results.innerHTML += `
            <div class="message warning" style="background: #fff3cd; color: #856404; border: 1px solid #ffeaa7;">
                <strong>⚠️ Avisos (${warnings.length}):</strong>
                <ul>${warnings.map(w => `<li>${w}</li>`).join('')}</ul>
            </div>
        `;
    }

    if (infos.length > 0) {
        results.innerHTML += `
            <div class="message info" style="background: #e3f2fd; color: #1565c0; border: 1px solid #64b5f6;">
                <strong>ℹ️ Informações (${infos.length}):</strong>
                <ul>${infos.map(i => `<li>${i}</li>`).join('')}</ul>
            </div>
        `;
    }

    if (errors.length === 0 && warnings.length === 0) {
        results.innerHTML = '<div class="message success">✅ Todos os dados estão válidos e prontos para exportação!</div>';
    }
}

// ========== FUNÇÕES DE EXPORTAÇÃO ==========

function exportarDados() {
    try {
        const dadosExportacao = {
            ...dados,
            metadata: {
                versao: '1.0',
                dataExportacao: new Date().toISOString(),
                totalRegistros: {
                    turmas: dados.turmas.length,
                    disciplinas: dados.disciplinas.length,
                    professores: dados.professores.length,
                    salas: dados.salas.length
                }
            }
        };

        const dataStr = JSON.stringify(dadosExportacao, null, 2);
        const dataBlob = new Blob([dataStr], {type: 'application/json'});
        const url = URL.createObjectURL(dataBlob);
        const link = document.createElement('a');
        link.href = url;

        const timestamp = new Date().toISOString().split('T')[0];
        link.download = `grade_horaria_${timestamp}.json`;

        document.body.appendChild(link);
        link.click();
        document.body.removeChild(link);

        URL.revokeObjectURL(url);
        mostrarMensagem('Dados exportados com sucesso!', 'success');
    } catch (e) {
        console.error('Erro ao exportar:', e);
        mostrarMensagem('Erro ao exportar dados', 'error');
    }
}
