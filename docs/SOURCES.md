[English](#english) | [Português](#portugues)

<a id="english"></a>

# Source provenance

- saturn-recomp upstream: https://github.com/callumjenkins/saturn-recomp
- Base revision: f20de940e8013fc84103a5b39c4069ed8071b1f9.
- Selected local sources: the26/10/2026 audit candidate that produced local binary2870742E.
- Project code in sotn-experiment/ and runtime changes were preserved in the export.
- Manifest: source-manifest.json records hashes of selected files; the final version is regenerated before commit.

Public adaptations: paths to engine/, test dependencies through SOTN_CLANG_CL/SDL3_ROOT,
launcher defaults aligned with the common build and the private map table separated into a local header.
Campaign logic was not rewritten and tests were not removed to obtain approval.

engine/README.md and engine/THIRD_PARTY.md retain upstream documentation/attribution, which may
refer to features outside the selected engine files. This port's applicable instructions are in the root README and docs/SETUP.md.

---

<a id="portugues"></a>

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
