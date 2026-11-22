[English](#english) | [Português](#portugues)

<a id="english"></a>

## SDK settings and CHD preparation — 2026-11-21

Map existing display/audio INI keys in the launcher instead of changing the runtime or defaults. Do not expose VSync or GPU renderer controls that are not proven in this launch path. CHD conversion is optional, external and only runs during explicit preparation; CUE behavior is preserved. Keep schema validation and package inclusion separate from launcher UI/gameplay validation. No verified whole-disc CRC is invented from the12 program hashes.


## RecompLauncher developer package — 2026-11-21

Ship public sources and our own bootstrap only. The existing saturn.exe incorporates translated disc functions and constants, so it stays local under the project publication boundary. Preparation fetches pinned public references, validates the original Japan disc and generates/compiles locally. Saves stay separate under {data}/saves; preparation never starts the game. Python, Git, Rust, MSVC, LLVM, CMake, Ninja and SDL remain explicit external prerequisites for this developer preview. Do not publish a ready-to-play release or request catalog inclusion until fresh-media preparation is validated.


# Decisions

## Bilingual public documentation — 2026-11-21

Keep complete English and Brazilian Portuguese versions in the same file, English first. Language anchors avoid separate copies drifting apart. An automated gate checks section order, fences and local links. Upstream license notices and command blocks retain their meaning. This translation does not complete or validate missing disc extraction and symbol preparation steps.

## CI and Codex review — 20/11/2026

Dependabot updates pip, uv.lock and Actions; PRs use the same CI without secrets. Standard Ubuntu
runners in a public repository avoid cost and do not need game files. Actions are pinned by SHA,
tools by checksum, and the token can only read contents. GPU/Windows and gameplay retain separate local evidence.

PyYAML6.0.3 is a development-only dependency for validating the real YAML structure with
SafeLoader. uv is in requirements-ci.txt so Dependabot can also update it.
Pillow12.3.0 makes explicit the dependency already used when collecting capture tests; the isolated
environment revealed its absence from the manifest. It is a test dependency, not part of the game runtime.

Recreating feature-dev:code-reviewer in TOML lets Codex reuse the review without choosing
another model. The session inherits its permissions; instructions require reading and do not authorize publication.
Catalogs loaded before installation require a new session. Tests do not replace this review.

## Public publication — 20/11/2026

The user explicitly authorized creating Neguchads/csotn-saturn-recomp and requested public visibility.
The isolated export preserves the installation, previous Git index and personal saves.
Keeping project sources and runtime/licenses allows recovering the work without redistributing the game.

Vendoring the engine in engine/ pins its revision and keeps patches visible. Do not use a submodule
pointing to upstream without patches, because that would lose fixes. Preserve MIT/GPL and third-party notices.

The map table derived from the private blueprint stays local. A validated generator and optional
provider preserve the data boundary and make the functional difference without the header explicit.
Tests use synthetic data and do not remove assertions to hide this dependency.

Vulkan remains Auto's preference based on local medians; D3D12 remains supported.
Do not change clocks/HD/CPU to promise60FPS. The GPU works on snapshots; RAM retains data,
and logic with write dependencies remains ordered.

## Combined dependencies and FSR checkout — 20/11/2026

Integrating PRs2–6 into one branch allows testing pytest9.2.0, uv0.13.9, Pillow13 and Actions7 together.
The local test revealed automatic CRLF conversion in FSR sources: pinning LF preserves upstream hashes.
A new test exercises real checkout-index with autocrlf=true; no previous assertion was changed.

---

<a id="portugues"></a>

## Configurações SDK e preparação CHD — 21/11/2026

Mapear chaves INI de tela/áudio existentes no launcher sem mudar runtime ou padrões. Não expor VSync ou seletor GPU sem prova nesse caminho de execução. A conversão CHD é externa, opcional e só roda na preparação explícita; comportamento CUE preservado. Schema e presença no ZIP não equivalem a validação da interface/gameplay. Não inventar CRC de disco inteiro a partir dos12 hashes por programa.


## Pacote de desenvolvimento RecompLauncher — 21/11/2026

Distribuir apenas fontes públicos e nosso próprio inicializador. O saturn.exe existente incorpora funções e constantes traduzidas do disco, portanto permanece local pela fronteira de publicação do projeto. A preparação obtém referências públicas fixadas, valida o disco japonês original e gera/compila localmente. Saves ficam separados em {data}/saves; preparar nunca inicia o jogo. Python, Git, Rust, MSVC, LLVM, CMake, Ninja e SDL são pré-requisitos externos explícitos desta prévia. Não publicar release pronta para jogar nem pedir inclusão no catálogo antes de validar a preparação com mídia real.


# Decisões

## Documentação pública bilíngue — 21/11/2026

Manter versões completas em inglês e português do Brasil no mesmo arquivo, inglês primeiro. Âncoras de idioma evitam cópias separadas divergentes. Um gate verifica ordem, blocos de código e links locais. Avisos de licença upstream e comandos preservam o sentido. A tradução não completa nem valida as etapas ainda ausentes de extração do disco e preparação de símbolos.

## CI e revisão Codex — 20/11/2026

Dependabot atualiza pip, uv.lock e Actions; PRs usam o mesmo CI sem secrets. Runners Ubuntu padrão
de repositório público evitam custo e não precisam dos arquivos do jogo. Actions são fixadas por SHA,
ferramentas por checksum e o token só lê o conteúdo. GPU/Windows e gameplay conservam provas locais separadas.

PyYAML 6.0.3 entra apenas nas dependências de desenvolvimento para validar a estrutura YAML real com
SafeLoader. uv fica em requirements-ci.txt para também receber atualizações do Dependabot.
Pillow 12.3.0 explicita a dependência já usada na coleta dos testes de captura; o ambiente isolado
revelou que ela faltava no manifesto. É dependência de testes, não do runtime do jogo.

Recriar o papel feature-dev:code-reviewer em TOML permite reutilizar a revisão no Codex sem escolher
outro modelo. A sessão herda suas permissões; as instruções exigem leitura e não autorizam publicação.
Catálogos carregados antes da instalação requerem uma sessão nova. Testes não substituem essa revisão.

## Publicação pública — 20/11/2026

O usuário autorizou explicitamente criar Neguchads/csotn-saturn-recomp e pediu visibilidade pública.
A exportação é isolada para preservar a instalação, índice Git anterior e saves pessoais.
Salvar fontes próprios e runtime/licenças permite recuperar o trabalho sem redistribuir o jogo.

Vendorizar o motor em engine/ fixa a revisão e mantém os patches visíveis. Não usar submódulo
apontando ao upstream sem patches, pois isso perderia correções. Manter avisos MIT/GPL e terceiros.

A tabela de mapa derivada do blueprint privado fica local. Um gerador validado e um provedor
opcional preservam a fronteira de dados e deixam explícita a diferença funcional sem o header.
Os testes usam dados sintéticos e não removem asserts para esconder essa dependência.

Vulkan permanece preferência de Auto com base nas medianas locais; D3D12 continua suportado.
Não alterar relógios/HD/CPU para prometer60FPS. GPU trabalha sobre snapshots; RAM conserva dados,
e a lógica com dependências de escrita continua ordenada.

## Dependências em conjunto e checkout FSR — 20/11/2026

Integrar os PRs2–6 em uma branch permite testar pytest9.2.0, uv0.13.9, Pillow13 e Actions7 juntos.
O teste local revelou CRLF automático nos fontes FSR: fixar LF nesses arquivos preserva os hashes upstream.
Um teste novo exercita checkout-index real com autocrlf=true; nenhum assert anterior foi alterado.
