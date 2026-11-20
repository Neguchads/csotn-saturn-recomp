> Manual histórico da instalação local, exportado em 20/11/2026. Builds, capturas e pastas `validacao/` citadas não são distribuídas. Para este checkout, siga [SETUP](SETUP.md), [VALIDATION](VALIDATION.md) e [SOURCES](SOURCES.md).

# Dev Menu do SotN Saturn

## Auditoria de código sem abrir o jogo — 26/10/2026

A build2870742E acrescenta preferência Vulkan no modo Auto do renderer quando iniciada pelo wrapper GPU. Seleções explícitas DirectX12, Direct3D11 ou Vulkan continuam prevalecendo; compute e renderer são devices separados. `-Backend direct3d12` continua disponível no wrapper. Não alterar preferências pessoais por benchmark.

FSR continua tendo prioridade e sua ativação no menu mantém a exclusão HD existente. Corrigido o teste de falha HD para que FSR não apague uma preferência HD persistida quando ambos vierem da configuração. Não mudou nenhum atalho/controle/hack. Nesta rodada o Dev Menu não foi aberto; as provas são de código/offscreen. Relatório: `../validacao/auditoria-codigo-20261026/RESULTADO.md`.

## Turbo e fluidez — 25/10/2026

Build GPU C618676F: acelerar 2×/4×/6×/8×/10× altera o alvo da velocidade da simulação. Se a CPU não alcançar esse alvo, o número pedido não equivale ao multiplicador real. A apresentação agora acompanha o relógio real em turbo; em 1× todos os campos são desenhados. O áudio permanece silenciado no turbo como antes. Mais desenhos podem reduzir a velocidade real alcançada; escolha 1× para avaliar quedas da velocidade normal. F8 e o log informam velocidade solicitada/real, fase/sala/VB. Original D90 e anterior 1D3549BD preservados.



## GPU ampliada e diagnóstico — 25/10/2026



O atalho GPU experimental usa 1D3549BD; original permanece D90EB056. F8 registra vdp2_cpu_fallback_reason e vsync_status, além do backend/contadores/tempos. Scroll/zoom por linha e mosaico não forçam mais retorno CPU; rotação, vertical cell scroll e outros modos sequenciais continuam com o compositor de referência. Deixe sala lenta 20 segundos em 1× e F8. HD8, controles, áudio e salvar nas salas permanecem iguais.






## Variante GPU experimental — 25/10/2026





O atalho **SotN HD - GPU experimental** usa a build 2D4087D1. O painel, seus atalhos e hacks são os mesmos; o atalho original continua D90EB056. F8 registra vdp2_backend, quadros GPU e retornos CPU. Fique cerca de 20 segundos na sala lenta em 1× antes de capturar. O perfil inclui mestre, GPU sincronizada e percentis por VBlank; não somar custos inclusivos. Percentis do perfil são aproximações limitadas; o benchmark offline guarda medidas próprias. A imagem original/captura e filtros continuam disponíveis, pois o compositor retorna Frame completo. Sem mudança de saves.










Build atual do atalho: `D90EB056`, com perfil por etapa ligado. F8 inclui os componentes gráficos completos em `graphics.bin`, sem CPU/WRAM/save; isso não é savestate. O salvamento do jogo permanece nas salas e savestate fica fora do escopo por pedido explícito do usuário.









Abra pelo atalho **SotN HD**. **F1** ou **L3 + R3** abre e fecha o painel. R3 é o clique no analógico direito. **Esc** volta nas telas abertas; não abre o painel. Solte L3/R3 antes de repetir. Start sozinho mantém o menu original.









**O mundo pausa enquanto o Dev Menu está aberto.** Não é o comportamento do menu do Curse of Darkness. Direcional/setas navegam, A/cruz ou Enter confirma, B/círculo volta. **L1/R1** ou **Shift+Tab/Tab** troca abas; clique nas abas também funciona. Digitar um relato usa o teclado.









| Aba | Recursos disponíveis |




|---|---|




| Dev | Velocidade1/2/4/6/8/10x, voltar1x, avanço de quadro, profiler, captura e perfil gráfico leve. |




| Relatar | F8 captura original/renderizado e abre título/descrição; Salvar relato local grava `relato.json` junto ao pacote. |




