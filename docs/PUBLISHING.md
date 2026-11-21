[English](#english) | [Português](#portugues)

<a id="english"></a>

# Publication

Authorized destination: https://github.com/Neguchads/csotn-saturn-recomp, public, account Neguchads.
Explicit authorization on20/11/2026 supersedes the historical publication ban for this task.

Included: engine/ sources/licenses, generator, synthetic tests, tools and documentation.
Excluded: ROM/BIOS/extraction, disc-translated C++, saves, dumps, media, private blueprint/table,
credentials, environments and executables. The licensed Inter fonts are two binary build exceptions.

Before each upload: review the index/diff, run scripts/check_repository.py, code tests,
runtime build and Gitleaks. Do not open the game to satisfy publication checks. A blocked privilege is
not a passing test. Actions uses only standard runners in a public repository, without secrets or uploads.
Dependabot checks pip, uv and Actions on Mondays at06:00 São Paulo time. Dependency PRs use
the same workflow as other PRs; updates are not automatically merged.

The GitHub bootstrap created only README/base main; PR1 contains the import and its CI integration.
PRs, review and gate limits remain explicit. Do not present the import as a playable release.

The Codex reviewer is in `.codex/agents/feature-dev-code-reviewer.toml`, with the exact name
`feature-dev:code-reviewer`. A new session must load the catalog; existing sessions do not acquire
the role retroactively. An inconclusive review keeps the PR open until the user's explicit decision.
For PR1, the user authorized merge after learning that the full review was inconclusive;
approval of the CI configuration remains a separately scoped review.

Check the integration locally (PowerShell):

```powershell
.\test-results\ci-venv\Scripts\python.exe -m pip install -r requirements-ci.txt
.\test-results\ci-venv\Scripts\python.exe -m unittest discover -s scripts -p 'test_*.py'
.\test-results\ci-venv\Scripts\python.exe scripts/check_ci_config.py
```

Use an already created local Python environment; the path above was used to validate this configuration.
`actionlint` validates the real workflow; `scripts/check_ci_config.py` and its tests protect events,
permissions, SHAs, checkout without credentials and coverage of all three Dependabot managers.

---

<a id="portugues"></a>

# Publicação

Destino autorizado: https://github.com/Neguchads/csotn-saturn-recomp, público, conta Neguchads.
Autorização explícita de20/11/2026 substitui a proibição histórica de publicação para esta tarefa.

Incluídos: engine/ com fontes/licenças, gerador, testes sintéticos, ferramentas e documentação.
Excluídos: ROM/BIOS/extração, C++ traduzido do disco, saves, dumps, mídia, blueprint/tabela privada,
credenciais, ambientes e executáveis. As fontes Inter licenciadas são duas exceções binárias de build.

Antes de cada envio: revisar índice/diff, rodar scripts/check_repository.py, testes de código,
build do runtime e Gitleaks. Não abrir o jogo para satisfazer publicação. Privilégio bloqueado não é
teste aprovado. Actions usa apenas runners padrão em repositório público, sem secrets e sem uploads.
Dependabot verifica pip, uv e Actions às segundas, 06:00 de São Paulo. PRs de dependências passam pelo
mesmo workflow dos demais PRs; atualizações não recebem merge automático.

Bootstrap do GitHub criou apenas README/base main; o PR1 contém a importação e sua integração de CI.
PR, revisão e limites dos gates ficam explícitos. Não apresentar a importação como release jogável.

O revisor Codex está em `.codex/agents/feature-dev-code-reviewer.toml`, com o nome exato
`feature-dev:code-reviewer`. Uma sessão nova precisa carregar o catálogo; sessões abertas não ganham
o papel retroativamente. Uma revisão inconclusiva mantém o PR aberto até decisão explícita do usuário.
No PR1, o usuário autorizou o merge após conhecer a revisão integral inconclusiva;
a aprovação da configuração de CI continua sendo uma revisão de escopo separado.

Conferência local da integração (PowerShell):

```powershell
.\test-results\ci-venv\Scripts\python.exe -m pip install -r requirements-ci.txt
.\test-results\ci-venv\Scripts\python.exe -m unittest discover -s scripts -p 'test_*.py'
.\test-results\ci-venv\Scripts\python.exe scripts/check_ci_config.py
```

Use um ambiente Python local já criado; o caminho acima foi usado na validação desta configuração.
`actionlint` valida o workflow real; `scripts/check_ci_config.py` e seus testes protegem eventos,
permissões, SHAs, checkout sem credenciais e a cobertura dos três gerenciadores do Dependabot.
