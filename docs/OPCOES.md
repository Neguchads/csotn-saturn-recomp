[English](#english) | [Português](#portugues)

<a id="english"></a>

> Historical manual from the local installation, exported on 20 November 2026. Referenced builds, captures and `validacao/` folders are not distributed. For this checkout, follow [SETUP](SETUP.md), [VALIDATION](VALIDATION.md) and [SOURCES](SOURCES.md).

# Game options

The current panel is the **Dev Menu**: [manual and limits](DEV-MENU.md). **F1 / L3+R3** opens/closes it, L1/R1 switches tabs. Start alone keeps the Saturn menu. F8 captures and opens the text report. The world pauses in the panel. Original room saves; savestates out of scope at the user's request.

The `Jogar-SotN-Windows.cmd` and `Jogar-SotN-Amplo.cmd` launchers keep `windows-reference-all` as default. The desktop shortcut explicitly uses the current `windows-reference-engine-20261025` build, hash `1CEDB170`, with `-VideoProfile`. Earlier executables remain preserved.

Arrow keys navigate and adjust the panel; Enter confirms. The mouse also works. Esc goes back or closes open screens; F1/L3+R3 opens the Dev Menu. Start opens the original Saturn menu.

The current menu separates Screen and resolution, Quality and shaders, Controls, Audio and system, and Hacks/GameShark. The panel uses a dark background, burgundy highlight and gold focus. The bottom line explains the selected option. It does not use a web interface or a new library.

## Capture a bug

Press **F8**, **F12** or **PrintScreen** when you notice a defect. The panel's capture button uses the same mechanism. The Report tab opens with a preview; each activation creates a separate folder.

Open `G:\Recomps e Mods\Sega Saturn\logs-e-bugs\partidas\` and enter the latest session. Packages in `bugs/bug-.../`: `original.png`, `rendered.png`, `report.json`, `settings.ini`, available session metadata and excerpts of logs/commands. The current build also writes `graphics.bin` and `graphics-capture.txt` with complete VDP1/VDP2 components for diagnosis. They contain no CPU, WRAM or save and cannot resume a game. Give Codex the path; optionally add `descricao.txt` with the defect and expected result.

The rendered image includes internal filters/shaders, scale and aspect ratio, before Esc. The original allows comparison of Saturn composition and presentation. No automatic capture, upload or save/ROM/BIOS copy. External ReShade effects may not appear in this readback.

If the game closes before the screenshot, preserve the session. The launcher retains logs and exit code; `erros.log` collects possible shutdown failures. A fallback can appear without being a crash. Normal configuration and personal save remain in their previous locations.

## Video / effects

| Option | Behavior |
|---|---|
| Resolution | Window at640×480,960×720,1280×720,1280×960,1920×1080,1920×1440,2560×1440 or3840×2160; image fitted to the selected aspect ratio. |
| Window scale | 1x to8x of a320×240 base window. Selecting scale disables the resolution preset. |
| Fullscreen | Uses the current desktop resolution. Does not change monitor mode. |
| Integer scale | Multiplies original frame pixels by integers, with borders. If the window is smaller than the frame, scales down proportionally. Can change the aspect ratio relative to4:3 fitting. |
| Aspect ratio | TV4:3, original pixels, fill,16:9,16:10,21:9,3:2,5:4 or1:1. Explicit ratios/fill take priority over integer scaling. Can stretch the image; do not reveal additional scenery. |
| HD filter | Off or2x/4x/6x/8x. Scale2x/Scale3x edge reconstruction in GPU passes. More than resizing the window. |
| Pixel / Smooth filter | Nearest neighbor or linear interpolation when enlarging the image. |
| Visual effect | Original, scanlines or LCD mask, calculated per pixel on the CPU. |
| GPU shader | Off, CRT with scanlines/vignette or LCD mask, executed on the GPU. |
| Show FPS | Presented frames per second and selected factor; does not measure unique game animation frames. F10 toggles. |
| Restore default video | Returns to a960×720 window, TV4:3, smooth filter, no effects/shaders, without changing saves or scripts. |
| Backend* | Auto GPU, GPU D3D12, Direct3D11 or Vulkan. Selection is saved and requires restarting the game; does not automatically end your session. Custom HD/shaders/FSR require GPU D3D12 or Vulkan. |
| AMD FSR1 | Two official AMD passes: EASU spatial upscaling and RCAS sharpening. Targets the displayed frame size; enabling it disables edge HD, and vice versa. Not temporal FSR2/3 or frame generation. |

Original graphics code and sprite resolution do not increase. These are Saturn frame presentation options. This delivery has no HD texture pack, RetroArch/ReShade shader loading or external graphics scripts.

The HD filter changes edges by comparing neighbors while preserving the palette; it does not invent new textures.4x combines two2x passes;6x combines2x and3x;8x uses three2x passes. GPU target dimensions do increase, but this reconstructs a completed frame. Do not confuse it with rendering geometry again at a higher internal resolution, as in the PSP hardware backend. Our VDP1/VDP2 still rasterizes/composes on the processor at Saturn resolution. Even PPSSPP distinguishes [internal resolution and texture scaling](https://www.ppsspp.org/docs/settings/graphics/); pixel-aligned sprites do not gain details from internal resolution alone.

The algorithm is Scale2x/Scale3x, not HQx or xBRZ. [Original rules](https://www.scale2x.it/algorithm). Factors were GPU-tested against every pixel of a CPU reference, including edges, frame updates, target restoration, memory limits and fallback. Visual preference must be checked in-game. Allocation/driver failures disable HD and preserve the original image.

The desktop launcher tries the SDL_GPU/D3D12 renderer; if unavailable, it uses the default renderer and reports the limitation on the page. Custom shaders are embedded DXIL, compatible with the SDL3 3.4.14 binary signature. The menu is drawn after removing the shader. A presentation failure disables the shader and logs the reason. CPU filters remain independent.

Vulkan uses the same shaders in SPIR-V with a combined sampler and an input signature confirmed by testing. Official portable DXC1.9.2609 was required because Windows SDK DXC lists the SPIR-V option in help but lacks that generator. The compiler stays local to the project, does not enter the game or change global PATH. The runtime still uses SDL3 and the installed graphics driver.

## ReShade, DLSS, DLAA and frame generation

ReShade is external, BSD3-clause, and [states D3D11/D3D12/Vulkan support](https://reshade.me/). Official package6.8.0 was extracted into test directories without running its installer or registering a global layer. An SDL probe confirmed loading/interception in D3D12 through a local DLL and Vulkan through `VK_LAYER_PATH`/`VK_INSTANCE_LAYERS` only in the child process. Presentations exited0. This **does not certify effects**: logs did not confirm test-effect compilation or the in-game ReShade overlay. The normal launcher received no DLLs/layer. Evidence: `validacao/hd-performance/reshade/result.json` and per-backend `ReShade.log`. ReShadeFX packages are not the internal SPIR-V/DXIL shaders.

DLSS and DLAA are not implemented. [Official NVIDIA integration](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS.md) requires color, depth, motion vectors and temporal data. The current compositor only provides a completed RGB frame; filling the other inputs with fictional values would not be valid integration. On the RTX3060 used in testing, DLSS Super Resolution is possible in a compatible engine, but [DLSS Frame Generation officially supports RTX40/50](https://forums.developer.nvidia.com/t/dlss-4-faq/321939). FSR3/framegen also requires a different integration and latency/artifact evidence. The menu has no fake options for these features.

## Measure a performance drop

The **SotN HD** shortcut already records profiling. In the slow room, keep1×, wait around20 seconds and press F8. F7 toggles measurement. For a disposable command-line run:

```powershell
.\Jogar-SotN-Windows.cmd -Executable ".\sotn-experiment\build\jp\windows-reference-engine-20261025\game\saturn.exe" -DisposableSave -VideoProfile
```

The launcher records actual commands in `input.txt` and timing summaries in `run.log` every300 presentations. `VIDEO_PROFILE` separates VDP1 rasterization, VDP2 composition, upload, drawing and presentation. Drawing includes presentation: do not add the two averages. Bracketed log values are virtual time, not elapsed real time. Preserve the session folder to reproduce the exact room. This mode does not touch the personal save.

## Controls

SDL detects compatible USB/Bluetooth controllers, including Xbox and PlayStation families. The page shows the first connected controller, enables/disables gamepad, selects Auto/Xbox/PlayStation names, adjusts the analog dead zone and restores default mapping. Choosing names does not turn physical hardware into another model.

| Default action | Xbox | PlayStation | Keyboard |
|---|---|---|---|
| Attack / Saturn A | X | Square | Z |
| Jump / Saturn B | A | Cross | Space or X |
| Cancel / Saturn C | B | Circle | C |
| Game Start | Menu | Options/Start | Enter |
| Dev Menu | Start + right stick click | Options + R3 | F1 or Esc |
| Map | View/Back | Share/Select | Start then L in original menu |

The driver must expose the device to SDL; connection of every physical model was not validated in this slice. Names and options were checked in the interface. Custom mappings remain available in the INI, including per-GUID mappings. Restoring defaults also resets sections for currently connected models.

## Speed and persistence

Speeds:1x,2x,4x,6x,8x and10x. The factor reduces PC waiting; the game's virtual clock does not change. Actual limits depend on the computer. Acceleration mutes audio;1x restores the selected volume. Every process starts at1x.

Video, volume and controls are stored in `sotn-experiment/tools/sotn-windows.ini`. The launcher no longer forces `--scale 3`, which would prevent restoring the selected resolution. Selecting resolution or scale exits fullscreen to show the selected size.

Mute (F9) preserves the selected volume. Pause on focus loss opens Esc when the window loses focus, for example with Alt+Tab; returning keeps it paused until Continue is selected. Capture image/F12 writes the original Saturn frame to `shot-VBLANK.png` in session output, without menu or shader. These options reuse behaviors studied in Ymir; comparison and limits (local reference not distributed).

Hacks/GameShark: [separate manual](MODMENU.md). Every hack starts disabled. To test without changing the personal save:

```powershell
.\Jogar-SotN-Windows.cmd -DisposableSave
```

Current evidence and limits: [VALIDATION](VALIDATION.md) and [HANDOFF](../HANDOFF.md). Alucard's original menu has its own investigation; Esc options do not replace fixing its SH-2 calls.

---

<a id="portugues"></a>

> Manual histórico da instalação local, exportado em 20/11/2026. Builds, capturas e pastas `validacao/` citadas não são distribuídas. Para este checkout, siga [SETUP](SETUP.md), [VALIDATION](VALIDATION.md) e [SOURCES](SOURCES.md).

# Opções do jogo

O painel atual é o **Dev Menu**: [manual e limites](DEV-MENU.md). **F1 / L3+R3** abre/fecha, L1/R1 troca abas. Start sozinho mantém o menu Saturn. F8 captura e abre o relato com texto. O mundo pausa no painel. Salvamento original nas salas; savestate fora do escopo por pedido do usuário.

Os launchers `Jogar-SotN-Windows.cmd` e `Jogar-SotN-Amplo.cmd` mantêm `windows-reference-all` como padrão. O atalho da Área de Trabalho usa explicitamente a build atual `windows-reference-engine-20261025`, hash `1CEDB170`, com `-VideoProfile`. Os executáveis anteriores continuam preservados.

Setas navegam e ajustam o painel; Enter confirma. O mouse também funciona. Esc volta ou fecha as telas abertas; F1/L3+R3 abre o Dev Menu. Start abre o menu original do Saturn.

O menu atual separa Tela e resolução, Qualidade e shaders, Controles, Áudio e sistema e Hacks/GameShark. O painel usa fundo escuro, destaque vinho e foco dourado. A linha inferior explica a opção selecionada. Não usa uma interface web nem biblioteca nova.

## Capturar um bug

Pressione **F8**, **F12** ou **PrintScreen** quando notar um defeito. O botão de captura do painel usa o mesmo mecanismo. A aba Relatar abre com a prévia; cada acionamento cria uma pasta diferente.

Abra `G:\Recomps e Mods\Sega Saturn\logs-e-bugs\partidas\` e entre na sessão mais recente. Pacotes em `bugs/bug-.../`: `original.png`, `rendered.png`, `report.json`, `settings.ini`, metadados disponíveis da sessão e trechos dos logs/comandos. A build atual também grava `graphics.bin` e `graphics-capture.txt` com os componentes VDP1/VDP2 completos para diagnóstico. Não contêm CPU, WRAM ou save e não retomam uma partida. Informe o caminho ao Codex; opcionalmente acrescente `descricao.txt` com o defeito e o resultado esperado.

A renderizada inclui filtros/shaders internos, escala e proporção, antes doEsc. A original permite comparar composiçãoSaturn e apresentação. Nenhuma captura automática, upload ou cópia de save/ROM/BIOS. Efeitos externos doReShade podem não aparecer nesta leitura.

Se o jogo fechar antes do print, preserve a sessão. O launcher guarda registros e código de saída; `erros.log` reúne possíveis falhas ao encerrar. Um fallback pode aparecer sem representar crash. Configuração normal e save pessoal continuam nos locais anteriores.

## Vídeo / efeitos

| Opção | Comportamento |
|---|---|
| Resolução | Janela de 640×480, 960×720, 1280×720, 1280×960, 1920×1080, 1920×1440, 2560×1440 ou 3840×2160; imagem ajustada à proporção escolhida. |
| Escala janela | 1x a 8x de uma janela-base 320×240. Selecionar escala desativa o preset de resolução. |
| Tela cheia | Usa a resolução atual da área de trabalho. Não altera o modo do monitor. |
| Escala inteira | Multiplica os pixels do quadro original por números inteiros, com bordas. Se a janela for menor que o quadro, reduz proporcionalmente. Pode mudar a proporção em relação ao ajuste 4:3. |
| Proporção | TV 4:3, pixels originais, preencher, 16:9, 16:10, 21:9, 3:2, 5:4 ou 1:1. As proporções explícitas/preencher têm prioridade sobre escala inteira. Podem esticar a imagem; não revelam mais cenário. |
| Filtro HD | Desligado ou 2x/4x/6x/8x. Reconstrução de contornos por Scale2x/Scale3x em passes GPU. Não é apenas ampliar a janela. |
| Filtro Pixel / Suave | Vizinho mais próximo ou interpolação linear ao ampliar a imagem. |
| Efeito visual | Original, scanlines ou máscara LCD, calculados por pixel na CPU. |
| Shader GPU | Desligado, CRT com scanlines/vinheta ou máscara LCD, executados na GPU. |
| Mostrar FPS | Indicador de quadros apresentados por segundo e fator selecionado; não mede quadros únicos da animação do jogo. F10 alterna. |
| Restaurar vídeo padrão | Volta à janela 960×720, TV 4:3, filtro suave, sem efeitos/shaders e sem alterar saves ou scripts. |
| Backend* | Auto GPU, GPU D3D12, Direct3D11 ou Vulkan. A escolha é salva e exige reiniciar o jogo; não encerra automaticamente sua partida. HD/shaders/FSR próprios requerem GPU D3D12 ou Vulkan. |
| AMD FSR 1 | Dois passes oficiais AMD: EASU para ampliação espacial e RCAS para nitidez. Usa o tamanho do quadro exibido como alvo; ao ativar, desliga HD por contornos, e vice-versa. Não é FSR2/3 temporal ou geração de quadros. |

A resolução do código gráfico original e dos sprites não aumenta. São opções de apresentação do quadro Saturn. Não há pacote de texturas HD, carregamento de shaders RetroArch/ReShade ou scripts gráficos externos nesta entrega.

O filtro HD altera os contornos comparando vizinhos, preservando a paleta; não inventa texturas novas. 4x combina dois passes2x;6x combina2x e3x;8x usa três passes2x. A dimensão do alvo GPU aumenta de fato, mas é uma reconstrução do quadro pronto. Não confundir com renderizar novamente a geometria em resolução interna maior, como o backend de hardware do PSP. O nosso VDP1/VDP2 ainda rasteriza/compõe no processador na resolução Saturn. Mesmo o PPSSPP distingue [resolução interna e escalonamento de texturas](https://www.ppsspp.org/docs/settings/graphics/); sprites alinhados aos pixels não ganham detalhes só pela resolução interna.

O algoritmo é Scale2x/Scale3x, não HQx nem xBRZ. [Regras originais](https://www.scale2x.it/algorithm). Os fatores foram testados na GPU contra todos os pixels de uma referência CPU, incluindo bordas, atualização de quadro, restauração de alvo, limites de memória e fallback. A preferência visual deve ser conferida no jogo. Falhas de alocação/driver desativam o HD e preservam a imagem original.

O launcher desktop tenta o renderer SDL_GPU/D3D12; se não estiver disponível, usa o renderer padrão e informa o limite na página. Shaders próprios são DXIL embutido, compatíveis com a assinatura binária do SDL3 3.4.14. O menu é desenhado depois de remover o shader. Uma falha de apresentação desliga o shader e registra o motivo. Os filtros de CPU continuam independentes.

Vulkan usa os mesmos shaders em SPIR-V com sampler combinado e assinatura de entrada confirmada por teste. Um DXC portátil oficial1.9.2609 foi necessário, porque o DXC do Windows SDK exibe a opção SPIR-V na ajuda mas não inclui esse gerador. O compilador fica local ao projeto, não entra no jogo nem muda o PATH global. O runtime continua usando SDL3 e o driver gráfico já instalado.

## ReShade, DLSS, DLAA e frame generation

ReShade é externo, BSD3-clause, e [declara suporte a D3D11/D3D12/Vulkan](https://reshade.me/). O pacote oficial6.8.0 foi extraído em diretórios de teste, sem executar seu instalador e sem registrar camada global. Um probe SDL confirmou carregamento/interceptação em D3D12 viaDLLlocal e Vulkan via `VK_LAYER_PATH`/`VK_INSTANCE_LAYERS` somente no processo filho. As apresentações terminaram com saída0. Isso **não certifica os efeitos**: os logs não confirmaram compilação do efeito de teste nem overlay do ReShade no jogo. O launcher normal não recebeu DLLs/camada. Evidências `validacao/hd-performance/reshade/result.json` e `ReShade.log` por backend. Pacotes ReShadeFX não são os shaders internos SPIR-V/DXIL.

DLSS e DLAA não foram implementados. A [integração oficial NVIDIA](https://github.com/NVIDIA-RTX/Streamline/blob/main/docs/ProgrammingGuideDLSS.md) exige cor, profundidade, vetores de movimento e dados temporais. O compositor atual entrega apenas o quadro RGB pronto; preencher os demais com valores fictícios não seria integração válida. Na RTX3060 usada nos testes, DLSS Super Resolution é possível em um motor compatível, mas [DLSS Frame Generation tem suporte oficial nas RTX40/50](https://forums.developer.nvidia.com/t/dlss-4-faq/321939). FSR3/framegen também exige outra integração e prova de latência/artefatos. Não há opções falsas desses recursos no menu.

## Medir uma queda de desempenho

O atalho **SotN HD** já registra o perfil. Na sala lenta, mantenha 1×, aguarde cerca de 20 segundos e pressione F8. F7 alterna a medição. Para uma execução descartável pela linha de comando:

```powershell
.\Jogar-SotN-Windows.cmd -Executable ".\sotn-experiment\build\jp\windows-reference-engine-20261025\game\saturn.exe" -DisposableSave -VideoProfile
```

O launcher registra os comandos reais em `input.txt` e resumos de tempo em `run.log` a cada300 apresentações. `VIDEO_PROFILE` separa rasterizaçãoVDP1, composiçãoVDP2, upload, desenho e apresentação. Desenho inclui apresentação: não somar as duas médias. Valores do log entre colchetes são tempo virtual, não a duração real. Preserve a pasta da sessão para reproduzir a sala exata. O modo acima não toca no save pessoal.

## Controles

SDL detecta controles compatíveis conectados por USB/Bluetooth, incluindo famílias Xbox e PlayStation. A página mostra o primeiro controle conectado, permite habilitar/desabilitar gamepad, escolher nomes Auto/Xbox/PlayStation, ajustar a zona morta do analógico e restaurar o mapeamento padrão. Escolher nomes não converte um controle físico em outro modelo.

| Ação padrão | Xbox | PlayStation | Teclado |
|---|---|---|---|
| Ataque / Saturn A | X | Quadrado | Z |
| Pulo / Saturn B | A | Cruz | Espaço ou X |
| Cancelar / Saturn C | B | Círculo | C |
| Start do jogo | Menu | Options/Start | Enter |
| Dev Menu | Start + clique analógico direito | Options + R3 | F1 ou Esc |
| Mapa | View/Back | Share/Select | Start e L no menu original |

O driver precisa entregar o dispositivo ao SDL; não foi validada nesta fatia a conexão de cada modelo físico. Os nomes e as opções foram conferidos na interface. Mapeamentos personalizados continuam disponíveis no INI, inclusive por GUID. Restaurar padrão também redefine as seções dos modelos atualmente conectados.

## Velocidade e persistência

Velocidades: 1x, 2x, 4x, 6x, 8x e 10x. O fator reduz a espera do PC; o relógio virtual do jogo não muda. O limite real depende do computador. Acelerar silencia o áudio; 1x restaura o volume escolhido. Cada processo começa em 1x.

Vídeo, volume e controles ficam em `sotn-experiment/tools/sotn-windows.ini`. O launcher não impõe mais `--scale 3`, que impediria restaurar a resolução escolhida. Selecionar resolução ou escala sai da tela cheia para mostrar o tamanho selecionado.

Mudo (F9) preserva o volume escolhido. Pausar ao sair abre o Esc quando a janela perde foco, por exemplo com Alt+Tab; voltar à janela mantém a pausa até escolher Continuar. Capturar imagem/F12 grava o quadro original do Saturn em `shot-VBLANK.png` na saída da sessão, sem menu ou shader. Essas opções reaproveitam comportamentos estudados no Ymir; comparação e limites (referência local não distribuída).

Hacks/GameShark: [manual separado](MODMENU.md). Todo hack começa desligado. Para testes sem alterar o save pessoal:

```powershell
.\Jogar-SotN-Windows.cmd -DisposableSave
```

Evidências e limites atuais: [VALIDATION](VALIDATION.md) e [HANDOFF](../HANDOFF.md). O menu original do Alucard tem investigação própria; as opções do Esc não substituem a correção de suas chamadas SH-2.
