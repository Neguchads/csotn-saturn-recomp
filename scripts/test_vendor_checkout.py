"""Exercita checkout Git real com autocrlf, preservando os hashes FSR upstream."""
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class VendorCheckout(unittest.TestCase):
    def test_fsr_checkout_preserves_upstream_bytes_on_windows(self):
        vendor = 'engine/runtime/third_party/fsr1/'
        manifest = json.loads((ROOT / vendor / 'manifest.json').read_text())
        with tempfile.TemporaryDirectory() as folder:
            paths = [vendor + entry['path'] for entry in manifest['files']]
            subprocess.run(['git', '-c', 'core.autocrlf=true', 'checkout-index',
                            '--prefix=' + Path(folder).as_posix() + '/', '--stdin', '-z'],
                           input=('\0'.join(paths) + '\0').encode(), cwd=ROOT, check=True,
                           capture_output=True)
            for entry, name in zip(manifest['files'], paths):
                data = (Path(folder) / name).read_bytes()
                self.assertEqual(hashlib.sha256(data).hexdigest(), entry['sha256'], name)
