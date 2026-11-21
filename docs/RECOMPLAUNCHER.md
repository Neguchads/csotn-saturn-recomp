# RecompLauncher integration / Integração com RecompLauncher

[English](#english) | [Português](#portugues)

<a id="english"></a>

## Status and distribution boundary

This is a **developer preview**, not a ready-to-play release. The package contains public sources and
our own `CsotnSetup.exe` bootstrap. The current game executable contains translated disc functions
and constants, so it is never included in this package. The game executable is generated and compiled
locally from the player's Japanese Saturn disc, **T-9527G V1.400**. No ROM, BIOS, extracted program,
disc-generated C++, personal save or private map table is downloaded or distributed.

The root `recomplauncher.json` uses format 1 and has passed validation against the launcher's official
schema. That validates the profile structure; it does not prove a successful installation or gameplay.
The full fresh-media preparation has **not been validated**: the project's `roms/` folder was empty
when this integration was prepared, and no original CUE was available for that test. No package has
been published and no catalog inclusion has been requested. Request inclusion only after validating
preparation with real media in a clean installation.

## External prerequisites

The preview requires Windows 11 x64 and these tools, supplied by the player or developer:

- Python 3.11 or later, available through `py -3`; alternatively set `SOTN_PYTHON` to its executable.
- Visual Studio Build Tools with **Desktop development with C++** and an x64 Windows SDK.
- LLVM `clang-cl`, CMake, Ninja, Git and Rust/Cargo. Put them on `PATH`; `SOTN_CLANG_CL` can name
  the compiler explicitly. `VCVARS64` can name `vcvars64.bat` if automatic discovery is unavailable.
- SDL3 **3.4.14**, Visual C development package. Set `SDL3_ROOT` to its root, containing
  `lib/x64/SDL3.dll` and its CMake development files.

The bootstrap checks these requirements but does **not install them automatically**. Network access
is needed to obtain the pinned public reference and splitter sources through Git and the Rust
dependencies through Cargo. It never downloads game media.

## Build a preview package

Run from the repository root. Commit the intended public sources first: the builder and packager
record and verify the Git revision and bootstrap source hash. Uncommitted tracked changes are refused.
Use the project's normal reviewed branch/PR workflow; this instruction does not authorize publishing
game-derived files.

```powershell
py -3 scripts/build_recomplauncher_bootstrap.py --output test-results/bootstrap
py -3 scripts/package_recomplauncher.py --output test-results/Csotn-Saturn-v0.1.0-preview-Windows-x64.zip --bootstrap test-results/bootstrap/CsotnSetup.exe --bootstrap-provenance test-results/bootstrap/bootstrap-provenance.json --version v0.1.0-preview
```

The ZIP contains `CsotnSetup.exe`, `recomplauncher.json`, `package-manifest.json` and the public tree
under `source/`. Provenance binds the bootstrap binary, its own source and the current revision.
The packager reads committed Git blobs, rejects private/untracked data and produces a deterministic
archive. Supplying no bootstrap creates a source-only archive: it is marked non-installable and the
profile is kept only under `source/`, rather than presented as a runnable launcher package.

## Check and prepare without starting the game

Extract the ZIP into a writable folder. From that folder, the following commands check prerequisites
and perform local compilation respectively. Keep the selected `.cue` and all referenced BIN tracks
together. `C:/CsotnData` below is a new example data directory, not a personal save directory.

```powershell
.\CsotnSetup.exe --check
.\CsotnSetup.exe --prepare --game 'C:/MyDiscs/SotN.cue' --data 'C:/CsotnData'
```

Preparation validates the disc edition, extracts locally, verifies the twelve reference program hashes,
obtains the public decomp and splitter at fixed revisions, and generates symbols/ASM from the player's
files. It then generates C++, compiles the Windows runtime and copies the resulting executable and
SDL DLL into the data directory. It does not copy private ASM or start the game.

Pinned reference revisions:

- `Xeeynamo/sotn-decomp`: `2be55ee17221ee306e330edf2df2d49d48ba47ae`.
- `saturn-splitter`: `68bc84e58de207c92e7ec0b8e887262fdcfdec2f`.

Each attempt uses a fresh isolated workspace. A failure preserves its logs and is not recorded as a
successful build. The profile's completion files are `{data}/build/ready.json`,
`{data}/game/saturn.exe` and `{data}/game/SDL3.dll`. The packager sets `prepare.version` to the
chosen release version so updates can request a new local build.

## Explicit player launch and saves

**Only `--play` starts the game.** Run this only when the player wants to play, after preparation:

```powershell
.\CsotnSetup.exe --play --data 'C:/CsotnData'
```

The original CUE/BIN media is still required at runtime. Normal saves remain in the game's save rooms;
there is no savestate setup here. The profile tells RecompLauncher where the player's data lives:

| Data | Path |
| --- | --- |
| Normal game save | `{data}/saves/backup.bin` |
| Settings | `{data}/settings.ini` |
| Logs | `{data}/logs` |
| Locally generated game executable | `{data}/game/saturn.exe` |

RecompLauncher supplies `{data}` through `RECOMPLAUNCHER_DATA` and uses the same directory for
preparation and launch. Save backup entries cover saves and settings, not disc data or generated builds.
Do not point a preparation test at the player's existing installation or personal saves.

See the [official integration guide](https://github.com/elripalda/RecompLauncher/blob/main/docs/DEVELOPERS.md)
for release packaging and catalog integration. A public source repository alone does not add a game
to the catalog. This preview does not certify campaign completion, stability or constant 60 FPS.

---

<a id="portugues"></a>

## Estado e limite de distribuição

Este é um **preview para desenvolvedores**, não uma release pronta para jogar. O pacote contém os
fontes públicos e nosso próprio inicializador `CsotnSetup.exe`. O executável atual do jogo contém
funções e constantes traduzidas do disco e, por isso, nunca entra neste pacote. O executável do jogo
é gerado e compilado localmente a partir do disco Saturn japonês do jogador, **T-9527G V1.400**.
Nenhuma ROM, BIOS, programa extraído, C++ gerado do disco, partida pessoal ou tabela privada do mapa
é baixada ou distribuída.

O `recomplauncher.json` da raiz usa o formato 1 e passou na validação pelo schema oficial do launcher.
Isso valida a estrutura do profile; não comprova instalação ou gameplay. A preparação completa a
partir da mídia original **não foi validada**: a pasta `roms/` do projeto estava vazia quando esta
integração foi preparada, sem CUE original disponível para esse teste. Nenhum pacote foi publicado e
nenhuma inclusão no catálogo foi solicitada. Solicite inclusão somente após validar a preparação com
mídia real em uma instalação limpa.

## Pré-requisitos externos

O preview exige Windows 11 x64 e estas ferramentas, fornecidas pelo jogador ou desenvolvedor:

- Python 3.11 ou posterior, disponível por `py -3`; alternativamente, configure `SOTN_PYTHON` com o executável.
- Visual Studio Build Tools com **Desenvolvimento para desktop com C++** e Windows SDK x64.
- LLVM `clang-cl`, CMake, Ninja, Git e Rust/Cargo. Coloque-os no `PATH`; `SOTN_CLANG_CL` pode indicar
  o compilador explicitamente. `VCVARS64` pode indicar `vcvars64.bat` quando a descoberta automática não estiver disponível.
- SDL3 **3.4.14**, pacote de desenvolvimento Visual C. Configure `SDL3_ROOT` com sua raiz, contendo
  `lib/x64/SDL3.dll` e os arquivos de desenvolvimento CMake.

O inicializador verifica esses requisitos, mas **não os instala automaticamente**. É necessária conexão
para obter os fontes públicos da referência e do splitter em revisões fixadas pelo Git e as dependências
Rust pelo Cargo. Nenhuma mídia do jogo é baixada.

## Gerar um pacote de preview

Execute na raiz do repositório. Primeiro faça commit dos fontes públicos pretendidos: o builder e o
empacotador registram e verificam a revisão Git e o hash do fonte do inicializador. Alterações rastreadas
sem commit são recusadas. Use o fluxo normal de branch/PR revisado do projeto; esta instrução não
autoriza publicar arquivos derivados do jogo.

```powershell
py -3 scripts/build_recomplauncher_bootstrap.py --output test-results/bootstrap
py -3 scripts/package_recomplauncher.py --output test-results/Csotn-Saturn-v0.1.0-preview-Windows-x64.zip --bootstrap test-results/bootstrap/CsotnSetup.exe --bootstrap-provenance test-results/bootstrap/bootstrap-provenance.json --version v0.1.0-preview
```

O ZIP contém `CsotnSetup.exe`, `recomplauncher.json`, `package-manifest.json` e a árvore pública em
`source/`. A procedência vincula o binário do inicializador, seu próprio fonte e a revisão atual.
O empacotador lê blobs do Git commitados, recusa dados privados/não rastreados e produz arquivo
determinístico. Sem fornecer o inicializador, gera um arquivo só de fontes: marcado como não instalável,
com o profile apenas em `source/`, sem apresentá-lo como pacote executável do launcher.

## Conferir e preparar sem iniciar o jogo

Extraia o ZIP em uma pasta gravável. Nessa pasta, os comandos abaixo verificam os pré-requisitos e
realizam a compilação local, respectivamente. Mantenha o `.cue` selecionado e todos os BINs referenciados
juntos. `C:/CsotnData` abaixo é um exemplo de pasta de dados nova, não uma pasta de saves pessoais.

```powershell
.\CsotnSetup.exe --check
.\CsotnSetup.exe --prepare --game 'C:/MyDiscs/SotN.cue' --data 'C:/CsotnData'
```

A preparação valida a edição do disco, extrai localmente, verifica os doze hashes de programas da
referência, obtém o decomp e o splitter públicos em revisões fixadas e gera símbolos/ASM dos arquivos
do jogador. Depois gera C++, compila o runtime Windows e copia o executável resultante e a DLL SDL
para a pasta de dados. Não copia ASM privado nem inicia o jogo.

Revisões públicas fixadas:

- `Xeeynamo/sotn-decomp`: `2be55ee17221ee306e330edf2df2d49d48ba47ae`.
- `saturn-splitter`: `68bc84e58de207c92e7ec0b8e887262fdcfdec2f`.

Cada tentativa usa um workspace isolado novo. Uma falha preserva seus logs e não é registrada como
build concluído. Os arquivos de conclusão do profile são `{data}/build/ready.json`,
`{data}/game/saturn.exe` e `{data}/game/SDL3.dll`. O empacotador define `prepare.version` com a
versão escolhida da release para que atualizações possam solicitar novo build local.

## Início explícito pelo jogador e saves

**Somente `--play` inicia o jogo.** Execute apenas quando o jogador quiser jogar, após a preparação:

```powershell
.\CsotnSetup.exe --play --data 'C:/CsotnData'
```

A mídia CUE/BIN original continua necessária em execução. O salvamento normal permanece nas salas
do jogo; não há configuração de savestate aqui. O profile informa ao RecompLauncher onde ficam os dados:

| Dado | Caminho |
| --- | --- |
| Save normal do jogo | `{data}/saves/backup.bin` |
| Configurações | `{data}/settings.ini` |
| Logs | `{data}/logs` |
| Executável do jogo gerado localmente | `{data}/game/saturn.exe` |

O RecompLauncher fornece `{data}` por `RECOMPLAUNCHER_DATA` e usa a mesma pasta para preparação e
execução. As entradas de backup cobrem saves e configurações, não os dados do disco ou builds gerados.
Não aponte um teste de preparação para a instalação existente ou os saves pessoais do jogador.

Veja o [guia oficial de integração](https://github.com/elripalda/RecompLauncher/blob/main/docs/DEVELOPERS.md)
para empacotamento de releases e integração no catálogo. Um repositório público de fontes, sozinho,
não inclui o jogo no catálogo. Este preview não certifica campanha completa, estabilidade ou 60 FPS constantes.
