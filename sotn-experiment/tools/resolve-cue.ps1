param([Parameter(Mandatory)][string]$RomDirectory, [string]$Cue)
$ErrorActionPreference = 'Stop'

if (-not $Cue) {
    $images = @(Get-ChildItem -LiteralPath $RomDirectory -Filter '*.cue' -File -Recurse)
    $originals = @($images | Where-Object {
        $_.Name -ieq 'Akumajou Dracula X - Gekka no Yasoukyoku (Japan) (2M).cue'
    })
    if ($originals.Count -gt 1) {
        throw 'Encontrado mais de um CUE japonês original. Informe -Cue ou SOTN_CUE.'
    }
    if ($originals.Count -eq 1) {
        $Cue = $originals[0].FullName
    } elseif ($images.Count -eq 1) {
        $Cue = $images[0].FullName
    } else {
        throw 'Informe -Cue ou SOTN_CUE; nenhum original japonês único e há zero ou vários CUEs em roms/.'
    }
}
if (-not (Test-Path -LiteralPath $Cue -PathType Leaf) -or [IO.Path]::GetExtension($Cue) -ine '.cue') {
    throw 'Informe um CUE existente, acompanhado dos BINs referenciados.'
}
(Resolve-Path -LiteralPath $Cue).ProviderPath
