import hashlib
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import recomplauncher_adapter as adapter


class AdapterTests(unittest.TestCase):
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