| Gráficos | Página integrada: resolução/tela cheia, proporções, escala inteira, backend, HD2/4/6/8x, FSR1, CRT/LCD, amostragem, FPS e Salvar opções. |




| Áudio | Volume, mudo e opções de sessão existentes. Saída do jogo44,1kHz estéreo. |




| Controles | Auto/Xbox/PlayStation, zona morta e restauração do mapeamento. |




| Cheats |15 presets e quatro scripts GameShark, páginas de 12, ajuda individual, cura, desligar todos e recarregar. |




| Diagnóstico | Fase/sala, personagem/HP, VBlank, tempo, PC/PR, módulos e renderizador. Somente leitura. |




| Estados | Limite técnico do savestate, com salvar/carregar indisponíveis até validar retomadaSH-2. |









## Mapa no Select









Select/Back deixou de abrir o Dev Menu. O SaturnJP abre o mapa pelo menuStart e depoisL. O atalho envia esses botões e espera o menu original ficar pronto; não altera memória nem desbloqueia mapa. Precisa do mapa disponível. O fluxoAlucard foi confirmado nas fontes; a abertura por controle na partida ainda precisa de teste manual. Fora de gameplay ou com mapa bloqueado, Select não força a abertura.









## Boot e preferências salvas









O atalhoSotN HD e os dois launchers iniciam normalmente pelo disco, sem entradas automáticas, pulo do boot ou seleção do prólogo. Você escolhe personagem/save pelo próprio jogo. O parâmetro `-Prologue` fica somente como teste explícito.









Nesta atualização a configuração local foi definida como1920×1080 e tela cheia. Depois, suas escolhas salvas prevalecem: o launcher não impõe FullHD/tela cheia a cada abertura. **Salvar opções** grava `sotn-experiment/tools/sotn-windows.ini`; as alterações também são persistidas ao ajustar os controles. F11/Alt+Enter agora passam pelo mesmo salvamento da tela cheia. Alterar a resolução mantém o modo atual de janela/tela cheia; o monitor pode oferecer um modo aproximado caso a resolução não esteja disponível. Backend exige reiniciar. O save do jogo não é movido.









## Relatar um problema









1. Pressione **F8** na cena com defeito. Captura ocorre antes da interface, e a aba Relatar abre pausada.




2. Escolha **Editar título** ou **Editar descrição**. Digite, useBackspace para apagar eEnter para concluir;Tab troca o campo em edição.




3. Escolha **Salvar relato local**. `relato.json` fica na mesma pasta de `report.json`, imagens e logs.









