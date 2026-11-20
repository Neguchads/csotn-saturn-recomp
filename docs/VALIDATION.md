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
