"""Protege a documentação pública bilíngue e sua navegação local."""
from pathlib import Path
import re
import subprocess
import unittest
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]


class BilingualDocumentation(unittest.TestCase):
    def test_all_public_markdown_has_english_first_and_portuguese_below(self):
        names = subprocess.check_output(['git', 'ls-files', '*.md'], cwd=ROOT, text=True).splitlines()
        self.assertIn('README.md', names)
        for name in names:
            with self.subTest(file=name):
                text = (ROOT / name).read_text(encoding='utf-8-sig')
                self.assertEqual(text.count('<a id="english"></a>'), 1)
                self.assertEqual(text.count('<a id="portugues"></a>'), 1)
                english, portuguese = text.split('<a id="english"></a>')[1].split('<a id="portugues"></a>')
                self.assertGreater(len(english.strip()), 100)
                self.assertGreater(len(portuguese.strip()), 100)
                self.assertIn('[English](#english)', text)
                self.assertIn('[Português](#portugues)', text)
                self.assertEqual(len(re.findall(r'^```', english, re.M)) % 2, 0)
                self.assertEqual(len(re.findall(r'^```', portuguese, re.M)) % 2, 0)

    def test_relative_markdown_links_resolve_within_public_checkout(self):
        names = subprocess.check_output(['git', 'ls-files', '*.md'], cwd=ROOT, text=True).splitlines()
        for name in names:
            source = ROOT / name
            text = re.sub(r'```.*?```', '', source.read_text(encoding='utf-8-sig'), flags=re.S)
            for target in re.findall(r'\[[^\]]*\]\(([^\s)]+)\)', text):
                url = urlsplit(target.strip('<>'))
                if url.scheme or url.netloc or not url.path:
                    continue
                with self.subTest(file=name, link=target):
                    path = (source.parent / unquote(url.path)).resolve()
                    self.assertTrue(path.is_relative_to(ROOT.resolve()), target)
                    self.assertTrue(path.exists(), target)
