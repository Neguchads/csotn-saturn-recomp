# Decisões

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
