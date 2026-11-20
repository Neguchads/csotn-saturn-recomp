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
