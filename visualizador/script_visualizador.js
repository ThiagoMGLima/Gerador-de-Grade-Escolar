let gradeData = null;
        let colorMap = {};
        let colorIndex = 0;

        // Carregar arquivo JSON
        document.getElementById('fileInput').addEventListener('change', function(e) {
            const file = e.target.files[0];
            if (file) {
                const reader = new FileReader();
                reader.onload = function(e) {
                    try {
                        gradeData = JSON.parse(e.target.result);
                        processData();
                        updateView();
                    } catch (error) {
                        alert('Erro ao carregar arquivo JSON: ' + error.message);
                    }
                };
                reader.readAsText(file);
            }
        });

        // Processar dados e criar mapa de cores
        function processData() {
            colorMap = {};
            colorIndex = 0;

            // Atribuir cores às disciplinas
            gradeData.aulas.forEach(aula => {
                if (!colorMap[aula.disciplina]) {
                    colorMap[aula.disciplina] = colorIndex++;
                }
            });

            updateLegend();
            updateStats();
        }

        // Atualizar visualização
        function updateView() {
            const viewType = document.getElementById('viewSelect').value;
            const filterValue = document.getElementById('filterSelect').value;

            updateFilterOptions(viewType);

            let schedules = {};

            // Organizar dados por tipo de visualização
            gradeData.aulas.forEach(aula => {
                let key;
                switch (viewType) {
                    case 'turma':
                        key = aula.turma;
                        break;
                    case 'professor':
                        key = aula.professor;
                        break;
                    case 'sala':
                        key = aula.sala;
                        break;
                }

                if (filterValue && key !== filterValue) return;

                if (!schedules[key]) {
                    schedules[key] = Array(5).fill(null).map(() => Array(6).fill(null));
                }

                schedules[key][aula.dia][aula.hora] = aula;
            });

            renderSchedules(schedules);
        }

        // Atualizar opções de filtro
        function updateFilterOptions(viewType) {
            const filterSelect = document.getElementById('filterSelect');
            filterSelect.innerHTML = '<option value="">Todos</option>';

            let options = new Set();
            gradeData.aulas.forEach(aula => {
                switch (viewType) {
                    case 'turma':
                        options.add(aula.turma);
                        break;
                    case 'professor':
                        options.add(aula.professor);
                        break;
                    case 'sala':
                        options.add(aula.sala);
                        break;
                }
            });

            Array.from(options).sort().forEach(option => {
                const optionEl = document.createElement('option');
                optionEl.value = option;
                optionEl.textContent = option;
                filterSelect.appendChild(optionEl);
            });
        }

        // Renderizar grades
        function renderSchedules(schedules) {
            const container = document.getElementById('scheduleContainer');
            container.innerHTML = '';

            Object.entries(schedules).forEach(([key, schedule]) => {
                const gridDiv = document.createElement('div');
                gridDiv.className = 'schedule-grid';

                const title = document.createElement('h2');
                title.style.padding = '15px';
                title.style.textAlign = 'center';
                title.style.backgroundColor = '#f8f9fa';
                title.style.margin = '0';
                title.textContent = key;
                gridDiv.appendChild(title);

                const table = document.createElement('table');

                // Cabeçalho
                const thead = document.createElement('thead');
                const headerRow = document.createElement('tr');
                headerRow.innerHTML = '<th class="time-header">Horário</th>';
                gradeData.metadata.dias.forEach(dia => {
                    headerRow.innerHTML += `<th>${dia}</th>`;
                });
                thead.appendChild(headerRow);
                table.appendChild(thead);

                // Corpo
                const tbody = document.createElement('tbody');
                gradeData.metadata.horarios.forEach((horario, h) => {
                    const row = document.createElement('tr');
                    row.innerHTML = `<td class="time-header">${horario}</td>`;

                    for (let d = 0; d < 5; d++) {
                        const cell = document.createElement('td');
                        cell.className = 'class-cell';

                        if (schedule[d][h]) {
                            const aula = schedule[d][h];
                            const colorClass = `color-${colorMap[aula.disciplina]}`;
                            cell.innerHTML = `
                                <div class="class-content ${colorClass}">
                                    <div class="subject-name">${aula.disciplina}</div>
                                    <div class="teacher-name">${aula.professor}</div>
                                </div>
                            `;
                        } else {
                            cell.innerHTML = '<div class="empty-cell">-</div>';
                        }

                        row.appendChild(cell);
                    }

                    tbody.appendChild(row);
                });
                table.appendChild(tbody);

                gridDiv.appendChild(table);
                container.appendChild(gridDiv);
            });
        }

        // Atualizar legenda
        function updateLegend() {
            const legendDiv = document.getElementById('legend');
            legendDiv.innerHTML = '<h3 style="width: 100%; margin-bottom: 10px;">Legenda das Disciplinas:</h3>';

            Object.entries(colorMap).forEach(([disciplina, colorIdx]) => {
                const item = document.createElement('div');
                item.className = 'legend-item';
                item.innerHTML = `
                    <div class="legend-color color-${colorIdx}"></div>
                    <span>${disciplina}</span>
                `;
                legendDiv.appendChild(item);
            });
        }

        // Atualizar estatísticas
        function updateStats() {
            const container = document.getElementById('statsContainer');
            container.innerHTML = '';

            // Card de aulas por turma
            if (gradeData.estatisticas && gradeData.estatisticas.aulasPorTurma) {
                const card = createStatsCard('Aulas por Turma', gradeData.estatisticas.aulasPorTurma);
                container.appendChild(card);
            }

            // Card de aulas por professor
            if (gradeData.estatisticas && gradeData.estatisticas.aulasPorProfessor) {
                const card = createStatsCard('Carga Horária dos Professores', gradeData.estatisticas.aulasPorProfessor);
                container.appendChild(card);
            }

            // Análise adicional
            analyzeScheduleQuality();
        }

        // Criar card de estatísticas
        function createStatsCard(title, data) {
            const card = document.createElement('div');
            card.className = 'stats-card';

            const titleEl = document.createElement('h3');
            titleEl.textContent = title;
            card.appendChild(titleEl);

            const maxValue = Math.max(...Object.values(data));

            Object.entries(data).forEach(([key, value]) => {
                const item = document.createElement('div');
                item.className = 'stat-item';
                item.innerHTML = `
                    <span>${key}</span>
                    <strong>${value} aulas</strong>
                `;

                const barContainer = document.createElement('div');
                barContainer.className = 'stat-bar';
                const fill = document.createElement('div');
                fill.className = 'stat-fill';
                fill.style.width = `${(value / maxValue) * 100}%`;
                barContainer.appendChild(fill);

                card.appendChild(item);
                card.appendChild(barContainer);
            });

            return card;
        }

        // Análise de qualidade da grade
        function analyzeScheduleQuality() {
            const card = document.createElement('div');
            card.className = 'stats-card';

            const title = document.createElement('h3');
            title.textContent = 'Análise de Qualidade';
            card.appendChild(title);

            // Análise de janelas de horário
            let totalGaps = 0;
            const professorSchedules = {};

            gradeData.aulas.forEach(aula => {
                if (!professorSchedules[aula.professor]) {
                    professorSchedules[aula.professor] = Array(5).fill(null).map(() => []);
                }
                professorSchedules[aula.professor][aula.dia].push(aula.hora);
            });

            Object.entries(professorSchedules).forEach(([prof, days]) => {
                days.forEach(hours => {
                    if (hours.length > 1) {
                        hours.sort((a, b) => a - b);
                        for (let i = 1; i < hours.length; i++) {
                            totalGaps += hours[i] - hours[i-1] - 1;
                        }
                    }
                });
            });

            // Análise de distribuição por dia
            const classesPerDay = Array(5).fill(0);
            gradeData.aulas.forEach(aula => {
                classesPerDay[aula.dia]++;
            });

            const avgClassesPerDay = gradeData.aulas.length / 5;
            const variance = classesPerDay.reduce((sum, count) =>
                sum + Math.pow(count - avgClassesPerDay, 2), 0) / 5;

            // Adicionar métricas ao card
            const metrics = [
                { label: 'Total de aulas', value: gradeData.aulas.length },
                { label: 'Janelas de horário', value: totalGaps },
                { label: 'Distribuição (desvio)', value: Math.sqrt(variance).toFixed(2) }
            ];

            metrics.forEach(metric => {
                const item = document.createElement('div');
                item.className = 'stat-item';
                item.innerHTML = `
                    <span>${metric.label}</span>
                    <strong>${metric.value}</strong>
                `;
                card.appendChild(item);
            });

            document.getElementById('statsContainer').appendChild(card);
        }

        // Event listeners
        document.getElementById('viewSelect').addEventListener('change', updateView);
        document.getElementById('filterSelect').addEventListener('change', updateView);

        /*
        // Dados de exemplo para teste
        const exemploData = {
            metadata: {
                turmas: ["6º Ano", "7º Ano", "8º Ano", "9º Ano"],
                dias: ["Segunda", "Terça", "Quarta", "Quinta", "Sexta"],
                horarios: ["7:30-8:15", "8:15-9:00", "9:00-9:45", "10:05-10:50", "10:50-11:35", "11:35-12:20"]
            },
            aulas: [
                { turma: "6º Ano", disciplina: "Matemática", professor: "Wanderlei", sala: "Sala 1", dia: 0, hora: 0 },
                { turma: "6º Ano", disciplina: "Português", professor: "Selma", sala: "Sala 1", dia: 0, hora: 1 },
                { turma: "7º Ano", disciplina: "História", professor: "Adilson", sala: "Sala 2", dia: 2, hora: 0 }
            ],
            estatisticas: {
                aulasPorTurma: { "6º Ano": 30, "7º Ano": 30, "8º Ano": 30, "9º Ano": 30 },
                aulasPorProfessor: { "Wanderlei": 24, "Selma": 20, "Adilson": 12 }
            }
        };
        */

        // Carregar dados de exemplo ao iniciar
        // gradeData = exemploData;
        // processData();
        // updateView();