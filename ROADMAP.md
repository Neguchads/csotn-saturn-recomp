# Roadmap

## Feito

- Exportação de fontes do motor, gerador, ferramentas, menu e testes com licenças.
- Correções offline de overlays, salvamento, DMA e divisão; compositor GPU D3D12/Vulkan.
- Separação explícita de mídia, saves e tabela privada de mapa.
- Comandos locais de teste sem janela e registro separado de gameplay.
- Snapshot público validado localmente: código, runtime nativo, host SDL e GPU offscreen.
- CI de PRs aprovado em Python 3.11/3.13, runtime Release e publicação/segredos.
- Dependabot pip/uv/Actions e revisor Codex configurados; alertas de segurança ativos.

## Em andamento

- Manter a seleção pública sincronizada com as futuras correções locais.
- Completar a revisão estática integral da importação; o usuário autorizou o merge com esse limite registrado.

## Planejado

- Instalação completa em máquina nova com mídia fornecida localmente.
- Cobrir DMA com IRQ/solicitações externas e serviços de hardware ainda não implementados.
- Ampliar VDP2 GPU para modos que ainda usam CPU, quando as medições justificarem.
- Atribuir custo do mestre SH-2 por função e confirmar FPS na campanha manual.
- Validar rotas, chefes, finais, cura e salvar/fechar/carregar nas salas.

O port ainda não é uma entrega de campanha integral nem uma garantia de 60 FPS constantes.
