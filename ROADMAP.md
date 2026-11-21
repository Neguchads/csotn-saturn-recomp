# Roadmap

[English](#english) | [Português](#portugues)

<a id="english"></a>

## Done

- Tool updates from PRs2–6 integrated together; FSR integrity protected on Windows checkout.
- Engine, generator, tools, menu and test sources exported with licenses.
- Offline overlay, save, DMA and division fixes; D3D12/Vulkan GPU composition.
- Explicit separation of media, saves and private map data.
- Local windowless test commands and separate gameplay evidence.
- Public snapshot validated locally: code, native runtime, SDL host and offscreen GPU.
- PR CI passed on Python 3.11/3.13, Release runtime and publication/secrets.
- Dependabot pip/uv/Actions and Codex reviewer configured; security alerts enabled.
- Public documentation in English first, followed by Brazilian Portuguese in each file.

## In progress

- Validate the developer RecompLauncher package with the original CUE/BIN on a clean data folder.

- Keep the public source selection synchronized with future local fixes.
- Complete the full static import review; the user authorized merging with this limit recorded.

## Planned

- Complete setup on a new machine with locally supplied media.
- Cover DMA IRQ/external requests and unimplemented hardware services.
- Extend GPU VDP2 to modes still using CPU when measurements justify it.
- Attribute master SH-2 cost per function and confirm FPS during manual campaign play.
- Validate routes, bosses, endings, healing and save/close/load in save rooms.

The port is not yet a complete campaign delivery or a guarantee of constant 60 FPS.

---

<a id="portugues"></a>

# Roadmap

## Feito

- Documentação pública em inglês primeiro e português abaixo no mesmo arquivo.

- Atualizações de ferramentas agrupadas dos PRs2–6; integridade FSR protegida no checkout Windows.

- Exportação de fontes do motor, gerador, ferramentas, menu e testes com licenças.
- Correções offline de overlays, salvamento, DMA e divisão; compositor GPU D3D12/Vulkan.
- Separação explícita de mídia, saves e tabela privada de mapa.
- Comandos locais de teste sem janela e registro separado de gameplay.
- Snapshot público validado localmente: código, runtime nativo, host SDL e GPU offscreen.
- CI de PRs aprovado em Python 3.11/3.13, runtime Release e publicação/segredos.
- Dependabot pip/uv/Actions e revisor Codex configurados; alertas de segurança ativos.

## Em andamento

- Validar o pacote de desenvolvimento RecompLauncher com CUE/BIN original numa pasta de dados limpa.

- Manter a seleção pública sincronizada com as futuras correções locais.
- Completar a revisão estática integral da importação; o usuário autorizou o merge com esse limite registrado.

## Planejado

- Instalação completa em máquina nova com mídia fornecida localmente.
- Cobrir DMA com IRQ/solicitações externas e serviços de hardware ainda não implementados.
- Ampliar VDP2 GPU para modos que ainda usam CPU, quando as medições justificarem.
- Atribuir custo do mestre SH-2 por função e confirmar FPS na campanha manual.
- Validar rotas, chefes, finais, cura e salvar/fechar/carregar nas salas.

O port ainda não é uma entrega de campanha integral nem uma garantia de 60 FPS constantes.
