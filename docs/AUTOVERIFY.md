[English](#english) | [Português](#portugues)

<a id="english"></a>

# NAVI AutoVerify — final release only

This optional integration verifies the exact authorized, non-commercial
public distribution asset of a stable release (vX.Y.Z). No PR, ordinary
push, nightly, beta, RC, developer build, ROM, BIOS or private artifact
is eligible. No upload, rescan, automatic publication or execution.

## Configure once after reviewing and merging

1. In Settings > Secrets and variables > Actions > Variables, set
   NAVI_RELEASE_ASSET to the exact final delivery filename, or a filename
   template containing {tag}. Leave it absent to disable automatic runs.
2. For VirusTotal lookup only, after confirming the free API terms and
   rights to the artifact, add repository secret VT_API_KEY and set
   NAVI_AUTOVERIFY_ENABLED to true. If unset, the automatic job checks
   integrity only. Never place keys in code or arguments.
3. Publish the reviewed stable release with the asset already attached.
   The release published event selects only that configured asset.
   A missing digest or mismatched bytes is a failure.
4. For an existing stable release, manually run AutoVerify final release
   from the default branch. Enter the exact tag, filename and independently
   verified SHA-256. The external query checkbox defaults to false.
5. Download integrity.json and, when opted in, virustotal.json from the
   Actions run. Only one selected file is checked. ZIP hashes do not
   establish the status of each executable inside.

Automatic verification compares GitHub asset metadata and downloaded bytes;
it is not independent build provenance. The published check is post-delivery,
not a publication gate. GitHub releases created using GITHUB_TOKEN may not
trigger further workflows; use a trusted manual dispatch in that case.

## Final build before publishing

For locally built packages, use the reviewed Core revision below and the
exact SHA-256 recorded by your final build receipt/checksum. Run only on the
final delivery package. The second command requires VT_API_KEY in the
process environment and explicit non-commercial authorization.

```powershell
git -C PATH_TO_NAVI_CORE rev-parse HEAD
python -I PATH_TO_NAVI_CORE/navi_core/autoverify.py PATH_TO_FINAL_PACKAGE --expected-sha256 EXPECTED_SHA256 --lookup-public-noncommercial
```

Review the script and verify the checkout has the approved revision and no
local modifications. Exit 0 is a matching recent zero-detection report,
not a malware-free guarantee; exit 2 means unverified/review required.
Use independent local antivirus and provenance checks. An unknown hash
requires separate authorized submission/review; this integration never
uploads files. Do not query the same hash twice automatically: choose the
pre-delivery check OR enable the post-publication VirusTotal query.

A tag-triggered CI release pipeline can instead call the Core reusable
autoverify.yml job with final_delivery=true, expected_sha256 and the
explicit approved_public_noncommercial input. See Core docs. Never hook
the integration into normal builds or PR tests.

## Quota and status

VirusTotal public API: 4 requests/minute, 500/day, account-wide; no
commercial use or substitution for antivirus. Serialize external lookups
across repositories; repository concurrency is not an account quota
manager. Stop on quota errors. Do not upload unauthorized material.
This integration has no paid-service fallback.

Core pin: c6037185830f24770f856d4cd69791c686095bc5, CI passed; independent review pending.
[Core PR #36](https://github.com/Neguchads/navi-core/pull/36).
[API conditions](https://docs.virustotal.com/reference/public-vs-premium-api).

---

<a id="portugues"></a>

# NAVI AutoVerify — somente release final

Esta integração opcional verifica o artefato exato de distribuição pública,
autorizada e não comercial de uma versão estável (vX.Y.Z). PRs, pushes
comuns, nightly, beta, RC, builds de desenvolvimento, ROMs, BIOS e
artefatos privados ficam excluídos. Não há upload, reanálise, publicação
automática ou execução do artefato.

## Configuração única após revisão e merge

1. Em Settings > Secrets and variables > Actions > Variables, configure
   NAVI_RELEASE_ASSET com o nome exato do pacote final ou um modelo contendo
   {tag}. Deixe ausente para desativar a execução automática.
2. Para consultar o VirusTotal, após confirmar os termos gratuitos e os
   direitos sobre o artefato, adicione o secret VT_API_KEY e configure
   NAVI_AUTOVERIFY_ENABLED como true. Sem isso, o job automático verifica
   somente a integridade. Nunca coloque chaves no código ou em argumentos.
3. Publique a versão estável revisada com o pacote já anexado. O evento
   release published seleciona somente o artefato configurado. Digest
   ausente ou bytes divergentes causam falha.
4. Para release estável existente, execute manualmente AutoVerify final
   release na branch padrão. Informe tag, nome exato e SHA-256 verificado
   independentemente. A consulta externa é desativada por padrão.
5. Baixe integrity.json e, quando autorizado, virustotal.json nos artefatos
   do Actions. Só um arquivo selecionado é verificado. O hash de um ZIP
   não demonstra a situação de cada executável dentro dele.

A verificação automática compara os metadados do GitHub com os bytes
baixados; não é prova independente de procedência do build. A verificação
da release publicada é posterior à entrega, não um bloqueio da publicação.
Releases criadas com GITHUB_TOKEN podem não disparar outros workflows;
nesse caso use execução manual confiável.

## Build final antes da publicação

Para pacotes montados localmente, use a revisão do Core indicada abaixo e
o SHA-256 exato registrado no recibo/checksum do build final. Execute
somente no pacote final de entrega. O segundo comando requer VT_API_KEY no
ambiente do processo e autorização explícita para uso não comercial.

```powershell
git -C PATH_TO_NAVI_CORE rev-parse HEAD
python -I PATH_TO_NAVI_CORE/navi_core/autoverify.py PATH_TO_FINAL_PACKAGE --expected-sha256 EXPECTED_SHA256 --lookup-public-noncommercial
```

Revise o script e confirme a revisão aprovada e a ausência de alterações
locais. Saída 0 indica relatório recente correspondente com zero detecções,
não garantia de ausência de malware; saída 2 exige revisão ou indica
ausência de verificação. Use antivírus local e provas de origem independentes.
Hash desconhecido requer envio autorizado separado e revisão; a integração
nunca envia arquivos. Escolha a consulta pré-entrega OU a consulta após
publicação, evitando repetir automaticamente o mesmo hash.

Um pipeline de release disparado por tag pode chamar o job reutilizável
autoverify.yml do Core com final_delivery=true, expected_sha256 e a entrada
approved_public_noncommercial explícita. Consulte a documentação do Core.
Nunca conecte a integração aos builds comuns ou testes de PR.

## Cota e situação

API pública do VirusTotal: 4 requisições/minuto, 500/dia por conta, sem
uso comercial nem substituição do antivírus. Serialize consultas entre
repositórios; a concorrência de um repositório não controla a cota da conta.
Pare em erros de cota. Não envie material sem autorização. Não há
fallback pago.

Revisão do Core: c6037185830f24770f856d4cd69791c686095bc5, CI aprovado,
revisão independente pendente.
[Core PR #36](https://github.com/Neguchads/navi-core/pull/36).
[Condições da API](https://docs.virustotal.com/reference/public-vs-premium-api).
