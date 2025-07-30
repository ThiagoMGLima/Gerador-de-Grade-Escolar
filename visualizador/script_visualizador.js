let dadosCadastrados = null;

// Carregar arquivo JSON
document.getElementById('fileInput').addEventListener('change', function(e) {
    const file = e.target.files[0];
    if (file) {
        const reader = new FileReader();
        reader.onload = function(e) {
            try {
                dadosCadastrados = JSON.parse(e.target.result);
                if (dadosCadastrados.aulas) {
                    // É um arquivo de grade horária
                    alert('Este é um arquivo de grade horária. Use o Visualizador de Grade Horária.');
                } else {
                    // É um arquivo de dados cadastrados
                    visualizarDados();
                }
            } catch (error) {
                alert('Erro ao carregar arquivo JSON: ' + error.message);
            }
        };
        reader.readAsText(file);
    }
});

function visualizarDados() {
    const container = document.getElementById('dataContainer');
    container.innerHTML = '';

    // Metadados
    if (dadosCadastrados.metadata) {
        const metaDiv = document.createElement('div');
        metaDiv.className = 'stats-card';
        metaDiv.innerHTML = `
            <h3>📋 Informações do Arquivo</h3>
            <div class="stat-item">
                <span>Exportado em:</span>
                <strong>${new Date(dadosCadastrados.metadata.exportadoEm).toLocaleString('pt-BR')}</strong>
            </div>
            <div class="stat-item">
                <span>Versão:</span>
                <strong>${dadosCadastrados.metadata.versao}</strong>
            </div>
        `;
        container.appendChild(metaDiv);
    }

    // Turmas
    if (dadosCadastrados.turmas) {
        const turmasDiv = criarSecao('🎓 Turmas', dadosCadastrados.turmas,
            ['ID', 'Nome', 'Turno'],
            t => [t.id, t.nome, t.turno]
        );
        container.appendChild(turmasDiv);
    }

    // Disciplinas
    if (dadosCadastrados.disciplinas) {
        const discDiv = criarSecao('📚 Disciplinas', dadosCadastrados.disciplinas,
            ['ID', 'Nome', 'Carga Horária'],
            d => {
                const carga = Object.entries(d.aulasPorTurma || {})
                    .map(([turma, horas]) => {
                        const turmaObj = dadosCadastrados.turmas.find(t => t.id == turma);
                        const turmaNome = turmaObj ? turmaObj.nome : `ID ${turma}`;
                        return `${turmaNome}: ${horas}h`;
                    })
                    .join(', ');
                return [d.id, d.nome, carga || 'Não definida'];
            }
        );
        container.appendChild(discDiv);
    }

    // Professores
    if (dadosCadastrados.professores) {
        const profDiv = criarSecao('👥 Professores', dadosCadastrados.professores,
            ['ID', 'Nome', 'Disciplina', 'Disponibilidade'],
            p => {
                const disc = dadosCadastrados.disciplinas.find(d => d.id === p.idDisciplina);
                const discNome = disc ? disc.nome : `ID ${p.idDisciplina}`;
                const dispDetalhes = analisarDisponibilidade(p.disponibilidade);
                return [
                    p.id,
                    p.nome,
                    discNome,
                    `${p.disponibilidade.length} horários (${dispDetalhes})`
                ];
            }
        );
        container.appendChild(profDiv);
    }

    // Salas
    if (dadosCadastrados.salas) {
        const salasDiv = criarSecao('🏫 Salas', dadosCadastrados.salas,
            ['ID', 'Nome', 'Tipo', 'Compartilhada', 'Capacidade'],
            s => [
                s.id,
                s.nome,
                formatarTipoSala(s.tipo),
                s.compartilhada ? 'Sim' : 'Não',
                `${s.capacidade || 30} alunos`
            ]
        );
        container.appendChild(salasDiv);
    }

    // Associações Turma-Sala
    if (dadosCadastrados.associacoes && dadosCadastrados.associacoes.turmaSala) {
        const assocDiv = criarSecaoAssociacoes();
        container.appendChild(assocDiv);
    }

    // Resumo e Análise
    const resumoDiv = criarResumoGeral();
    container.appendChild(resumoDiv);

    // Análise de Problemas
    const problemasDiv = analisarProblemas();
    if (problemasDiv) {
        container.appendChild(problemasDiv);
    }
}

