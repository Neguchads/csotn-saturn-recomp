import copy
from pathlib import Path
import tomllib
import unittest
import tempfile
import yaml
from check_ci_config import inspect, load

ROOT = Path(__file__).resolve().parents[1]


class CiContracts(unittest.TestCase):
    def setUp(self):
        self.workflow = load(ROOT / '.github/workflows/ci.yml')
        self.dependabot = load(ROOT / '.github/dependabot.yml')

    def test_production_configuration_passes(self):
        self.assertEqual(inspect(self.workflow, self.dependabot), [])

    def test_loader_rejects_python_object_tags(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'untrusted.yml'
            path.write_text('!!python/tuple [1, 2]', encoding='utf-8')
            with self.assertRaises(yaml.YAMLError):
                load(path)

    def test_privileged_event_and_token_permissions_are_refused(self):
        workflow = copy.deepcopy(self.workflow)
        workflow['on']['pull_request_target'] = ''
        workflow['permissions']['contents'] = 'write'
        errors = inspect(workflow, self.dependabot)
        self.assertTrue(any('pull_request_target' in error for error in errors))
        self.assertTrue(any('contents:read' in error for error in errors))

    def test_untrusted_shell_expression_and_unpinned_action_are_refused(self):
        job = self.workflow['jobs']['quality']
        job['steps'].append({'run': 'echo "${{ github.event.pull_request.title }}"'})
        job['steps'].append({'uses': 'actions/checkout@main'})
        errors = inspect(self.workflow, self.dependabot)
        self.assertTrue(any('interpolada' in error for error in errors))
        self.assertTrue(any('SHA completo' in error for error in errors))

    def test_uv_lock_has_its_own_dependabot_manager(self):
        for entry in self.dependabot['updates']:
            if entry['directory'] == '/engine':
                entry['package-ecosystem'] = 'pip'
        self.assertTrue(any('uv.lock' in error for error in inspect(self.workflow, self.dependabot)))

    def test_reviewer_uses_exact_role_and_inherits_model(self):
        path = ROOT / '.codex/agents/feature-dev-code-reviewer.toml'
        reviewer = tomllib.loads(path.read_text(encoding='utf-8'))
        self.assertEqual(reviewer['name'], 'feature-dev:code-reviewer')
        self.assertTrue(reviewer['developer_instructions'])
        self.assertNotIn('model', reviewer)
        self.assertNotIn('sandbox_mode', reviewer)


if __name__ == '__main__':
    unittest.main()
