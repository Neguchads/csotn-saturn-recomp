"""Valida os contratos de integração CI/Dependabot; actionlint valida a sintaxe das Actions."""
from pathlib import Path
import re
import yaml

ROOT = Path(__file__).resolve().parents[1]


def load(path):
    # SafeLoader não constrói objetos Python a partir de tags do documento.
    with path.open(encoding='utf-8') as stream:
        result = yaml.safe_load(stream)
    if not isinstance(result, dict):
        raise ValueError('Esperado um documento YAML com chaves')
    return result


def inspect(workflow, dependabot):
    errors = []
    events = workflow.get('on', {})
    if not isinstance(events, dict) or not {'pull_request', 'push', 'workflow_dispatch'} <= set(events):
        errors.append('CI deve testar PRs, pushes e execução manual')
    if isinstance(events, dict) and 'pull_request_target' in events:
        errors.append('Não usar pull_request_target para executar código de PR')
    if workflow.get('permissions') != {'contents': 'read'}:
        errors.append('CI exige somente contents:read')
    for job in workflow.get('jobs', {}).values():
        if job.get('runs-on') != 'ubuntu-24.04' or 'permissions' in job:
            errors.append('Usar runner público padrão e não ampliar permissões por job')
        for step in job.get('steps', []):
            if 'uses' in step and not re.fullmatch(r'[\w.-]+/[\w./-]+@[0-9a-f]{40}', step['uses']):
                errors.append('Actions precisam estar fixadas por SHA completo')
            if 'secrets.' in str(step):
                errors.append('PRs do Dependabot devem funcionar sem secrets')
            if re.search(r'\$\{\{\s*github\.(?:event|head_ref)', step.get('run', '')):
                errors.append('Entrada do evento não pode ser interpolada em comandos')
            if step.get('uses', '').startswith('actions/checkout@'):
                if step.get('with', {}).get('persist-credentials') is not False or 'ref' in step.get('with', {}):
                    errors.append('Checkout deve usar o evento e não persistir credenciais')
    entries = dependabot.get('updates', [])
    managers = {(entry.get('package-ecosystem'), entry.get('directory')) for entry in entries}
    if dependabot.get('version') != 2 or managers != {('pip', '/'), ('uv', '/engine'), ('github-actions', '/')}:
        errors.append('Dependabot deve cobrir pip, uv.lock e GitHub Actions')
    for entry in entries:
        if entry.get('schedule', {}).get('interval') != 'weekly' or entry.get('open-pull-requests-limit') != 3:
            errors.append('Manter revisão semanal e limite de três PRs por ecossistema')
    return errors


def main():
    errors = inspect(load(ROOT / '.github/workflows/ci.yml'), load(ROOT / '.github/dependabot.yml'))
    if errors:
        raise SystemExit('\n'.join(errors))
    print('PASS: integração CI/Dependabot, eventos, SHAs e permissões')


if __name__ == '__main__':
    main()
