import hashlib
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest
import zipfile

from package_recomplauncher import BOOTSTRAP_MARKER, build_package, validate_path


class SourcePackage(unittest.TestCase):
    def setUp(self):
        self.folder = tempfile.TemporaryDirectory()
        self.addCleanup(self.folder.cleanup)
        self.root = Path(self.folder.name)
        self.run_git('init', '-q')
        self.run_git('config', 'user.name', 'Package Test')
        self.run_git('config', 'user.email', 'package-test@example.invalid')
        self.write('LICENSE', 'Synthetic license fixture\n')
        self.write('README.md', 'Synthetic source fixture\n')
        self.write('recomplauncher.json', json.dumps({'format': 1, 'start': {'program': ['CsotnSetup.exe']}}))
        self.write('scripts/recomplauncher_bootstrap.cpp', '// synthetic bootstrap source fixture\n')
        self.commit()

    def run_git(self, *args, data=None):
        return subprocess.check_output(['git', *args], cwd=self.root, input=data)

    def write(self, name, data):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(data, encoding='utf-8')
        self.run_git('add', '--', name)

    def commit(self):
        self.run_git('commit', '-qm', 'test: package fixture')

    def test_source_zip_is_deterministic_and_ignores_private_untracked_files(self):
        (self.root / 'saturn.exe').write_bytes(b'MZ-private-game')
        (self.root / 'backup.bin').write_bytes(b'private-save')
        one, two = self.root / 'one.zip', self.root / 'two.zip'
        manifest = build_package(self.root, one)
        build_package(self.root, two)
        self.assertEqual(one.read_bytes(), two.read_bytes())
        self.assertFalse(manifest['installable'])
        self.assertFalse(manifest['ready_to_play'])
        with zipfile.ZipFile(one) as archive:
            self.assertNotIn('recomplauncher.json', archive.namelist())
            self.assertIn('source/recomplauncher.json', archive.namelist())
            self.assertNotIn('source/saturn.exe', archive.namelist())
            self.assertNotIn('source/backup.bin', archive.namelist())
            for entry in manifest['files']:
                self.assertEqual(hashlib.sha256(archive.read(entry['path'])).hexdigest(), entry['sha256'])

    def test_dirty_tracked_sources_are_refused(self):
        (self.root / 'README.md').write_text('uncommitted change')
        with self.assertRaisesRegex(ValueError, 'Commitar'):
            build_package(self.root, self.root / 'result.zip')

    def test_traversal_and_windows_ambiguous_paths_are_refused(self):
        for name in ('../x.py', '/x.py', 'C:/x.py', 'scripts\\x.py', 'docs/a\nb.md', 'docs/./x.md',
                     'docs/x.md.', 'scripts/CON.py', 'docs/COM1.md'):
            with self.subTest(name=name), self.assertRaises(ValueError):
                validate_path(name)

    def test_tracked_private_data_and_generated_sources_are_refused(self):
        for name in ('roms/game.bin', 'engine/build/generated.cpp', 'scripts/.env',
                     'engine/runtime/src/host/sotn_map_markers.local.h'):
            with self.subTest(name=name):
                self.write(name, 'private fixture')
                self.commit()
                with self.assertRaises(ValueError):
                    build_package(self.root, self.root / 'result.zip')
                self.run_git('rm', '-q', '--', name)
                self.commit()

    def test_git_symlink_entry_is_refused_without_requiring_windows_privilege(self):
        blob = self.run_git('hash-object', '-w', '--stdin', data=b'../outside').decode().strip()
        self.run_git('update-index', '--add', '--cacheinfo', '120000,' + blob + ',scripts/link.py')
        self.commit()
        # A symlink can be in the index without materialization on Windows.
        self.run_git('config', 'core.symlinks', 'false')
        self.run_git('checkout-index', '--force', '--', 'scripts/link.py')
        with self.assertRaisesRegex(ValueError, 'Symlinks'):
            build_package(self.root, self.root / 'result.zip')

    def bootstrap(self):
        data = bytearray(128)
        data[:2] = b'MZ'
        struct.pack_into('<I', data, 0x3C, 64)
        data[64:70] = b'PE\0\0\x64\x86'
        data.extend(BOOTSTRAP_MARKER)
        executable = self.root / 'CsotnSetup.exe'
        executable.write_bytes(data)
        provenance = {'kind': 'csotn-bootstrap', 'sha256': hashlib.sha256(data).hexdigest(),
                      'source_sha256': hashlib.sha256(self.run_git('show', 'HEAD:scripts/recomplauncher_bootstrap.cpp')).hexdigest(),
                      'source_rev': self.run_git('rev-parse', 'HEAD').decode().strip()}
        path = self.root / 'provenance.json'
        path.write_text(json.dumps(provenance))
        return executable, path

    def test_only_identified_matching_own_bootstrap_makes_profile_active(self):
        executable, provenance = self.bootstrap()
        output = self.root / 'bootstrap.zip'
        manifest = build_package(self.root, output, executable, provenance, version='v0.1.0-preview')
        self.assertTrue(manifest['installable'])
        self.assertFalse(manifest['ready_to_play'])
        self.assertEqual(manifest['version'], 'v0.1.0-preview')
        with zipfile.ZipFile(output) as archive:
            self.assertIn('CsotnSetup.exe', archive.namelist())
            self.assertIn('recomplauncher.json', archive.namelist())
            self.assertEqual(json.loads(archive.read('recomplauncher.json'))['prepare']['version'], 'v0.1.0-preview')
            self.assertNotIn('prepare', json.loads(archive.read('source/recomplauncher.json')))
        executable.write_bytes(executable.read_bytes() + b'altered')
        with self.assertRaisesRegex(ValueError, 'Proveniência'):
            build_package(self.root, output, executable, provenance)

    def test_wrong_bootstrap_name_or_old_source_revision_is_refused(self):
        executable, provenance = self.bootstrap()
        wrong = executable.with_name('saturn.exe')
        wrong.write_bytes(executable.read_bytes())
        with self.assertRaises(ValueError):
            build_package(self.root, self.root / 'x.zip', wrong, provenance)
        data = json.loads(provenance.read_text())
        data['source_rev'] = '0' * 40
        provenance.write_text(json.dumps(data))
        with self.assertRaises(ValueError):
            build_package(self.root, self.root / 'x.zip', executable, provenance)
