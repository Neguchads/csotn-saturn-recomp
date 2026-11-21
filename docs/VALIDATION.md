[English](#english) | [Português](#portugues)

<a id="english"></a>

# Evidence and limits

## Original local audit — 26/10/2026

Before export: 205 modules/11020 functions and 606 generated C++ files; Windows build2870742E in a clean destination.
Passed:126 project tests without a window,203 native tests,163 focused SH-2/discovery tests,54 VDP2 tests per backend
with140 compared fields and96 additional D3D12/Vulkan combinations. These counts overlap; do not add them together.
Extended Python suite:238 passed, one upstream skip and one WinError1314 symlink failure. No game was opened in this audit.

This historical evidence does not certify the public snapshot on its own. The public snapshot redirects
paths to engine/, takes external dependencies from environment variables and provides local marker data.
Published tests retain their assertions; real private markers were replaced by an explicit synthetic
fixture only in the test that depends on them.

## Publication — 20/11/2026

New results, run against this checkout's files:

- `python -m unittest discover -s scripts -p 'test_*.py'`: seven tests passed.
- `python -m pytest sotn-experiment/tests -p code_only_plugin -q`:126 passed, seven deselected because of the window restriction.
- `python -m pytest engine/tests/test_sh2.py engine/tests/test_discover.py -q`:158 passed.
- Runtime CMake/CTest in a fresh directory:203 native tests, zero failures; profiler and CTest2/2 passed.
- Separate Release compilation of real `saturn_host`, `saturn_core` and `saturn_machine` objects with SDL3: passed.
- `python scripts/check-gpu.py`: eight offscreen runs passed; CRT/LCD, HD2/4/6/8, FSR, capture,
  VDP2 54 tests/140 fields and96 more combinations on each D3D12/Vulkan backend.
- Public index: path gate, data boundary, Python syntax and credential patterns.
- Gitleaks runs against the final commit before push; its result is recorded in the PR description.

An ImGui keyboard enum comparison was incorrectly classified as generic-api-key.
The file is identical to upstream v1.92.9b, blob7fa2d49e8dfa3d676399b4863126ba9ab1609ca6.
The .gitleaks.toml exception requires the exact path AND the complete expression on that line; a test
checks that a different file or extra content on the line is not accepted. Default rules remain active.

The first run found two tests depending on external sotn-decomp and a hardcoded compiler path.
Environment overrides were fixed without reducing assertions; the entire suite was rerun successfully.
The first export build revealed two omitted extensionless SoftFloat files. They were included with
their notices, and the gate explicitly allowed those two source files; compilation/tests then passed.

Window integrations and the three launcher tests with a WindowStyle Normal child were not run.
Offscreen drivers cover graphics pipelines, not the window contract. Both relic oracles
used the exact public sotn-decomp revision specified in SETUP. No game-derived executable,
dump, save or private evidence package was uploaded. The campaign was not run.

## Not proven

The complete campaign, all transitions/bosses/endings, saving/healing/loading in gameplay and constant60FPS.
DMA with IRQ/external requests and some hardware services/modes remain explicit gaps.
The audio68000 core is interpreted. An x64 binary does not imply static recompilation of all hardware.

## CI and Dependabot integration — 20/11/2026

Thirteen publication/configuration tests passed, including rejection of YAML Python objects,
privileged events, write tokens, event interpolation in shell commands and Actions without a full SHA.
actionlint1.7.12 passed against the real workflow; actionlint/Gitleaks downloads have fixed SHA-256 checksums.
Local CTest rerun:2/2, without game/SDL. The isolated test found Pillow missing from the manifest;
it was then declared in requirements-dev.txt at the local version12.3.0.
The complete code suite was rerun in that isolated environment:126 passed, seven window integrations
deselected under the user's restriction, without changing tests/assertions for this configuration.

GitHub Actions tests Python3.11/3.13, the upstream suite through uv.lock, the native Release runtime
and publication/secrets on Ubuntu24.04. The remote run and verified SHA are recorded in PR #1.
It does not cover SDL/Windows, GPU or the campaign. Earlier local evidence remains separate.

The feature-dev:code-reviewer role was loaded and run in a new Codex session.
The initial import review was inconclusive because reading was partial, with no confirmed finding;
this is not approval. The final review result is recorded in the PR, without inferring approval from tests.

The CI/Dependabot/reviewer slice was approved by the exact role, without a confirmed finding, in diff
351d59c..c8ec6d4. The user authorized merging PR1 after learning the limit of the full review.
[Actions44785895307](https://github.com/Neguchads/csotn-saturn-recomp/actions/runs/44785895307)
passed:13 unittest,94 portable and309 upstream tests on each Python3.11/3.13; runtime/CTest2/2;
actionlint, publication boundaries and Gitleaks. The final documentation adjustment preserves tested sources.

Before merge, the282 export-manifest records were compared with the current private sources:
no source changed since export and none was missing. Documented public adaptations
remain distinct from the private files. This check confirms that selected sources are current,
not that all installation data was copied or that the complete game is certified.

---

<a id="portugues"></a>

# Provas e limites

## Auditoria local de origem — 26/10/2026

Antes da exportação: 205 módulos/11020 funções e 606 C++ gerados; build Windows 2870742E em destino limpo.
Passaram 126 testes de projeto sem janela, 203 nativos, 163 SH-2/descoberta focais, 54 testes VDP2 por backend
com 140 campos comparados e 96 combinações adicionais D3D12/Vulkan. Esses números se sobrepõem; não somar.
Python ampliado: 238 passaram, um skip upstream e uma falha WinError 1314 de symlink. Não foi aberto jogo nessa auditoria.

Essas provas históricas não certificam o snapshot público por si sós. O snapshot público possui
redirecionamento de paths para engine/, dependências externas por ambiente e provedor local de marcadores.
Os testes publicados mantêm os asserts; os marcadores reais privados foram substituídos por fixture
sintética explícita apenas no teste que depende deles.

## Publicação — 20/11/2026

Resultados novos, executados contra os arquivos deste checkout:

- `python -m unittest discover -s scripts -p 'test_*.py'`: sete testes passaram.
- `python -m pytest sotn-experiment/tests -p code_only_plugin -q`: 126 passaram, sete deselecionados por restrição de janela.
- `python -m pytest engine/tests/test_sh2.py engine/tests/test_discover.py -q`: 158 passaram.
- CMake/CTest do runtime, em diretório novo: 203 testes nativos, zero falhas; profiler e CTest 2/2 passaram.
- Compilação Release separada dos objetos reais `saturn_host`, `saturn_core` e `saturn_machine` com SDL3: passou.
- `python scripts/check-gpu.py`: oito execuções offscreen passaram; CRT/LCD, HD2/4/6/8, FSR, captura,
  VDP2 54 testes/140 campos e mais 96 combinações em cada backend D3D12/Vulkan.
- Índice público: gate de caminhos, fronteira de dados, sintaxe Python e padrões de credenciais.
- Gitleaks é executado sobre o commit final antes do push; resultado registrado na descrição do PR.

Uma comparação de enums de teclado do ImGui foi classificada incorretamente como generic-api-key.
O arquivo é idêntico ao upstream v1.92.9b, blob7fa2d49e8dfa3d676399b4863126ba9ab1609ca6.
A exceção de .gitleaks.toml exige o caminho exato E a expressão completa dessa linha; um teste
confere que outro arquivo ou conteúdo acrescentado à linha não é aceito. As regras padrão continuam ativas.

O primeiro ensaio encontrou dois testes dependentes do sotn-decomp externo e um path fixo de compilador.
Corrigidos os overrides de ambiente, sem reduzir os asserts; repetida a suíte inteira com sucesso.
O primeiro build do export revelou dois arquivos SoftFloat sem extensão omitidos. Foram incluídos com
seus avisos e o gate ganhou permissão explícita para os dois fontes; compilação/testes passaram depois.

As integrações de janela e os três testes do launcher com filho WindowStyle Normal não foram executados.
Os drivers offscreen cobrem os pipelines gráficos, não o contrato da janela. Os dois oráculos de relíquias
consultaram a revisão pública exata de sotn-decomp indicada em SETUP. Nenhum executável derivado do jogo,
dump, save ou pacote de evidência privada foi enviado. A campanha não foi executada.

## Não comprovado

Campanha integral, todas as transições/chefes/finais, salvar/curar/carregar em gameplay e 60 FPS constantes.
DMA com IRQ/solicitações externas e alguns serviços/modos do hardware continuam lacunas explícitas.
O núcleo 68000 de áudio é interpretado. Não confundir binário x64 com recompilação estática de todo hardware.

## Integração CI e Dependabot — 20/11/2026

Treze testes de publicação/configuração passaram, incluindo recusa de YAML com objetos Python,
evento privilegiado, token de escrita, interpolação de eventos no shell e Action sem SHA completo.
actionlint 1.7.12 passou no workflow real; downloads de actionlint/Gitleaks têm SHA-256 fixo.
CTest local repetido: 2/2, sem jogo/SDL. O teste isolado encontrou Pillow ausente do manifesto,
agora declarado em requirements-dev.txt com a versão local 12.3.0.
A suíte completa de código foi repetida nesse ambiente isolado: 126 passaram, sete integrações
com janela deselecionadas pela restrição do usuário, sem alterar testes/asserts para essa configuração.

GitHub Actions testa Python 3.11/3.13, a suíte upstream pelo uv.lock, runtime nativo Release e
publicação/segredos em Ubuntu 24.04. A execução remota e o SHA verificado constam no PR #1.
Ela não cobre SDL/Windows, GPU ou campanha. As provas locais anteriores continuam separadas.

O papel feature-dev:code-reviewer foi carregado e executado em uma sessão nova do Codex.
A revisão inicial do import foi inconclusiva por leitura parcial, sem achado confirmado;
isso não é aprovação. Resultado final da revisão é registrado no PR, sem deduzir aprovação dos testes.

A fatia CI/Dependabot/revisor foi aprovada pelo papel exato, sem achado confirmado, no diff
351d59c..c8ec6d4. O usuário autorizou o merge do PR1 após conhecer o limite da revisão integral.
[Actions44785895307](https://github.com/Neguchads/csotn-saturn-recomp/actions/runs/44785895307)
passou: 13 unittest, 94 testes portáveis e 309 upstream em cada Python3.11/3.13; runtime/CTest2/2;
actionlint, fronteiras de publicação e Gitleaks. O ajuste final de documentação preserva os fontes testados.

Antes do merge, os 282 registros do manifesto de exportação foram comparados aos fontes privados
atuais: nenhum fonte alterado desde a exportação e nenhum ausente. As adaptações públicas documentadas
permanecem distintas dos arquivos privados. Essa conferência confirma a atualização dos fontes selecionados,
não uma cópia de todos os dados da instalação nem uma certificação do jogo completo.
