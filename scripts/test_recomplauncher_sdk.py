import configparser
import json
from pathlib import Path
import re
import unittest
ROOT=Path(__file__).resolve().parents[1]
class SdkSettings(unittest.TestCase):
 def test_choices_and_defaults_match_actual_runtime_ini(self):
  profile=json.loads((ROOT/'recomplauncher.json').read_text(encoding='utf-8'))
  text=(ROOT/'engine/runtime/src/host/settings.cpp').read_text(encoding='utf-8')
  defaults=text.split('R"(',1)[1].split(')"',1)[0]
  ini=configparser.ConfigParser();ini.read_string(defaults)
  self.assertEqual(profile['config'],[{'id':'main','format':'ini','file':[{'path':'{data}/settings.ini'}]}])
  self.assertGreaterEqual(len(profile['settings']),10)
  for setting in profile['settings']:
   for key,value in setting['defaults'].items():
    section,name=key.split('/')
    self.assertEqual(ini[section][name],value,key)
   for choice in setting.get('choices',[]):
    for key,value in choice['set'].items():
     section,name=key.split('/')
     self.assertIn(name,ini[section]);self.assertIsInstance(value,str)
  self.assertFalse(any('vsync' in json.dumps(s).lower() for s in profile['settings']))
 def test_badge_and_copy_identity_are_explicit(self):
  text=(ROOT/'README.md').read_text(encoding='utf-8')
  self.assertIn('badge-light.svg',text);self.assertIn('badge-dark.svg',text)
  self.assertIn('T-9527G V1.400 (Japan 2M)',text)
  profile=json.loads((ROOT/'recomplauncher.json').read_text(encoding='utf-8'))
  self.assertNotIn('dumps',profile['game'])
  self.assertEqual(profile['system']['windows']['version'],'11')
