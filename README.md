# csotn-saturn-recomp

<p align="center">
  <a href="https://recomplauncher.com/game/csotn-saturn-recomp"><picture><source media="(prefers-color-scheme: dark)" srcset="https://recomplauncher.com/assets/press/launcher/badge-light.svg"><img src="https://recomplauncher.com/assets/press/launcher/badge-dark.svg" alt="Get it on RecompLauncher" height="48"></picture></a>
  <a href="https://recompstore.com/dev/neguchads"><img src="https://recompstore.com/assets/press/verified-dark.svg" alt="Verified developer on RecompStore" height="48"></a>
</p>

[English](#english) | [Português](#portugues)

<a id="english"></a>

**How to prepare:** bring your original Japanese Saturn T-9527G V1.400 (Japan 2M) disc. See [RecompLauncher setup](docs/RECOMPLAUNCHER.md); this developer preview requires external build tools.

Experimental **Castlevania: Symphony of the Night, Sega Saturn JP** port for Windows x64.
SH-2 code is translated into C++ before compilation; the runtime reconstructs Saturn devices.
The goal is to make the engine, tools and fixes reproducible and editable.

**Work in progress. A complete campaign, universal stability and constant 60 FPS have not been certified.**
No game, BIOS, game executable, disc-derived C++, extracted files, audio, images or saves are included.
You must supply your own media locally. No ROM downloads are performed.

## Repository contents

- `engine/`: engine sources, SH-2 generator and tests, including local overlay, normal save,
  DMA/division, developer menu and GPU VDP2 composition fixes.
- `sotn-experiment/`: preparation/generation/build tools, launchers, configuration and synthetic tests.
- `scripts/`: publication gate, windowless tests and local generation of private map markers.
- `docs/`: setup, decisions, manuals and limits of the available evidence.

VDP2 compute and presentation support **Vulkan and DirectX 12**. Vulkan was preferred based on
local offscreen benchmark medians; DirectX 12 remains available. Graphics modes not covered by
compute use CPU composition. Audio still uses SCSP and an interpreted Musashi 68000 core.
The developer menu provides pause, frame advance, graphics settings, controls, optional hacks
and bug reports. Normal saving remains in the game's save rooms.

## Getting started

Windows 11, PowerShell and Python 3.11 or later. LLVM/clang-cl, MSVC Build Tools, CMake,
Ninja and SDL3 are required to build and run native tests.

```powershell
git clone https://github.com/Neguchads/csotn-saturn-recomp.git
cd csotn-saturn-recomp
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements-dev.txt
$env:PYTHONPATH = "$PWD/engine/src"
```

The [setup and test commands](docs/SETUP.md) use locally supplied dependencies. Full provisioning
on a new machine and generation from a fresh disc copy still require validation.
This repository is not a ready-to-play release. Disc extraction and symbol/ASM preparation remain
prerequisites; the guide does not yet provide a validated end-to-end fresh-machine procedure.

For the source/bootstrap developer preview, prerequisites and local player compilation, see
[RecompLauncher integration](docs/RECOMPLAUNCHER.md#english). It is not a published, ready-to-play package.

## Review and dependencies

[GitHub Actions](.github/workflows/ci.yml) checks PRs, including Dependabot PRs: Python 3.11/3.13,
native runtime without SDL, publication, workflow syntax and secrets. It does not open the game
or certify GPU behavior or the campaign.
[Dependabot](.github/dependabot.yml) checks Python, `engine/uv.lock` and Actions weekly,
with up to three open PRs per ecosystem. Its configuration is active on the default branch.

The Codex [`feature-dev:code-reviewer`](.codex/agents/feature-dev-code-reviewer.toml) role is included
and inherits the session model. Start a new session after installing the file to load the role.
This is assisted review separate from tests, not a GitHub Actions job or automatic merging.

## Evidence and differences from the local installation

The source snapshot comes from the local audit of 2026-10-26: 205 configured modules and 11020
functions; Windows x64 build 2870742E was produced locally. Generated files and that executable
are not included. The public export is tested separately; see [VALIDATION](docs/VALIDATION.md).
A successful build does not demonstrate a complete campaign.

The room marker table came from a private blueprint and **is not published**. Explored-map display
and navigation remain implemented; save/teleport colors require a local user-supplied table.
The generator accepts JSON and creates a Git-ignored header. Integration tests use two synthetic
markers while retaining the previous assertions.

## License and provenance

Project code: [GPL-3.0-or-later](LICENSE). The engine preserves its [MIT license](engine/LICENSE)
and [third-party notices](engine/THIRD_PARTY.md); the optional VDP1 rasterizer follows Mednafen under GPL2+.
See [THIRD_PARTY](THIRD_PARTY.md) and [source provenance](docs/SOURCES.md).
Tool licenses do not grant rights to game files.

Not affiliated with Konami or Sega. Developed with AI assistance; tests and limitations are recorded
without treating generation or compilation as gameplay evidence.

---

<a id="portugues"></a>

**Como preparar:** use seu disco japonês original Saturn T-9527G V1.400 (Japan 2M). Veja a [preparação RecompLauncher](docs/RECOMPLAUNCHER.md); esta prévia exige ferramentas externas.

# csotn-saturn-recomp

Port experimental de **Castlevania: Symphony of the Night, Sega Saturn JP**, para Windows x64.
O SH-2 é traduzido para C++ antes da compilação; o runtime reconstrói os dispositivos do Saturn.
O objetivo é tornar o motor, as ferramentas e as correções reproduzíveis e editáveis.

**Projeto em desenvolvimento. Campanha integral, estabilidade universal e 60 FPS constantes não estão certificados.**
Não inclui jogo, BIOS, executável do jogo, C++ traduzido do disco, extrações, áudio, imagens ou saves.
O usuário precisa fornecer sua própria mídia localmente. Nenhum download de ROM é feito.

## O que está salvo aqui

- `engine/`: fontes do motor, gerador SH-2 e testes, com as correções locais de overlays,
  salvamento normal, DMA/divisão, menu de desenvolvimento e composição VDP2 na GPU.
- `sotn-experiment/`: ferramentas de preparação/geração/build, launchers, configurações e testes sintéticos.
- `scripts/`: gate de publicação, testes sem janela e geração local dos marcadores privados.
- `docs/`: instalação, decisões, manual e limites das provas existentes.

VDP2 compute e apresentação suportam **Vulkan e DirectX 12**. Vulkan foi preferido nas medianas
dos ensaios offscreen locais; DirectX 12 continua disponível. Modos gráficos não cobertos pelo compute
usam o compositor CPU. O áudio ainda utiliza SCSP e o núcleo Musashi 68000 interpretado.
O menu de desenvolvimento oferece pausa, avanço de quadro, configuração gráfica, controles,
hacks opcionais e relatórios de bug. O salvamento normal continua nas salas do jogo.

## Começar

Windows 11, PowerShell e Python 3.11 ou posterior. LLVM/clang-cl, MSVC Build Tools, CMake,
Ninja e SDL3 são necessários para compilar e executar os testes nativos.

```powershell
git clone https://github.com/Neguchads/csotn-saturn-recomp.git
cd csotn-saturn-recomp
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements-dev.txt
$env:PYTHONPATH = "$PWD/engine/src"
```

Os comandos de [instalação e testes](docs/SETUP.md) usam dependências fornecidas localmente.
O provisionamento inteiro de uma máquina nova e a geração a partir de uma nova cópia do disco ainda
precisam de validação. Este repositório não é uma release pronta para jogar. A extração do disco e a preparação de símbolos/ASM
continuam como pré-requisitos; ainda falta um procedimento completo validado em máquina nova.

Para o preview de fontes/inicializador, pré-requisitos e compilação local pelo jogador, veja
[integração com RecompLauncher](docs/RECOMPLAUNCHER.md#portugues). Não é um pacote publicado pronto para jogar.

## Revisão e dependências

[GitHub Actions](.github/workflows/ci.yml) verifica PRs, inclusive os do Dependabot: Python 3.11/3.13,
runtime nativo sem SDL, publicação, sintaxe do workflow e segredos. Não abre o jogo nem certifica GPU ou campanha.
[Dependabot](.github/dependabot.yml) verifica Python, `engine/uv.lock` e Actions semanalmente,
com até três PRs abertos por ecossistema. A configuração está ativa na branch padrão.

O papel Codex [`feature-dev:code-reviewer`](.codex/agents/feature-dev-code-reviewer.toml) está incluído
no projeto e herda o modelo da sessão. Abra uma sessão nova após instalar o arquivo para carregar o papel.
É revisão assistida separada dos testes; não é um job do GitHub Actions nem faz merge automático.

## Provas e diferenças da cópia local

O snapshot de código vem da auditoria local de 26/10/2026: perfil com 205 módulos e 11020 funções
configurados, build Windows x64 2870742E produzida localmente. Esses arquivos gerados e esse executável
não acompanham o repositório. A exportação pública é testada separadamente; resultados em
[VALIDATION](docs/VALIDATION.md). Build não demonstra uma campanha completa.

A tabela de marcadores de salas veio de um blueprint privado e **não está publicada**.
O mapa explorado e sua navegação continuam implementados; cores de save/teleporte exigem a tabela
local fornecida pelo usuário. O gerador aceita JSON e cria um header ignorado pelo Git.
Os testes dessa integração usam dois marcadores sintéticos, mantendo as verificações anteriores.

## Licença e origem

Código próprio: [GPL-3.0-or-later](LICENSE). O motor preserva [licença MIT](engine/LICENSE)
e [avisos de terceiros](engine/THIRD_PARTY.md); o rasterizador VDP1 opcional segue Mednafen sob GPL2+.
Veja [THIRD_PARTY](THIRD_PARTY.md) e [origem das alterações](docs/SOURCES.md).
Nenhuma licença das ferramentas concede direitos sobre arquivos do jogo.

Não afiliado à Konami ou à Sega. Trabalho desenvolvido com assistência de IA; testes e limitações
são registrados sem converter geração ou compilação em prova de gameplay.

## Automação de CI e distribuição — Etapa 2

As verificações de navegador, as propostas de versão e os artefatos de
integridade estão descritos em [docs/CI_AUTOMATION.md](docs/CI_AUTOMATION.md).
A publicação de releases permanece sujeita a revisão humana.
