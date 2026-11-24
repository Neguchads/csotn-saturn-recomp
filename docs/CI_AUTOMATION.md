[English](#english) | [Português](#portugues)

<a id="english"></a>

# Stage 2 — CI and integrity automation

The `Package integrity and SBOM (manual)` workflow is available via GitHub Actions →
Run workflow. It checks the published source boundary using the repository's
existing publication gate, calculates SHA-256 hashes of eligible public source
files, generates an SPDX-JSON SBOM, creates an evidence-only archive, and signs
both the archive provenance and its SBOM with GitHub Artifact Attestations.

**No commercial Saturn disc, ROM, game executable, generated recompiled code,
BIOS, private symbols, or game assets are packaged.** The workflow does not
create a GitHub Release and cannot certify gameplay, Vulkan, or DirectX behavior.

The existing source-only native runtime tests (including backup saves),
Python tests, and Zizmor/CodeQL continue to run in pull requests. Full native
packages need a separately reviewed clean-room build and legally supplied disc.

To verify a downloaded archive, use `gh attestation verify ./source-evidence.tar.gz
-R Neguchads/csotn-saturn-recomp` and
`sha256sum -c source-evidence.tar.gz.sha256` in the evidence directory.

<a id="portugues"></a>

# Etapa 2 — Automação da CI e integridade

O workflow `Package integrity and SBOM (manual)` pode ser executado em
GitHub Actions → Run workflow. Ele valida a fronteira de publicação usando
os verificadores existentes, calcula SHA-256 de arquivos de código público,
gera um inventário SPDX-JSON (SBOM), cria um pacote somente de evidências
e assina a origem do pacote e a SBOM com Artifact Attestations do GitHub.

**Nenhum disco comercial de Saturn, ROM, executável do jogo, código recompilado
extraído, BIOS, símbolos privados ou arquivos do jogo entram no pacote.**
Esse workflow não publica releases e não comprova funcionamento de todo o jogo,
do Vulkan ou do DirectX. Os testes existentes de runtime, saves, Python,
CodeQL e Zizmor continuam independentes deste empacotamento manual.

Pacotes executáveis completos exigem revisão específica, build limpo e
mídia legítima do usuário. Para conferir o arquivo, use
`gh attestation verify ./source-evidence.tar.gz -R Neguchads/csotn-saturn-recomp`
e `sha256sum -c source-evidence.tar.gz.sha256` após baixar as evidências.
