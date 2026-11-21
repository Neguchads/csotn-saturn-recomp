# Continuity — 2026-11-21

[English](#english) | [Português](#portugues)

<a id="english"></a>

Public publication in Neguchads/csotn-saturn-recomp was explicitly authorized by the user.
This checkout is a separate export. The private installation and personal saves remain on the PC.

Current engine and generator: engine/, derived from saturn-recomp f20de940e8013fc84103a5b39c4069ed8071b1f9.
Overlay residence/CRC, STAGE15/ENDING, save, GPU composition, turbo presentation cadence,
DMA TS3, division overflow and FSR/HD selection fixes are preserved. No game binary is in Git.

Content exception: the public sotn_map_markers.h provides local data support without the private table.
Generate sotn_map_markers.local.h as described in docs/SETUP.md. Do not publish the result.

Next: follow ROADMAP.md and check docs/VALIDATION.md. Do not open game/windows routinely.
The campaign is manual; keep normal room saves. Passing tests do not certify 100% functionality/60 FPS.

Integration on2026-11-20: Python3.11/3.13 CI, Release runtime and publication/secrets passed;
weekly pip/uv/Actions Dependabot, alerts and security updates are enabled. The CI configuration was
approved by feature-dev:code-reviewer. Full import review was inconclusive; the user explicitly
authorized merging PR1 after being informed of that limitation.

Current local sources were compared with the export manifest:282 records, no drift or missing file.
Public differences are identified path, fixture and documentation adaptations. The two extensionless
SoftFloat files were added during export build validation; private map data remains excluded.
Future local installation changes require an explicit GitHub update.

PR7 integrated dependency updates and the Windows FSR LF checkout fix; tests, review and CI passed.
The bilingual documentation update places English first and Portuguese below in the same files.

---

<a id="portugues"></a>

# Continuidade — 20/11/2026

Publicação autorizada explicitamente pelo usuário em Neguchads/csotn-saturn-recomp, com visibilidade pública.
Este checkout é uma exportação separada. A instalação privada e os saves pessoais ficaram no PC.

Motor e gerador atuais em engine/, derivados de saturn-recomp f20de940e8013fc84103a5b39c4069ed8071b1f9.
Preservadas correções de residência/CRC de overlays, STAGE15/ENDING, salvamento, compositor GPU,
cadência do turbo, DMA TS3, overflow da divisão e seleção FSR/HD. Não há binário do jogo no Git.

Exceção de conteúdo: sotn_map_markers.h público é o provedor de dados locais, sem a tabela privada.
Gere sotn_map_markers.local.h conforme docs/SETUP.md. Não publicar o resultado.

Próximo trabalho: seguir ROADMAP.md e conferir docs/VALIDATION.md. Não abrir jogo ou janela por rotina.
Campanha é manual; manter saves normais nas salas. Não alegar 100% funcional/60FPS por suíte verde.

Integração de20/11/2026: CI de Python3.11/3.13, runtime Release e publicação/segredos aprovado;
Dependabot pip/uv/Actions semanal, alertas e atualizações de segurança ativos. A configuração de CI
foi aprovada pelo subagente feature-dev:code-reviewer. A revisão integral do import ficou inconclusiva;
o usuário autorizou explicitamente o merge do PR1 após esse limite ser informado.

Conferência dos fontes locais atuais contra o manifesto de exportação: 282 registros sem drift ou
arquivo ausente. Diferenças públicas são adaptações de caminhos, fixtures e documentação identificadas.
Os dois arquivos SoftFloat sem extensão foram acrescentados no build do export; marcadores privados
continuam excluídos. Mudanças futuras na instalação local precisam de envio explícito para o GitHub.

PR7 integrou atualizações de dependências e a correção LF do FSR no Windows; testes, revisão e CI passaram.
A atualização bilíngue coloca inglês primeiro e português abaixo nos mesmos arquivos.