function criarSecao(titulo, dados, colunas, extrator) {
    const secaoDiv = document.createElement('div');
    secaoDiv.className = 'schedule-grid';

    const titleEl = document.createElement('h2');
    titleEl.style.padding = '15px';
    titleEl.style.textAlign = 'center';
    titleEl.style.backgroundColor = '#f8f9fa';
    titleEl.style.margin = '0';
    titleEl.textContent = `${titulo} (${dados.length})`;
    secaoDiv.appendChild(titleEl);

    const table = document.createElement('table');

    // Cabeçalho
    const thead = document.createElement('thead');
    const headerRow = document.createElement('tr');
    colunas.forEach(col => {
        const th = document.createElement('th');
        th.textContent = col;
        headerRow.appendChild(th);
    });
    thead.appendChild(headerRow);
    table.appendChild(thead);

    // Corpo
    const tbody = document.createElement('tbody');
    dados.forEach(item => {
        const row = document.createElement('tr');
        const valores = extrator(item);
        valores.forEach((valor, index) => {
            const td = document.createElement('td');
            td.textContent = valor;
            if (index === 0) td.style.fontWeight = 'bold'; // ID em negrito
            row.appendChild(td);
        });
        tbody.appendChild(row);
    });
    table.appendChild(tbody);

    secaoDiv.appendChild(table);
    return secaoDiv;
}

function criarSecaoAssociacoes() {
    const secaoDiv = document.createElement('div');
    secaoDiv.className = 'schedule-grid';

    const titleEl = document.createElement('h2');
    titleEl.style.padding = '15px';
    titleEl.style.textAlign = 'center';
    titleEl.style.backgroundColor = '#f8f9fa';
    titleEl.style.margin = '0';
    titleEl.textContent = '🔗 Associações Turma-Sala';
    secaoDiv.appendChild(titleEl);

    const table = document.createElement('table');

    // Cabeçalho
    const thead = document.createElement('thead');
    const headerRow = document.createElement('tr');
    ['Turma', 'Sala Atribuída'].forEach(col => {
        const th = document.createElement('th');
        th.textContent = col;
        headerRow.appendChild(th);
    });
    thead.appendChild(headerRow);
    table.appendChild(thead);

    // Corpo
    const tbody = document.createElement('tbody');
    const associacoes = dadosCadastrados.associacoes.turmaSala;

    Object.entries(associacoes).forEach(([turmaId, salaId]) => {
        const turma = dadosCadastrados.turmas.find(t => t.id == turmaId);
        const sala = dadosCadastrados.salas.find(s => s.id == salaId);

        if (turma && sala) {
            const row = document.createElement('tr');
            const td1 = document.createElement('td');
            td1.textContent = turma.nome;
            const td2 = document.createElement('td');
            td2.textContent = sala.nome;
            row.appendChild(td1);
            row.appendChild(td2);
            tbody.appendChild(row);
        }
    });

    table.appendChild(tbody);
    secaoDiv.appendChild(table);
    return secaoDiv;
}

function criarResumoGeral() {
    const resumoDiv = document.createElement('div');
    resumoDiv.className = 'stats-container';

    // Card de resumo quantitativo
    const cardQuantidade = document.createElement('div');
    cardQuantidade.className = 'stats-card';
    cardQuantidade.innerHTML = `
        <h3>📊 Resumo Quantitativo</h3>
        <div class="stat-item">
            <span>Total de Turmas:</span>
            <strong>${dadosCadastrados.turmas?.length || 0}</strong>
        </div>
        <div class="stat-item">
            <span>Total de Disciplinas:</span>
            <strong>${dadosCadastrados.disciplinas?.length || 0}</strong>
        </div>
        <div class="stat-item">
            <span>Total de Professores:</span>
            <strong>${dadosCadastrados.professores?.length || 0}</strong>
        </div>
        <div class="stat-item">
            <span>Total de Salas:</span>
            <strong>${dadosCadastrados.salas?.length || 0}</strong>
        </div>
    `;

    // Card de carga horária
    const cardCarga = document.createElement('div');
    cardCarga.className = 'stats-card';
    cardCarga.innerHTML = '<h3>📚 Análise de Carga Horária</h3>';

    let totalAulas = 0;
    dadosCadastrados.turmas.forEach(turma => {
        let aulasTurma = 0;
        dadosCadastrados.disciplinas.forEach(disc => {
            if (disc.aulasPorTurma && disc.aulasPorTurma[turma.id]) {
                aulasTurma += disc.aulasPorTurma[turma.id];
            }
        });
        totalAulas += aulasTurma;

        const item = document.createElement('div');
        item.className = 'stat-item';
        item.innerHTML = `
            <span>${turma.nome}:</span>
            <strong>${aulasTurma}/30 aulas</strong>
        `;
        cardCarga.appendChild(item);

        // Barra de progresso
        const barContainer = document.createElement('div');
        barContainer.className = 'stat-bar';
        const fill = document.createElement('div');
        fill.className = 'stat-fill';
        fill.style.width = `${(aulasTurma / 30) * 100}%`;
        if (aulasTurma > 30) fill.style.backgroundColor = '#ff9800';
        barContainer.appendChild(fill);
        cardCarga.appendChild(barContainer);
    });

    // Card de tipos de sala
    const cardSalas = document.createElement('div');
    cardSalas.className = 'stats-card';
    cardSalas.innerHTML = '<h3>🏫 Distribuição de Salas</h3>';

    const tiposSala = {};
    let salasCompartilhadas = 0;

    dadosCadastrados.salas.forEach(sala => {
        tiposSala[sala.tipo] = (tiposSala[sala.tipo] || 0) + 1;
        if (sala.compartilhada) salasCompartilhadas++;
    });

    Object.entries(tiposSala).forEach(([tipo, qtd]) => {
        const item = document.createElement('div');
        item.className = 'stat-item';
        item.innerHTML = `
            <span>${formatarTipoSala(tipo)}:</span>
            <strong>${qtd}</strong>
        `;
        cardSalas.appendChild(item);
    });

    const itemComp = document.createElement('div');
    itemComp.className = 'stat-item';
    itemComp.style.marginTop = '10px';
    itemComp.style.borderTop = '1px solid #eee';
    itemComp.style.paddingTop = '10px';
    itemComp.innerHTML = `
        <span>Salas Compartilhadas:</span>
        <strong>${salasCompartilhadas}</strong>
    `;
    cardSalas.appendChild(itemComp);

    resumoDiv.appendChild(cardQuantidade);
    resumoDiv.appendChild(cardCarga);
    resumoDiv.appendChild(cardSalas);

    return resumoDiv;
}

