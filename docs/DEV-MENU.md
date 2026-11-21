[English](#english) | [Português](#portugues)

<a id="english"></a>

> Historical manual from the local installation, exported on 20 November 2026. Referenced builds, captures and `validacao/` folders are not distributed. For this checkout, follow [SETUP](SETUP.md), [VALIDATION](VALIDATION.md) and [SOURCES](SOURCES.md).

# SotN Saturn Dev Menu

## Code audit without opening the game — 26 October 2026

Build2870742E adds Vulkan preference in renderer Auto mode when started by the GPU wrapper. Explicit DirectX12, Direct3D11 or Vulkan selections still take priority; compute and renderer are separate devices. `-Backend direct3d12` remains available in the wrapper. Do not change personal preferences based on a benchmark.

FSR retains priority and enabling it in the menu preserves existing HD exclusion. The HD failure test was fixed so that FSR does not erase a persisted HD preference when both come from configuration. No shortcut/control/hack changed. The Dev Menu was not opened in this round; evidence is code/offscreen only. Report: `../validacao/auditoria-codigo-20261026/RESULTADO.md`.

## Turbo and smoothness — 25 October 2026

GPU build C618676F: acceleration2×/4×/6×/8×/10× changes the simulation speed target. If the CPU cannot reach it, the requested number is not the actual multiplier. Presentation now follows real time in turbo; at1× every field is drawn. Audio remains muted in turbo as before. More drawing may reduce achieved speed; choose1× to evaluate normal-speed drops. F8 and logs report requested/actual speed, stage/room/VB. Original D90 and previous1D3549BD preserved.

## Extended GPU and diagnostics — 25 October 2026

The experimental GPU shortcut uses1D3549BD; the original remains D90EB056. F8 records vdp2_cpu_fallback_reason and vsync_status in addition to backend/counters/timings. Per-line scroll/zoom and mosaic no longer force CPU fallback; rotation, vertical cell scroll and other sequential modes still use the reference compositor. Stay in the slow room for20 seconds at1×, then F8. HD8, controls, audio and room saves remain unchanged.

## Experimental GPU variant — 25 October 2026

The **SotN HD - GPU experimental** shortcut uses build2D4087D1. The panel, shortcuts and hacks are the same; the original shortcut remains D90EB056. F8 records vdp2_backend, GPU frames and CPU fallbacks. Stay around20 seconds in the slow room at1× before capture. Profiling includes master, synchronized GPU and per-VBlank percentiles; do not add inclusive costs. Profile percentiles are bounded approximations; the offline benchmark retains its own measurements. Original image/capture and filters remain available because the compositor returns a complete Frame. No save changes.

Current shortcut build: `D90EB056`, with per-stage profiling enabled. F8 includes full graphics components in `graphics.bin`, without CPU/WRAM/save; this is not a savestate. Game saving remains in rooms and savestates are out of scope at the user's explicit request.

Open through **SotN HD**. **F1** or **L3 + R3** opens/closes the panel. R3 is the right stick click. **Esc** goes back in open screens; it does not open the panel. Release L3/R3 before repeating. Start alone keeps the original menu.

**The world pauses while the Dev Menu is open.** This is not the Curse of Darkness menu behavior. D-pad/arrows navigate, A/cross or Enter confirms, B/circle goes back. **L1/R1** or **Shift+Tab/Tab** switches tabs; clicking tabs works too. Writing a report uses the keyboard.

| Tab | Available features |
|---|---|
| Dev | Speed1/2/4/6/8/10x, return1x, frame advance, profiler, capture and lightweight graphics profile. |
| Report | F8 captures original/rendered images and opens title/description; Save local report writes `relato.json` beside the package. |
| Graphics | Integrated page: resolution/fullscreen, aspect ratios, integer scaling, backend, HD2/4/6/8x, FSR1, CRT/LCD, sampling, FPS and Save options. |
| Audio | Volume, mute and existing session options. Game output44.1kHz stereo. |
| Controls | Auto/Xbox/PlayStation, dead zone and mapping reset. |
| Cheats |15 presets and four GameShark scripts, pages of12, individual help, healing, disable all and reload. |
| Diagnostics | Stage/room, character/HP, VBlank, time, PC/PR, modules and renderer. Read-only. |
| States | Technical savestate limit, with save/load unavailable until SH-2 resumption is validated. |

## Map on Select

Select/Back no longer opens the Dev Menu. SaturnJP opens the map through the Start menu followed by L. The shortcut sends these buttons and waits for the original menu to be ready; it does not modify memory or unlock the map. The map must be available. Alucard's flow was confirmed in source; controller opening in gameplay still needs a manual test. Outside gameplay or with a locked map, Select does not force opening.

## Boot and saved preferences

The SotN HD shortcut and both launchers boot normally from disc, without automatic inputs, boot skipping or prologue selection. Choose character/save through the game itself. `-Prologue` is retained only as an explicit test.

This update set local configuration to1920×1080 fullscreen. Subsequently your saved choices take priority: the launcher does not force FullHD/fullscreen on every launch. **Salvar opções** (Save options) writes `sotn-experiment/tools/sotn-windows.ini`; changes also persist when controls are adjusted. F11/Alt+Enter now use the same fullscreen saving path. Changing resolution preserves current window/fullscreen mode; the monitor may offer a close mode if the resolution is unavailable. Backend requires a restart. The game save is not moved.

## Report a problem

1. Press **F8** in the defective scene. Capture occurs before the interface, and the Report tab opens paused.
2. Choose **Editar título** (Edit title) or **Editar descrição** (Edit description). Type, use Backspace to delete and Enter to finish; Tab switches the edited field.
3. Choose **Salvar relato local** (Save local report). `relato.json` stays in the same folder as `report.json`, images and logs.

Folder: `G:\Recomps e Mods\Sega Saturn\logs-e-bugs\partidas\<sessão>\session-NNN\bugs\bug-...\`. Do not create another `reports/` root: old and new reports stay together. Nothing is sent automatically. Capture failure is explicit in the report. Limits: title160 UTF-8 bytes, description4096bytes; log copies are bounded, full records stay in the session.

## Speed and diagnostics

The requested factor is a ceiling, not a performance promise. Turbo stops composing/presenting every intermediate frame; simulation, interrupts and input reading continue. The footer and `run.log` show the **actual factor**, separately from the request. Audio is muted above1x; do not change its pitch as in COD. **F5** returns to1x, **F6** advances one frame and pauses, **F7** toggles profiling; **F9** toggles mute. Measurement excludes time paused in the menu.

**Lightweight profile** disables HD/FSR/shaders/effects and selects nearest neighbor; preserves resolution and aspect ratio. It is a comparison tool, not a proven fix for slow rooms. **Save options** retains preferences; hacks always start disabled and speed at1x in the normal launcher.

## Cheats

Godmode, mana, hearts, gold; infinite life without invincibility; HP/MP999 and hearts99 maximums; base STR/CON/INT/LCK99. New stats/maximums are guarded for Alucard. Presets do not reduce values already higher. Totals depend on normal game recalculation. Disabling a hack does not undo values the game already saved. GameShark codes must target SaturnJP; Xbox/PS1/PS2 codes are not interchangeable.

## Savestates: a real pending item

The runtime has a machine serializer but records only at safe task-switch points. The current SotN build has `g_sh2_tasks={0,0}` and `g_sh2_ntask_switches=0`; the recorder is not enabled. Live restoration also requires reconstructing SH-2 execution, not just copying RAM. Therefore the States tab reports unavailability and does not write fake files. Normal in-game room saving remains separate.

Historical technical note for a possible future request, with no action in the current roadmap: resumption needs real boundaries for both processors and evidence of save/restore/continue. Do not use the personal save for tests. Coverage checkpoints are not savestates.

## Curse of Darkness ideas not yet ported

Complete per-action remapping, audio balance, mouse camera, BMP packs/file replacement/texture export,50–200% internal resolution,16x anisotropy and VSync/FPS limit controls were not imported. SotN is2D and this engine composes VDP1/VDP2 on the CPU; sliders with those names would not implement those features. Existing HD/FSR/aspect options remain. There is no empty Mods tab or HD texture claim without an implemented, tested loader.

## Original menu and map

Start opens Alucard's original inventory. Select/Back triggers Start→L for the map; F1/L3+R3 remain reserved for the development panel. Build357B fixes compressed text and colored strips on these screens in the compositor. Comparison test and limits in `validacao/map-menu-fix/`.

Current minimap: upper-right corner, gold border and transparent background, enabled when starting gameplay. Select/Back or Tab hides/shows it; gameplay continues. Squares reduced by10%, with a local region following the player.

## Compact minimap and browsable map

The minimap starts enabled in the screen's upper-right corner, with minimal margin, a smaller gold frame and transparent background. Select/Back (Share on PS4) alone shows/hides it; Tab does the same on keyboard. Gameplay continues in real time.

**Select + PS4 X** (Back + Xbox A) opens the enlarged map. D-pad or left stick moves the view; on keyboard, Shift+Tab opens and arrows navigate. Select, circle/B or Esc returns to the minimap. The combination reserves up to180ms to distinguish Select alone; jumping is blocked while Select is held. The world stays active in the enlarged map, but navigation commands do not move the character.

Discovered save rooms receive red fill; discovered teleporters green. Positions come from fills in the original Saturn JP disc's graphical blueprint, mirrored in the inverted castle. The initial rule based on0x50 rooms was discarded: they were transitions, not saves. Undiscovered rooms receive no markers. The local generator and source/hash manifest are in `validacao/minimap-navigation/`; the derived header is private and excluded from publication.

## Relics, transformations and mobility

**F1 or L3+R3 → Cheats → Próxima página** (Next page). The three new options are Alucard-only and start disabled. Enabled during menus/title/cutscenes, they wait for valid gameplay.

- **All relics:** unlocks31 normal relics, including Saturn running boots and all familiars. Select a familiar in the original menu; current selection is preserved.
- **Complete transformations:** bat, wolf, mist and their powers.
- **Complete mobility:** double jump, super jump and Saturn running boots.

Each option applies once when enabled. Close the panel so gameplay can execute the grant. You can then adjust relics in the original menu. To reapply, disable and enable the hack again. Disabling does not remove unlocked items; saving in a room can retain the relics. The game's hidden entry31 is not forced.

## Extended profile — 25 October 2026

Performance measurement also records periodic processing (devices), audio (sound), reader (cd), auxiliary processor (slave) and intentional waiting (pace). These are inclusive costs: do not add the averages. Panel pauses are excluded by restarting the window. The shortcut already enables profiling; stay in the slow room around20 seconds at1× and press F8 to attach measurements. Profiling locates costs; it does not change game speed.

---

<a id="portugues"></a>

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



