import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import prepare_player_build as pipeline


class PlayerPreparation(unittest.TestCase):
    def test_only_the_supported_retail_identity_is_accepted(self):
        pipeline.validate_identity('T-9527G  ', 'V1.400 ')
        for identity in (('T-9527G', 'V1.000'), ('SLUS-00067', 'V1.400'), ('', '')):
            with self.subTest(identity=identity), self.assertRaises(pipeline.PreparationError):
                pipeline.validate_identity(*identity)

    def test_disc_extraction_paths_cannot_escape_the_workspace(self):
        pipeline.validate_disc_path('/SD/MUSIC.PCM')
        for name in ('/../outside', '/SD/../../outside', 'C:/outside', 'SD\\outside', '/'):
            with self.subTest(name=name), self.assertRaises(pipeline.PreparationError):
                pipeline.validate_disc_path(name)

    def fixture(self, folder):
        extract = folder / 'extract'
        extract.mkdir()
        entries = []
        for index in range(12):
            data = bytes([index]) * 64
            name = f'PROGRAM{index}.PRG'
            (extract / name).write_bytes(data)
            entries.append(hashlib.sha1(data).hexdigest() + '  build/saturn/' + name)
        manifest = folder / 'check.saturn.sha'
        manifest.write_text('\n'.join(entries), encoding='utf-8')
        return extract, manifest

    def test_all_twelve_programs_must_match_before_generation(self):
        with tempfile.TemporaryDirectory() as directory:
            extract, manifest = self.fixture(Path(directory))
            self.assertEqual(len(pipeline.validate_retail_hashes(extract, manifest)), 12)
            (extract / 'PROGRAM7.PRG').write_bytes(b'wrong disc')
            with self.assertRaisesRegex(pipeline.PreparationError, 'PROGRAM7.PRG'):
                pipeline.validate_retail_hashes(extract, manifest)

    def test_incomplete_or_duplicate_reference_manifest_is_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            extract, manifest = self.fixture(Path(directory))
            rows = manifest.read_text().splitlines()
            for changed in (rows[:-1], rows + [rows[0]], ['not-a-hash PROGRAM.PRG']):
                manifest.write_text('\n'.join(changed))
                with self.assertRaises(pipeline.PreparationError):
                    pipeline.validate_retail_hashes(extract, manifest)

    def test_existing_reference_is_preserved_instead_of_reusing_stale_assembly(self):
        with tempfile.TemporaryDirectory() as directory:
            workspace = Path(directory)
            reference = workspace / 'referencias/sotn-decomp-analise'
            reference.mkdir(parents=True)
            sentinel = reference / 'personal.txt'
            sentinel.write_text('preserve')
            commands = []
            with self.assertRaises(pipeline.PreparationError):
                pipeline.pinned_reference(workspace, {}, lambda command, **kwargs: commands.append(command))
            self.assertEqual(commands, [])
            self.assertEqual(sentinel.read_text(), 'preserve')

    def test_reference_and_splitter_commits_are_verified(self):
        with tempfile.TemporaryDirectory() as directory:
            commands = []
            def run(command, **kwargs):
                commands.append(command)
                if command[:2] == ['git', 'clone']:
                    Path(command[-1]).mkdir()
            with patch.object(pipeline, '_git_head', side_effect=[pipeline.REFERENCE_REVISION, '0' * 40]):
                with self.assertRaisesRegex(pipeline.PreparationError, 'splitter revision'):
                    pipeline.pinned_reference(Path(directory), {}, run)
            self.assertIn(['git', 'checkout', '--detach', pipeline.REFERENCE_REVISION], commands)
            self.assertIn(['git', 'submodule', 'update', '--init', '--', 'tools/saturn-splitter'], commands)

    def test_wrong_disc_is_rejected_by_the_actual_disc_validation_path(self):
        with tempfile.TemporaryDirectory() as directory:
            workspace = Path(directory)
            reader = workspace / 'engine/src/saturnrecomp/disc.py'
            reader.parent.mkdir(parents=True)
            reader.write_text('''class Disc:
    def __init__(self, path):
        self.ip = type('IP', (), dict(product='OTHER', version='V1.400'))()
        self._fh = {}
''')
            with self.assertRaisesRegex(pipeline.PreparationError, 'another disc'):
                pipeline._validate_disc(workspace, workspace / 'wrong.cue')

    def test_render_command_is_valid_python_and_points_at_the_isolated_workspace(self):
        with tempfile.TemporaryDirectory() as directory:
            workspace = Path(directory)
            commands = []
            pipeline.write_configuration(workspace, workspace / 'reference', workspace / 'extract',
                                         workspace / 'sotn-experiment/build/jp-all', {},
                                         lambda command, **kwargs: commands.append(command))
            self.assertEqual(len(commands), 1)
            command = commands[0]
            compile(command[2], '<configuration-child>', 'exec')
            self.assertEqual(command[-1], str(workspace / 'sotn-experiment/game-jp-all.toml'))
            self.assertNotIn('referencias/sotn-decomp-analise', command[2])


if __name__ == '__main__':
    unittest.main()
