# Origem das fontes

- saturn-recomp upstream: https://github.com/callumjenkins/saturn-recomp
- Revisão de base: f20de940e8013fc84103a5b39c4069ed8071b1f9.
- Fontes locais escolhidas: candidata da auditoria26/10/2026 que produziu o binário local2870742E.
- Código próprio emsotn-experiment/ e alterações do runtime foram preservados na exportação.
- Manifesto: source-manifest.json registra hashes dos arquivos selecionados; a versão final é regenerada antes do commit.

Adaptações públicas: paths para engine/, dependências de teste por SOTN_CLANG_CL/SDL3_ROOT,
defaults do launcher alinhados à build comum e tabela privada de mapa separada emheaderlocal.
Não houve reescrita de lógica de campanha ou exclusão de testes para obter aprovação.

engine/README.md e engine/THIRD_PARTY.md mantêm a documentação/atribuições upstream, que podem
referir recursos fora da seleção do motor. As instruções válidas deste port estão no README da raiz e docs/SETUP.md.
