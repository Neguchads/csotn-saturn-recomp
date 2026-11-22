import hashlib
import json
from pathlib import Path
import tempfile
import os
import subprocess
import sys
import unittest
from unittest.mock import patch

import recomplauncher_adapter as adapter


class AdapterTests(unittest.TestCase):
    def test_missing_chdman_refuses_before_writing_player_data(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            game = root / 'original.CHD'
            game.write_bytes(b'synthetic CHD input')
            environment = {'PATH': '', 'SOTN_CHDMAN': str(root / 'missing.exe')}
            with patch.object(adapter, 'check_tools', return_value=([], environment)):
                with self.assertRaisesRegex(ValueError, 'CHD requires chdman'):
                    adapter.prepare_player(game, root / 'data')
            self.assertFalse((root / 'data').exists())
            self.assertEqual(game.read_bytes(), b'synthetic CHD input')

    def test_cue_media_path_does_not_create_outputs_or_invoke_converter(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            cue = root / 'original.cue'
            cue.write_text('FILE "original.bin" BINARY')
            with patch.object(adapter.subprocess, 'run') as run:
                self.assertEqual(adapter.prepare_media(cue, root / 'absent', {}), cue.resolve())
                run.assert_not_called()
            self.assertFalse((root / 'absent').exists())

    def test_chd_conversion_process_uses_fresh_attempt_and_preserves_original(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            original = root / 'original disc.chd'
            original.write_bytes(b'player media unchanged')
            converter = root / 'fake_converter.py'
            converter.write_text("import pathlib,sys\nargs=sys.argv[1:]\nif args[0]=='extractcd':\n    pathlib.Path(args[args.index('-o')+1]).write_text('FILE \"disc.bin\" BINARY')\n    pathlib.Path(args[args.index('-ob')+1]).write_bytes(b'synthetic sectors')\nprint(args[0])\n", encoding='utf-8')
            commands = []
            def run(command, **kwargs):
                commands.append(command)
                return subprocess.run([sys.executable, str(converter), *command[1:]], **kwargs)
            attempt = root / 'data/build/work-test'
            cue = adapter.prepare_media(original, attempt, os.environ.copy(), sys.executable, run)
            self.assertEqual(cue, attempt / 'media/disc.cue')
            self.assertEqual([command[1] for command in commands], ['verify', 'extractcd'])
            self.assertNotIn('-f', commands[1])
            self.assertEqual(original.read_bytes(), b'player media unchanged')
            self.assertEqual(cue.with_suffix('.bin').read_bytes(), b'synthetic sectors')
            self.assertIn('extractcd', (cue.parent / 'chdman.log').read_text())
            with self.assertRaises(FileExistsError):
                adapter.prepare_media(original, attempt, {}, sys.executable, run)

    def test_chd_success_exit_without_output_is_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            game = root / 'own.chd'
            game.write_bytes(b'input')
            with self.assertRaisesRegex(ValueError, 'complete local CUE/BIN'):
                adapter.prepare_media(game, root / 'attempt', {}, sys.executable,
                                      lambda *args, **kwargs: None)
            self.assertFalse((root / 'attempt/source').exists())

    def test_chd_verify_failure_stops_before_extraction_and_preserves_logs(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            game = root / 'own.chd'
            game.write_bytes(b'input')
            commands = []
            def run(command, **kwargs):
                commands.append(command)
                raise subprocess.CalledProcessError(1, command)
            with self.assertRaises(subprocess.CalledProcessError):
                adapter.prepare_media(game, root / 'attempt', {}, sys.executable, run)
            self.assertEqual(len(commands), 1)
            self.assertTrue((root / 'attempt/media/chdman.log').exists())
            self.assertEqual(game.read_bytes(), b'input')

    def test_chd_preparation_records_converted_cue_for_later_runtime(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source = root / 'source'
            source.mkdir()
            (source / 'build-tool.py').write_text('pass')
            game = root / 'own.chd'
            game.write_bytes(b'original CHD')
            data = root / 'data'
            converted = []
            def media(selected, attempt, environment, chdman):
                self.assertEqual(selected, game)
                cue = attempt / 'media/disc.cue'
                cue.parent.mkdir()
                cue.write_text('FILE "disc.bin" BINARY')
                cue.with_suffix('.bin').write_bytes(b'converted')
                converted.append(cue)
                return cue
            def build(workspace, cue, env):
                self.assertEqual(cue, converted[0])
                self.assertTrue((workspace / 'build-tool.py').is_file())
                result = workspace / 'out/saturn.exe'
                result.parent.mkdir()
                result.write_bytes(b'local synthetic executable, never started')
                result.with_name('SDL3.dll').write_bytes(b'local synthetic dll')
                return result
            with patch.object(adapter, 'check_tools', return_value=([], {})), \
                    patch.object(adapter, 'find_chdman', return_value=sys.executable), \
                    patch.object(adapter, 'prepare_media', side_effect=media), \
                    patch.object(adapter, 'prepare', side_effect=build):
                adapter.prepare_player(game, data, source)
            record = json.loads((data / 'build/ready.json').read_text())
            self.assertEqual(record['cue'], str(converted[0]))
            self.assertTrue(converted[0].is_relative_to(data))
            self.assertTrue(converted[0].is_file())
            self.assertTrue((data / 'game/saturn.exe').is_file())
            self.assertFalse((data / 'saves').exists())
            self.assertEqual(game.read_bytes(), b'original CHD')

    def test_source_identity_ignores_build_data_and_refuses_links(self):
        with tempfile.TemporaryDirectory() as folder:
            source = Path(folder)
            (source / 'entry.py').write_text('print(1)')
            before, files = adapter.source_identity(source)
            (source / 'build').mkdir()
            (source / 'build/private.bin').write_bytes(b'private')
            after, _ = adapter.source_identity(source)
            self.assertEqual(before, after)
            self.assertEqual([path.name for path in files], ['entry.py'])

    def test_missing_toolchain_refuses_before_copying_or_preparing(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            cue = root / 'game.cue'
            cue.write_text('FILE "own.bin" BINARY')
            with patch.object(adapter, 'check_tools', return_value=(['missing compiler'], {})):
                with self.assertRaisesRegex(ValueError, 'missing compiler'):
                    adapter.prepare_player(cue, root / 'data')
            self.assertFalse((root / 'data').exists())

    def test_play_builds_explicit_save_paths_and_does_not_run_process(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source = root / 'source'
            source.mkdir()
            (source / 'tool.py').write_text('pass')
            data = root / 'data'
            (data / 'game').mkdir(parents=True)
            executable = data / 'game/saturn.exe'
            executable.write_bytes(b'synthetic test executable, never run')
            cue = root / 'own.cue'
            cue.write_text('synthetic')
            identity, _ = adapter.source_identity(source)
            record = dict(source_sha256=identity, cue=str(cue), executable='../../other.exe',
                          executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest())
            adapter.atomic_json(data / 'build/ready.json', record)
            with patch.object(adapter.subprocess, 'run') as run:
                command = adapter.play_command(data, source)
                run.assert_not_called()
            self.assertEqual(command[0], str(executable))
            self.assertIn(str(data / 'saves/backup.bin'), command)
            executable.write_bytes(b'changed')
            with self.assertRaisesRegex(ValueError, 'missing or changed'):
                adapter.play_command(data, source)

    def test_profile_is_local_prepare_and_never_bundles_the_game(self):
        profile = json.loads((adapter.SOURCE / 'recomplauncher.json').read_text(encoding='utf-8'))
        self.assertEqual(profile['prepare']['program'], ['CsotnSetup.exe'])
        self.assertEqual(profile['start']['program'], ['CsotnSetup.exe'])
        self.assertIn('{game}', profile['prepare']['args'])
        self.assertEqual(profile['saves'], ['{data}/saves', '{data}/settings.ini'])
