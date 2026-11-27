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

Core pin: c6037185830f24770f856d4cd69791c686095bc5, pending latest CI and independent review.
[Core PR #36](https://github.com/Neguchads/navi-core/pull/36).
[API conditions](https://docs.virustotal.com/reference/public-vs-premium-api).
