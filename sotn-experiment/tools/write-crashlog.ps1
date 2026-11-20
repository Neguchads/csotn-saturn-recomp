param([Parameter(Mandatory=$true)][string]$SessionDirectory,
      [Parameter(Mandatory=$true)][int]$ExitCode)
$ErrorActionPreference = 'Stop'
if ($ExitCode -eq 0) { throw 'Crashlog exige uma saida com falha.' }
$taskSession = (Resolve-Path -LiteralPath $SessionDirectory).Path
$taskMetadataPath = Join-Path $taskSession 'session.json'
$taskMetadata = if (Test-Path -LiteralPath $taskMetadataPath) {
    Get-Content -LiteralPath $taskMetadataPath -Raw | ConvertFrom-Json
} else { $null }
$taskRunLog = Join-Path $taskSession 'run.log'
$taskLines = if (Test-Path -LiteralPath $taskRunLog) { @(Get-Content -LiteralPath $taskRunLog -Tail 400) } else { @() }
$taskFatalIndex = -1
for ($taskIndex = 0; $taskIndex -lt $taskLines.Count; $taskIndex++) {
    if ($taskLines[$taskIndex] -match '\bFATAL\]') { $taskFatalIndex = $taskIndex }
}
$taskFatal = if ($taskFatalIndex -ge 0) {
    ($taskLines[$taskFatalIndex..($taskLines.Count - 1)] -join [Environment]::NewLine)
} else { '' }
$taskFoundProfile = $taskLines | Where-Object { $_ -like 'VIDEO_PROFILE *' } | Select-Object -Last 1
# Interpolacao remove metadados ETS do Get-Content antes da serializacao JSON.
$taskProfile = if ($null -ne $taskFoundProfile) { "$taskFoundProfile" } else { $null }
$taskUnsignedExit = [BitConverter]::ToUInt32([BitConverter]::GetBytes($ExitCode), 0)
$taskKind = if ($taskFatal) { 'runtime_fatal' } elseif ($taskUnsignedExit.ToString('X8') -eq 'C0000005') {
    'native_access_violation_without_fatal'
} else { 'process_exit_without_fatal' }
$taskEvidence = @()
# Somente dumps diagnosticos conhecidos. Nao enumerar/ler saves ou backup.bin.
foreach ($taskName in @('overlay-failure-fatal.bin', 'overlay-failure-identity-mismatch.bin')) {
    $taskPath = Join-Path $taskSession $taskName
    if (Test-Path -LiteralPath $taskPath -PathType Leaf) {
        $taskFile = Get-Item -LiteralPath $taskPath
        $taskStream = [IO.File]::OpenRead($taskPath)
        $taskSha = [Security.Cryptography.SHA256]::Create()
        try { $taskHash = [BitConverter]::ToString($taskSha.ComputeHash($taskStream)).Replace('-', '') }
        finally { $taskStream.Dispose(); $taskSha.Dispose() }
        $taskEvidence += [pscustomobject]@{name=$taskName;bytes=$taskFile.Length;sha256=$taskHash}
    }
}
$taskRecord = [ordered]@{
    schema_version = 1
    created = [DateTimeOffset]::Now.ToString('o')
    source = 'Existing session metadata, final 400 run.log lines and diagnostic dumps only'
    session_directory = $taskSession
    executable = $(if ($taskMetadata) { $taskMetadata.executable } else { $null })
    executable_sha256 = $(if ($taskMetadata) { $taskMetadata.executable_sha256 } else { $null })
    started = $(if ($taskMetadata) { $taskMetadata.started } else { $null })
    finished = $(if ($taskMetadata) { $taskMetadata.finished } else { $null })
    exit_code = $ExitCode
    exit_code_hex = $taskUnsignedExit.ToString('X8')
    failure_kind = $taskKind
    fatal_context = $taskFatal
    last_video_profile = $taskProfile
    evidence = @($taskEvidence)
    missing_context = $(if ($taskFatal) { '' } else { 'No runtime FATAL block; native stack was not captured. Exit status is not a root-cause diagnosis.' })
}
$taskRecord | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $taskSession 'crash.json') -Encoding UTF8
$taskText = @(
    'SotN Saturn - crashlog'
    ('Session: ' + $taskSession)
    ('Executable SHA256: ' + $taskRecord.executable_sha256)
    ('Exit code: ' + $ExitCode + ' (0x' + $taskRecord.exit_code_hex + ')')
    ('Classification: ' + $taskKind)
    ('Missing context: ' + $taskRecord.missing_context)
    ''
    'Runtime fatal context:'
    $taskFatal.Replace('\n', [Environment]::NewLine)
    ''
    'Diagnostic evidence:'
    ($taskEvidence | ForEach-Object { '{0} bytes={1} SHA256={2}' -f $_.name,$_.bytes,$_.sha256 })
    ''
    'Final log excerpt (original run.log preserved):'
    ($taskLines -join [Environment]::NewLine)
) -join [Environment]::NewLine
$taskText | Set-Content -LiteralPath (Join-Path $taskSession 'crash.log') -Encoding UTF8
Write-Output (Join-Path $taskSession 'crash.log')
