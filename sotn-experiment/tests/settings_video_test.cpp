#include "settings.h"
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
    Settings defaults;
    assert(defaults.number("display", "resolution") == -1);
    assert(defaults.number("display", "scale") == 3);
    assert(!defaults.flag("display", "integer_scale"));
    assert(defaults.number("display", "filter") == 0);
    assert(defaults.number("display", "sampling") == 1);
    assert(defaults.number("display", "shader") == 0);
    assert(defaults.number("display", "hd_filter") == 0);
    assert(defaults.number("display", "renderer") == 0);
    assert(!defaults.flag("display", "fsr"));
    assert(defaults.number("display", "aspect") == 0);
    assert(!defaults.flag("display", "show_fps"));
    assert(!defaults.flag("display", "pause_unfocused"));
    assert(!defaults.flag("audio", "muted"));
    assert(defaults.flag("gamepad", "enabled"));
    assert(defaults.number("gamepad", "profile") == 0);
    std::vector<std::string> problems;
    defaults.read(Settings::default_text(), problems);
    assert(problems.empty());

    // Arquivos antigos continuam usando a escala salva, sem impor resolução nova.
    const std::string old = "; personal settings\n[display]\nscale = 5\n; keep this comment\n"
                            "[gamepad]\n; keep bindings\na = rightshoulder\nstick = 65\ntrigger = 35\n";
    Settings settings;
    settings.read(old, problems);
    assert(problems.empty());
    assert(settings.number("display", "resolution") == -1);
    assert(settings.number("display", "scale") == 5);
    assert(settings.get("gamepad", "a") == "rightshoulder");
    assert(settings.number("gamepad", "stick") == 65);
    assert(settings.number("gamepad", "trigger") == 35);
    assert(settings.write_over(old) == old);

    const std::string changed = "[display]\nresolution = 4\nscale = 8\ninteger_scale = true\nfilter = 2\nsampling = 0\nshader = 2\naspect = 1\nshow_fps = true\npause_unfocused = true\n[audio]\nmuted = true\n"
                                "[gamepad]\nenabled = false\nprofile = 2\n";
    settings.read(changed, problems);
    assert(problems.empty());
    const std::string saved = settings.write_over(old);
    assert(saved.find("; personal settings") != std::string::npos);
    assert(saved.find("; keep this comment") != std::string::npos);
    assert(saved.find("; keep bindings") != std::string::npos);
    assert(saved.find("a = rightshoulder") != std::string::npos);
    Settings restored;
    restored.read(saved, problems);
    assert(problems.empty());
    assert(restored.number("display", "resolution") == 4);
    assert(restored.number("display", "scale") == 8);
    assert(restored.flag("display", "integer_scale"));
    assert(restored.number("display", "filter") == 2);
    assert(restored.number("display", "sampling") == 0);
    assert(restored.number("display", "shader") == 2);
    assert(restored.number("display", "aspect") == 1);
    assert(restored.flag("display", "show_fps"));
    assert(restored.flag("display", "pause_unfocused"));
    assert(restored.flag("audio", "muted"));
    assert(!restored.flag("gamepad", "enabled"));
    assert(restored.number("gamepad", "profile") == 2);

    // Cada faixa aceita seus extremos e recusa valores fora dela sem substituir o valor válido.
    struct Range { const char* section; const char* key; int lo, hi; };
    for (const Range& range : {Range{"display", "resolution", -1, 7}, {"display", "scale", 1, 8},
                              {"display", "filter", 0, 2}, {"display", "sampling", 0, 1}, {"display", "shader", 0, 2}, {"display", "aspect", 0, 8}, {"display", "hd_filter", 0, 4}, {"display", "renderer", 0, 3},
                              {"gamepad", "profile", 0, 2}}) {
        for (int valid : {range.lo, range.hi}) {
            const std::string prefix = "[" + std::string(range.section) + "]\n" + range.key + " = ";
            Settings checked;
            std::vector<std::string> errors;
            checked.read(prefix + std::to_string(valid) + "\n", errors);
            assert(errors.empty());
            assert(checked.number(range.section, range.key) == valid);
            for (const std::string& invalid : {std::to_string(range.lo - 1), std::to_string(range.hi + 1),
                                                std::string("abc"), std::string("1.5"), std::string("")}) {
                errors.clear();
                checked.read(prefix + invalid + "\n", errors);
                assert(errors.size() == 1);
                assert(checked.number(range.section, range.key) == valid);
            }
        }
    }
    for (const auto& setting : {std::pair{"display", "fsr"}, std::pair{"display", "integer_scale"}, std::pair{"gamepad", "enabled"}, std::pair{"display", "show_fps"}, std::pair{"display", "pause_unfocused"}, std::pair{"audio", "muted"}}) {
        for (const std::string& value : {std::string("true"), std::string("false")}) {
            Settings checked;
            std::vector<std::string> errors;
            const std::string prefix = "[" + std::string(setting.first) + "]\n" + setting.second + " = ";
            checked.read(prefix + value + "\n", errors);
            assert(errors.empty());
            checked.read(prefix + "yes\n", errors);
            assert(errors.size() == 1);
            assert(checked.get(setting.first, setting.second) == value);
        }
    }

    // A seção por GUID também entende os controles novos sem mudar seu mapeamento.
    Settings model;
    problems.clear();
    model.read("[gamepad test-guid]\nenabled = false\nprofile = 1\na = b\n", problems);
    assert(problems.empty());
    assert(!model.flag("gamepad test-guid", "enabled"));
    assert(model.number("gamepad test-guid", "profile") == 1);
    assert(model.get("gamepad test-guid", "a") == "b");
    Settings model_restored;
    model_restored.read(model.write_over(""), problems);
    assert(problems.empty());
    assert(model_restored.number("gamepad test-guid", "profile") == 1);
    assert(model_restored.get("gamepad test-guid", "a") == "b");
    std::cout << "PASS: production video and gamepad settings\n";
}
