[English](#english) | [Português](#portugues)

<a id="english"></a>

# Installation and tests

For the source/bootstrap developer preview and the local preparation pipeline, see
[RecompLauncher integration](RECOMPLAUNCHER.md#english), including external prerequisites and validation limits.

Local environment version references: Windows11, Python3.13, clang-cl21.1.8, MSVC14.44,
SDL3 3.4.14, CMake3.31.6, Ninja1.11.1.4, pytest9.1.1. Binary packages are not stored in Git.

## Tests without opening the game or a window

Prepare Python as described in README and supply the local dependencies. In PowerShell:

```powershell
$env:SDL3_ROOT = 'C:/dependencias/SDL3-3.4.14'
$env:SOTN_CLANG_CL = 'C:/dependencias/llvm/bin/clang-cl.exe'
$env:SOTN_DECOMP_REFERENCE = 'C:/referencias/sotn-decomp'
.\scripts\check-code.ps1 -Python '.\.venv\Scripts\python.exe'
```

The command tests the published sources and synthetic fixtures. Seven integrations that create a window
or a child process with a normal window are deselected and recorded in test-results/selection.json.
`-FullWindowTests` includes those integrations and must only be used when opening windows is authorized.
Neither mode should start the real campaign.

Two offset/relic tests use the public sotn-decomp project as an independent oracle.
Set SOTN_DECOMP_REFERENCE to a local clone at revision 2be55ee17221ee306e330edf2df2d49d48ba47ae
(https://github.com/Xeeynamo/sotn-decomp). These two tests do not require running the game or copying
disc files. The reference is not vendored in this repository; it is licensed under AGPL3.
Without this dependency, these tests fail explicitly; they are not skipped or replaced by mocks.

The native runtime tests can be compiled without SDL or a ROM:

```powershell
cmake -S engine/runtime/tests -B build/runtime-tests -G Ninja `
  -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl `
  -DSATURN_NO_SDL=ON -DSATURN_VDP1_GPL=ON
cmake --build build/runtime-tests --target runtime_tests video_perf_tests -j 4
ctest --test-dir build/runtime-tests --output-on-failure
```

Run in the Visual Studio development environment, where the compiler can find headers and libraries.
The upstream Python tests in engine/tests include Linux-only cases and a symlink case that may
fail on Windows with privilege error1314. Do not remove marks or weaken the test to make it pass.

## Prepare the game locally

1. Provide your Saturn JP T-9527G V1.400 disc in roms/ and check its identity with analyze-jp.py.
2. Provide the external sotn-decomp-analise copy in referencias/, revision
   2be55ee17221ee306e330edf2df2d49d48ba47ae, and prepare the Saturn symbols/asm required by the generator.
3. Put engine/src in PYTHONPATH. make_all_config.py reads the local files and prepares game-jp-all.toml.
4. generate-jp.py takes that TOML and produces local C++. Set SOTN_RECOMP_DIR to the corresponding output.
5. build-windows-reference.cmd compiles with SATURN_RUNTIME pointing to engine/runtime.

```powershell
$env:PYTHONPATH = "$PWD/engine/src"
.\.venv\Scripts\python.exe sotn-experiment/tools/make_all_config.py
.\.venv\Scripts\python.exe sotn-experiment/tools/generate-jp.py sotn-experiment/game-jp-all.toml
$env:SATURN_RUNTIME = "$PWD/engine/runtime"
$env:SOTN_RECOMP_DIR = "$PWD/sotn-experiment/build/jp-all/recomp"
.\sotn-experiment\tools\build-windows-reference.cmd
```

Extraction/symbol prerequisites are real requirements. This workflow has not been certified end to end
on a new machine and does not download game media. Do not publish operational TOML, extracted files
or the resulting C++. Do not run the launcher during code-only testing tasks.

## Graphics without a window

```powershell
.\.venv\Scripts\python.exe scripts/check-gpu.py
```

Uses SOTN_CLANG_CL and SDL3_ROOT and writes evidence only to test-results/. It compiles the project's
drivers with production shaders and compares pixels on D3D12/Vulkan, without SDL_Window or game media.

## Private map markers

scripts/generate-map-markers.py accepts a local JSON list of objects containing inverted (boolean),
x/y (0..63) and kind (1=save,2=teleport). The repository does not contain the real coordinate table.

```powershell
.\.venv\Scripts\python.exe scripts/generate-map-markers.py 'C:/dados-locais/markers.json'
```

This creates engine/runtime/src/host/sotn_map_markers.local.h, which Git ignores. Without the table,
the explored map remains available but does not classify saves/teleports by color.
Tests generate synthetic data in a temporary folder without using the private blueprint.

---

<a id="portugues"></a>

# Instalação e testes

Para o preview de fontes/inicializador e o fluxo de preparação local, veja
[integração com RecompLauncher](RECOMPLAUNCHER.md#portugues), incluindo pré-requisitos externos e limites de validação.

Referências de versão do ambiente local: Windows11, Python3.13, clang-cl21.1.8, MSVC14.44,
SDL3 3.4.14, CMake3.31.6, Ninja1.11.1.4, pytest9.1.1. Os pacotes binários não estão no Git.

## Testes sem abrir jogo ou janela

Prepare Python conforme README e informe as dependências locais. Em PowerShell:

```powershell
$env:SDL3_ROOT = 'C:/dependencias/SDL3-3.4.14'
$env:SOTN_CLANG_CL = 'C:/dependencias/llvm/bin/clang-cl.exe'
$env:SOTN_DECOMP_REFERENCE = 'C:/referencias/sotn-decomp'
.\scripts\check-code.ps1 -Python '.\.venv\Scripts\python.exe'
```

O comando testa os fontes publicados e fixtures sintéticas. Sete integrações que criam janela ou
filho com janela normal são deselecionadas e registradas em test-results/selection.json.
`-FullWindowTests` inclui essas integrações, devendo ser usado só quando autorizado a abrir janelas.
Nenhum dos dois modos deve iniciar a campanha real.

Dois testes de offsets/relíquias consultam o projeto público sotn-decomp como oráculo independente.
Defina SOTN_DECOMP_REFERENCE para um clone local na revisão 2be55ee17221ee306e330edf2df2d49d48ba47ae
(https://github.com/Xeeynamo/sotn-decomp). Não é necessário executar o jogo nem copiar arquivos do
disco para esses dois testes. A referência não foi vendorizada neste repositório; sua licença é AGPL3.
Sem essa dependência, esses testes falham explicitamente; não são pulados ou trocados por mock.

Os testes nativos do runtime podem ser compilados sem SDL e sem ROM:

```powershell
cmake -S engine/runtime/tests -B build/runtime-tests -G Ninja `
  -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl `
  -DSATURN_NO_SDL=ON -DSATURN_VDP1_GPL=ON
cmake --build build/runtime-tests --target runtime_tests video_perf_tests -j 4
ctest --test-dir build/runtime-tests --output-on-failure
```

Execute no ambiente de desenvolvimento do Visual Studio, onde o compilador encontra os headers e libs.
Os testes Python upstream em engine/tests incluem casos Linux-only e um caso de symlink que pode
falhar no Windows por privilégio1314. Não remover marcações nem enfraquecer o teste para aprovar.

## Preparar o jogo localmente

1. Forneça seu disco Saturn JP T-9527G V1.400 em roms/ e confira a identidade com analyze-jp.py.
2. Forneça a cópia externa de sotn-decomp-analise em referencias/, revisão
   2be55ee17221ee306e330edf2df2d49d48ba47ae, e prepare os símbolos/asm Saturn exigidos pelo gerador.
3. Use engine/src no PYTHONPATH. make_all_config.py lê os arquivos locais e prepara game-jp-all.toml.
4. generate-jp.py recebe esse TOML e produz C++ local. Defina SOTN_RECOMP_DIR para a saída correspondente.
5. build-windows-reference.cmd compila com SATURN_RUNTIME apontando a engine/runtime.

```powershell
$env:PYTHONPATH = "$PWD/engine/src"
.\.venv\Scripts\python.exe sotn-experiment/tools/make_all_config.py
.\.venv\Scripts\python.exe sotn-experiment/tools/generate-jp.py sotn-experiment/game-jp-all.toml
$env:SATURN_RUNTIME = "$PWD/engine/runtime"
$env:SOTN_RECOMP_DIR = "$PWD/sotn-experiment/build/jp-all/recomp"
.\sotn-experiment\tools\build-windows-reference.cmd
```

Os pré-requisitos de extração/símbolos são reais. Esse fluxo não foi certificado de ponta a ponta em
máquina nova e não baixa mídia do jogo. Não publicar TOML operacional, extração ou C++ resultante.
Não executar o launcher durante tarefas de teste apenas de código.

## Gráficos sem janela

```powershell
.\.venv\Scripts\python.exe scripts/check-gpu.py
```

Usa SOTN_CLANG_CL e SDL3_ROOT e gera provas só em test-results/. Compila os drivers próprios com
shaders de produção e compara pixels em D3D12/Vulkan, sem SDL_Window ou mídia do jogo.

## Marcadores privados de mapa

scripts/generate-map-markers.py aceita um JSON local com lista de objetos contendo inverted (booleano),
x/y (0..63) e kind (1=save,2=teleporte). Não há tabela real de coordenadas no repositório.

```powershell
.\.venv\Scripts\python.exe scripts/generate-map-markers.py 'C:/dados-locais/markers.json'
```

Isso cria engine/runtime/src/host/sotn_map_markers.local.h, ignorado pelo Git. Sem a tabela,
o mapa explorado continua disponível, mas não classifica saves/teleportes por cores.
Os testes geram dados sintéticos em pasta temporária, sem usar o blueprint privado.
