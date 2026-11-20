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
