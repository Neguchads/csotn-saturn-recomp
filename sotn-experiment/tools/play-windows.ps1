param([switch]$Prologue, [string]$Cue = $env:SOTN_CUE,
      [string]$Executable = '', [string]$OutputRoot = '', [string]$Save = '', [switch]$DisposableSave,
      [string]$SettingsFile = '', [switch]$VideoProfile)
$ErrorActionPreference = 'Stop'
if ($DisposableSave) { $Save = '-' }
$experiment = Split-Path $PSScriptRoot -Parent
$base = Split-Path $experiment -Parent
if (-not $SettingsFile) { $SettingsFile = Join-Path $PSScriptRoot 'sotn-windows.ini' }
if (-not $Executable) { $Executable = Join-Path $experiment 'build\jp\windows-reference-all\game\saturn.exe' }
$Cue = & (Join-Path $PSScriptRoot 'resolve-cue.ps1') -RomDirectory (Join-Path $base 'roms') -Cue $Cue
if (-not (Test-Path -LiteralPath $Executable)) { throw 'Executável Windows ausente; execute tools\build-windows-reference.cmd.' }
if (-not $OutputRoot) { $OutputRoot = Join-Path $base ('logs-e-bugs\partidas\' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '-' + $PID) }
$hashStream = [IO.File]::OpenRead($Executable)
$hashAlgorithm = [Security.Cryptography.SHA256]::Create()
try { $executableHash = [BitConverter]::ToString($hashAlgorithm.ComputeHash($hashStream)).Replace('-', '') }
finally { $hashStream.Dispose(); $hashAlgorithm.Dispose() }
$session = 0
do {
    $session++
    $output = Join-Path $OutputRoot ('session-{0:d3}' -f $session)
    if (Test-Path -LiteralPath $output) { throw "Saída já existe: $output" }
    New-Item -ItemType Directory -Path $output -Force | Out-Null
    $after = Join-Path $output 'menu-action.txt'
    $arguments = @('--cue', $Cue, '--out', $output, '--title-menu', '--after', $after,
                   '--settings', $SettingsFile, '--record-input', (Join-Path $output 'input.txt'))
    $arguments += @('--sotn-mods', (Join-Path $PSScriptRoot 'sotn-gameshark.txt'))
    if ($VideoProfile) { $arguments += '--video-profile' }
    if ($Save) { $arguments += @('--save', $Save) }
    if ($Prologue) {
        $arguments += @('--resume', '9000', '--input', '1600:START,1610:,1800:A,1810:,2300:A,2310:,2800:C,2810:,3300:START,3310:,3800:A,3810:')
    } elseif ($session -gt 1) {
        # Volta ao título anterior à escolha de personagem, sem repetir a seleção do prólogo.
        $arguments += @('--resume', '1800', '--input', '1600:START,1610:')
    }
    $quoted = $arguments | ForEach-Object { '"' + $_ + '"' }
    $sessionMetadata = [ordered]@{
        started = [DateTimeOffset]::Now.ToString('o')
        executable = $Executable
        executable_sha256 = $executableHash
        output = $output
        save_mode = $(if ($DisposableSave -or $Save -eq '-') { 'disposable' } else { 'normal' })
        exit_code = $null
        capture_bug = 'F8, PrintScreen ou menu Esc > Capturar bug'
        video_profile_requested = [bool]$VideoProfile
        startup_mode = $(if ($Prologue) { 'prologue-test' } elseif ($session -gt 1) { 'return-title' } else { 'disc-boot' })
    }
    $metadataPath = Join-Path $output 'session.json'
    $sessionMetadata | ConvertTo-Json | Set-Content -LiteralPath $metadataPath -Encoding UTF8
    $process = Start-Process -FilePath $Executable -ArgumentList $quoted -RedirectStandardError (Join-Path $output 'run.log') -RedirectStandardOutput (Join-Path $output 'stdout.log') -WindowStyle Normal -PassThru -Wait
    $sessionMetadata['finished'] = [DateTimeOffset]::Now.ToString('o')
    $sessionMetadata['exit_code'] = $process.ExitCode
    $sessionMetadata | ConvertTo-Json | Set-Content -LiteralPath $metadataPath -Encoding UTF8
    $errorsPath = Join-Path $output 'erros.log'
    Select-String -LiteralPath (Join-Path $output 'run.log') -Pattern '(?i)fatal|error|failed|failure|disabled|unsupported|CRC mismatch' |
        ForEach-Object { $_.Line } | Set-Content -LiteralPath $errorsPath -Encoding UTF8
    if ($process.ExitCode -ne 0) { Add-Content -LiteralPath $errorsPath -Value "Process exit code: $($process.ExitCode)" -Encoding UTF8 }
    if ($process.ExitCode -ne 0) {
        try {
            & (Join-Path $PSScriptRoot 'write-crashlog.ps1') -SessionDirectory $output -ExitCode $process.ExitCode | Out-Null
        } catch {
            Add-Content -LiteralPath $errorsPath -Value ('Crashlog generation failed: ' + $_.Exception.Message)
        }
    }
    if ($process.ExitCode -ne 0) { throw "O jogo encerrou com código $($process.ExitCode). Veja $output\run.log" }
    $action = if (Test-Path -LiteralPath $after) { (Get-Content -Raw -LiteralPath $after).Trim() } else { '' }
    if ($action -and $action -ne 'title') { throw "Ação desconhecida do menu: $action" }
    $Prologue = $false
} while ($action -eq 'title')
