let dadosCarregados = null;
        let gradeGerada = null;
        let geradorModule = null;

        // Inicializar WebAssembly
        window.onload = async function() {
            mostrarStatus('Carregando módulo C++...', 'loading');
            try {
                geradorModule = await GeradorModule();
                mostrarStatus('Módulo C++ carregado com sucesso!', 'success');
            } catch (error) {
                mostrarStatus('Erro ao carregar módulo: ' + error.message, 'error');
            }
        };

        function mostrarStatus(mensagem, tipo) {
            const status = document.getElementById('status');
            status.textContent = mensagem;
            status.className = 'status ' + tipo;
        }

        function carregarDados() {
            const input = document.getElementById('fileInput');
            const file = input.files[0];

            if (!file) {
                mostrarStatus('Por favor, selecione um arquivo', 'error');
                return;
            }

            const reader = new FileReader();
            reader.onload = function(e) {
                try {
                    dadosCarregados = JSON.parse(e.target.result);

                    // Verificar se é arquivo de dados cadastrados
                    if (!dadosCarregados.turmas || !dadosCarregados.disciplinas) {
                        mostrarStatus('Arquivo inválido. Use o arquivo exportado do Sistema de Cadastro.', 'error');
                        return;
                    }

                    mostrarStatus('Dados carregados com sucesso!', 'success');
                    document.getElementById('btnGerar').disabled = false;

                    // Mostrar preview
                    mostrarPreview();

                } catch (error) {
                    mostrarStatus('Erro ao ler arquivo: ' + error.message, 'error');
                }
            };

            reader.readAsText(file);
        }

        function mostrarPreview() {
            const preview = document.getElementById('preview');
            preview.innerHTML = `
                <h3>📊 Dados Carregados:</h3>
                <ul>
                    <li>Turmas: ${dadosCarregados.turmas.length}</li>
                    <li>Disciplinas: ${dadosCarregados.disciplinas.length}</li>
                    <li>Professores: ${dadosCarregados.professores.length}</li>
                    <li>Salas: ${dadosCarregados.salas.length}</li>
                </ul>
            `;
        }

        async function gerarGrade() {
            if (!dadosCarregados || !geradorModule) {
                mostrarStatus('Dados não carregados ou módulo não disponível', 'error');
                return;
            }

            mostrarStatus('Processando... Isso pode levar alguns segundos.', 'loading');
            document.getElementById('progressBar').style.display = 'block';

            // Simular progresso
            let progress = 0;
            const progressInterval = setInterval(() => {
                progress += 10;
                document.querySelector('#progressBar .progress').style.width = progress + '%';
                if (progress >= 90) clearInterval(progressInterval);
            }, 200);

            try {
                // Chamar função C++ via WebAssembly
                const resultado = await new Promise((resolve) => {
                    setTimeout(() => {
                        const json = JSON.stringify(dadosCarregados);
                        const resultadoStr = geradorModule.processarGradeHoraria(json);
                        resolve(JSON.parse(resultadoStr));
                    }, 100);
                });

                clearInterval(progressInterval);
                document.querySelector('#progressBar .progress').style.width = '100%';

                if (resultado.erro) {
                    mostrarStatus('Erro: ' + resultado.erro, 'error');
                } else {
                    gradeGerada = resultado;
                    mostrarStatus('Grade gerada com sucesso!', 'success');
                    document.getElementById('btnVisualizar').disabled = false;
                    document.getElementById('btnBaixar').disabled = false;

                    // Mostrar estatísticas
                    preview.innerHTML += `
                        <h3>✅ Grade Gerada:</h3>
                        <ul>
                            <li>Total de aulas alocadas: ${resultado.aulas.length}</li>
                        </ul>
                    `;
                }

            } catch (error) {
                clearInterval(progressInterval);
                mostrarStatus('Erro ao processar: ' + error.message, 'error');
            }

            setTimeout(() => {
                document.getElementById('progressBar').style.display = 'none';
            }, 1000);
        }

        function visualizarGrade() {
            if (!gradeGerada) return;

            // Salvar grade no localStorage
            localStorage.setItem('gradeHoraria', JSON.stringify(gradeGerada));

            // Abrir visualizador em nova aba
            window.open('visualizador/Visualizador.html', '_blank');
        }

        function baixarGrade() {
            if (!gradeGerada) return;

            const dataStr = JSON.stringify(gradeGerada, null, 2);
            const dataBlob = new Blob([dataStr], {type: 'application/json'});
            const url = URL.createObjectURL(dataBlob);
            const link = document.createElement('a');
            link.href = url;
            link.download = 'grade_horaria_gerada.json';
            link.click();
            URL.revokeObjectURL(url);

            mostrarStatus('Grade baixada com sucesso!', 'success');
        }
        
        if (typeof GeradorModule === 'undefined') {
        console.error('Módulo não carregado. Tentando caminho alternativo...');
        var script = document.createElement('script');
        script.src = './gerador.js';
        document.head.appendChild(script);
    }