import shutil
import subprocess
from pathlib import Path

import pytest


PWSH = shutil.which('pwsh')
RESOLVER = Path(__file__).resolve().parents[1] / 'tools' / 'resolve-cue.ps1'
JAPANESE = 'Akumajou Dracula X - Gekka no Yasoukyoku (Japan) (2M).cue'


def cue(root, name):
    path = root / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text('REM Synthetic fixture; no disc data\n', encoding='utf-8')
    return path


def resolve(root, explicit=None):
    assert PWSH, 'PowerShell 7 (pwsh) precisa estar instalado para validar o launcher real.'
    command = [PWSH, '-NoProfile', '-NonInteractive', '-File', str(RESOLVER),
               '-RomDirectory', str(root)]
    if explicit is not None:
        command.extend(['-Cue', str(explicit)])
    return subprocess.run(command, capture_output=True, text=True, check=False)


def test_prefers_original_japanese_among_translations(tmp_path):
    original = cue(tmp_path, Path('original [Japan]') / JAPANESE)
    cue(tmp_path, 'translated [pt-br].cue')
    cue(tmp_path, 'ultimate.cue')
    result = resolve(tmp_path)
    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == str(original.resolve())


def test_explicit_override_preserves_literal_path(tmp_path):
    cue(tmp_path, JAPANESE)
    translated = cue(tmp_path, 'traducao [pt-br].cue')
    result = resolve(tmp_path, translated)
    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == str(translated.resolve())


def test_ambiguous_cues_without_original_fail(tmp_path):
    for name in ('a.cue', 'b.cue', 'c.cue'):
        cue(tmp_path, name)
    result = resolve(tmp_path)
    assert result.returncode != 0
    assert 'Informe -Cue ou SOTN_CUE' in result.stderr


@pytest.mark.parametrize('name', ['missing.cue', 'invalid.bin'])
def test_explicit_missing_or_wrong_extension_fails(tmp_path, name):
    path = tmp_path / name
    if name.endswith('.bin'):
        cue(tmp_path, name)
    result = resolve(tmp_path, path)
    assert result.returncode != 0
    assert 'CUE existente' in result.stderr


def test_duplicate_japanese_originals_fail(tmp_path):
    cue(tmp_path, Path('one') / JAPANESE)
    cue(tmp_path, Path('two') / JAPANESE)
    result = resolve(tmp_path)
    assert result.returncode != 0
    assert 'mais de um CUE japon' in result.stderr


def test_single_cue_fallback(tmp_path):
    only = cue(tmp_path, 'other [single].cue')
    result = resolve(tmp_path)
    assert result.returncode == 0, result.stderr
    assert result.stdout.strip() == str(only.resolve())
