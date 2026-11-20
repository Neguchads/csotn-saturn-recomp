# Decisões

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
