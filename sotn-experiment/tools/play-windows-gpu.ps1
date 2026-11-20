param(
    [string]$Executable = '',
    [ValidateSet('direct3d12', 'vulkan')][string]$Backend = 'vulkan',
    [switch]$VideoProfile = $true,
    [switch]$CpuReference
)
$ErrorActionPreference = 'Stop'
$taskRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $Executable) {
    $Executable = Join-Path $taskRoot 'sotn-experiment\build\jp\windows-reference-all\game\saturn.exe'
}
if (-not (Test-Path -LiteralPath $Executable)) { throw 'Build GPU ausente; conferir o relatório antes de jogar.' }
$taskOldGpu = [Environment]::GetEnvironmentVariable('SOTN_VDP2_GPU', 'Process')
$taskOldBackend = [Environment]::GetEnvironmentVariable('SOTN_VDP2_GPU_BACKEND', 'Process')
$taskExitCode = 1
try {
    $env:SOTN_VDP2_GPU = if ($CpuReference) { '0' } else { '1' }
    $env:SOTN_VDP2_GPU_BACKEND = $Backend
    & (Join-Path $PSScriptRoot 'play-windows.ps1') -Executable $Executable -VideoProfile:$VideoProfile
    $taskExitCode = $LASTEXITCODE
} finally {
    [Environment]::SetEnvironmentVariable('SOTN_VDP2_GPU', $taskOldGpu, 'Process')
    [Environment]::SetEnvironmentVariable('SOTN_VDP2_GPU_BACKEND', $taskOldBackend, 'Process')
}
exit $taskExitCode
