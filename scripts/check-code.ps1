param([string]$Python = '', [switch]$FullWindowTests)
$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path $PSScriptRoot -Parent
if (-not $Python) {
    $taskLocal = Join-Path $taskRoot '.venv/Scripts/python.exe'
    $Python = if (Test-Path -LiteralPath $taskLocal) { $taskLocal } else { 'python' }
}
$taskPreviousPath = $env:PYTHONPATH
Push-Location $taskRoot
try {
    $env:PYTHONPATH = "$taskRoot/scripts;$taskRoot/engine/src"
    & $Python -m unittest discover -s scripts -p 'test_*.py'
    if ($LASTEXITCODE) { throw 'Falha no gate de publicação.' }
    if ($FullWindowTests) { & $Python -m pytest sotn-experiment/tests -q }
    else { & $Python -m pytest sotn-experiment/tests -p code_only_plugin -q }
    if ($LASTEXITCODE) { throw 'Falha nos testes de código.' }
    Write-Output 'PASS: testes de código; build, GPU offscreen e gameplay têm provas separadas.'
} finally {
    $env:PYTHONPATH = $taskPreviousPath
    Pop-Location
}
