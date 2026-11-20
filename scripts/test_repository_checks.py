import importlib.util
from pathlib import Path
import unittest
import re
import tomllib
from check_repository import allowed, inspect


class PublicationChecks(unittest.TestCase):
    def test_rejects_game_and_private_files(self):
        for name in ('roms/game.cue', 'engine/build/recomp/modules.cpp', 'referencias/a.py',
                     'validacao/dump.bin', 'scripts/.env', 'engine/private.exe',
                     'engine/runtime/src/host/sotn_map_markers.local.h', 'docs/../roms/a.md'):
            self.assertFalse(allowed(name), name)

    def test_accepts_runtime_generator_tests_and_licenses(self):
        for name in ('engine/runtime/src/host/host.cpp', 'engine/src/saturnrecomp/sh2.py',
                     'engine/runtime/tests/onchip_test.cpp', 'engine/LICENSE', 'README.md',
                     'engine/runtime/third_party/musashi/softfloat/softfloat-macros',
                     'engine/runtime/third_party/musashi/softfloat/softfloat-specialize',
                     'engine/runtime/third_party/inter/Inter-Regular.ttf'):
            self.assertTrue(allowed(name), name)

    def test_rejects_private_marker_table(self):
        self.assertTrue(inspect('engine/runtime/src/host/sotn_map_markers.h',
                                b'inline Marker markers[]={{false,7,30,1}};'))

    def test_secret_is_not_echoed(self):
        secret = 'ghp_' + 'A' * 36
        result = inspect('README.md', secret.encode())
        self.assertTrue(result)
        self.assertNotIn(secret, str(result))

    def test_source_must_be_text_and_valid_python(self):
        self.assertTrue(inspect('scripts/a.py', b'abc\0def'))
        self.assertTrue(inspect('scripts/a.py', b'def broken('))
        self.assertEqual(inspect('scripts/a.py', b'pass\n'), [])

    def test_local_map_generator_validates_and_uses_synthetic_data(self):
        path = Path(__file__).with_name('generate-map-markers.py')
        spec = importlib.util.spec_from_file_location('marker_generator', path)
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        self.assertIn('{false, 3, 4, 1}', module.render([dict(inverted=False, x=3, y=4, kind=1)]))
        self.assertIn('std::array<Marker, 0>', module.render([]))
        for invalid in (None, [{}], [dict(inverted=0, x=3, y=4, kind=1)],
                        [dict(inverted=True, x=64, y=4, kind=1)]):
            with self.assertRaises(ValueError):
                module.render(invalid)

    def test_secret_allowlist_only_matches_one_known_vendor_expression(self):
        root = Path(__file__).resolve().parents[1]
        config = tomllib.loads((root / '.gitleaks.toml').read_text())
        self.assertTrue(config['extend']['useDefault'])
        allow = config['rules'][0]['allowlists'][0]
        self.assertEqual(allow['condition'], 'AND')
        path = 'engine/runtime/third_party/imgui/imgui.cpp'
        self.assertIsNotNone(re.fullmatch(allow['paths'][0], path))
        self.assertIsNone(re.fullmatch(allow['paths'][0], 'scripts/private.py'))
        line = (root / path).read_text().splitlines()[4501]
        self.assertIsNotNone(re.fullmatch(allow['regexes'][0], line))
        self.assertIsNone(re.fullmatch(allow['regexes'][0], line + ' api_key=unrelated'))


if __name__ == '__main__':
    unittest.main()
