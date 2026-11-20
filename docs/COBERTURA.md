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
