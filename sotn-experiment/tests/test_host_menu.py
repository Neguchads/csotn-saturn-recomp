import os
import json
import hashlib
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]
HOST = ROOT / 'engine/runtime/src/host'


def build_driver(tmp_path):
    vswhere = Path(os.environ['ProgramFiles(x86)']) / 'Microsoft Visual Studio/Installer/vswhere.exe'
    installation = subprocess.check_output([
        str(vswhere), '-latest', '-products', '*', '-requires',
        'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath',
    ], text=True).strip()
    executable = tmp_path / 'host-menu-test.exe'
    compiler = Path(os.environ.get('SOTN_CLANG_CL', str(ROOT / 'sotn-experiment/build/windows-deps/llvm/bin/clang-cl.exe')))
    script = tmp_path / 'build.cmd'
    script.write_text(
        '@echo off\ncall "' + installation + '\\VC\\Auxiliary\\Build\\vcvars64.bat" >nul\n'
        'if errorlevel 1 exit /b 1\n'
        f'"{compiler}" /nologo /std:c++20 /EHsc /MT /I"{HOST}" '
        f'"{ROOT / "sotn-experiment/tests/host_menu_test.cpp"}" /Fo:"{tmp_path / "driver.obj"}" /Fe:"{executable}"\n',
        encoding='utf-8',
    )
    build = subprocess.run(['cmd.exe', '/d', '/c', str(script)], cwd=tmp_path,
                           text=True, capture_output=True)
    assert build.returncode == 0, build.stdout + build.stderr
    return executable


def test_production_pacing_scales_host_wait_and_recovers_from_stalls(tmp_path):
    executable = build_driver(tmp_path)
    result = subprocess.run([str(executable)], text=True, capture_output=True)
    assert result.returncode == 0, result.stdout + result.stderr


def test_launcher_restarts_in_title_mode_without_repeating_prologue(tmp_path):
    executable = build_driver(tmp_path)
    cue = tmp_path / 'test.cue'
    cue.write_text('FILE "unused.bin" BINARY\n', encoding='utf-8')
    output = tmp_path / 'play'
    result = subprocess.run([
        'powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
        str(ROOT / 'sotn-experiment/tools/play-windows.ps1'), '-Prologue',
        '-Cue', str(cue), '-Executable', str(executable), '-OutputRoot', str(output), '-DisposableSave',
    ], text=True, capture_output=True, timeout=30)
    assert result.returncode == 0, result.stdout + result.stderr
    sessions = sorted(output.glob('session-*'))
    assert len(sessions) == 2
    first = (sessions[0] / 'stdout.log').read_text()
    second = (sessions[1] / 'stdout.log').read_text()
    assert '--title-menu' in first and '--title-menu' in second
    assert '9000' in first and '1800:A' in first
    assert '9000' not in second and '1800:A' not in second
    assert '1600:START,1610:' in second
    assert '--save\n-\n' in first and '--save\n-\n' in second
    for session in sessions:
        metadata = json.loads((session / 'session.json').read_text(encoding='utf-8-sig'))
        assert metadata['save_mode'] == 'disposable' and metadata['exit_code'] == 0
        assert metadata['executable_sha256'].lower() == hashlib.sha256(executable.read_bytes()).hexdigest()
        assert not (session / 'crash.log').exists() and not (session / 'crash.json').exists()


def test_launcher_preserves_failure_metadata_and_shader_log(tmp_path):
    executable = build_driver(tmp_path)
    cue = tmp_path / 'fatal.cue'
    cue.write_text('FILE "unused.bin" BINARY\n', encoding='utf-8')
    output = tmp_path / 'failed-play'
    result = subprocess.run([
        'powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
        str(ROOT / 'sotn-experiment/tools/play-windows.ps1'), '-Cue', str(cue),
        '-Executable', str(executable), '-OutputRoot', str(output), '-DisposableSave',
    ], text=True, capture_output=True, timeout=30)
    assert result.returncode != 0
    session = output / 'session-001'
    metadata = json.loads((session / 'session.json').read_text(encoding='utf-8-sig'))
    assert metadata['exit_code'] == 7 and 'finished' in metadata
    errors = (session / 'erros.log').read_text(encoding='utf-8-sig')
    assert 'shader failure test' in errors and 'Process exit code: 7' in errors
    crash = json.loads((session / 'crash.json').read_text(encoding='utf-8-sig'))
    assert crash['exit_code'] == 7
    assert crash['failure_kind'] == 'process_exit_without_fatal'
    assert crash['executable_sha256'].lower() == hashlib.sha256(executable.read_bytes()).hexdigest()
    assert 'shader failure test' in (session / 'crash.log').read_text(encoding='utf-8-sig')


