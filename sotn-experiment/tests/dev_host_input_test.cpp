// SDL virtual real alimenta os helpers de produção. Não inclui host.cpp completo.
#include "sotn_dev_input.h"
#include <SDL3/SDL.h>
#include <cassert>
#include <cstdio>

static void press(SDL_Joystick* joystick, SDL_GamepadButton button, bool down) {
    assert(SDL_SetJoystickVirtualButton(joystick, int(button), down));
    SDL_UpdateJoysticks();
    SDL_UpdateGamepads();
    SDL_PumpEvents();
}

static SotnDevInput::MenuChord::Sample sample(SotnDevInput::MenuChord& chord, SDL_Gamepad* pad) {
    return chord.sample(SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_START),
        SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_STICK), SDL_GetTicks(),
        SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_STICK));
}

int main() {
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD));
    SDL_VirtualJoystickDesc description;
    SDL_INIT_INTERFACE(&description);
    description.type = SDL_JOYSTICK_TYPE_GAMEPAD;
    description.naxes = SDL_GAMEPAD_AXIS_COUNT;
    description.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
    description.name = "SotN dev input helper gate";
    const auto id = SDL_AttachVirtualJoystick(&description);
    assert(id);
    auto* joystick = SDL_OpenJoystick(id);
    auto* pad = SDL_OpenGamepad(id);
    assert(joystick && pad);

    // Botão Select chega pelo SDL real e não participa do chord.
    SotnDevInput::MenuChord chord;
    press(joystick, SDL_GAMEPAD_BUTTON_BACK, true);
    assert(SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_BACK));
    assert(!sample(chord, pad).open_menu);
    SotnDevInput::MapShortcut map;
    assert(map.request(100, true, false));
    assert(map.buttons(100, false) == B_START);
    assert(map.buttons(103, false) == B_START);
    press(joystick, SDL_GAMEPAD_BUTTON_BACK, false);
    assert(!SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_BACK));
    assert(map.buttons(104, true) == 0);
    assert(map.buttons(105, true) == B_L);
    assert(map.buttons(109, true) == 0 && !map.active());
    assert(!map.request(110, false, false));
    assert(map.buttons(110, false) == 0);

    // Ordem R3 -> Start, latch e soltura parcial.
    press(joystick, SDL_GAMEPAD_BUTTON_RIGHT_STICK, true);
    assert(!sample(chord, pad).open_menu);
    press(joystick, SDL_GAMEPAD_BUTTON_START, true);
    auto state = sample(chord, pad);
    assert(!state.open_menu && !state.guest_start && !chord.consumed());
    state = sample(chord, pad);
    assert(!state.open_menu && !state.guest_start);
    press(joystick, SDL_GAMEPAD_BUTTON_RIGHT_STICK, false);
    assert(!sample(chord, pad).guest_start);
    press(joystick, SDL_GAMEPAD_BUTTON_START, false);
    assert(!sample(chord, pad).open_menu && !chord.consumed());

    // Start -> R3 também não abre o Dev Menu; Start permanece normal.
    press(joystick, SDL_GAMEPAD_BUTTON_START, true);
    assert(!sample(chord, pad).open_menu);
    press(joystick, SDL_GAMEPAD_BUTTON_RIGHT_STICK, true);
    state = sample(chord, pad);
    assert(!state.open_menu);
    press(joystick, SDL_GAMEPAD_BUTTON_START, false);
    press(joystick, SDL_GAMEPAD_BUTTON_RIGHT_STICK, false);
    assert(!sample(chord, pad).open_menu);

    // L3+R3 nas duas ordens pelo joystick SDL real; nenhum Start entregue.
    for (bool left_first : {false,true}) {
        const auto first=left_first ? SDL_GAMEPAD_BUTTON_LEFT_STICK : SDL_GAMEPAD_BUTTON_RIGHT_STICK;
        const auto second=left_first ? SDL_GAMEPAD_BUTTON_RIGHT_STICK : SDL_GAMEPAD_BUTTON_LEFT_STICK;
        press(joystick,first,true);
        assert(!sample(chord,pad).open_menu);
        press(joystick,second,true);
        state=sample(chord,pad);
        assert(state.open_menu && !state.guest_start);
        assert(!sample(chord,pad).open_menu);
        press(joystick,first,false);
        assert(!sample(chord,pad).open_menu);
        press(joystick,second,false);
        assert(!sample(chord,pad).guest_start);
    }

    // Start sozinho mantém toque e botão segurado após a janela.
    press(joystick, SDL_GAMEPAD_BUTTON_START, true);
    assert(!sample(chord, pad).guest_start);
    press(joystick, SDL_GAMEPAD_BUTTON_START, false);
    assert(sample(chord, pad).guest_start);
    SDL_Delay(40);
    assert(!sample(chord, pad).guest_start);
    press(joystick, SDL_GAMEPAD_BUTTON_START, true);
    assert(!sample(chord, pad).guest_start);
    SDL_Delay(190);
    state = sample(chord, pad);
    assert(state.guest_start && !state.open_menu);
    press(joystick, SDL_GAMEPAD_BUTTON_START, false);
    assert(!sample(chord, pad).guest_start);

    SDL_CloseGamepad(pad);
    SDL_CloseJoystick(joystick);
    assert(SDL_DetachVirtualJoystick(id));
    SDL_Quit();
    std::puts("PASS SDL virtual gamepad and production helpers; host.cpp integration not covered");
}