function analisarProblemas() {
    const problemas = [];

    // Verificar disciplinas sem professor
    dadosCadastrados.disciplinas.forEach(disc => {
        const temProfessor = dadosCadastrados.professores.some(p => p.idDisciplina === disc.id);
        if (!temProfessor) {
            problemas.push({
                tipo: 'erro',
                mensagem: `Disciplina "${disc.nome}" não tem professor atribuído`
            });
        }
    });

    // Verificar carga horária x disponibilidade
    dadosCadastrados.professores.forEach(prof => {
        const disc = dadosCadastrados.disciplinas.find(d => d.id === prof.idDisciplina);
        if (disc) {
            const cargaTotal = Object.values(disc.aulasPorTurma || {})
                .reduce((sum, val) => sum + val, 0);
            if (prof.disponibilidade.length < cargaTotal) {
                problemas.push({
                    tipo: 'aviso',
                    mensagem: `Professor "${prof.nome}": ${prof.disponibilidade.length}h disponível mas precisa ${cargaTotal}h`
                });
            }
        }
    });

    // Verificar turmas sem sala
    const turmasComSala = Object.keys(dadosCadastrados.associacoes?.turmaSala || {});
    dadosCadastrados.turmas.forEach(turma => {
        if (!turmasComSala.includes(turma.id.toString())) {
            problemas.push({
                tipo: 'erro',
                mensagem: `Turma "${turma.nome}" não tem sala atribuída`
            });
        }
    });

    if (problemas.length === 0) return null;

    const problemasDiv = document.createElement('div');
    problemasDiv.className = 'stats-card';
    problemasDiv.style.borderLeft = '4px solid #ff9800';
    problemasDiv.innerHTML = '<h3>⚠️ Problemas Detectados</h3>';

    problemas.forEach(problema => {
        const item = document.createElement('div');
        item.className = 'stat-item';
        item.style.color = problema.tipo === 'erro' ? '#f44336' : '#ff9800';
        item.innerHTML = `<strong>${problema.tipo.toUpperCase()}:</strong> ${problema.mensagem}`;
        problemasDiv.appendChild(item);
    });

    return problemasDiv;
}

function analisarDisponibilidade(disponibilidade) {
    const diasCount = {};
    const dias = ['Seg', 'Ter', 'Qua', 'Qui', 'Sex'];

    disponibilidade.forEach(slot => {
        diasCount[slot.dia] = (diasCount[slot.dia] || 0) + 1;
    });

    return Object.entries(diasCount)
        .map(([dia, count]) => `${dias[dia]}: ${count}`)
        .join(', ');
}

function formatarTipoSala(tipo) {
    const tipos = {
        'normal': 'Sala Normal',
        'laboratorio': 'Laboratório',
        'quadra': 'Quadra Esportiva',
        'biblioteca': 'Biblioteca'
    };
    return tipos[tipo] || tipo;
}