def test_crashlog_retains_fatal_context_and_hashes_only_diagnostic_evidence(tmp_path):
    session = tmp_path / 'session-001'
    session.mkdir()
    metadata = {'executable': 'diagnostic-fixture.exe', 'executable_sha256': 'ABC123',
                'started': '2026-10-25T14:27:51-03:00', 'finished': '2026-10-25T14:32:26-03:00'}
    (session / 'session.json').write_text(json.dumps(metadata), encoding='utf-8')
    (session / 'run.log').write_text(
        'prefix\nVIDEO_PROFILE vb=10 size=320x240\n'
        '[ 1.0 FATAL] overlay reload has no exact identity\n'
        '  cpu 0 pr 0606C38C pc 06071FDC\n  active STAGE_01\n', encoding='utf-8')
    dump = session / 'overlay-failure-fatal.bin'
    dump.write_bytes(bytes(64))
    (session / 'backup.bin').write_bytes(b'private fixture must not be inventoried')
    result = subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
        str(ROOT / 'sotn-experiment/tools/write-crashlog.ps1'), '-SessionDirectory', str(session),
        '-ExitCode', '1'], text=True, capture_output=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
    crash = json.loads((session / 'crash.json').read_text(encoding='utf-8-sig'))
    assert crash['exit_code_hex'] == '00000001' and crash['failure_kind'] == 'runtime_fatal'
    assert 'cpu 0 pr 0606C38C pc 06071FDC' in crash['fatal_context']
    assert crash['last_video_profile'] == 'VIDEO_PROFILE vb=10 size=320x240'
    assert crash['evidence'] == [{'name': dump.name, 'bytes': 64,
                                  'sha256': hashlib.sha256(dump.read_bytes()).hexdigest().upper()}]
    assert 'backup.bin' not in json.dumps(crash)
    assert (session / 'backup.bin').read_bytes() == b'private fixture must not be inventoried'


def test_crashlog_classifies_native_exit_without_inventing_stack_or_fatal(tmp_path):
    result = subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
        str(ROOT / 'sotn-experiment/tools/write-crashlog.ps1'), '-SessionDirectory', str(tmp_path),
        '-ExitCode', '-1073741819'], text=True, capture_output=True, timeout=15)
    assert result.returncode == 0, result.stdout + result.stderr
    crash = json.loads((tmp_path / 'crash.json').read_text(encoding='utf-8-sig'))
    assert crash['exit_code_hex'] == 'C0000005'
    assert crash['failure_kind'] == 'native_access_violation_without_fatal'
    assert crash['fatal_context'] == '' and crash['evidence'] == []
    assert 'native stack was not captured' in crash['missing_context']


def test_normal_disc_boot_preserves_settings_across_launches(tmp_path):
    executable = build_driver(tmp_path)
    cue = tmp_path / 'test.cue'
    cue.write_text('FILE "unused.bin" BINARY\n', encoding='utf-8')
    settings = tmp_path / 'saved options.ini'
    for attempt, preferences in enumerate(('[display]\nfullscreen=true\nresolution=4\n',
                                            '[display]\nfullscreen=false\nresolution=2\n')):
        settings.write_text(preferences, encoding='utf-8')
        output = tmp_path / f'boot-{attempt}'
        result = subprocess.run(['powershell.exe','-NoProfile','-ExecutionPolicy','Bypass','-File',
            str(ROOT / 'sotn-experiment/tools/play-windows.ps1'), '-Cue', str(cue),
            '-Executable',str(executable),'-OutputRoot',str(output),'-SettingsFile',str(settings),
            '-DisposableSave'],text=True,capture_output=True,timeout=30)
        assert result.returncode == 0,result.stdout + result.stderr
        args = (output/'session-001/stdout.log').read_text()
        assert '--resume\n' not in args and '--input\n' not in args
        assert '--settings\n' + str(settings) + '\n' in args
        assert settings.read_text(encoding='utf-8') == preferences
        metadata = json.loads((output/'session-001/session.json').read_text(encoding='utf-8-sig'))
        assert metadata['startup_mode'] == 'disc-boot'
        assert metadata['save_mode'] == 'disposable'
