[English](#english) | [Português](#portugues)

<a id="english"></a>

> Historical manual from the local installation, exported on 20 November 2026. Referenced builds, captures and `validacao/` folders are not distributed. For this checkout, follow [SETUP](SETUP.md), [VALIDATION](VALIDATION.md) and [SOURCES](SOURCES.md).

# Main game and mod menu

The main game uses the broad profile's 205 modules, together with the previous version's Dev Menu, return to title, speed, volume and fullscreen features. `Jogar-SotN-Windows.cmd`, the existing desktop shortcut and `Jogar-SotN-Amplo.cmd` use the same executable: `sotn-experiment/build/jp/windows-reference-stage15-identity-20261025/game/saturn.exe`. The second name is only a compatibility alias.

The old executable in `windows-reference-current` and the copies in `validacao/mod-menu/baseline` are historical recovery versions. They were not deleted. Two different campaigns are not being combined: the broad profile already preserved the earlier modules, seeds and features; the entry points now use it as the main profile.

## Using the menu

Press **F1 or L3+R3 → Cheats**. Use the mouse or arrow keys and Enter; left disables, right enables. L1/R1 switches tabs. Esc returns to the main menu; another Esc resumes. Hacks start disabled in each process; returning to title also disables them because it creates a new process. There are 15 native presets and four existing scripts, in pages of 12; see the [updated manual](DEV-MENU.md).

| Option | Effect |
|---|---|
| Godmode | Refills HP to its current maximum and maintains the normal invincibility timer. Does not change cutscene timing. |
| Infinite life | Refills HP without an invincibility timer; damage/knockback remain possible. |
| Maximums | HP/MP999 and hearts99 for Alucard, without reducing higher values. |
| Base stats99 | STR/CON/INT/LCK for Alucard; totals depend on the game's normal recalculation. |
| Infinite mana | Refills MP to its current maximum. |
| Infinite hearts | Refills hearts to their current maximum. |
| Gold 999999 | Maintains that value for Alucard; does not change other characters' gold. |
| All relics | Grants Alucard's 31 normal relics once when enabled; preserves the selected familiar and hidden slot. |
| Complete transformations | Grants bat/wolf/mist and their powers once when enabled. |
| Complete mobility | Grants double jump, super jump and Saturn running boots once when enabled. |
| Restore HP / MP / hearts | One refill at the next valid gameplay moment. |
| Disable all | Stops presets/scripts and cancels a pending refill. Previously changed values are not undone. |
| Reload scripts | Reads the file again. Success replaces the scripts and leaves them disabled; an error preserves the last valid set and appears in status/log. Presets are independent. |
| Previous / next page | Browses presets and scripts, 12 options per page. |

**Hacks can change the save if you save the game.** To test without persistence:

```powershell
.\Jogar-SotN-Windows.cmd -DisposableSave
```

The main launcher also accepts parameters forwarded to the PowerShell script. Disabling a hack does not restore previous resources, maximums or money. Do not use hacks as evidence of legitimate campaign progression. They were requested for local play; regression routes still use disposable saves with everything disabled.

## Saturn GameShark / Action Replay scripts

Edit `sotn-experiment/tools/sotn-gameshark.txt`, save, and choose **Recarregar scripts** (Reload scripts). The file includes four disabled examples: HP/maximum999, MP/maximum999, hearts/maximum99 and gold999999 conditional on Alucard.

Text format, without an external scripting language or command execution:

```text
[Nome do script]
1605C940 0000
1605C942 03E7
```

The two lines write a 32-bit big-endian value to Saturn JP HP. Writing only the low half can leave the high half incorrect. These addresses are specific to Japanese V1.400; PS1 codes are incompatible.

| Type | Support |
|---|---|
| `0XXXXXXX YYYY` | One 16-bit write per activation, at the next valid gameplay moment. Reactivating resets that execution. |
| `1XXXXXXX YYYY` | Continuous 16-bit write. |
| `3XXXXXXX 00YY` | Continuous 8-bit write. |
| `DXXXXXXX YYYY` | 16-bit comparison; executes the next write only if the value is equal. No chained conditions. |
| Master codes `F/B`, PS1, other types | Rejected with an error, not silently ignored. The recomp does not need the cartridge master code. |

Allowed addresses: Work RAM `06000000..060FFFFF` and `00200000..002FFFFF`; 16-bit operations require even alignment. Up to64 scripts, 256 codes per script, ASCII names of1..32 characters, 512-byte lines and a1MiB file. Comments start with `#` or `;`. An invalid line rejects the whole update.

This support targets **RAM data**. Modifying Saturn instruction bytes does not rewrite already recompiled C++. Do not claim full compatibility with all existing codes, native instructions, master codes or encrypted formats. Type0 semantics were adapted to activation during gameplay, rather than cartridge boot application.

Writes are restricted to normal gameplay: correct dispatcher/engine, valid character and stage, coherent living HP, no cutscene control or dead state, and an active player module. This prevents applying presets/scripts during boot/title/cutscenes. Custom scripts can still write bad values to their chosen RAM addresses; format validation does not prove that a code belongs to this game version.

Format sources: [Saturn types on GameHacking](https://wiki.gamehacking.org/Code_Types_(Sega_Saturn)) and [examples published by a Saturn code author](https://gist.github.com/bbayles/6725bba5481c6a527382bba481f5eda4). Preset addresses came from local Saturn structures and17 literals checked against original binaries, recorded in `validacao/mod-menu/addresses-study.json`; no PS1 address was reused.

## Recompile and verify

```powershell
.\Recompilar-SotN.cmd
& '.\referencias\pc-saturnbomberman\.venv\Scripts\python.exe' -m pytest sotn-experiment/tests -q
```

The single command prepares the broad profile, generates C++ and builds the main executable. `build-windows-reference.cmd` and `build-windows-all.cmd` also build the same profile. `generate-jp.py` and `inventory-jp.py` now default to `game-jp-all.toml`; `make_config.py` remains the historical helper for known modules, used internally by `make_all_config.py`.

Implementation: existing SDL menu; `sotn_mods.h` parses/executes codes, `sotn_presets.h` applies typed resources, and `machine.cpp` connects to existing polling. No library was added. Evidence, native tests, runtime patch and replays are in `validacao/mod-menu/`; the actually proven state is in [HANDOFF](../HANDOFF.md).

The three relic presets are on the second page. Close the Dev Menu to apply; disabling/enabling rearms them. The interface and campaign effects still require manual testing; RAM tests do not certify gameplay. Current evidence is in `validacao/relic-hacks-20261025/RESULTADO.md`.

---

<a id="portugues"></a>

> Manual histórico da instalação local, exportado em 20/11/2026. Builds, capturas e pastas `validacao/` citadas não são distribuídas. Para este checkout, siga [SETUP](SETUP.md), [VALIDATION](VALIDATION.md) e [SOURCES](SOURCES.md).

# Jogo principal e modmenu

O jogo principal usa os 205 módulos do perfil amplo, junto com o Dev Menu, retorno ao título, velocidade, volume e tela cheia da versão anterior. `Jogar-SotN-Windows.cmd`, o atalho existente da Área de Trabalho e `Jogar-SotN-Amplo.cmd` usam o mesmo executável: `sotn-experiment/build/jp/windows-reference-stage15-identity-20261025/game/saturn.exe`. O segundo nome é apenas um alias de compatibilidade.

O executável antigo em `windows-reference-current` e as cópias em `validacao/mod-menu/baseline` são históricos para recuperação. Não foram apagados. Não há duas campanhas diferentes sendo combinadas: o perfil amplo já preservava módulos, sementes e recursos do anterior; agora os pontos de entrada usam esse perfil como principal.

## Usar o menu

Pressionar **F1 ou L3+R3 → Cheats**. Usar mouse ou setas e Enter; esquerda/desligar, direita/ligar. L1/R1 troca abas. Esc volta ao menu principal, outro Esc continua. Os hacks iniciam desligados em cada processo; voltar ao título também os desliga porque cria um processo novo. São 15 presets nativos e quatro scripts existentes, em páginas de 12; [manual atualizado](DEV-MENU.md).

| Opção | Efeito |
|---|---|
| Godmode | Repõe HP até o máximo atual e mantém o timer normal de invencibilidade. Não altera o timer de cenas. |
| Vida infinita | RepõeHP sem timer de invencibilidade; dano/knockback continuam possíveis. |
| Máximos | HP/MP999 e corações99 paraAlucard, sem reduzir valores superiores. |
| Atributos base99 | STR/CON/INT/LCK paraAlucard; totais dependem do recálculo normal do jogo. |
| Mana infinita | Repõe MP até o máximo atual. |
| Corações infinitos | Repõe corações até o máximo atual. |
| Ouro 999999 | Mantém esse valor para Alucard; não altera o ouro dos outros personagens. |
| Todas as relíquias | Concede as 31 relíquias normais de Alucard uma vez ao ligar; preserva familiar selecionado e slot oculto. |
| Transformações completas | Concede morcego/lobo/névoa e seus poderes uma vez ao ligar. |
| Mobilidade completa | Concede salto duplo, super salto e botas de corrida Saturn uma vez ao ligar. |
| Restaurar HP / MP / corações | Reposição única no próximo momento válido de gameplay. |
| Desligar todos | Para os presets/scripts e cancela a reposição pendente. Valores já modificados não são desfeitos. |
| Recarregar scripts | Lê o arquivo novamente. Sucesso substitui os scripts e os deixa desligados; erro preserva o último conjunto válido e aparece no status/log. Presets são independentes. |
| Página anterior / próxima | Navega pelos presets e scripts, 12 opções por página. |

**Hacks podem alterar o save se a partida for salva.** Para testar sem persistência:

```powershell
.\Jogar-SotN-Windows.cmd -DisposableSave
```

O launcher principal também aceita parâmetros encaminhados ao script PowerShell. Desligar um hack não restaura recursos, máximos ou dinheiro anteriores. Não usar os hacks como prova de progressão legítima da campanha. Eles foram pedidos para jogo local; as rotas de regressão continuam com tudo desligado e save descartável.

## Scripts GameShark / Action Replay Saturn

Editar `sotn-experiment/tools/sotn-gameshark.txt`, salvar e usar **Recarregar scripts**. O arquivo inclui quatro exemplos desligados: HP/máximo999, MP/máximo999, corações/máximo99 e ouro999999 condicionado a Alucard.

Formato de texto, sem linguagem de script externa ou execução de comandos:

```text
[Nome do script]
1605C940 0000
1605C942 03E7
```

As duas linhas escrevem um valor de 32 bits big-endian no HP Saturn JP. Usar só a metade baixa pode deixar a metade alta incorreta. Esses endereços são específicos da edição japonesa V1.400; códigos PS1 não são compatíveis.

| Tipo | Suporte |
|---|---|
| `0XXXXXXX YYYY` | Escrita16 uma vez por ativação, no próximo gameplay válido. Reativar reseta essa execução. |
| `1XXXXXXX YYYY` | Escrita16 contínua. |
| `3XXXXXXX 00YY` | Escrita8 contínua. |
| `DXXXXXXX YYYY` | Comparação16; executa a próxima escrita somente se o valor for igual. Sem encadeamento de condições. |
| Master codes `F/B`, PS1, outros tipos | Recusados com erro; não são ignorados silenciosamente. O recomp não precisa de master code do cartucho. |

Endereços permitidos: Work RAM `06000000..060FFFFF` e `00200000..002FFFFF`; operações16 precisam de alinhamento par. Até64 scripts, 256 códigos por script, nomes ASCII de1..32 caracteres, linhas de512 bytes e arquivo de1MiB. Comentários começam com `#` ou `;`. Uma linha inválida recusa a atualização inteira.

Esse suporte é voltado a **dados em RAM**. Modificar bytes de instruções do Saturn não reescreve o C++ já recompilado. Não anunciar compatibilidade completa com todos os códigos existentes, instruções nativas, master codes ou formatos criptografados. A semântica do tipo0 foi adaptada para ativação no gameplay, não aplicação no boot do cartucho.

As escritas são limitadas ao gameplay normal: dispatcher/engine corretos, personagem e fase válidos, HP vivo e coerente, sem controle de cena ou estado morto e com módulo de jogador ativo. Isso evita aplicar presets/scripts no boot/título/cenas. Os scripts personalizados ainda podem escrever valores ruins nos endereços RAM escolhidos; validação de formato não prova que um código pertence a esta versão do jogo.

Fontes do formato: [tipos Saturn no GameHacking](https://wiki.gamehacking.org/Code_Types_(Sega_Saturn)) e [exemplos publicados pelo autor de códigos Saturn](https://gist.github.com/bbayles/6725bba5481c6a527382bba481f5eda4). Os endereços dos presets vieram das estruturas Saturn locais e de17 literais conferidos nos binários originais, registrados em `validacao/mod-menu/addresses-study.json`; nenhum endereço de PS1 foi reutilizado.

## Recompilar e verificar

```powershell
.\Recompilar-SotN.cmd
& '.\referencias\pc-saturnbomberman\.venv\Scripts\python.exe' -m pytest sotn-experiment/tests -q
```

O comando único prepara o perfil amplo, gera C++ e compila o executável principal. `build-windows-reference.cmd` e `build-windows-all.cmd` também constroem o mesmo perfil. `generate-jp.py` e `inventory-jp.py` passaram a usar `game-jp-all.toml` por padrão; `make_config.py` permanece o auxiliar histórico dos módulos conhecidos, aproveitado internamente por `make_all_config.py`.

Implementação: menu SDL existente; `sotn_mods.h` faz parsing/execução dos códigos, `sotn_presets.h` aplica recursos tipados e `machine.cpp` conecta ao polling já existente. Não foi adicionada biblioteca. Evidências, testes nativos, patch do runtime e replays em `validacao/mod-menu/`; estado efetivamente comprovado no [HANDOFF](../HANDOFF.md).

Os três presets de relíquias ficam na segunda página. Feche o Dev Menu para aplicar; desligar/ligar rearma. A interface e os efeitos na campanha ainda dependem do teste manual; testes de RAM não certificam gameplay. Provas atuais em `validacao/relic-hacks-20261025/RESULTADO.md`.
