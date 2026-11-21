[English](#english) | [Português](#portugues)

<a id="english"></a>

> Historical manual from the local installation, exported on 20 November 2026. Referenced builds, captures and `validacao/` folders are not distributed. For this checkout, follow [SETUP](SETUP.md), [VALIDATION](VALIDATION.md) and [SOURCES](SOURCES.md).

# Experimental broad profile

## Recompilation of all programs present — 25 October 2026

New disc generation: **80 PRG files present plus0.BIN, 205 modules, 11020 functions**. Includes18 previously missing entries: ZERO `0600EE64`, 5 from BOSS_04, 9 from BOSS_07 and3 from BOSS_R09. JSR consumers and targets checked in the original. Discovery also added `060E72C0` and removed10 false functions starting in literal pools; therefore the net count grows by9. Slices and identity CRCs preserved, all present programs represented and inventory valid, with4121836 mapped instruction bytes. `BOSS_15.PRG` does not exist on disc; do not fabricate a file.

Clean Windows build651/651 in `sotn-experiment/build/jp/windows-reference-disc-20261025/game/saturn.exe`, SHA-256 **969604F7B30F78E7C42C49DB85EB633224D0A515523B05D3493CE19389EDA09E**. The **SotN HD** shortcut and default launcher point to this build; version `1CEDB170` preserved. **122 project tests and158 SH-2/discovery tests** passed. The130 engine sources are identical to the previous snapshot, which passed197 native tests; that suite was not repeated in this slice. Boot without inputs, a window or personal save up to3000 VBlanks exited0, without a fatal error, with all three RGB frames identical to the previous version.

This delivers recompilation of all programs present and fixes proven gaps; it does not demonstrate discovery of every possible indirect call or the full campaign. The user will perform gameplay: Maria/STAGE16/STAGE01 transitions, saving/healing/loading in rooms and FPS in reported rooms remain manual criteria. Savestates are out of scope. No personal save read, copied or modified. No commit, remote, push, PR, merge or publication. Evidence: `validacao/recomp-disc-20261025/RESULTADO.md`. Configuration: `sotn-experiment/game-jp-disc-20261025.toml`.

Requested on23 October2026: compile available disc programs in advance, before fixing individual areas. Later the user requested unification: the broad profile is the launchers' main target; previous builds are preserved historical versions. Broader generation does not prove every room playable.

## Prepared content

- 80 extracted PRG files classified, plus `0.BIN`.
- 59 stage programs from the game table: normal/inverted stages, bosses, teleporters and library.
- Boot, menus, players, RICHTER2, ending and seven familiars.
- 128 weapon records; two complete duplicates are aliases, leaving126 distinct records.
- Total:205 modules. After preserving the SUB submenu export:11,011 functions,2,075,513 instructions and606 files.

The table contains100 stageIDs and60 unique program names. `BOSS_15.PRG`, associated with stageID `0x11`, is absent from the extraction and local disc copy used for analysis. This reference remains recorded as absent. Do not create a fictional binary or substitute another boss.

Later investigation: the original executable itself contains PRG/CHR/MAP names, but the JP CUE's ISO directory does not contain those files. None of the targets or reloads in171 teleport records, checked against GAME.PRG, selects `0x11`; the boss teleport source table does not use it either. Current classification is a likely residual reference, without evidence of missing required content. Absence of every special/debug path was not proven. Evidence is in `validacao/all-programs/boss15-disc-proof.json` and the game/mapping study.

## Reproducing on this machine

PowerShell from the root:

```powershell
& '.\referencias\pc-saturnbomberman\.venv\Scripts\python.exe' '.\sotn-experiment\tools\make_all_config.py'
& '.\referencias\pc-saturnbomberman\.venv\Scripts\python.exe' '.\sotn-experiment\tools\generate-jp.py' '.\sotn-experiment\game-jp-all.toml'
& '.\sotn-experiment\tools\build-windows-all.cmd'
```

Configuration: `sotn-experiment/game-jp-all.toml`; slices/C++: `sotn-experiment/build/jp-all/`; Windows build: `sotn-experiment/build/jp/windows-reference-all/`. `game-jp.toml`, the previous C++ and `windows-reference-current` executable are preserved. Learned seeds remain shared through `seeds-jp.json`, without being overwritten during generation.

The local engine contains the discovery fix recorded in `validacao/all-programs/discover-delay-slots.patch`. The heuristic accepted `bt/bf` inside delay slots, promoting data to functions in four modules. Invalid candidates are now rejected before promotion. An illegal explicit entry still produces a diagnostic/error; there is no silent fallback. This fix must accompany any reproduction in another engine checkout.

After compilation, test the broad version:

```powershell
.\Jogar-SotN-Amplo.cmd
```

`Jogar-SotN-Windows.cmd` and `Jogar-SotN-Amplo.cmd` default to `windows-reference-all/game/saturn.exe`. On25 October, the **SotN HD** shortcut explicitly uses `windows-reference-engine-20261025/game/saturn.exe`, hash `1CEDB170`, retaining the STAGE16 profile, engine fixes and enabled profiler. The previous STAGE16 candidate `9553656C` and `windows-reference-current` are preserved. To test without touching the personal save, add `-DisposableSave` to the launcher. `Recompilar-SotN.cmd` prepares, generates and builds the main profile; the shortcut uses the explicit target stated in the handoff.

## Coverage criterion

25 October audit:436 files match the manifest; both profiles represent the80 PRGs present, with205 modules/11011 functions. New inventories valid, zero omitted discovered instructions and no gaps in the typed callbacks checked. Report and limits (local reference not distributed). The user will play the campaign manually; the audit did not open the game.

Stages use `EntityEntry.func` as seeds, keeping headers, banks and layouts as data. The two known shared entries belong to ZERO/GAME; their registration addresses do not become stage functions. Proven YAML/RAM boundaries take priority. For new stages, discovery is bounded by the first typed data structure. Weapons/familiars use callback fields whose size is confirmed by the consumer, excluding the resource at `+0x40`.

Slices are checked byte for byte against sources and offsets. No identical slice identity was duplicated at the same address; the two weapon aliases have fully identical original records. These are static proofs: mutable data can still invalidate new modules' CRCs. CRC protection remains active.

Generation and build expand available code. Accepting a room requires executing its legitimate entry and exit, confirming the correct module and expected image/state. Saving, the full campaign, every weapon/familiar and every boss still require their own tests. Do not write RAM to simulate progress.

## Local evidence

The Windows build finished and the profile passed the old Laboratory failure up to55000 VBlanks, exiting0 with earlier captures identical. Another replay up to55600 demonstrated movement, jumping and sword use in the entrance room. This enables broader manual testing; it does not certify other rooms or the boss.

`validacao/all-programs/` contains program surveys, slice audit, original emission failure, discovery regressions, generation, build, tests, inventory and replay. The Ghidra database remains prepared for STAGE_02; Serena/GhidraMCP are available in this session. Actually proven state and next blocker: [HANDOFF](../HANDOFF.md) and [ROADMAP](../ROADMAP.md).

---

<a id="portugues"></a>

> Manual histórico da instalação local, exportado em 20/11/2026. Builds, capturas e pastas `validacao/` citadas não são distribuídas. Para este checkout, siga [SETUP](SETUP.md), [VALIDATION](VALIDATION.md) e [SOURCES](SOURCES.md).

# Perfil amplo experimental

## Recompilação de todos os programas presentes — 25/10/2026

Geração nova do disco: **80 PRG presentes mais 0.BIN, 205 módulos, 11020 funções**. Inclui 18 entradas antes ausentes: ZERO `0600EE64`, 5 de BOSS_04, 9 de BOSS_07 e 3 de BOSS_R09. Consumidores JSR e destinos conferidos no original. A descoberta acrescentou também `060E72C0` e retirou 10 falsas funções que começavam em pools de literais; por isso a contagem líquida cresce 9. Recortes e CRCs de identidade preservados, todos os programas presentes representados e inventário válido, com 4121836 bytes de instruções mapeadas. `BOSS_15.PRG` não existe no disco; não fabricar arquivo.

Build Windows limpa 651/651 em `sotn-experiment/build/jp/windows-reference-disc-20261025/game/saturn.exe`, SHA-256 **969604F7B30F78E7C42C49DB85EB633224D0A515523B05D3493CE19389EDA09E**. Atalho **SotN HD** e launcher padrão apontam para essa build; versão `1CEDB170` preservada. Passaram **122 testes do projeto e 158 SH-2/descoberta**. Os 130 fontes do motor são idênticos ao snapshot anterior, que passou 197 testes nativos; essa suíte não foi repetida nesta fatia. Boot sem entradas, sem janela e sem save pessoal até 3000 VBlanks saiu 0, sem fatal, com os três quadros RGB idênticos ao anterior.

Isso entrega a recompilação de todos os programas presentes e corrige as lacunas comprovadas; não demonstra descoberta de toda chamada indireta possível nem campanha integral. O usuário fará o gameplay: transições Maria/STAGE16/STAGE01, salvar/curar/carregar nas salas e FPS das salas reportadas continuam critérios manuais. Savestate fora do escopo. Nenhum save pessoal lido, copiado ou modificado. Sem commit, remoto, push, PR, merge ou publicação. Provas: `validacao/recomp-disc-20261025/RESULTADO.md`. Configuração: `sotn-experiment/game-jp-disc-20261025.toml`.

Solicitado em 23/10/2026: antecipar a compilação dos programas disponíveis no disco antes de corrigir os problemas de cada área. Posteriormente, o usuário pediu a unificação: o perfil amplo é o destino principal dos launchers; os builds anteriores são históricos preservados. Ampliar geração não comprova todas as salas jogáveis.

## Conteúdo preparado

- 80 arquivos PRG presentes na extração classificados, além de `0.BIN`.
- 59 programas de cenário da tabela do jogo: fases normais/invertidas, chefes, teletransportes e biblioteca.
- Boot, menus, jogadores, RICHTER2, final e sete familiares.
- 128 registros de armas; duas duplicatas integrais são aliases, deixando 126 registros distintos.
- Total: 205 módulos. Após preservar a exportação de submenu SUB: 11.011 funções, 2.075.513 instruções e 606 arquivos.

A tabela tem 100 stageIDs e 60 nomes de programa únicos. `BOSS_15.PRG`, associado ao stageID `0x11`, não está na extração nem na cópia local do disco usada pela análise. Essa referência permanece registrada como ausente. Não criar um binário fictício nem substituir outro chefe.

Investigação posterior: o próprio executável original contém os nomes PRG/CHR/MAP, mas o diretório ISO do CUE JP não contém esses arquivos. Nenhum destino ou reload nos 171 registros de teletransporte, conferidos contra GAME.PRG, seleciona `0x11`; a tabela fonte de teletransporte de chefes também não o usa. A classificação atual é provável referência residual, sem evidência de conteúdo obrigatório faltando. Não foi provada a ausência de qualquer caminho especial/debug. Provas em `validacao/all-programs/boss15-disc-proof.json` e no estudo de jogo/mapeamento.

## Como reproduzir nesta máquina

PowerShell a partir da raiz:

```powershell
& '.\referencias\pc-saturnbomberman\.venv\Scripts\python.exe' '.\sotn-experiment\tools\make_all_config.py'
& '.\referencias\pc-saturnbomberman\.venv\Scripts\python.exe' '.\sotn-experiment\tools\generate-jp.py' '.\sotn-experiment\game-jp-all.toml'
& '.\sotn-experiment\tools\build-windows-all.cmd'
```

Configuração: `sotn-experiment/game-jp-all.toml`; recortes/C++: `sotn-experiment/build/jp-all/`; build Windows: `sotn-experiment/build/jp/windows-reference-all/`. O `game-jp.toml`, C++ anterior e executável `windows-reference-current` são preservados. As sementes aprendidas continuam compartilhadas pelo arquivo `seeds-jp.json`, sem sobrescrita durante geração.

O motor local contém a correção de descoberta registrada em `validacao/all-programs/discover-delay-slots.patch`. A heurística aceitava `bt/bf` dentro de delay slots, promovendo dados a funções em quatro módulos. Agora os candidatos inválidos são rejeitados antes dessa promoção. Uma entrada explícita ilegal ainda gera diagnóstico/erro; não existe fallback silencioso. Essa correção deve acompanhar qualquer reprodução em outro checkout do motor.

Após a compilação, testar a versão ampla:

```powershell
.\Jogar-SotN-Amplo.cmd
```

`Jogar-SotN-Windows.cmd` e `Jogar-SotN-Amplo.cmd` usam por padrão `windows-reference-all/game/saturn.exe`. Em 25/10, o atalho **SotN HD** usa explicitamente `windows-reference-engine-20261025/game/saturn.exe`, hash `1CEDB170`, mantendo o perfil STAGE16, as correções do motor e o profiler habilitado. A candidata anterior STAGE16 `9553656C` e `windows-reference-current` estão preservadas. Para teste sem tocar no save pessoal, acrescentar `-DisposableSave` ao launcher. `Recompilar-SotN.cmd` prepara, gera e compila o perfil principal; o atalho usa o destino explícito indicado no handoff.

## Critério de cobertura

Auditoria de 25/10: 436 arquivos conferem com o manifesto; ambos perfis representam os 80 PRG presentes, com 205 módulos/11011 funções. Inventários novos válidos, zero instruções descobertas omitidas e nenhuma lacuna nos callbacks tipados conferidos. Relatório e limites (referência local não distribuída). O usuário fará a campanha manualmente; a auditoria não abriu o jogo.

As fases usam `EntityEntry.func` como sementes, mantendo cabeçalhos, bancos e layouts como dados. As duas entradas compartilhadas conhecidas pertencem a ZERO/GAME; seus endereços de registro não viram funções da fase. Fronteiras YAML/RAM já comprovadas têm prioridade. Para fases novas, a descoberta é limitada pela primeira estrutura tipada de dados. Armas/familiares usam os campos de callback com tamanho confirmado pelo consumidor, excluindo o recurso em `+0x40`.

Os recortes são conferidos byte a byte contra as fontes e offsets. Nenhuma identidade de recorte igual foi duplicada no mesmo endereço; os dois aliases de arma têm registros originais integralmente iguais. Essas provas são estáticas: dados mutáveis ainda podem invalidar o CRC de módulos novos. A proteção por CRC permanece ativa.

Geração e build ampliam o código disponível. Para aceitar uma sala, executar sua entrada e saída legítimas, confirmar módulo correto e imagem/estado esperado. Salvamento, campanha completa, todas as armas/familiares e todos os chefes continuam exigindo testes próprios. Não escrever na RAM para simular progresso.

## Evidências locais

O build Windows terminou e o perfil atravessou a antiga falha do Laboratório até 55000 VBlanks, com saída0 e capturas anteriores idênticas. Outro replay até55600 comprovou movimento, pulo e espada na sala de entrada. Isso libera um teste manual mais amplo; não certifica as demais salas ou o chefe.

`validacao/all-programs/` contém levantamentos de programas, auditoria de recortes, falha original de emissão, regressões da descoberta, geração, build, testes, inventário e replay. O banco Ghidra continua preparado para STAGE_02; Serena/GhidraMCP estão disponíveis nesta sessão. Estado efetivamente comprovado e próximo bloqueio: [HANDOFF](../HANDOFF.md) e [ROADMAP](../ROADMAP.md).
