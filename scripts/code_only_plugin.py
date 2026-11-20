"""Exclui explicitamente integrações de janela nesta execução; não altera asserts."""
import json
from pathlib import Path

EXCLUDED = {
    'test_bug_report_captures_real_gpu_and_handles_missing_images': 'cria SDL_Window',
    'test_fsr1_real_easu_rcas_d3d12_vulkan': 'cria SDL_Window',
    'test_production_gpu_shaders_on_real_d3d12_renderer': 'cria SDL_Window',
    'test_production_shader_and_hd_scaler_on_real_vulkan': 'cria SDL_Window',
    'test_launcher_restarts_in_title_mode_without_repeating_prologue': 'filho WindowStyle Normal',
    'test_launcher_preserves_failure_metadata_and_shader_log': 'filho WindowStyle Normal',
    'test_normal_disc_boot_preserves_settings_across_launches': 'filho WindowStyle Normal',
}


def pytest_collection_modifyitems(config, items):
    excluded = [item for item in items if item.originalname in EXCLUDED]
    if {item.originalname for item in excluded} != set(EXCLUDED):
        raise RuntimeError('Revisar registro de testes que criam janelas antes de executar')
    config.hook.pytest_deselected(items=excluded)
    items[:] = [item for item in items if item.originalname not in EXCLUDED]
    root = Path(__file__).resolve().parents[1]
    output = root / 'test-results'
    output.mkdir(exist_ok=True)
    (output / 'selection.json').write_text(json.dumps({
        'selected': [item.nodeid for item in items],
        'deselected': [{'case': item.nodeid, 'reason': EXCLUDED[item.originalname]} for item in excluded]
    }, indent=2), encoding='utf-8')
