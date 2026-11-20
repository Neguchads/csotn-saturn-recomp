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

Bootstrap do GitHub criou apenas README/base main; importação trabalha em codex/import-project.
PR, revisão e limites dos gates ficam explícitos. Não apresentar a importação como release jogável.

O revisor Codex está em `.codex/agents/feature-dev-code-reviewer.toml`, com o nome exato
`feature-dev:code-reviewer`. Uma sessão nova precisa carregar o catálogo; sessões abertas não ganham
o papel retroativamente. Uma revisão inconclusiva mantém o PR aberto.

Conferência local da integração (PowerShell):

```powershell
.\test-results\ci-venv\Scripts\python.exe -m pip install -r requirements-ci.txt
.\test-results\ci-venv\Scripts\python.exe -m unittest discover -s scripts -p 'test_*.py'
.\test-results\ci-venv\Scripts\python.exe scripts/check_ci_config.py
```

Use um ambiente Python local já criado; o caminho acima foi usado na validação desta configuração.
`actionlint` valida o workflow real; `scripts/check_ci_config.py` e seus testes protegem eventos,
permissões, SHAs, checkout sem credenciais e a cobertura dos três gerenciadores do Dependabot.