Pasta: `G:\Recomps e Mods\Sega Saturn\logs-e-bugs\partidas\<sessão>\session-NNN\bugs\bug-...\`. Não criar outra raiz `reports/`: os relatórios antigos e novos continuam juntos. Nada enviado automaticamente. Falha de captura fica explícita no relatório. Limites: título160bytesUTF-8, descrição4096bytes; cópias dos logs limitadas, registros completos continuam na sessão.









## Velocidade e diagnóstico









O fator solicitado é um teto, não promessa de desempenho. O turbo deixa de compor/apresentar todos os quadros intermediários; simulação, interrupções e leitura de entrada continuam. O rodapé e `run.log` mostram o **fator real**, separado do pedido. Áudio fica silenciado acima de1x; não alterar tom do áudio como noCOD. **F5** volta1x, **F6** avança um quadro e pausa, **F7** liga/desliga o profiler; **F9** alterna mudo. A medição exclui o tempo parado no menu.









**Perfil leve** desligaHD/FSR/shaders/efeitos e seleciona vizinho próximo; preserva resolução e proporção. É ferramenta de comparação, não correção comprovada das salas lentas. **Salvar opções** guarda preferências; hacks sempre começam desligados e velocidade1x no launcher normal.









## Cheats









Godmode, mana, corações, ouro; vida infinita sem invencibilidade; máximosHP/MP999 e corações99; atributos baseSTR/CON/INT/LCK99. Os atributos/máximos novos são guardados paraAlucard. Presets não reduzem valores já maiores. Os totais dependem do recálculo normal do jogo. Desligar um hack não desfaz valores que o jogo já salvou. CódigosGameShark precisam ser da ediçãoSaturnJP; códigosXbox/PS1/PS2 não são intercambiáveis.









## Savestates: pendência real









O runtime tem serializador de máquina, mas só grava em pontos seguros de troca de tarefas. O buildSotN atual tem `g_sh2_tasks={0,0}` e `g_sh2_ntask_switches=0`; o gravador não é habilitado. Restaurar ao vivo também exige reconstruir a execuçãoSH-2, não basta copiarRAM. Por isso a abaEstados informa indisponibilidade e não grava arquivos falsos. O save normal nas salas do jogo continua separado.









Nota técnica histórica para eventual pedido futuro, sem ação no roadmap atual: uma retomada exige fronteiras reais dos dois processadores e prova de salvar/restaurar/continuar. Não usar o save pessoal nos testes. Checkpoint de cobertura não é savestate.









## Ideias do Curse of Darkness ainda não portadas









Remapeamento completo por ação, balanço de áudio, câmera pelo mouse, packsBMP/substituição de arquivos/exportação de texturas, resolução interna50–200%, anisotropia16x e controlesVSync/limiteFPS não foram importados. O SotN é2D e este motor compõeVDP1/VDP2 emCPU; sliders com esses nomes não implementariam tais recursos. As opções existentes deHD/FSR/proporção continuam. Não há abaMods vazia ou afirmação de texturaHD sem um carregador implementado e testado.









## Menu original e mapa









Start abre o inventário original de Alucard. Select/Back aciona Start→L para o mapa; F1/L3+R3 continuam reservados ao painel de desenvolvimento. A build357B corrige os textos comprimidos e as faixas coloridas dessas telas no compositor. Teste comparativo e limites em `validacao/map-menu-fix/`.









Minimapa atual: canto superior direito, borda dourada e fundo transparente, habilitado ao iniciar a partida. Select/Back ou Tab oculta e mostra; o jogo continua. Tamanho dos quadrados reduzido em 10%, com região local acompanhando o jogador.









## Minimapa compacto e mapa navegável









O minimapa inicia habilitado no canto superior direito da tela, com margem mínima, moldura dourada menor e fundo transparente. Select/Back (Share no PS4) sozinho mostra ou oculta; Tab faz o mesmo no teclado. O jogo continua em tempo real.









**Select + X do PS4** (Back + A no Xbox) abre o mapa ampliado. Direcional ou analógico esquerdo desloca a visão; no teclado, Shift+Tab abre e as setas navegam. Select, círculo/B ou Esc volta ao minimapa. A combinação reserva até180ms para distinguir Select sozinho; o pulo é bloqueado enquanto Select está pressionado. O mundo continua ativo no mapa ampliado, mas os comandos de navegação não movem o personagem.









Saves descobertos recebem preenchimento vermelho; teleportes descobertos, verde. As posições vêm dos preenchimentos do blueprint gráfico original da mídia Saturn JP, espelhados no castelo invertido. A regra inicial baseada em salas0x50 foi descartada: eram transições, não saves. Salas não descobertas não ganham marcadores. O gerador local e o manifesto de fontes/hashes ficam em `validacao/minimap-navigation/`; o cabeçalho derivado é privado e não entra em publicação.














## Relíquias, transformações e mobilidade









**F1 ou L3+R3 → Cheats → Próxima página**. As três opções novas são exclusivas de Alucard e começam desligadas. Ligadas durante menu/título/cena, aguardam gameplay válido.









- **Todas as relíquias:** libera 31 relíquias normais, incluindo botas de corrida Saturn e todos os familiares. Selecione um familiar no menu original; a seleção atual é preservada.




- **Transformações completas:** morcego, lobo, névoa e seus poderes.




- **Mobilidade completa:** salto duplo, super salto e botas de corrida Saturn.









Cada opção aplica uma vez ao ligar. Feche o painel para o gameplay executar a concessão. Depois você pode ajustar as relíquias pelo menu original. Para reaplicar, desligue e ligue novamente o hack. Desligar não retira o que já foi liberado; salvar na sala pode guardar as relíquias. A entrada oculta 31 do jogo não é forçada.








## Perfil ampliado — 25/10/2026







Medir desempenho registra também processamento periódico (devices), áudio (sound), leitor (cd), processador auxiliar (slave) e espera intencional (pace). São custos inclusivos: não somar as médias. Pausas no painel são excluídas ao reiniciar a janela. O atalho já liga o perfil; deixe a sala lenta cerca de 20 segundos em 1× e pressione F8 para anexar as medidas. O perfil localiza o custo; não altera a velocidade do jogo.



