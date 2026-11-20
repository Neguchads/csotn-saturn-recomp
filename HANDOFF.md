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
