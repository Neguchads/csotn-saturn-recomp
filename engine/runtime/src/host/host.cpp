// saturn-recomp runtime — the host: a window (SDL3's renderer) that shows each
// field VDP2 composes, the pad from the keyboard and a gamepad, the pace
// and the audio stream.
//
// The frame is shown at VBlank-IN (video.cpp), scaled to a 4:3 picture as
// large as the window allows; for a PAL disc the screen is 256 lines tall,
// and a 224-line picture sits in the middle of it, as on a European TV. Time stays virtual (a run with the same pad
// is the same run); with a window, each VBlank-IN waits until the host's
// clock has caught up with it, so the game runs at the Saturn's speed, and
// a host that falls behind by more than 100 ms is not made to catch up.
// --realtime takes the host's clock itself instead.
//
// The pads (standard Saturn pads; smpc.cpp numbers the players across the ports):
//   keyboard  player 1: arrows, Enter START, Z X C = A B C, A S D = X Y Z, Q W = L R
//   gamepads  a player each, in the order they connect (pad_slots.h), the first
//             player 1 beside the keyboard: d-pad or left stick, Start, south A,
//             east B, right shoulder C, west X, north Y, left shoulder Z, triggers L and R
// unless the settings (settings.h) bind them otherwise.
// The touchscreen has a Saturn pad of its own for player 1 (touch_pad.h). Android's Back, Escape, a
// gamepad's Back or Guide, or the menu button at the top of a touchscreen opens a menu that pauses
// the game, sets the touch pad, and quits. F12 saves the picture (out/shot-VBLANK.png); F11 or Alt+Enter switches
// fullscreen. Closing the window, or Quit in the menu, ends the run. Until --resume's VBlank the run
// goes as fast as it can, without sound, dimmed under a card saying how far it has got.
//
// Sound (sound.cpp) comes as it is made, in virtual time, into an SDL audio
// stream at 44 100 Hz: the window's pace keeps it level with the device.
// The stream starts 50 ms ahead; a chunk that would take it past 250 ms is
// dropped (the host's clock and the device's drift apart), and one that
// finds it empty (the host fell behind) is preceded by 50 ms of silence.
#include "saturn.h"
#include "sound.h"
#include "video.h"
#include "host.h"
#include "host_sdl.h"
#include "pace.h"
#include "sotn_video.h"
#include "sotn_gpu_shader.h"
#include "sotn_vdp2_gpu.h"
#include "sotn_hd_scaler.h"
#include "sotn_fsr.h"
#include "sotn_bug_report.h"
#include "sotn_dev_layout.h"
#include "sotn_dev_input.h"
#include "sotn_dev_speed.h"
#include "sotn_dev_report.h"
#include "sotn_map_overlay.h"
#include "sotn_report_layout.h"
#include "video_perf.h"
#include "pad_slots.h"
#include "settings.h"
#include "touch_pad.h"
#include "uinput_pad.h"
#include "unplugs.h"
#include "ui.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <map>

static SDL_Window* g_win;
static PadSlots g_slots(kHostSlots);
static SDL_Renderer* g_ren;
static SDL_Texture* g_tex;
static int g_tex_w, g_tex_h;
static uint16_t g_buttons[kHostSlots];
static const Frame* g_last;
using Clock = PaceClock;
static Clock::time_point g_base;                // host time of virtual time 0
static bool g_paced;
static unsigned g_speed = 1;
static bool g_picture_dirty;
static SotnGpuShader g_shader;
static SotnHdScaler g_hd;
static SotnFsr g_fsr;
static SotnVdp2Gpu g_vdp2_gpu;
static uint64_t g_vdp2_gpu_frames = 0, g_vdp2_cpu_fallbacks = 0;
static std::string g_vdp2_backend = "CPU reference";
static std::string g_vdp2_fallback_reason = "GPU not requested";
static std::string g_vsync_status = "not queried";

bool host_vdp2_gpu_compose(Frame& frame) {
    static bool initialized = false, requested = false, available = false;
    static Vdp2GpuSnapshot snapshot;
    if (!initialized) {
        initialized = true;
        const char* value = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), "SOTN_VDP2_GPU");
        requested = value && *value && std::strcmp(value, "0") != 0;
        if (requested) {
            const char* backend = SDL_GetEnvironmentVariable(SDL_GetEnvironment(), "SOTN_VDP2_GPU_BACKEND");
            available = g_vdp2_gpu.open(backend && *backend ? backend : nullptr);
            g_vdp2_backend = available ? std::string("GPU experimental: ") + g_vdp2_gpu.backend()
                                       : std::string("CPU fallback: ") + SDL_GetError();
            sat_note("VDP2 compositor: %s", g_vdp2_backend.c_str());
        }
    }
    if (!requested) return false;
    if (!available) { g_vdp2_fallback_reason = "GPU unavailable"; ++g_vdp2_cpu_fallbacks; return false; }
    const char* fallback_reason = nullptr;
    if (!vdp2_gpu_snapshot(snapshot, &fallback_reason)) {
        g_vdp2_fallback_reason = fallback_reason ? fallback_reason : "unsupported graphics mode";
        ++g_vdp2_cpu_fallbacks; return false;
    }
    if (!g_vdp2_gpu.compose(snapshot, frame)) {
        available = false;
        g_vdp2_backend = std::string("CPU fallback after GPU error: ") + SDL_GetError();
        sat_note("VDP2 compositor: %s", g_vdp2_backend.c_str());
        ++g_vdp2_cpu_fallbacks;
        g_vdp2_fallback_reason = "GPU operation failed";
        return false;
    }
    ++g_vdp2_gpu_frames;
    g_vdp2_fallback_reason = "none";
    return true;
}
static double g_display_fps;
static unsigned g_fps_frames;
static Clock::time_point g_fps_from;
static bool g_bug_capture_pending;
static Uint64 g_bug_notice_until;
static std::string g_bug_notice;
static SDL_FRect g_dst;                         // Área da imagem renderizada.
static bool g_dev_step;
static std::filesystem::path g_report_folder;
static std::string g_report_title, g_report_description;
static int g_report_edit;
static bool g_open_report;
static SotnMapOverlay::Toggle g_live_map;
static SDL_Texture* g_report_preview;
static int g_report_w, g_report_h;
static SotnDevSpeed::Meter g_sim_meter;
static SotnDevSpeed::Presentation g_presentation;
static bool g_menu_open, g_quitting;
static void menu_open();
static void menu_close();
static void fullscreen_set(bool value);
static void dev_tab(int direction);

static void reset_sim_rate() {
    g_presentation.reset();
    g_sim_meter.reset(sat_now(), SDL_GetTicksNS());
    VideoPerf::reset_window(sat_vblanks());
}

bool host_wants_frame() {
    return g_win && g_presentation.wants(sat_vblanks(), sat_resuming(), g_speed,
        g_bug_capture_pending || g_dev_step, SDL_GetTicksNS(), video_frame_ns());
}
uint16_t host_pad(int slot) { return g_buttons[slot]; }
bool host_pad_connected(int slot) { return g_win && (slot == 0 || g_slots.taken(slot)); }

static void virtual_open();
static bool g_touchscreen;                      // a touchscreen is there, or a finger has been seen

// ---- the settings (settings.h) -----------------------------------------------------------------
static Settings g_settings;
static std::string g_settings_path, g_settings_text;     // the file and its text, which a save writes over
static void bind_keyboard();

static void load_settings(const std::string& named) {
    std::string path = named;
    if (path == "-") path.clear();
    else if (path.empty() && !sat_data_dir().empty()) path = sat_data_dir() + "/saturn-recomp/settings.ini";
    g_settings_path = path;
    g_settings_text = Settings::default_text();
    if (!path.empty()) {
        bool found = false;
        std::string text = settings_load(path, found);
        if (!found) {
            if (settings_store(path, Settings::default_text())) sat_note("settings: %s, written with the defaults", path.c_str());
            else sat_note("settings: cannot write %s (the defaults)", path.c_str());
        } else {
            std::string upgraded = settings_upgrade(text);
            if (upgraded != text) {
                if (settings_store(path, upgraded)) sat_note("settings: %s, the gamepad's default layout updated", path.c_str());
                text = upgraded;
            }
            g_settings_text = text;
            std::vector<std::string> problems;
            g_settings.read(text, problems);
            sat_note("settings: %s", path.c_str());
            for (auto& p : problems) sat_note("%s, so its default", p.c_str());
        }
    }
    bind_keyboard();
}

Settings& host_settings() { return g_settings; }

bool host_settings_save() {
    if (g_settings_path.empty()) return true;
    std::string text = g_settings.write_over(g_settings_text);
    if (!settings_store(g_settings_path, text)) {
        sat_note("settings: cannot write %s", g_settings_path.c_str());
        return false;
    }
    g_settings_text = text;
    return true;
}

SDL_Window* host_sdl_window() { return g_win; }
SDL_Renderer* host_sdl_renderer() { return g_ren; }

bool host_window(const SaturnConfig& cfg) {
    if (g_win) return true;
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) { sat_note("SDL: %s", SDL_GetError()); return false; }
    load_settings(cfg.settings);
    g_speed = cfg.host_speed;
    int s = cfg.scale > 0 ? cfg.scale : g_settings.number("display", "scale");
    int width = 320 * s, height = 240 * s;
    const int resolution = g_settings.number("display", "resolution");
    if (cfg.scale <= 0 && resolution >= 0 && resolution < int(SotnVideo::kResolutions.size())) {
        width = SotnVideo::kResolutions[resolution].width;
        height = SotnVideo::kResolutions[resolution].height;
    }
    bool full = cfg.fullscreen || g_settings.flag("display", "fullscreen");
#if defined(__ANDROID__)
    // SDL picks the phone's orientation itself, from the window, over the manifest's
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    full = true;
#endif
    g_win = SDL_CreateWindow("saturn-recomp", width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                                     (full ? SDL_WINDOW_FULLSCREEN : 0));
    if (!g_win) { sat_note("SDL window: %s", SDL_GetError()); return false; }
    if (full) {
        SDL_DisplayMode mode{};
        if (SDL_GetClosestFullscreenDisplayMode(SDL_GetDisplayForWindow(g_win), width, height, 0, true, &mode))
            if (!SDL_SetWindowFullscreenMode(g_win, &mode)) sat_note("fullscreen mode: %s", SDL_GetError());
    }
    const char* preferred_backend = SDL_getenv("SOTN_VDP2_GPU_BACKEND");
    const int backend = SotnVideo::gpu_backend(g_settings.number("display", "renderer"),
        preferred_backend ? preferred_backend : "");
    if (cfg.title_menu && backend != 2) {
        if (backend == 3) SDL_SetHint(SDL_HINT_GPU_DRIVER, "vulkan");
        else if (backend == 1) SDL_SetHint(SDL_HINT_GPU_DRIVER, "direct3d12");
        const auto properties = SDL_CreateProperties();
        SDL_SetPointerProperty(properties, SDL_PROP_RENDERER_CREATE_WINDOW_POINTER, g_win);
        SDL_SetStringProperty(properties, SDL_PROP_RENDERER_CREATE_NAME_STRING, SDL_GPU_RENDERER);
        SDL_SetBooleanProperty(properties, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_DXIL_BOOLEAN, backend != 3);
        SDL_SetBooleanProperty(properties, SDL_PROP_RENDERER_CREATE_GPU_SHADERS_SPIRV_BOOLEAN, backend == 3);
        g_ren = SDL_CreateRendererWithProperties(properties);
        SDL_DestroyProperties(properties);
        if (!g_ren) sat_note("GPU renderer unavailable, using default: %s", SDL_GetError());
    }
    if (!g_ren) g_ren = SDL_CreateRenderer(g_win, backend == 2 ? "direct3d11" : nullptr);
    if (!g_ren) {
        sat_note("SDL: %s", SDL_GetError());
        return false;
    }
    g_shader.open(g_ren);
    g_hd.open(g_ren);
    g_fsr.open(g_ren);
    sat_note("video: %s", g_shader.status());
    sat_note("SDL: drawing with %s", SDL_GetRendererName(g_ren));
    if (SDL_GPUDevice* device = SDL_GetGPURendererDevice(g_ren))
        sat_note("GPU device driver: %s", SDL_GetGPUDeviceDriver(device));
    ui_open(g_ren);
    // host_pace keeps real time; a present that waited for the display would stall the whole machine
    // whenever the compositor stops sending frames, as KWin does for a window that is out of sight
    const bool vsync_disabled = SDL_SetRenderVSync(g_ren, 0);
    g_vsync_status = vsync_disabled ? "disable accepted" : std::string("disable failed: ") + SDL_GetError();
    int actual_vsync = 0;
    if (SDL_GetRenderVSync(g_ren, &actual_vsync))
        g_vsync_status += "; queried=" + std::to_string(actual_vsync);
    sat_note("renderer VSync: %s", g_vsync_status.c_str());
    return true;
}

bool host_open() {
    if (g_cfg.headless) return true;
    if (!host_window(g_cfg)) return false;
    if (!g_cfg.virtual_input.empty()) virtual_open();
    // Android stops SDL's event pump inside SDL_PollEvent while the app is in the background, before
    // the events reach the run, so a watch sees them as they are sent
    SDL_AddEventWatch([](void*, SDL_Event* e) {
        if (e->type == SDL_EVENT_DID_ENTER_BACKGROUND) ++g_host_paused;
        if (e->type == SDL_EVENT_WILL_ENTER_FOREGROUND) --g_host_paused;
        return true;
    }, nullptr);
    int touch_devices = 0;
    SDL_free(SDL_GetTouchDevices(&touch_devices));
    g_touchscreen = touch_devices > 0;
    g_base = Clock::now();
    g_paced = !g_cfg.realtime;
    return true;
}

// marks the host call the machine's thread is in while it lasts
struct HostCall {
    explicit HostCall(const char* what) { g_host_call = what; }
    ~HostCall() { g_host_call = ""; }
};

// ---- sound ------------------------------------------------------------------------------------
static SDL_AudioStream* g_audio;
static const int kLead = 2205, kMax = 11025;    // frames: 50 ms ahead, 250 ms at most
static int g_underruns, g_dropped;

static void audio_silence(int frames) {
    std::vector<int16_t> z((size_t)frames * 2);
    SDL_PutAudioStreamData(g_audio, z.data(), (int)(z.size() * sizeof(int16_t)));
}

bool host_audio_open() {
    if (!g_win) return false;
    SDL_AudioSpec spec = {SDL_AUDIO_S16, 2, 44100};
    g_audio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!g_audio) { sat_note("SDL audio: %s (no sound)", SDL_GetError()); return false; }
    audio_silence(kLead);
    SDL_SetAudioStreamGain(g_audio, g_settings.flag("audio", "muted") ? 0.0f : g_settings.number("audio", "volume") / 100.0f);
    SDL_ResumeAudioStreamDevice(g_audio);
    return true;
}

void host_audio_push(const int16_t* lr, int frames) {
    if (sat_resuming() || g_speed != 1) return;
    int queued = [] { HostCall in("SDL_GetAudioStreamQueued"); return SDL_GetAudioStreamQueued(g_audio) / 4; }();
    if (queued + frames > kMax) { ++g_dropped; return; }
    if (queued == 0) { ++g_underruns; audio_silence(kLead); }
    HostCall in("SDL_PutAudioStreamData");
    SDL_PutAudioStreamData(g_audio, lr, frames * 4);
}

void host_audio_report() {
    VideoPerf::report(sat_vblanks(), g_tex_w, g_tex_h, true);
    if (g_audio) std::fprintf(stderr, "  audio stream: %d times empty, %d chunks dropped\n", g_underruns, g_dropped);
}

// ---- the pad ----------------------------------------------------------------------------------
// The Saturn's buttons by the settings' names for them.
static const struct { const char* name; uint16_t bit; } kSaturn[] = {
    {"up", B_UP}, {"down", B_DOWN}, {"left", B_LEFT}, {"right", B_RIGHT}, {"start", B_START}, {"a", B_A},
    {"b", B_B}, {"c", B_C}, {"x", B_X}, {"y", B_Y}, {"z", B_Z}, {"l", B_L}, {"r", B_R}};

// The names in a setting's comma-separated list that `known` takes, or, if one is not, the default's.
template <typename F>
static std::vector<int> names(const Settings& set, const std::string& section, const std::string& key, F known) {
    std::vector<int> out;
    std::string list = set.get(section, key);
    for (size_t i = 0; i <= list.size();) {
        size_t j = std::min(list.find(',', i), list.size());
        std::string n = list.substr(i, j - i);
        n.erase(0, n.find_first_not_of(' '));
        n.erase(n.find_last_not_of(' ') + 1);
        i = j + 1;
        if (n.empty()) continue;
        int v = known(n);
        if (v < 0) {
            sat_note("settings: [%s] %s: no %s, so its default", section.c_str(), key.c_str(), n.c_str());
            return names(Settings(), Settings::kind_of(section), key, known);
        }
        out.push_back(v);
    }
    return out;
}

static std::vector<std::pair<SDL_Scancode, uint16_t>> g_keys;

static void bind_keyboard() {
    g_keys.clear();
    for (auto& b : kSaturn)
        for (int k : names(g_settings, "keyboard", b.name, [](const std::string& n) {
                 SDL_Scancode c = SDL_GetScancodeFromName(n.c_str());
                 return c == SDL_SCANCODE_UNKNOWN ? -1 : (int)c;
             }))
            g_keys.push_back({(SDL_Scancode)k, b.bit});
}

// A gamepad's bindings: its buttons and trigger axes, with the stick's and the triggers' travel.
struct PadInput { int button, axis; uint16_t bit; };
struct PadMap { std::vector<PadInput> inputs; int stick, trigger; };
struct OpenPad {
    SDL_Gamepad* pad; SDL_JoystickID id; PadMap map;
    SotnDevInput::MenuChord chord;
    SotnMapOverlay::Shortcut map_chord;
    Uint64 map_last_tick = 0;
};
static std::vector<OpenPad> g_pads;

static PadMap bind_gamepad(const std::string& guid) {
    std::string section = g_settings.has_section("gamepad " + guid) ? "gamepad " + guid : "gamepad";
    PadMap m;
    for (auto& b : kSaturn)
        for (int v : names(g_settings, section, b.name, [](const std::string& n) {
                 if (n == "lefttrigger") return 1000 + (int)SDL_GAMEPAD_AXIS_LEFT_TRIGGER;
                 if (n == "righttrigger") return 1000 + (int)SDL_GAMEPAD_AXIS_RIGHT_TRIGGER;
                 SDL_GamepadButton btn = SDL_GetGamepadButtonFromString(n.c_str());
                 return btn == SDL_GAMEPAD_BUTTON_INVALID ? -1 : (int)btn;
             }))
            m.inputs.push_back(v >= 1000 ? PadInput{-1, v - 1000, b.bit} : PadInput{v, -1, b.bit});
    m.stick = SDL_JOYSTICK_AXIS_MAX * g_settings.number(section, "stick") / 100;
    m.trigger = SDL_JOYSTICK_AXIS_MAX * g_settings.number(section, "trigger") / 100;
    return m;
}

static uint16_t gamepad_buttons(SDL_Gamepad* pad, const PadMap& m) {
    uint16_t b = 0;
    for (auto& in : m.inputs)
        if (in.button >= 0 ? SDL_GetGamepadButton(pad, (SDL_GamepadButton)in.button)
                           : SDL_GetGamepadAxis(pad, (SDL_GamepadAxis)in.axis) > m.trigger)
            b |= in.bit;
    int x = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX), y = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY);
    if (x > m.stick) b |= B_RIGHT;
    if (x < -m.stick) b |= B_LEFT;
    if (y > m.stick) b |= B_DOWN;
    if (y < -m.stick) b |= B_UP;
    return b;
}

static uint16_t dev_gamepad_buttons(OpenPad& source) {
    SDL_Gamepad* pad = source.pad;
    uint16_t b = gamepad_buttons(pad, source.map);
    // Replay de pads virtuais já contém os botões Saturn e não deve ganhar atraso.
    if (!g_cfg.virtual_input.empty()) return b;
    const bool start = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_START);
    const bool r3 = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
    const auto chord = source.chord.sample(start, r3, SDL_GetTicks(),
        SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_LEFT_STICK));
    b &= uint16_t(~B_START);
    if (chord.guest_start) b |= B_START;
    if (chord.open_menu) {
        if (g_menu_open) menu_close(); else menu_open();
    }
    const bool select = SDL_GetGamepadButton(pad, SDL_GAMEPAD_BUTTON_BACK);
    const auto now=SDL_GetTicks();
    const auto map_action=source.map_chord.sample(select,SDL_GetGamepadButton(pad,SDL_GAMEPAD_BUTTON_SOUTH),now);
    if (!g_menu_open && g_slots.slot_of(source.id)==0) {
        const bool available=ld16(0x0605CD70)==5 && ld16(0x0605D744)!=0;
        if (map_action.expand) g_live_map.expand(available);
        else if (map_action.toggle) {
            if (g_live_map.fullscreen()) g_live_map.close(); else g_live_map.toggle(available);
        }
        if (map_action.consume_cross) b &= uint16_t(~B_B);
        if (g_live_map.fullscreen()) {
            const float step=float(std::min<Uint64>(now-source.map_last_tick,50))*.025f;
            g_live_map.pan((bool(b&B_RIGHT)-bool(b&B_LEFT))*step,(bool(b&B_DOWN)-bool(b&B_UP))*step);
            if (b&B_C) g_live_map.close();
            b=0; // Navegar no mapa não movimenta nem ataca com o personagem.
        }
    }
    source.map_last_tick=now;
    return b;
}

static uint16_t g_held_over[kHostSlots];        // held as the menu closed: ignored until let go
static uint16_t touch_read();

static void read_pads() {
    uint16_t b[kHostSlots] = {};
    b[0] |= touch_read();
    const bool* k = SDL_GetKeyboardState(nullptr);
    bool alt = SDL_GetModState() & SDL_KMOD_ALT;
    for (auto& [key, bit] : g_keys)
        if (k[key] && !(alt && key == SDL_SCANCODE_RETURN)) b[0] |= bit;   // Alt+Enter is fullscreen
    if (g_live_map.fullscreen()) b[0]=0;
    for (auto& p : g_pads) {
        if (!g_settings.flag("gamepad", "enabled")) continue;
        int s = g_slots.slot_of(p.id);
        if (s >= 0) b[s] |= dev_gamepad_buttons(p);
    }
    for (int s = 0; s < kHostSlots; ++s) {
        if ((b[s] & (B_LEFT | B_RIGHT)) == (B_LEFT | B_RIGHT)) b[s] &= (uint16_t)~(B_LEFT | B_RIGHT);   // not on a real pad
        if ((b[s] & (B_UP | B_DOWN)) == (B_UP | B_DOWN)) b[s] &= (uint16_t)~(B_UP | B_DOWN);
        g_held_over[s] &= b[s];
        g_buttons[s] = b[s] & (uint16_t)~g_held_over[s];
    }
}

static bool virtual_owns(SDL_JoystickID id);

static void pad_added(SDL_JoystickID id) {
    SDL_Gamepad* pad = SDL_OpenGamepad(id);
    if (!pad) return;
    if (!virtual_owns(id)) {
        sat_note("gamepad: %s, not used: --virtual-input plays on its own pads", SDL_GetGamepadName(pad));
        SDL_CloseGamepad(pad);
        return;
    }
    char guid[33];
    SDL_GUIDToString(SDL_GetGamepadGUIDForID(id), guid, sizeof guid);
    int s = g_slots.connect(id, guid);
    if (s < 0) {
        sat_note("gamepad: %s, not used: every player has one", SDL_GetGamepadName(pad));
        SDL_CloseGamepad(pad);
        return;
    }
    g_pads.push_back({pad, id, bind_gamepad(guid)});
    sat_note("gamepad: %s (GUID %s), player %d", SDL_GetGamepadName(pad), guid, s + 1);
}

static void pad_removed(SDL_JoystickID id) {
    for (size_t i = 0; i < g_pads.size(); ++i)
        if (g_pads[i].id == id) {
            int s = g_slots.disconnect(id);
            sat_note("gamepad: %s gone, player %d free", SDL_GetGamepadName(g_pads[i].pad), s + 1);
            SDL_CloseGamepad(g_pads[i].pad);
            g_pads.erase(g_pads.begin() + (std::ptrdiff_t)i);
            return;
        }
}

void host_rebind() {
    bind_keyboard();
    for (auto& p : g_pads) {
        char guid[33];
        SDL_GUIDToString(SDL_GetGamepadGUIDForID(p.id), guid, sizeof guid);
        p.map = bind_gamepad(guid);
    }
}

void host_pad_event(const SDL_Event& e) {
    if (e.type == SDL_EVENT_GAMEPAD_ADDED) pad_added(e.gdevice.which);
    if (e.type == SDL_EVENT_GAMEPAD_REMOVED) pad_removed(e.gdevice.which);
}

std::vector<HostController> host_controllers() {
    read_pads();
    std::vector<HostController> out;
    for (auto& p : g_pads) {
        int s = g_slots.slot_of(p.id);
        if (s >= 0) out.push_back({SDL_GetGamepadName(p.pad), s, g_buttons[s] != 0});
    }
    std::sort(out.begin(), out.end(), [](const HostController& a, const HostController& b) { return a.slot < b.slot; });
    return out;
}

static void request_bug_report() {
    if (!g_report_folder.empty() && (!g_report_title.empty() || !g_report_description.empty()) &&
        !SotnDevReport::save(g_report_folder,g_report_title,g_report_description)) {
        g_bug_notice="Falha ao salvar anotacao anterior; tente Salvar anotacao";
        g_bug_notice_until=SDL_GetTicks()+5000;
        return;
    }
    g_bug_capture_pending = true;
}

static void capture_bug_report() {
    if (!g_bug_capture_pending) return;
    g_bug_capture_pending = false;
    auto hex = [](uint32_t value) { char text[16]; std::snprintf(text, sizeof text, "%08X", value); return std::string(text); };
    SotnBugReport::Fields fields = {
        {"build", __DATE__ " " __TIME__}, {"reason", "Captura manual para investigar renderizacao"},
        {"stage_id_hex", hex(ld16(0x0605D750))}, {"room_id_hex", hex(ld16(0x0605D754))},
        {"character_id", std::to_string(ld32(0x0605C6D0))}, {"hp", std::to_string(ld32(0x0605C940))},
        {"guest_pc_hex", hex(g_master.pc)}, {"guest_pr_hex", hex(g_master.pr)},
        {"guest_time_ns", std::to_string(sat_now())}, {"display_fps", std::to_string(g_display_fps)},
        {"speed", std::to_string(g_speed)}, {"shader_status", g_shader.status()},
        {"actual_speed", std::to_string(g_sim_meter.actual())},
        {"video_profile_enabled", VideoPerf::enabled() ? "true" : "false"},
        {"vdp2_backend", g_vdp2_backend}, {"vdp2_gpu_frames", std::to_string(g_vdp2_gpu_frames)},
        {"vdp2_cpu_fallbacks", std::to_string(g_vdp2_cpu_fallbacks)},
        {"vdp2_cpu_fallback_reason", g_vdp2_fallback_reason}, {"vsync_status", g_vsync_status},
        {"hd_status", g_hd.status()}, {"fsr_status", g_fsr.status()},
        {"capture_layer", "Game after host effects and viewport; before Esc interface"},
        {"viewport", std::to_string(g_dst.x) + "," + std::to_string(g_dst.y) + "," + std::to_string(g_dst.w) + "," + std::to_string(g_dst.h)}
    };
    std::string enabled_mods;
    for (size_t i = 0; i < sotn_mod_count(); ++i) if (sotn_mod_enabled(i)) {
        if (!enabled_mods.empty()) enabled_mods += "; ";
        enabled_mods += sotn_mod_name(i);
    }
    fields.emplace_back("enabled_mods", enabled_mods.empty() ? "none" : enabled_mods);
    const std::filesystem::path session(std::u8string(g_cfg.out.begin(), g_cfg.out.end()));
    VideoPerf::report(sat_vblanks(),g_tex_w,g_tex_h,true);
    auto result = SotnBugReport::capture(g_ren, session, sat_vblanks(),
        g_last ? g_last->px.data() : nullptr, g_last ? g_last->w : 0, g_last ? g_last->h : 0,
        std::move(fields), g_settings.write_over(g_settings_text));
    if (result.created) {
        const auto graphics_path = (result.folder / "graphics.bin").u8string();
        const bool graphics = video_capture_graphics(std::string(graphics_path.begin(),graphics_path.end()));
        SotnBugReport::text_file(result.folder / "graphics-capture.txt", graphics ?
            "graphics.bin: componentes VDP1/VDP2 completos para diagnostico offline, do mesmo instante da captura.\n"
            "Inclui os dois framebuffers e o interlace. Nao contem save/CPU/WRAM nem retoma partida.\n"
            "Formato SOTNVDP1: magic8, tamanhos u64 little-endian, partes VDP1 e VDP2 deste build.\n" :
            "Falha ao gravar graphics.bin; imagens e logs permanecem disponiveis.\n");
        g_report_folder = result.folder;
        g_report_title.clear(); g_report_description.clear();
        if (g_report_preview) SDL_DestroyTexture(g_report_preview);
        g_report_preview = nullptr;
        if (g_last) {
            g_report_w=g_last->w; g_report_h=g_last->h;
            g_report_preview=SDL_CreateTexture(g_ren,SDL_PIXELFORMAT_XRGB8888,SDL_TEXTUREACCESS_STATIC,g_report_w,g_report_h);
            if (g_report_preview && !SDL_UpdateTexture(g_report_preview,nullptr,g_last->px.data(),g_report_w*4)) {
                SDL_DestroyTexture(g_report_preview); g_report_preview=nullptr;
            }
        }
        g_bug_notice = result.rendered && result.original ? "Captura pronta: anote o problema" : "Captura parcial: anote o problema";
        sat_note("bug report saved: %s", result.folder.string().c_str());
    } else {
        g_bug_notice = "Falha ao salvar relatorio; confira o log";
        sat_note("bug report error: %s", result.error.c_str());
    }
    g_bug_notice_until = SDL_GetTicks() + 5000;
    g_open_report = result.created;
}

// --virtual-input: virtual gamepads, plugged in a player each in order, pressed by a script, so a
// test reaches the game through SDL and the slots as a player's controllers do. They are SDL's own, or
// with --virtual-pads uinput the kernel's (uinput_pad.h), half with an Xbox 360 pad's USB IDs and half
// a pad's SDL does not know. A kernel pad presses each step's directions on the d-pad and the left stick
// by turns. --virtual-unplug unplugs them for a while.
struct VirtualPad {
    SDL_JoystickID id = 0;                      // while plugged in
    UinputPad uinput;
    uint16_t held = 0;                          // the script's buttons, plugged in or not
    bool stick = false;                         // the uinput pad's last directions were on the stick
};
static std::vector<VirtualPad> g_virtual;
static std::vector<PadStep> g_virtual_steps;
static size_t g_virtual_pos;
static std::vector<PlugStep> g_plugs;
static size_t g_plug_pos;
static bool g_uinput;
static void on_event(const SDL_Event& e);

static const OpenPad* open_pad(SDL_JoystickID id) {
    for (auto& p : g_pads)
        if (p.id == id) return &p;
    return nullptr;
}

// With --virtual-input, the run's own pads drive the players, whatever else is plugged in, another
// run's kernel pads included.
static bool virtual_owns(SDL_JoystickID id) {
    if (g_cfg.virtual_input.empty()) return true;
    if (g_cfg.virtual_pads != "uinput") return SDL_IsJoystickVirtual(id);
    const char* path = SDL_GetGamepadPathForID(id);
    for (auto& v : g_virtual)
        if (path && v.uinput.path == path) return true;
    return false;
}

// Pumps the events until done says the host has seen the change.
template <typename F> static void virtual_wait(int k, const char* what, F done) {
    for (Uint64 end = SDL_GetTicks() + 5000; !done();) {
        if (SDL_GetTicks() > end) sat_fatal("--virtual-input: pad %d %s, but the host never saw it", k + 1, what);
        SDL_Event e;
        while (SDL_PollEvent(&e)) on_event(e);
        SDL_Delay(1);
    }
}

static void virtual_send(VirtualPad& v);

// Presses each Saturn button alone and reads it back through the pad's bindings, before the next read of
// the pads, so the game sees none of it. A kernel pad presses the directions on the d-pad, then the stick.
static void virtual_check(VirtualPad& v, int k) {
    const OpenPad* p = open_pad(v.id);
    VirtualPad was = v;
    for (bool stick : {false, true})
        for (uint16_t bit : {B_UP, B_DOWN, B_LEFT, B_RIGHT, B_START, B_A, B_B, B_C, B_X, B_Y, B_Z, B_L, B_R}) {
            v.held = bit;
            v.stick = stick;
            virtual_send(v);
            SDL_Event e;
            while (SDL_PollEvent(&e)) on_event(e);
            if (uint16_t got = gamepad_buttons(p->pad, p->map); got != bit)
                sat_fatal("--virtual-input: pad %d pressed %04X and the host read %04X", k + 1, bit, got);
        }
    v.held = was.held;
    v.stick = was.stick;
}

static void virtual_plug(int k) {
    VirtualPad& v = g_virtual[k];
    if (g_uinput) {
        std::string error;
        v.uinput = uinput_pad_open(k % 2 ? 0x3537 : 0x045E, k % 2 ? 0x0575 : 0x028E, error);
        if (v.uinput.fd < 0) sat_fatal("--virtual-pads uinput: %s", error.c_str());
        virtual_wait(k, "was plugged in", [&] {
            for (auto& p : g_pads)
                if (const char* path = SDL_GetGamepadPath(p.pad); path && v.uinput.path == path) return v.id = p.id, true;
            return false;
        });
    } else {
        SDL_VirtualJoystickDesc d;
        SDL_INIT_INTERFACE(&d);
        d.type = SDL_JOYSTICK_TYPE_GAMEPAD;
        d.naxes = SDL_GAMEPAD_AXIS_COUNT;
        d.nbuttons = SDL_GAMEPAD_BUTTON_COUNT;
        d.name = "saturn-recomp virtual pad";
        v.id = SDL_AttachVirtualJoystick(&d);
        if (!v.id) sat_fatal("--virtual-input: %s", SDL_GetError());
        virtual_wait(k, "was plugged in", [&] { return open_pad(v.id) != nullptr; });
    }
    int s = g_slots.slot_of(v.id);
    if (s != k) sat_fatal("--virtual-input: pad %d drives player %d", k + 1, s + 1);
    virtual_check(v, k);
    virtual_send(v);
}

static void virtual_unplug(int k) {
    VirtualPad& v = g_virtual[k];
    if (g_uinput) uinput_pad_close(v.uinput);
    else SDL_DetachVirtualJoystick(v.id);
    virtual_wait(k, "was unplugged", [&] { return open_pad(v.id) == nullptr; });
    v.id = 0;
}

static void virtual_open() {
    g_virtual_steps = smpc_parse_script(g_cfg.virtual_input);
    if (!parse_unplugs(g_cfg.virtual_unplug, g_plugs)) sat_fatal("--virtual-unplug %s: not PAD:FROM-TO,... with pads 1 to %d and no pad out twice at once", g_cfg.virtual_unplug.c_str(), kHostSlots);
    if (g_cfg.virtual_pads != "sdl" && g_cfg.virtual_pads != "uinput") sat_fatal("--virtual-pads %s: sdl or uinput", g_cfg.virtual_pads.c_str());
    g_uinput = g_cfg.virtual_pads == "uinput";
    int n = 0;
    for (auto& s : g_virtual_steps) n = std::max(n, s.pad + 1);
    for (auto& p : g_plugs) n = std::max(n, p.pad + 1);
    if (n > kHostSlots) sat_fatal("--virtual-input: %d pads, more than the %d players", n, kHostSlots);
    g_virtual.resize(n);
    for (int k = 0; k < n; ++k) virtual_plug(k);
}

static void virtual_send(VirtualPad& v) {
    uint16_t b = v.held;
    if (!g_uinput) {
        static const struct { uint16_t bit; SDL_GamepadButton btn; } kButtons[] = {
            {B_RIGHT, SDL_GAMEPAD_BUTTON_DPAD_RIGHT}, {B_LEFT, SDL_GAMEPAD_BUTTON_DPAD_LEFT},
            {B_DOWN, SDL_GAMEPAD_BUTTON_DPAD_DOWN}, {B_UP, SDL_GAMEPAD_BUTTON_DPAD_UP},
            {B_START, SDL_GAMEPAD_BUTTON_START}, {B_A, SDL_GAMEPAD_BUTTON_WEST}, {B_B, SDL_GAMEPAD_BUTTON_SOUTH},
            {B_C, SDL_GAMEPAD_BUTTON_EAST}, {B_X, SDL_GAMEPAD_BUTTON_NORTH},
            {B_L, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER}, {B_R, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER}};
        SDL_Joystick* j = SDL_GetJoystickFromID(v.id);
        for (auto& m : kButtons) SDL_SetJoystickVirtualButton(j, m.btn, (b & m.bit) != 0);
        // a trigger at rest is the axis's minimum: 0 is half pulled
        SDL_SetJoystickVirtualAxis(j, SDL_GAMEPAD_AXIS_LEFT_TRIGGER, b & B_Y ? SDL_JOYSTICK_AXIS_MAX : SDL_JOYSTICK_AXIS_MIN);
        SDL_SetJoystickVirtualAxis(j, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, b & B_Z ? SDL_JOYSTICK_AXIS_MAX : SDL_JOYSTICK_AXIS_MIN);
        return;
    }
    int x = (b & B_RIGHT ? 1 : 0) - (b & B_LEFT ? 1 : 0), y = (b & B_DOWN ? 1 : 0) - (b & B_UP ? 1 : 0);
    auto on = [&](uint16_t bit) { return (b & bit) != 0; };
    UinputState s = {on(B_B), on(B_C), on(B_A), on(B_X), on(B_L), on(B_R), on(B_START), on(B_Y), on(B_Z),
                     v.stick ? 0 : x, v.stick ? 0 : y, v.stick ? x : 0, v.stick ? y : 0};
    uinput_pad_set(v.uinput, s);
}

static void virtual_press() {
    for (; g_plug_pos < g_plugs.size() && g_plugs[g_plug_pos].vblank <= sat_vblanks(); ++g_plug_pos) {
        const PlugStep& p = g_plugs[g_plug_pos];
        if (p.plugged) virtual_plug(p.pad);
        else virtual_unplug(p.pad);
    }
    for (; g_virtual_pos < g_virtual_steps.size() && g_virtual_steps[g_virtual_pos].vblank <= sat_vblanks(); ++g_virtual_pos) {
        const PadStep& s = g_virtual_steps[g_virtual_pos];
        VirtualPad& v = g_virtual[s.pad];
        v.held = s.pressed;
        if (s.pressed & (B_UP | B_DOWN | B_LEFT | B_RIGHT)) v.stick = !v.stick;
        if (v.id) virtual_send(v);
    }
}

// ---- the touchscreen's pad (touch_pad.h) ------------------------------------------------------
static std::map<SDL_FingerID, TouchFinger> g_fingers;
// Fingers lifted before the pads were read since they went down: each holds its button for that one read,
// so a tap quicker than a frame still presses it.
static std::vector<SDL_FingerID> g_lifted;
static std::map<SDL_FingerID, bool> g_unread;
static uint16_t g_touch_held;

static int g_out_w, g_out_h;                    // the window's size in pixels, as last drawn

static bool touch_shown() {
    std::string mode = g_settings.get("touch", "controls");
    if (mode == "on") return true;
    if (mode == "off") return false;
    return g_touchscreen && g_pads.empty();
}

static bool menu_button_shown() { return g_touchscreen && g_settings.flag("touch", "menu_button"); }

// The window's pixels to a density-independent pixel: Android's density, 1 on most desktops.
static float display_scale() {
    float s = g_win ? SDL_GetWindowDisplayScale(g_win) : 0;
    return s > 0 ? s : 1;
}

// The pad's height on the screen: 480 density-independent pixels at its default size.
static TouchLayout pad_layout() {
    return touch_layout(g_out_w, g_out_h, 4.8f * display_scale() * (float)g_settings.number("touch", "size"));
}

static uint16_t touch_read() {
    std::vector<TouchFinger> fingers;
    for (auto& [id, f] : g_fingers) fingers.push_back(f);
    for (SDL_FingerID id : g_lifted) g_fingers.erase(id);
    g_lifted.clear();
    g_unread.clear();
    if (!touch_shown() || fingers.empty()) return g_touch_held = 0;
    return g_touch_held = touch_buttons(pad_layout(), fingers);
}

// ---- the menu ---------------------------------------------------------------------------------
// It pauses the game: virtual time stands still while it is open, and the host's clock is moved on
// by as long as it was, so the run is the same run with or without it. A panel of two columns where
// the screen is wide enough, the game's choices in one and the touch pad's in the other, each a row:
// an action, a choice of a few, a slider, or a switch.
enum MenuId { M_RESUME, M_SAVES, M_QUIT, M_PAD, M_SIZE, M_STYLE, M_MENU_BUTTON,
              M_TITLE, M_SPEED, M_VOLUME, M_FULLSCREEN,
              M_GRAPHICS, M_CONTROLLERS, M_RESOLUTION, M_SCALE, M_INTEGER, M_FILTER, M_SAMPLING, M_SHADER,
              M_PAD_ENABLED, M_PAD_PROFILE, M_DEADZONE, M_PAD_RESET,
              M_ASPECT, M_FPS, M_MUTE, M_PAUSE_FOCUS, M_SCREENSHOT, M_VIDEO_RESET, M_HD, M_RENDERER,
              M_EFFECTS, M_AUDIO, M_FSR, M_BUG_REPORT,
              M_DEV_DIAGNOSTICS, M_DEV_STEP, M_DEV_NORMAL, M_DEV_PROFILE, M_DEV_DUMP, M_DEV_LIGHT,
              M_INFO_STAGE, M_INFO_PLAYER, M_INFO_TIME, M_INFO_PC, M_INFO_MODULES, M_INFO_RENDERER, M_INFO_EFFECTS,
              M_MODS, M_MOD_BACK, M_MOD_HEAL, M_MOD_OFF, M_MOD_RELOAD, M_MOD_PREV, M_MOD_NEXT,
              M_REPORT, M_REPORT_TITLE, M_REPORT_DESCRIPTION, M_REPORT_SAVE, M_OPTIONS_SAVE, M_STATES,
              M_STATE_STATUS, M_STATE_SAVE, M_STATE_LOAD,
              M_MOD_FIRST = 100 };
enum class Control { Action, Choice, Cycle, Slider, Switch, Readout, Disabled };
struct MenuRow { MenuId id; Control control; int column; SDL_FRect r; };
struct MenuLayout { SDL_FRect panel; float u; float title_y; float section_y[2]; float column_x[2]; std::vector<MenuRow> rows; };

static bool g_title_confirm;
static bool g_mod_menu;
static int g_options_menu; // 0: principal, 1: vídeo, 2: controles.
static size_t g_mod_page;
static const size_t kModsPerPage = 12;
static int g_menu_at;                           // the focused row, by index into the layout's rows
static SDL_FingerID g_slider_finger;            // the finger moving the size slider, or 0
static MenuId g_slider_id = M_SIZE;
static const char* const kPadModes[] = {"auto", "on", "off"};
static const char* const kPadModeNames[] = {"Auto", "On", "Off"};
static const char* const kStyles[] = {"western", "japanese"};
static const char* const kStyleNames[] = {"Western", "Japanese"};

static void setting_change(const char* key, const char* value) {
    g_settings.set("touch", key, value);
    host_settings_save();
}

static int choice_count(MenuId id) {
    if (id == M_SPEED) return int(SotnVideo::kSpeeds.size());
    if (id == M_RESOLUTION) return int(SotnVideo::kResolutions.size()) + 1;
    if (id == M_SCALE) return 8;
    if (id == M_HD) return int(SotnVideo::kHdFactors.size());
    if (id == M_ASPECT) return int(SotnVideo::kAspectNames.size());
    if (id == M_RENDERER) return 4;
    if (id == M_PAD || id == M_FILTER || id == M_PAD_PROFILE || id == M_SHADER || id == M_RENDERER) return 3;
    return 2;
}
static const char* choice_name(MenuId id, int i) {
    static const char* speeds[] = {"1x", "2x", "4x", "6x", "8x", "10x"};
    static const char* scales[] = {"1x", "2x", "3x", "4x", "5x", "6x", "7x", "8x"};
    static const char* filters[] = {"Original", "Scanlines", "LCD"};
    static const char* shaders[] = {"Desligado", "CRT GPU", "LCD GPU"};
    static const char* profiles[] = {"Auto", "Xbox", "PlayStation"};
    static const char* sampling[] = {"Pixel", "Suave"};
    static const char* hd[] = {"Desligado", "HD 2x", "HD 4x", "HD 6x", "HD 8x"};
    static const char* renderers[] = {"Auto GPU", "GPU D3D12", "Direct3D11", "Vulkan"};
    if (id == M_SPEED) return speeds[i];
    if (id == M_SCALE) return scales[i];
    if (id == M_RESOLUTION) return i == 0 ? "Escala / livre" : SotnVideo::kResolutions[i - 1].name;
    if (id == M_FILTER) return filters[i];
    if (id == M_SHADER) return shaders[i];
    if (id == M_ASPECT) return SotnVideo::kAspectNames[i];
    if (id == M_HD) return hd[i];
    if (id == M_RENDERER) return renderers[i];
    if (id == M_SAMPLING) return sampling[i];
    if (id == M_PAD_PROFILE) return profiles[i];
    return id == M_PAD ? kPadModeNames[i] : kStyleNames[i];
}

static int choice_at(MenuId id) {
    if (id == M_SPEED) {
        for (int i = 0; i < choice_count(id); ++i) if (SotnVideo::kSpeeds[i] == g_speed) return i;
        return 0;
    }
    if (id == M_RESOLUTION) return std::clamp(g_settings.number("display", "resolution") + 1, 0, choice_count(id) - 1);
    if (id == M_SCALE) return std::clamp(g_settings.number("display", "scale") - 1, 0, 7);
    if (id == M_FILTER || id == M_SAMPLING) return std::clamp(g_settings.number("display", id == M_FILTER ? "filter" : "sampling"), 0, choice_count(id) - 1);
    if (id == M_SHADER) return g_shader.enabled() ? std::clamp(g_settings.number("display", "shader"), 0, 2) : 0;
    if (id == M_ASPECT) return std::clamp(g_settings.number("display", "aspect"), 0, int(SotnVideo::kAspectNames.size()) - 1);
    if (id == M_HD) return std::clamp(g_settings.number("display", "hd_filter"), 0, int(SotnVideo::kHdFactors.size()) - 1);
    if (id == M_RENDERER) return std::clamp(g_settings.number("display", "renderer"), 0, 3);
    if (id == M_PAD_PROFILE) return std::clamp(g_settings.number("gamepad", "profile"), 0, 2);
    std::string v = g_settings.get("touch", id == M_PAD ? "controls" : "style");
    for (int i = 0; i < choice_count(id); ++i)
        if (v == (id == M_PAD ? kPadModes[i] : kStyles[i])) return i;
    return 0;
}

static void choose(MenuId id, int i) {
    i = (i + choice_count(id)) % choice_count(id);
    if (id == M_SPEED) {
        g_speed = SotnVideo::kSpeeds[i];
        reset_sim_rate();
        if (g_audio) { SDL_ClearAudioStream(g_audio); audio_silence(kLead); }
        sat_note("host speed: %ux (audio %s)", g_speed, g_speed == 1 ? "on" : "muted");
        return;
    }
    if (id == M_RESOLUTION || id == M_SCALE) {
        int width = 320 * (i + 1), height = 240 * (i + 1);
        if (id == M_RESOLUTION) {
            g_settings.set("display", "resolution", std::to_string(i - 1));
            if (i == 0) { width = 320 * g_settings.number("display", "scale"); height = 240 * g_settings.number("display", "scale"); }
            else { width = SotnVideo::kResolutions[i - 1].width; height = SotnVideo::kResolutions[i - 1].height; }
        } else {
            g_settings.set("display", "scale", std::to_string(i + 1));
            g_settings.set("display", "resolution", "-1");
        }
        const bool full = (SDL_GetWindowFlags(g_win) & SDL_WINDOW_FULLSCREEN) != 0;
        if (full) {
            SDL_DisplayMode mode{};
            if (SDL_GetClosestFullscreenDisplayMode(SDL_GetDisplayForWindow(g_win), width, height, 0, true, &mode)) {
                if (!SDL_SetWindowFullscreenMode(g_win, &mode)) sat_note("fullscreen mode: %s", SDL_GetError());
            } else sat_note("fullscreen resolution unavailable: %s", SDL_GetError());
        } else if (!SDL_SetWindowSize(g_win, width, height)) sat_note("window size: %s", SDL_GetError());
        host_settings_save();
        return;
    }
    if (id == M_FILTER || id == M_SAMPLING || id == M_PAD_PROFILE) {
        g_settings.set(id == M_PAD_PROFILE ? "gamepad" : "display", id == M_PAD_PROFILE ? "profile" : id == M_FILTER ? "filter" : "sampling", std::to_string(i));
        g_picture_dirty = true;
        host_settings_save();
        return;
    }
    if (id == M_SHADER) {
        g_settings.set("display", "shader", std::to_string(g_shader.enabled() ? i : 0));
        host_settings_save(); return;
    }
    if (id == M_ASPECT) {
        g_settings.set("display", "aspect", std::to_string(i)); host_settings_save(); return;
    }
    if (id == M_HD || id == M_RENDERER) {
        if (id == M_HD && i > 0) g_hd.open(g_ren);
        if (id == M_HD && i > 0) g_settings.set("display", "fsr", "false");
        g_settings.set("display", id == M_HD ? "hd_filter" : "renderer", std::to_string(i));
        host_settings_save(); return;
    }
    setting_change(id == M_PAD ? "controls" : "style", id == M_PAD ? kPadModes[i] : kStyles[i]);
}

// The pad's size in percent, from half to twice its default, a tenth at a time.
static void pad_size_set(int size) {
    size = std::clamp((size + 5) / 10 * 10, 50, 200);
    setting_change("size", std::to_string(size).c_str());
}

static void volume_set(int volume) {
    volume = std::clamp(volume, 0, 100);
    g_settings.set("audio", "volume", std::to_string(volume));
    host_settings_save();
    if (g_audio) SDL_SetAudioStreamGain(g_audio, g_settings.flag("audio", "muted") ? 0.0f : volume / 100.0f);
}

static void mute_set(bool mute) {
    g_settings.set("audio", "muted", mute ? "true" : "false");
    if (g_audio) SDL_SetAudioStreamGain(g_audio, mute ? 0.0f : g_settings.number("audio", "volume") / 100.0f);
    host_settings_save();
}

static void deadzone_set(int value) {
    const auto text = std::to_string(std::clamp(value, 5, 95));
    g_settings.set("gamepad", "stick", text);
    for (const auto& p : g_pads) {
        char guid[33]; SDL_GUIDToString(SDL_GetGamepadGUIDForID(p.id), guid, sizeof guid);
        if (g_settings.has_section(std::string("gamepad ") + guid))
            g_settings.set(std::string("gamepad ") + guid, "stick", text);
    }
    host_rebind(); host_settings_save();
}

static bool fullscreen_on() { return (SDL_GetWindowFlags(g_win) & SDL_WINDOW_FULLSCREEN) != 0; }
static void fullscreen_set(bool value) {
    if (!SDL_SetWindowFullscreen(g_win, value)) { sat_note("fullscreen: %s", SDL_GetError()); return; }
    g_settings.set("display", "fullscreen", value ? "true" : "false");
    host_settings_save();
}

static const char* menu_label(MenuId id) {
    if (id >= M_MOD_FIRST) return sotn_mod_name(size_t(id - M_MOD_FIRST));
    switch (id) {
    case M_REPORT: return "Relatar bug";
    case M_REPORT_TITLE: return "Editar titulo";
    case M_REPORT_DESCRIPTION: return "Editar descricao";
    case M_REPORT_SAVE: return "Salvar relato local";
    case M_OPTIONS_SAVE: return "Salvar opcoes";
    case M_STATES: return "Save states";
    case M_STATE_STATUS: return "Suporte neste SotN";
    case M_STATE_SAVE: return "Salvar estado (indisponivel)";
    case M_STATE_LOAD: return "Carregar estado (indisponivel)";
    case M_DEV_DIAGNOSTICS: return "Diagnostico da sessao";
    case M_DEV_STEP: return "Avancar um quadro";
    case M_DEV_NORMAL: return "Voltar para 1x";
    case M_DEV_PROFILE: return "Medir desempenho";
    case M_DEV_DUMP: return "Gravar ultima medicao";
    case M_DEV_LIGHT: return "Perfil leve (sem efeitos)";
    case M_INFO_STAGE: return "Fase / sala";
    case M_INFO_PLAYER: return "Personagem / HP";
    case M_INFO_TIME: return "VBlank / tempo virtual";
    case M_INFO_PC: return "SH-2 PC / PR";
    case M_INFO_MODULES: return "Modulos ativos";
    case M_INFO_RENDERER: return "Renderizador";
    case M_INFO_EFFECTS: return "Efeito ativo";
    case M_ASPECT: return "Proporcao";
    case M_EFFECTS: return "Qualidade e shaders";
    case M_AUDIO: return "Audio e sistema";
    case M_FSR: return "AMD FSR 1";
    case M_HD: return "Filtro HD";
    case M_RENDERER: return "Backend*";
    case M_FPS: return "Mostrar FPS";
    case M_MUTE: return "Sem som (F9)";
    case M_PAUSE_FOCUS: return "Pausar ao sair";
    case M_SCREENSHOT: return "Capturar e anotar (F12)";
    case M_BUG_REPORT: return "Capturar bug (F8)";
    case M_VIDEO_RESET: return "Restaurar video padrao";
    case M_GRAPHICS: return "Tela e resolucao";
    case M_CONTROLLERS: return "Controles";
    case M_RESOLUTION: return "Resolucao";
    case M_SCALE: return "Escala janela";
    case M_INTEGER: return "Escala inteira";
    case M_FILTER: return "Efeito visual";
    case M_SHADER: return "Shader GPU";
    case M_SAMPLING: return "Filtro";
    case M_PAD_ENABLED: return "Usar controle";
    case M_PAD_PROFILE: return "Botoes";
    case M_DEADZONE: return "Zona morta";
    case M_PAD_RESET: return "Restaurar botoes padrao";
    case M_MODS: return "Laboratorio de hacks";
    case M_MOD_BACK: return "Voltar ao menu";
    case M_MOD_HEAL: return "Restaurar HP / MP / coracoes";
    case M_MOD_OFF: return "Desligar todos os hacks";
    case M_MOD_RELOAD: return "Recarregar scripts do arquivo";
    case M_MOD_PREV: return "Pagina anterior";
    case M_MOD_NEXT: return "Proxima pagina";
    default: break;
    }
    if (g_cfg.title_menu) {
        switch (id) {
        case M_RESUME: return "Continuar";
        case M_TITLE: return g_title_confirm ? "Confirmar retorno" : "Voltar ao titulo";
        case M_QUIT: return "Sair do jogo";
        case M_SPEED: return "Velocidade";
        case M_VOLUME: return "Volume";
        case M_FULLSCREEN: return "Tela cheia";
        default: break;
        }
    }
    switch (id) {
    case M_REPORT_TITLE: case M_REPORT_DESCRIPTION: return "Digite pelo teclado. Enter conclui; Backspace apaga. Tab troca de campo.";
    case M_REPORT_SAVE: return "Acrescenta titulo e descricao a ultima captura. Nada enviado automaticamente.";
    case M_STATES: case M_STATE_SAVE: case M_STATE_LOAD: case M_STATE_STATUS:
        return "Pendente: pontos seguros e restauracao de tarefas SH-2. Save normal do jogo continua disponivel.";
    case M_OPTIONS_SAVE: return "Persiste video/audio/controles. Hacks e velocidade nao sao reativados no proximo inicio.";
    case M_RESUME: return "Resume";
    case M_SAVES: return "Save files";
    case M_QUIT: return "Quit";
    case M_PAD: return "Show pad";
    case M_SIZE: return "Size";
    case M_STYLE: return "Style";
    default: return "Menu button";
    }
}

static const char* menu_help(MenuId id) {
    if (id >= M_MOD_FIRST) return sotn_mod_help(size_t(id - M_MOD_FIRST));
    switch (id) {
    case M_DEV_DIAGNOSTICS: return "Estado SH-2, modulos e video. Leitura sem modificar memoria.";
    case M_DEV_STEP: return "Avanca ate o proximo quadro apresentado e pausa novamente em 1x.";
    case M_DEV_NORMAL: return "Restaura 1x; o audio volta se nao estiver no mudo.";
    case M_DEV_PROFILE: return "Registra tempos de VDP1, VDP2, upload e apresentacao a cada 300 quadros.";
    case M_DEV_DUMP: return "Grava a ultima janela ativa no run.log; tempo de pausa fica fora da medicao.";
    case M_DEV_LIGHT: return "Desliga HD, FSR e shaders. Preserva resolucao, proporcao e save.";
    case M_INFO_STAGE: case M_INFO_PLAYER: case M_INFO_TIME: case M_INFO_PC:
    case M_INFO_MODULES: case M_INFO_RENDERER: case M_INFO_EFFECTS:
        return "Somente leitura; valores no titulo podem nao representar uma partida ativa.";
    case M_RENDERER: return "Vulkan e D3D12 aceitam nossos shaders. Reinicie para aplicar.";
    case M_HD: return "2x / 4x / 6x / 8x: reconstrucao de contornos na GPU.";
    case M_FSR: return "FSR1 oficial AMD: escala espacial e nitidez. Substitui o filtro HD.";
    case M_ASPECT: return "Ajusta a proporcao da imagem. Nao amplia o campo de visao.";
    case M_RESOLUTION: case M_SCALE: return "Muda o tamanho da janela; sprites mantem sua arte original.";
    case M_SHADER: return "CRT e LCD afetam a imagem do jogo; o menu continua limpo.";
    case M_SPEED: return "Turbo reduz quadros exibidos; simulacao e controles continuam. Velocidade real no rodape.";
    case M_TITLE: return "Pede confirmacao. Progresso ainda nao salvo sera perdido.";
    case M_PAUSE_FOCUS: return "Abre a pausa quando outra janela recebe o foco.";
    case M_FPS: return "Quadros apresentados por segundo; nao conta animacoes unicas.";
    case M_BUG_REPORT: return "Salva imagens original/renderizada, configuracao e logs em logs-e-bugs.";
    case M_PAD_PROFILE: return "Muda os nomes exibidos; preserve suas posicoes de botoes.";
    case M_GRAPHICS: return "Resolucao, proporcao, tela cheia e escolha do renderizador.";
    case M_EFFECTS: return "Filtros HD, AMD FSR1 e shaders para ajustar a imagem.";
    case M_AUDIO: return "Volume, mudo, velocidade e pausa ao trocar de janela.";
    case M_CONTROLLERS: return "Xbox, PlayStation e outros controles reconhecidos pelo SDL.";
    case M_VIDEO_RESET: return "Restaura a janela e os efeitos; preserva saves e scripts.";
    default: return "Setas: navegar e ajustar    Enter / A: confirmar    Esc / B: voltar";
    }
}

static MenuLayout menu_layout() {
    std::vector<std::pair<MenuId, Control>> game = {{M_RESUME, Control::Action}};
    if (!g_cfg.after.empty()) game.push_back({g_cfg.title_menu ? M_TITLE : M_SAVES, Control::Action});
    if (g_cfg.title_menu) {
        game.push_back({M_GRAPHICS, Control::Action});
        game.push_back({M_CONTROLLERS, Control::Action});
        game.push_back({M_AUDIO, Control::Action});
        game.push_back({M_DEV_DIAGNOSTICS, Control::Action});
        game.push_back({M_REPORT, Control::Action});
        game.push_back({M_STATES, Control::Action});
    }
    if (!g_cfg.sotn_mods.empty()) game.push_back({M_MODS, Control::Action});
    game.push_back({M_QUIT, Control::Action});
    std::vector<std::pair<MenuId, Control>> touch = g_cfg.title_menu ?
        std::vector<std::pair<MenuId, Control>>{{M_SPEED, Control::Cycle}, {M_DEV_NORMAL, Control::Action},
            {M_DEV_STEP, Control::Action}, {M_DEV_PROFILE, Control::Switch}, {M_DEV_DUMP, Control::Action},
            {M_BUG_REPORT, Control::Action}, {M_DEV_LIGHT, Control::Action}, {M_OPTIONS_SAVE, Control::Action}} :
        std::vector<std::pair<MenuId, Control>>{{M_PAD, Control::Choice}, {M_SIZE, Control::Slider}, {M_STYLE, Control::Choice}, {M_MENU_BUTTON, Control::Switch}};
    if (g_options_menu == 1) {
        game = {{M_MOD_BACK, Control::Action}, {M_RESOLUTION, Control::Cycle}, {M_FULLSCREEN, Control::Switch},
            {M_ASPECT, Control::Cycle}, {M_INTEGER, Control::Switch}, {M_RENDERER, Control::Cycle}, {M_OPTIONS_SAVE, Control::Action}};
        touch = {{M_HD, Control::Cycle}, {M_FSR, Control::Switch}, {M_SHADER, Control::Cycle},
            {M_SAMPLING, Control::Choice}, {M_FILTER, Control::Cycle}, {M_FPS, Control::Switch}, {M_VIDEO_RESET, Control::Action}};
    } else if (g_options_menu == 3) {
        game = {{M_MOD_BACK, Control::Action}, {M_HD, Control::Cycle}, {M_FSR, Control::Switch}};
        touch = {{M_SHADER, Control::Cycle}, {M_SAMPLING, Control::Choice}, {M_FILTER, Control::Cycle}, {M_GRAPHICS, Control::Action}};
    } else if (g_options_menu == 4) {
        game = {{M_MOD_BACK, Control::Action}, {M_VOLUME, Control::Slider}, {M_MUTE, Control::Switch}};
        touch = {{M_SPEED, Control::Cycle}, {M_PAUSE_FOCUS, Control::Switch}, {M_SCREENSHOT, Control::Action}};
    } else if (g_options_menu == 2) {
        game = {{M_MOD_BACK, Control::Action}, {M_PAD_ENABLED, Control::Switch}, {M_PAD_RESET, Control::Action}};
        touch = {{M_PAD_PROFILE, Control::Cycle}, {M_DEADZONE, Control::Slider}};
    } else if (g_options_menu == 5) {
        game = {{M_MOD_BACK, Control::Action}, {M_BUG_REPORT, Control::Action}, {M_SCREENSHOT, Control::Action},
            {M_DEV_PROFILE, Control::Switch}, {M_DEV_DUMP, Control::Action}, {M_DEV_STEP, Control::Action},
            {M_DEV_NORMAL, Control::Action}, {M_DEV_LIGHT, Control::Action}};
        touch = {{M_INFO_STAGE, Control::Readout}, {M_INFO_PLAYER, Control::Readout}, {M_INFO_TIME, Control::Readout},
            {M_INFO_PC, Control::Readout}, {M_INFO_MODULES, Control::Readout}, {M_INFO_RENDERER, Control::Readout},
            {M_INFO_EFFECTS, Control::Readout}};
    } else if (g_options_menu == 6) {
        game = {{M_MOD_BACK, Control::Action}, {M_BUG_REPORT, Control::Action}, {M_REPORT_TITLE, Control::Action},
            {M_REPORT_DESCRIPTION, Control::Action}, {M_REPORT_SAVE, Control::Action}};
        touch = {{M_INFO_STAGE, Control::Readout}, {M_INFO_EFFECTS, Control::Readout}, {M_INFO_TIME, Control::Readout}};
    } else if (g_options_menu == 7) {
        game = {{M_MOD_BACK, Control::Action}, {M_STATE_SAVE, Control::Disabled}, {M_STATE_LOAD, Control::Disabled}};
        touch = {{M_STATE_STATUS, Control::Readout}};
    } else if (g_mod_menu) {
        game = {{M_MOD_BACK, Control::Action}, {M_MOD_HEAL, Control::Action},
                {M_MOD_OFF, Control::Action}, {M_MOD_RELOAD, Control::Action}};
        if (sotn_mod_count() > kModsPerPage) {
            game.push_back({M_MOD_PREV, Control::Action});
            game.push_back({M_MOD_NEXT, Control::Action});
        }
        touch.clear();
        const size_t pages = std::max(size_t(1), (sotn_mod_count() + kModsPerPage - 1) / kModsPerPage);
        g_mod_page = std::min(g_mod_page, pages - 1);
        for (size_t i = g_mod_page * kModsPerPage; i < std::min(sotn_mod_count(), (g_mod_page + 1) * kModsPerPage); ++i)
            touch.push_back({MenuId(M_MOD_FIRST + i), Control::Switch});
    }
    const auto geometry = SotnDevLayout::make(g_out_w, g_out_h, game.size(), touch.size(), display_scale());
    MenuLayout l;
    l.u = geometry.u;
    l.panel = {geometry.panel.x, geometry.panel.y, geometry.panel.w, geometry.panel.h};
    l.title_y = geometry.title_y;
    for (int c = 0; c < 2; ++c) {
        auto& items = c == 0 ? game : touch;
        l.column_x[c] = geometry.column_x[c];
        l.section_y[c] = geometry.section_y[c];
        for (size_t i = 0; i < items.size(); ++i) {
            const auto rect = geometry.cell(c, i);
            l.rows.push_back({items[i].first, items[i].second, c, {rect.x, rect.y, rect.w, rect.h}});
        }
    }
    return l;
}

// The parts of a row's control: a choice's segments, or a slider's minus, track and plus.
static SDL_FRect control_area(const MenuLayout& l, const MenuRow& r) {
    float w = r.control == Control::Switch ? 52 * l.u : 0.58f * r.r.w;
    return {r.r.x + r.r.w - w - 10 * l.u, r.r.y + 9 * l.u, w, r.r.h - 18 * l.u};
}

static SDL_FRect segment(const MenuLayout& l, const MenuRow& r, int i) {
    SDL_FRect a = control_area(l, r);
    float w = a.w / choice_count(r.id);
    return {a.x + i * w, a.y, w, a.h};
}

static SDL_FRect slider_track(const MenuLayout& l, const MenuRow& r) {
    SDL_FRect a = control_area(l, r);
    return {a.x + a.h + 10 * l.u, a.y, a.w - 2 * (a.h + 10 * l.u), a.h};
}

static bool inside(const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

static void menu_close() {
    if (!g_report_folder.empty() && (!g_report_title.empty() || !g_report_description.empty()))
        if (!SotnDevReport::save(g_report_folder,g_report_title,g_report_description)) {
            g_bug_notice="Falha ao salvar anotacao; reabra Relatar e tente novamente";
            g_bug_notice_until=SDL_GetTicks()+5000;
        }
    g_report_edit = 0; SDL_StopTextInput(g_win);
    g_menu_open = false;
    g_slider_finger = 0;
    g_title_confirm = false;
    for (auto& h : g_held_over) h = 0xFFFF;
}

static void menu_quit(bool to_saves, bool to_title = false) {
    if (to_saves || to_title) {
        if (FILE* f = std::fopen(g_cfg.after.c_str(), "w")) {
            std::fputs(to_title ? "title\n" : "saves\n", f);
            std::fclose(f);
        } else { sat_note("cannot write menu request: %s", g_cfg.after.c_str()); return; }
    }
    menu_close();
    g_quitting = true;
    state_stop(to_saves ? "Save files was chosen in the menu" : "Quit was chosen in the menu");
}

// Enter, A, or a tap on the row's label: an action acts, and the rest move on a step.
static void menu_activate(MenuId id) {
    if (id != M_TITLE) g_title_confirm = false;
    if (id >= M_MOD_FIRST) {
        const size_t index = size_t(id - M_MOD_FIRST);
        sotn_mod_enable(index, !sotn_mod_enabled(index)); return;
    }
    switch (id) {
    case M_REPORT: g_options_menu = 6; g_mod_menu = false; g_menu_at = 0; break;
    case M_STATES: g_options_menu = 7; g_mod_menu = false; g_menu_at = 0; break;
    case M_STATE_STATUS: case M_STATE_SAVE: case M_STATE_LOAD: break;
    case M_OPTIONS_SAVE:
        g_bug_notice = host_settings_save() ? "Opcoes salvas" : "Falha ao salvar opcoes";
        g_bug_notice_until = SDL_GetTicks() + 5000; break;
    case M_REPORT_TITLE: case M_REPORT_DESCRIPTION:
        g_report_edit = id == M_REPORT_TITLE ? 1 : 2; SDL_StartTextInput(g_win); break;
    case M_REPORT_SAVE:
        g_bug_notice = SotnDevReport::save(g_report_folder, g_report_title, g_report_description) ?
            "Relato salvo na pasta da captura" : "Capture um bug com F8 antes de salvar";
        g_bug_notice_until = SDL_GetTicks() + 5000; break;
    case M_DEV_DIAGNOSTICS: g_options_menu = 5; g_mod_menu = false; g_menu_at = 0; break;
    case M_DEV_NORMAL: choose(M_SPEED, 0); break;
    case M_DEV_STEP: choose(M_SPEED, 0); g_dev_step = true; menu_close(); break;
    case M_DEV_PROFILE:
        VideoPerf::set_enabled(!VideoPerf::enabled(), sat_vblanks());
        sat_note("dev profiler: %s", VideoPerf::enabled() ? "on" : "off"); break;
    case M_DEV_DUMP:
        if (!VideoPerf::repeat_last_report()) sat_note("dev profiler: activate Medir desempenho and resume before collecting timings");
        break;
    case M_DEV_LIGHT:
        g_settings.set("display", "hd_filter", "0"); g_settings.set("display", "shader", "0");
        g_settings.set("display", "fsr", "false"); g_settings.set("display", "filter", "0");
        g_settings.set("display", "sampling", "0");
        g_hd.close(); g_fsr.close(); g_picture_dirty = true;
        host_settings_save(); sat_note("dev video: lightweight profile applied"); break;
    case M_EFFECTS: g_options_menu = 1; g_menu_at = 0; break;
    case M_AUDIO: g_options_menu = 4; g_menu_at = 0; break;
    case M_FSR:
        g_settings.set("display", "fsr", g_settings.flag("display", "fsr") ? "false" : "true");
        if (g_settings.flag("display", "fsr")) { g_settings.set("display", "hd_filter", "0"); g_fsr.open(g_ren); }
        host_settings_save(); break;
    case M_SCREENSHOT: request_bug_report(); break;
    case M_BUG_REPORT: request_bug_report(); break;
    case M_MUTE: mute_set(!g_settings.flag("audio", "muted")); break;
    case M_FPS: case M_PAUSE_FOCUS: {
        const char* key = id == M_FPS ? "show_fps" : "pause_unfocused";
        g_settings.set("display", key, g_settings.flag("display", key) ? "false" : "true"); host_settings_save(); break;
    }
    case M_VIDEO_RESET: {
        const Settings defaults;
        for (const auto& key : g_settings.keys("display")) g_settings.set("display", key, defaults.get("display", key));
        fullscreen_set(false);
        SDL_SetWindowSize(g_win, 960, 720);
        g_picture_dirty = true;
        host_settings_save(); break;
    }
    case M_GRAPHICS: g_options_menu = 1; g_menu_at = 0; break;
    case M_CONTROLLERS: g_options_menu = 2; g_menu_at = 0; break;
    case M_RESOLUTION: case M_SCALE: case M_FILTER: case M_SAMPLING: case M_PAD_PROFILE: case M_SHADER: case M_ASPECT: case M_HD: case M_RENDERER:
        choose(id, choice_at(id) + 1); break;
    case M_INTEGER:
        g_settings.set("display", "integer_scale", g_settings.flag("display", "integer_scale") ? "false" : "true"); host_settings_save(); break;
    case M_PAD_ENABLED:
        g_settings.set("gamepad", "enabled", g_settings.flag("gamepad", "enabled") ? "false" : "true"); host_settings_save(); break;
    case M_DEADZONE:
        deadzone_set(g_settings.number("gamepad", "stick") >= 95 ? 5 : g_settings.number("gamepad", "stick") + 5); break;
    case M_PAD_RESET: {
        const Settings defaults;
        for (const auto& key : g_settings.keys("gamepad")) g_settings.set("gamepad", key, defaults.get("gamepad", key));
        for (const auto& p : g_pads) {
            char guid[33]; SDL_GUIDToString(SDL_GetGamepadGUIDForID(p.id), guid, sizeof guid);
            for (const auto& key : g_settings.keys("gamepad")) g_settings.set(std::string("gamepad ") + guid, key, defaults.get("gamepad", key));
        }
        host_rebind(); host_settings_save(); break;
    }
    case M_MODS: g_mod_menu = true; g_mod_page = 0; g_menu_at = 0; break;
    case M_MOD_BACK: g_mod_menu = false; g_options_menu = 0; g_menu_at = 0; break;
    case M_MOD_HEAL: sotn_mod_heal(); break;
    case M_MOD_OFF: sotn_mod_disable_all(); break;
    case M_MOD_RELOAD: sotn_mod_reload(); g_mod_page = 0; g_menu_at = 0; break;
    case M_MOD_PREV: {
        size_t pages = std::max(size_t(1), (sotn_mod_count() + kModsPerPage - 1) / kModsPerPage);
        g_mod_page = (g_mod_page + pages - 1) % pages; break;
    }
    case M_MOD_NEXT: {
        size_t pages = std::max(size_t(1), (sotn_mod_count() + kModsPerPage - 1) / kModsPerPage);
        g_mod_page = (g_mod_page + 1) % pages; break;
    }
    case M_RESUME: menu_close(); break;
    case M_SAVES: menu_quit(true); break;
    case M_QUIT: menu_quit(false); break;
    case M_TITLE:
        if (g_title_confirm) menu_quit(false, true);
        else g_title_confirm = true;
        break;
    case M_SPEED: choose(id, choice_at(id) + 1); break;
    case M_VOLUME: volume_set((g_settings.number("audio", "volume") + 10) % 110); break;
    case M_FULLSCREEN: fullscreen_set(!fullscreen_on()); break;
    case M_PAD: case M_STYLE: choose(id, choice_at(id) + 1); break;
    case M_SIZE: {
        int size = g_settings.number("touch", "size") + 10;
        pad_size_set(size > 200 ? 50 : size);
        break;
    }
    case M_MENU_BUTTON: setting_change("menu_button", g_settings.flag("touch", "menu_button") ? "false" : "true"); break;
    case M_MOD_FIRST: break; // IDs dinâmicos são tratados antes do switch.
    case M_INFO_STAGE: case M_INFO_PLAYER: case M_INFO_TIME: case M_INFO_PC:
    case M_INFO_MODULES: case M_INFO_RENDERER: case M_INFO_EFFECTS: break;
    }
}

// Left and right: a step along a choice or the slider, a switch set off or on.
static void menu_adjust(MenuId id, int dir) {
    if (id >= M_MOD_FIRST) { sotn_mod_enable(size_t(id - M_MOD_FIRST), dir > 0); return; }
    if (id == M_PAD || id == M_STYLE || id == M_SPEED || id == M_RESOLUTION || id == M_SCALE || id == M_FILTER || id == M_SAMPLING || id == M_PAD_PROFILE || id == M_SHADER || id == M_ASPECT || id == M_HD || id == M_RENDERER) choose(id, choice_at(id) + dir);
    else if (id == M_MUTE) mute_set(dir > 0);
    else if (id == M_DEV_PROFILE) VideoPerf::set_enabled(dir > 0, sat_vblanks());
    else if (id == M_FSR) {
        g_settings.set("display", "fsr", dir > 0 ? "true" : "false");
        if (dir > 0) { g_settings.set("display", "hd_filter", "0"); g_fsr.open(g_ren); }
        host_settings_save();
    }
    else if (id == M_FPS || id == M_PAUSE_FOCUS) {
        g_settings.set("display", id == M_FPS ? "show_fps" : "pause_unfocused", dir > 0 ? "true" : "false"); host_settings_save();
    }
    else if (id == M_INTEGER || id == M_PAD_ENABLED) {
        g_settings.set(id == M_INTEGER ? "display" : "gamepad", id == M_INTEGER ? "integer_scale" : "enabled", dir > 0 ? "true" : "false"); host_settings_save();
    } else if (id == M_DEADZONE) {
        deadzone_set(g_settings.number("gamepad", "stick") + 5 * dir);
    }
    else if (id == M_VOLUME) volume_set(g_settings.number("audio", "volume") + 10 * dir);
    else if (id == M_FULLSCREEN) fullscreen_set(dir > 0);
    else if (id == M_SIZE) pad_size_set(g_settings.number("touch", "size") + 10 * dir);
    else if (id == M_MENU_BUTTON) setting_change("menu_button", dir > 0 ? "true" : "false");
}

static void slide(const MenuLayout& l, const MenuRow& r, float x) {
    SDL_FRect t = slider_track(l, r);
    if (r.id == M_VOLUME) {
        volume_set((int)std::lround(std::clamp((x - t.x) / t.w, 0.0f, 1.0f) * 100));
        return;
    }
    if (r.id == M_DEADZONE) {
        const int value = 5 + int(std::lround(std::clamp((x - t.x) / t.w, 0.0f, 1.0f) * 90));
        deadzone_set(value); return;
    }
    pad_size_set(50 + (int)std::lround(std::clamp((x - t.x) / t.w, 0.0f, 1.0f) * 150));
}

// A tap or click at (x, y), from `finger` (0 for the mouse).
static void menu_tap(float x, float y, SDL_FingerID finger) {
    MenuLayout l = menu_layout();
    const float tabs_x = l.panel.x + 24*l.u, tabs_width = (l.panel.w-48*l.u)/8;
    if (y >= l.title_y+25*l.u && y <= l.title_y+48*l.u && x >= tabs_x && x < tabs_x+8*tabs_width) {
        static const int pages[] = {0,6,1,4,2,-1,5,7};
        const int target = pages[std::clamp(int((x-tabs_x)/tabs_width),0,7)];
        g_mod_menu = target == -1; g_options_menu = target == -1 ? 0 : target;
        g_menu_at = 0; g_report_edit = 0; SDL_StopTextInput(g_win); return;
    }
    if (g_options_menu == 6) {
        const auto form=SotnReportLayout::make(l.panel,l.title_y+65*l.u,l.u);
        if (inside(form.title,x,y)) { g_menu_at=2; menu_activate(M_REPORT_TITLE); }
        else if (inside(form.description,x,y)) { g_menu_at=3; menu_activate(M_REPORT_DESCRIPTION); }
        else if (inside(form.capture,x,y)) menu_activate(M_BUG_REPORT);
        else if (inside(form.save,x,y)) { g_report_edit=0; SDL_StopTextInput(g_win); menu_activate(M_REPORT_SAVE); }
        else if (inside(form.back,x,y)) menu_activate(M_MOD_BACK);
        return;
    }
    for (size_t i = 0; i < l.rows.size(); ++i) {
        const MenuRow& r = l.rows[i];
        if (!inside(r.r, x, y)) continue;
        g_menu_at = (int)i;
        SDL_FRect a = control_area(l, r);
        if (r.control == Control::Choice && inside(a, x, y)) {
            for (int k = 0; k < choice_count(r.id); ++k)
                if (inside(segment(l, r, k), x, y)) choose(r.id, k);
        } else if (r.control == Control::Cycle && inside(a, x, y)) {
            choose(r.id, choice_at(r.id) + (x < a.x + a.w / 2 ? -1 : 1));
        } else if (r.control == Control::Slider && inside(a, x, y)) {
            if (x < a.x + a.h + 5 * l.u) menu_adjust(r.id, -1);
            else if (x > a.x + a.w - a.h - 5 * l.u) menu_adjust(r.id, 1);
            else {
                slide(l, r, x);
                g_slider_finger = finger;
                g_slider_id = r.id;
            }
        } else menu_activate(r.id);
        return;
    }
}

static void menu_open() {
    if (g_menu_open || g_quitting) return;
    VideoPerf::report(sat_vblanks(), g_tex_w, g_tex_h, true);
    VideoPerf::paused() = true;
    g_menu_open = true;
    g_menu_at = 0;
    g_title_confirm = false;
    g_mod_menu = false;
    g_options_menu = 0;
    g_fingers.clear();
    g_lifted.clear();
    g_unread.clear();
}

static void dev_tab(int direction) {
    static const int pages[] = {0,6,1,4,2,-1,5,7};
    const int current = g_mod_menu ? -1 : g_options_menu;
    int index = 0;
    for (int i = 0; i < 8; ++i) if (pages[i] == current) index = i;
    const int next = pages[(index + direction + 8) % 8];
    g_mod_menu = next == -1; g_options_menu = next == -1 ? 0 : next;
    g_menu_at = 0; g_report_edit = 0; SDL_StopTextInput(g_win);
}

static bool escape_is_a_button() {
    for (auto& [key, bit] : g_keys)
        if (key == SDL_SCANCODE_ESCAPE) return true;
    return false;
}

// ---- the events -------------------------------------------------------------------------------
// Up and down go through the rows, and left and right adjust one; on an action, they go to the other column.
static void on_menu_key(int dy, bool activate, bool back, int dx) {
    MenuLayout l = menu_layout();
    int n = (int)l.rows.size();
    g_menu_at = std::clamp(g_menu_at, 0, n - 1);
    const MenuRow& r = l.rows[g_menu_at];
    if (back && (g_mod_menu || g_options_menu)) { g_mod_menu = false; g_options_menu = 0; g_menu_at = 0; }
    else if (back && g_title_confirm) g_title_confirm = false;
    else if (back) menu_close();
    else if (activate) menu_activate(r.id);
    else if (dx && r.control != Control::Action && r.control != Control::Readout) menu_adjust(r.id, dx);
    else if (dx) {
        for (int i = 0; i < n; ++i)
            if (l.rows[i].column != r.column) { g_menu_at = i; break; }
    } else if (dy) { g_title_confirm = false; g_menu_at = (g_menu_at + dy + n) % n; }
}

static void on_event(const SDL_Event& e) {
    switch (e.type) {
    case SDL_EVENT_TEXT_INPUT:
        if (g_menu_open && g_report_edit) SotnDevReport::append(g_report_edit == 1 ? g_report_title : g_report_description,
            e.text.text, g_report_edit == 1 ? 160 : 4096);
        break;
    case SDL_EVENT_WINDOW_FOCUS_LOST:
        if (g_settings.flag("display", "pause_unfocused")) menu_open();
        break;
    case SDL_EVENT_QUIT:
        menu_close();
        state_stop("the window was closed");
        break;
    case SDL_EVENT_GAMEPAD_ADDED:
        pad_added(e.gdevice.which);
        break;
    case SDL_EVENT_GAMEPAD_REMOVED:
        pad_removed(e.gdevice.which);
        break;
    case SDL_EVENT_KEY_DOWN: {
        SDL_Scancode k = e.key.scancode;
        if (!g_menu_open && !e.key.repeat && k == SDL_SCANCODE_TAB) {
            const bool available=SotnMapOverlay::snapshot(ld8,ld16,ld32).available;
            if (e.key.mod & SDL_KMOD_SHIFT) g_live_map.expand(available);
            else if (g_live_map.fullscreen()) g_live_map.close();
            else g_live_map.toggle(available);
            sat_note("live map: %s, state %u, VB %llu",g_live_map.open() ? "open" : "closed",
                unsigned(ld16(0x0605CD70)),(unsigned long long)sat_vblanks());
            break;
        }
        if (!g_menu_open && g_live_map.open() && k == SDL_SCANCODE_ESCAPE) { g_live_map.close(); break; }
        if (!g_menu_open && g_live_map.fullscreen()) {
            if (k==SDL_SCANCODE_LEFT) { g_live_map.pan(-2,0); break; }
            if (k==SDL_SCANCODE_RIGHT) { g_live_map.pan(2,0); break; }
            if (k==SDL_SCANCODE_UP) { g_live_map.pan(0,-2); break; }
            if (k==SDL_SCANCODE_DOWN) { g_live_map.pan(0,2); break; }
        }
        if (g_menu_open && g_report_edit) {
            auto& field = g_report_edit == 1 ? g_report_title : g_report_description;
            if (k == SDL_SCANCODE_BACKSPACE) SotnDevReport::backspace(field);
            if (k == SDL_SCANCODE_TAB) g_report_edit = g_report_edit == 1 ? 2 : 1;
            if (k == SDL_SCANCODE_RETURN && (e.key.mod & SDL_KMOD_CTRL)) menu_activate(M_REPORT_SAVE);
            else if (k == SDL_SCANCODE_RETURN && g_report_edit == 1) g_report_edit=2;
            else if (k == SDL_SCANCODE_RETURN) SotnDevReport::append(field,"\n",4096);
            if (k == SDL_SCANCODE_ESCAPE || k == SDL_SCANCODE_F1) {
                g_report_edit = 0; SDL_StopTextInput(g_win);
            }
            break;
        }
        if (k == SDL_SCANCODE_F1 && !e.key.repeat) { if (g_menu_open) menu_close(); else menu_open(); break; }
        if (g_menu_open && k == SDL_SCANCODE_TAB && !e.key.repeat) { dev_tab(e.key.mod & SDL_KMOD_SHIFT ? -1 : 1); break; }
        if (k == SDL_SCANCODE_F5 && !e.key.repeat) choose(M_SPEED, 0);
        if (k == SDL_SCANCODE_F6 && !e.key.repeat) { menu_activate(M_DEV_STEP); break; }
        if (k == SDL_SCANCODE_F7 && !e.key.repeat) menu_activate(M_DEV_PROFILE);
        if (k == SDL_SCANCODE_F12 && !e.key.repeat) request_bug_report();
        if ((k == SDL_SCANCODE_F8 || k == SDL_SCANCODE_PRINTSCREEN) && !e.key.repeat) request_bug_report();
        if (k == SDL_SCANCODE_F9 && !e.key.repeat) mute_set(!g_settings.flag("audio", "muted"));
        if (k == SDL_SCANCODE_F10 && !e.key.repeat) {
            g_settings.set("display", "show_fps", g_settings.flag("display", "show_fps") ? "false" : "true"); host_settings_save();
        }
        if ((k == SDL_SCANCODE_F11 || (k == SDL_SCANCODE_RETURN && (e.key.mod & SDL_KMOD_ALT))) && !e.key.repeat) {
            fullscreen_set(!(SDL_GetWindowFlags(g_win) & SDL_WINDOW_FULLSCREEN));
            break;
        }
        bool back = k == SDL_SCANCODE_AC_BACK || (k == SDL_SCANCODE_ESCAPE && !escape_is_a_button());
        if (!g_menu_open) {
            break;
        }
        on_menu_key(k == SDL_SCANCODE_UP ? -1 : k == SDL_SCANCODE_DOWN ? 1 : 0,
                    !e.key.repeat && (k == SDL_SCANCODE_RETURN || k == SDL_SCANCODE_SPACE), back && !e.key.repeat,
                    k == SDL_SCANCODE_LEFT ? -1 : k == SDL_SCANCODE_RIGHT ? 1 : 0);
        break;
    }
    case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
        int b = e.gbutton.button;
        if (!g_menu_open) {
            // Select fica para o mapa. O chord L3+R3 é reconhecido em read_pads.
            break;
        }
        if (b == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER || b == SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER) {
            dev_tab(b == SDL_GAMEPAD_BUTTON_LEFT_SHOULDER ? -1 : 1); break;
        }
        on_menu_key(b == SDL_GAMEPAD_BUTTON_DPAD_UP ? -1 : b == SDL_GAMEPAD_BUTTON_DPAD_DOWN ? 1 : 0,
                    b == SDL_GAMEPAD_BUTTON_SOUTH,
                    b == SDL_GAMEPAD_BUTTON_EAST,
                    b == SDL_GAMEPAD_BUTTON_DPAD_LEFT ? -1 : b == SDL_GAMEPAD_BUTTON_DPAD_RIGHT ? 1 : 0);
        break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        if (g_menu_open && e.button.which != SDL_TOUCH_MOUSEID) {
            float x = e.button.x, y = e.button.y;
            SDL_RenderCoordinatesFromWindow(g_ren, e.button.x, e.button.y, &x, &y);
            menu_tap(x, y, 0);
        }
        break;
    case SDL_EVENT_FINGER_DOWN:
    case SDL_EVENT_FINGER_MOTION: {
        g_touchscreen = true;
        float x = e.tfinger.x * g_out_w, y = e.tfinger.y * g_out_h;
        if (g_menu_open) {
            if (e.type == SDL_EVENT_FINGER_DOWN) menu_tap(x, y, e.tfinger.fingerID);
            else if (g_slider_finger && e.tfinger.fingerID == g_slider_finger) {
                MenuLayout l = menu_layout();
                for (auto& r : l.rows)
                    if (r.control == Control::Slider && r.id == g_slider_id) slide(l, r, x);
            }
            break;
        }
        TouchLayout l = pad_layout();
        if (e.type == SDL_EVENT_FINGER_DOWN && menu_button_shown() && touch_on_menu(l, x, y)) {
            menu_open();
            break;
        }
        if (!touch_shown()) break;
        if (e.type == SDL_EVENT_FINGER_DOWN) {
            g_fingers[e.tfinger.fingerID] = {x, y, touch_on_dpad(l, x, y)};
            g_unread[e.tfinger.fingerID] = true;
        } else if (auto f = g_fingers.find(e.tfinger.fingerID); f != g_fingers.end()) {
            f->second.x = x;
            f->second.y = y;
        }
        break;
    }
    case SDL_EVENT_FINGER_UP:
    case SDL_EVENT_FINGER_CANCELED:
        if (e.type == SDL_EVENT_FINGER_UP && g_unread.count(e.tfinger.fingerID)) g_lifted.push_back(e.tfinger.fingerID);
        else g_fingers.erase(e.tfinger.fingerID);
        if (e.tfinger.fingerID == g_slider_finger) g_slider_finger = 0;
        break;
    }
}

static void events() {
    if (!g_virtual.empty()) virtual_press();
    SDL_Event e;
    HostCall in("SDL_PollEvent");
    while (SDL_PollEvent(&e)) on_event(e);
    read_pads();
}

// ---- drawing ----------------------------------------------------------------------------------
static SDL_FColor rgba(float r, float g, float b, float a) { return {r, g, b, a}; }

static void fill_rounded(SDL_FRect r, float rad, SDL_FColor c) { ui_fill_rounded(r, rad, c); }
static void fill_circle(float x, float y, float r, SDL_FColor c) { ui_fill_circle(x, y, r, c); }
static void draw_text(float x, float y, float size, const std::string& s, SDL_FColor c) { ui_text(x, y, size, s, c, true); }

// The pads' colours: the black pad sold in the West, or the white Japanese one.
struct ButtonLook { SDL_FColor fill, label; };
static ButtonLook look_of(uint16_t bit, bool japanese) {
    const SDL_FColor light = rgba(0.92f, 0.92f, 0.94f, 1), dark = rgba(0.22f, 0.22f, 0.25f, 1);
    if (!japanese) {
        if (bit & (B_L | B_R)) return {rgba(0.36f, 0.36f, 0.39f, 1), light};
        if (bit == B_START) return {rgba(0.27f, 0.27f, 0.3f, 1), light};
        return {rgba(0.13f, 0.13f, 0.15f, 1), light};
    }
    switch (bit) {
    case B_A: return {rgba(0.2f, 0.66f, 0.45f, 1), light};
    case B_B: return {rgba(0.93f, 0.77f, 0.18f, 1), dark};
    case B_C: return {rgba(0.2f, 0.45f, 0.82f, 1), light};
    case B_X: case B_Y: case B_Z: return {rgba(0.8f, 0.8f, 0.83f, 1), dark};
    default: return {rgba(0.9f, 0.42f, 0.63f, 1), light};     // START, L and R
    }
}

static SDL_FColor faded(SDL_FColor c, bool held) {
    if (held) return {std::min(1.0f, c.r * 1.3f + 0.15f), std::min(1.0f, c.g * 1.3f + 0.15f), std::min(1.0f, c.b * 1.3f + 0.15f), 0.85f};
    return {c.r, c.g, c.b, 0.55f};
}

static void draw_touch_pad() {
    bool jp = g_settings.get("touch", "style") == "japanese";
    TouchLayout l = pad_layout();
    const SDL_FColor shade = rgba(0, 0, 0, 0.3f);

    // the d-pad: a cross, each arm lit while held
    float r = l.dpad_r, arm = 0.62f * r;
    SDL_FColor pad = jp ? rgba(0.42f, 0.42f, 0.45f, 1) : rgba(0.13f, 0.13f, 0.15f, 1);
    fill_circle(l.dpad_x, l.dpad_y, 1.08f * r, shade);
    fill_rounded({l.dpad_x - r, l.dpad_y - arm / 2, 2 * r, arm}, 0.15f * arm, faded(pad, false));
    fill_rounded({l.dpad_x - arm / 2, l.dpad_y - r, arm, 2 * r}, 0.15f * arm, faded(pad, false));
    const struct { uint16_t bit; float dx, dy; } arms[] = {{B_UP, 0, -1}, {B_DOWN, 0, 1}, {B_LEFT, -1, 0}, {B_RIGHT, 1, 0}};
    for (auto& a : arms) {
        float cx = l.dpad_x + a.dx * 0.62f * r, cy = l.dpad_y + a.dy * 0.62f * r;
        if (g_touch_held & a.bit) fill_rounded({cx - arm / 2, cy - arm / 2, arm, arm}, 0.15f * arm, faded(pad, true));
        fill_circle(cx + a.dx * 0.12f * r, cy + a.dy * 0.12f * r, 0.09f * r, rgba(1, 1, 1, 0.35f));
    }

    for (auto& b : l.buttons) {
        ButtonLook k = look_of(b.bit, jp);
        bool held = g_touch_held & b.bit;
        SDL_FColor label = {k.label.r, k.label.g, k.label.b, held ? 1.0f : 0.8f};
        if (b.shape == TouchButton::Round) {
            fill_circle(b.x, b.y, 1.1f * b.w, shade);
            fill_circle(b.x, b.y, b.w, faded(k.fill, held));
            draw_text(b.x, b.y, 0.8f * b.w, b.label, label);
        } else {
            float rad = b.shape == TouchButton::Pill ? b.h / 2 : 0.3f * b.h;
            fill_rounded({b.x - b.w / 2 - 4, b.y - b.h / 2 - 4, b.w + 8, b.h + 8}, rad + 4, shade);
            fill_rounded({b.x - b.w / 2, b.y - b.h / 2, b.w, b.h}, rad, faded(k.fill, held));
            draw_text(b.x, b.y, (b.shape == TouchButton::Pill ? 0.42f : 0.5f) * b.h, b.label, label);
        }
    }
}

static void draw_menu_button() {
    TouchLayout l = pad_layout();
    // the menu's button: three bars
    fill_circle(l.menu_x, l.menu_y, l.menu_r, rgba(0.13f, 0.13f, 0.15f, 0.55f));
    for (int i = -1; i <= 1; ++i)
        fill_rounded({l.menu_x - 0.5f * l.menu_r, l.menu_y + i * 0.32f * l.menu_r - 0.07f * l.menu_r, l.menu_r, 0.14f * l.menu_r},
                     0.07f * l.menu_r, rgba(1, 1, 1, 0.8f));
}

static std::string dev_value(MenuId id) {
    auto hex = [](uint32_t n) { char value[16]; std::snprintf(value, sizeof value, "%08X", n); return std::string(value); };
    switch (id) {
    case M_STATE_STATUS: return "Indisponivel: retomada SH-2 nao validada";
    case M_INFO_STAGE: return hex(ld16(0x0605D750)) + " / " + hex(ld16(0x0605D754));
    case M_INFO_PLAYER: return std::to_string(ld32(0x0605C6D0)) + " | HP " +
        std::to_string(ld32(0x0605C940)) + "/" + std::to_string(ld32(0x0605C944));
    case M_INFO_TIME: return std::to_string(sat_vblanks()) + " / " + std::to_string(sat_now() / 1000000000) + "s";
    case M_INFO_PC: return hex(g_master.pc) + " / " + hex(g_master.pr);
    case M_INFO_MODULES: {
        std::string names;
        for (int i = 0; i < sh2_active_slots(); ++i) if (const auto* m = sh2_active_slot(i)) {
            if (!names.empty()) names += " / ";
            names += m->name;
        }
        return names.empty() ? "Nenhum" : names;
    }
    case M_INFO_RENDERER: {
        std::string backend = SDL_GetRendererName(g_ren);
        if (auto* device = SDL_GetGPURendererDevice(g_ren)) backend += std::string(" / ") + SDL_GetGPUDeviceDriver(device);
        return backend;
    }
    case M_INFO_EFFECTS: return g_settings.flag("display", "fsr") ? g_fsr.status() :
        g_settings.number("display", "hd_filter") ? g_hd.status() : g_shader.status();
    default: return {};
    }
}

static void dev_text(float x, float y, float size, float width, std::string value,
                     SDL_FColor color, bool bold = false, UiAlign align = UiAlign::Left) {
    if (ui_text_width(size, value, bold) > width) {
        while (!value.empty() && ui_text_width(size, value + "...", bold) > width) {
            std::size_t last = value.size() - 1;
            while (last > 0 && (static_cast<unsigned char>(value[last]) & 0xC0u) == 0x80u) --last;
            value.resize(last);
        }
        value += "...";
    }
    ui_text(x, y, size, value, color, bold, align);
}

static void draw_report_editor(const MenuLayout& l) {
    const float u=l.u;
    const auto form=SotnReportLayout::make(l.panel,l.title_y+65*u,u);
    const auto text=rgba(.96f,.94f,.90f,1), muted=rgba(.67f,.68f,.72f,1);
    const auto gold=rgba(.82f,.67f,.41f,1), well=rgba(.025f,.028f,.04f,1);
    const auto frame=[&](SDL_FRect rect,bool selected) {
        ui_fill_rounded({rect.x-u,rect.y-u,rect.w+2*u,rect.h+2*u},5*u,selected ? gold : rgba(.22f,.23f,.28f,1));
        ui_fill_rounded(rect,4*u,well);
    };
    frame(form.preview,false);
    if (g_report_preview && g_report_w>0 && g_report_h>0) {
        const float fit=std::min((form.preview.w-12*u)/g_report_w,(form.preview.h-12*u)/g_report_h);
        const SDL_FRect image={form.preview.x+(form.preview.w-g_report_w*fit)/2,
            form.preview.y+(form.preview.h-g_report_h*fit)/2,g_report_w*fit,g_report_h*fit};
        SDL_RenderTexture(g_ren,g_report_preview,nullptr,&image);
    } else dev_text(form.preview.x+12*u,form.preview.y+28*u,13*u,form.preview.w-24*u,"Capture a cena com F8 ou F12",muted);
    dev_text(form.preview.x,form.preview.y-10*u,11*u,form.preview.w,"PREVIA ORIGINAL DA CAPTURA",gold,true);
    dev_text(form.title.x,form.title.y-12*u,12*u,form.title.w,"O que esta errado? / Titulo",text,true);
    frame(form.title,g_report_edit==1);
    dev_text(form.title.x+10*u,form.title.y+form.title.h/2,14*u,form.title.w-20*u,
        g_report_title.empty() ? "Ex.: parede piscando na biblioteca" : g_report_title+(g_report_edit==1 ? "|" : ""),
        g_report_title.empty() ? muted : text);
    dev_text(form.description.x,form.description.y-12*u,12*u,form.description.w,"Anotacao / como aconteceu",text,true);
    frame(form.description,g_report_edit==2);
    std::vector<std::string> lines;
    std::string line;
    const auto value=g_report_description+(g_report_edit==2 ? "|" : "");
    for (size_t i=0;i<value.size();) {
        if (value[i]=='\n') { lines.push_back(line); line.clear(); ++i; continue; }
        size_t next=i+1;
        while (next<value.size() && (uint8_t(value[next])&0xc0)==0x80) ++next;
        const auto codepoint=value.substr(i,next-i);
        if (!line.empty() && ui_text_width(13*u,line+codepoint,false)>form.description.w-20*u) {
            lines.push_back(line); line.clear();
        }
        line+=codepoint; i=next;
    }
    lines.push_back(line);
    if (g_report_description.empty() && g_report_edit!=2) lines={"Descreva o defeito, a sala ou os passos."};
    const size_t count=size_t(std::max(1,int((form.description.h-20*u)/(18*u))));
    const size_t first=g_report_edit==2 && lines.size()>count ? lines.size()-count : 0;
    for (size_t i=first;i<std::min(lines.size(),first+count);++i)
        dev_text(form.description.x+10*u,form.description.y+(18+float(i-first)*18)*u,13*u,
            form.description.w-20*u,lines[i],text);
    const auto button=[&](SDL_FRect rect,const char* label,bool primary) {
        ui_fill_rounded(rect,6*u,primary ? rgba(.42f,.12f,.19f,1) : rgba(.13f,.14f,.18f,1));
        dev_text(rect.x+rect.w/2,rect.y+rect.h/2,13*u,rect.w-12*u,label,text,true,UiAlign::Centre);
    };
    button(form.capture,"Nova captura",false); button(form.save,"Salvar anotacao (Ctrl+Enter)",true); button(form.back,"Voltar",false);
    const auto id=g_report_folder.empty() ? "Nenhuma captura selecionada" : g_report_folder.filename().string();
    dev_text(l.panel.x+24*u,l.panel.y+l.panel.h-8*u,10*u,l.panel.w-48*u,
        id+" | Tab: trocar campo | Enter: nova linha | Salvo apenas neste PC",muted);
}

static void draw_menu() {
    const SDL_FColor panel = rgba(0.055f, 0.06f, 0.085f, 0.98f), row = rgba(0.095f, 0.10f, 0.13f, 1), well = rgba(0.045f, 0.05f, 0.07f, 1);
    const SDL_FColor accent = rgba(0.38f, 0.12f, 0.20f, 1), ring = rgba(0.78f, 0.64f, 0.40f, 1), text = rgba(0.95f, 0.93f, 0.88f, 1);
    const SDL_FColor muted = rgba(0.65f, 0.66f, 0.70f, 1), knob = rgba(0.97f, 0.94f, 0.85f, 1), off = rgba(0.25f, 0.26f, 0.30f, 1);
    SDL_FRect all = {0, 0, (float)g_out_w, (float)g_out_h};
    SDL_SetRenderDrawColorFloat(g_ren, 0, 0, 0, 0.72f);
    SDL_RenderFillRect(g_ren, &all);
    MenuLayout l = menu_layout();
    float u = l.u;
    ui_fill_rounded({l.panel.x-1,l.panel.y-1,l.panel.w+2,l.panel.h+2},16*u,rgba(0.36f,0.30f,0.23f,0.8f));
    ui_fill_rounded(l.panel, 15 * u, panel);
    const char* title = g_options_menu == 1 ? "Tela e resolucao" : g_options_menu == 2 ? "Controles" : g_options_menu == 3 ? "Qualidade e shaders" : g_options_menu == 4 ? "Audio e sistema" : g_options_menu == 5 ? "Diagnostico da sessao" : g_options_menu == 6 ? "Relatar bug" : g_options_menu == 7 ? "Save states: suporte pendente" : g_mod_menu ? "Laboratorio de hacks" : (g_cfg.title_menu ? "Painel de desenvolvimento" : "Paused");
    ui_text(l.panel.x + 28 * u, l.title_y - 24*u, 11*u,"DEV MENU / CASTLEVANIA SOTN SATURN",ring,true,UiAlign::Left);
    ui_text(l.panel.x + 28 * u, l.title_y + 6*u, 29 * u, title, text, true, UiAlign::Left);
    const char* hint = g_options_menu ? "Esc / B: voltar" : g_mod_menu ? "Mudancas podem ir para o save" : g_cfg.title_menu ? (g_title_confirm ? "Perde progresso nao salvo" : "PAUSADO") : "Back to resume";
    ui_text(l.panel.x + l.panel.w - 20 * u, l.title_y, 13 * u, hint, muted, false, UiAlign::Right);
    static const char* tabs[] = {"Dev", "Relatar", "Graficos", "Audio", "Controles", "Cheats", "Diagnostico", "Estados"};
    static const int pages[] = {0,6,1,4,2,-1,5,7};
    const float tab_width = (l.panel.w - 48*u) / 8;
    for (int i = 0; i < 8; ++i) {
        const bool selected = pages[i] == (g_mod_menu ? -1 : g_options_menu);
        SDL_FRect tab = {l.panel.x+24*u+i*tab_width, l.title_y+25*u, tab_width-3*u, 23*u};
        ui_fill_rounded(tab, 3*u, selected ? accent : well);
        dev_text(tab.x+tab.w/2,tab.y+tab.h/2,10*u,tab.w-8*u,tabs[i],selected ? text : muted,selected,UiAlign::Centre);
    }
    if (g_options_menu==6) { draw_report_editor(l); return; }
    for (int c = 0; c < 2; ++c)
        ui_text(l.column_x[c], l.section_y[c], 12 * u,
                g_options_menu == 1 ? (c == 0 ? "Janela e proporcao" : "Apresentacao") : g_options_menu == 2 ? (c == 0 ? "Dispositivos" : "Mapeamento") : g_options_menu == 3 ? (c == 0 ? "Reconstrucao da imagem" : "Acabamento") : g_options_menu == 4 ? (c == 0 ? "Audio" : "Sessao") : g_options_menu == 5 ? (c == 0 ? "Ferramentas" : "Estado somente leitura") : g_mod_menu ? (c == 0 ? "Acoes e paginas" : "Presets e scripts") : g_cfg.title_menu ? (c == 0 ? "Ferramentas de desenvolvimento" : "Execucao e diagnostico") : (c == 0 ? "Game" : "Touch controls"), muted, true, UiAlign::Left);
    if (g_options_menu == 2) {
        std::string info = g_pads.empty() ? "Nenhum controle conectado" : SDL_GetGamepadName(g_pads.front().pad);
        int profile = g_settings.number("gamepad", "profile");
        if (profile == 0 && !g_pads.empty()) {
            const auto type = SDL_GetGamepadType(g_pads.front().pad);
            profile = type == SDL_GAMEPAD_TYPE_PS3 || type == SDL_GAMEPAD_TYPE_PS4 || type == SDL_GAMEPAD_TYPE_PS5 ? 2 : 1;
        }
        info = info.substr(0, 42);
        dev_text(l.panel.x + l.panel.w - 24*u, l.panel.y + l.panel.h - 88*u, 11*u, l.panel.w * .45f, info, muted, false, UiAlign::Right);
        // Os nomes mudam; SDL mantém a mesma posição física dos botões.
        const char* mapping = profile == 2 ? "Padrao: Quadrado ataque | X pulo | Circulo cancela" : "Padrao: X ataque | A pulo | B cancela";
        dev_text(l.panel.x + l.panel.w - 24*u, l.panel.y + l.panel.h - 70*u, 11*u, l.panel.w * .45f, mapping, muted, false, UiAlign::Right);
    }
    if (g_options_menu == 1 || g_options_menu == 3) {
        const auto status = std::string(g_settings.flag("display", "fsr") ? g_fsr.status() : g_settings.number("display", "hd_filter") ? g_hd.status() : g_shader.status()) + (g_options_menu == 1 ? " | *Reinicie para trocar backend" : "");
        dev_text(l.panel.x + l.panel.w - 24*u, l.panel.y + l.panel.h - 88*u, 11*u, l.panel.w * .45f, status, muted, false, UiAlign::Right);
    }
    if (g_mod_menu) {
        const std::string status = "Pagina " + std::to_string(g_mod_page + 1) + "/" +
            std::to_string(std::max(size_t(1), (sotn_mod_count() + kModsPerPage - 1) / kModsPerPage)) +
            " | " + sotn_mod_status();
        dev_text(l.panel.x + l.panel.w - 24*u, l.panel.y + l.panel.h - 88*u, 11*u, l.panel.w * .45f, status, muted, false, UiAlign::Right);
    }
    if (g_options_menu == 6) {
        const float x = l.panel.x + l.panel.w*.52f;
        const float y = l.panel.y + l.panel.h*.48f;
        const float width = l.panel.w*.43f;
        dev_text(x,y,12*u,width,(g_report_edit == 1 ? "> " : "") + std::string("Titulo: ") + g_report_title,text,true);
        const std::string value = (g_report_edit == 2 ? "> " : "") + std::string("Descricao: ") + g_report_description;
        std::size_t begin = 0;
        for (int line = 0; line < 6 && begin < value.size(); ++line) {
            std::size_t end = begin;
            while (end < value.size() && ui_text_width(11*u,value.substr(begin,end-begin+1),false) < width) ++end;
            while (end > begin && end < value.size() && (static_cast<unsigned char>(value[end]) & 0xc0u) == 0x80u) --end;
            if (end == begin) break;
            ui_text(x,y+(24+line*18)*u,11*u,value.substr(begin,end-begin),muted,false,UiAlign::Left);
            begin = end;
        }
    }
    g_menu_at = std::clamp(g_menu_at, 0, (int)l.rows.size() - 1);
    char timing[180];
    std::snprintf(timing, sizeof timing, "Real %.2fx | pedido %ux | %.0f quadros/s | VB %llu",
        g_sim_meter.actual(), g_speed, g_display_fps, (unsigned long long)sat_vblanks());
    dev_text(l.panel.x + 24*u,l.panel.y+l.panel.h-107*u,12*u,l.panel.w-48*u,timing,ring,true);
    dev_text(l.panel.x + 24*u,l.panel.y+l.panel.h-88*u,11*u,l.panel.w*.47f,
        "Fase/sala " + dev_value(M_INFO_STAGE),muted);
    dev_text(l.panel.x + 24*u,l.panel.y+l.panel.h-66*u,11*u,l.panel.w-48*u,
        "Select: mapa | L3 + R3 / F1: Dev Menu | F6: quadro | F7: profiler | F8: bug",muted);
    dev_text(l.panel.x + 24*u,l.panel.y+l.panel.h-42*u,11*u,l.panel.w-48*u,menu_help(l.rows[g_menu_at].id),text);
    dev_text(l.panel.x + 24*u,l.panel.y+l.panel.h-20*u,10*u,l.panel.w-48*u,
        "Setas / D-pad: navegar | Enter / A: confirmar | Esc / B: voltar | estado da partida preservado na pausa",muted);
    for (size_t i = 0; i < l.rows.size(); ++i) {
        const MenuRow& r = l.rows[i];
        float cy = r.r.y + r.r.h / 2;
        if ((int)i == g_menu_at) ui_fill_rounded({r.r.x - 2 * u, r.r.y - 2 * u, r.r.w + 4 * u, r.r.h + 4 * u}, 9 * u, ring);
        ui_fill_rounded(r.r, 7 * u, r.id == M_RESUME || (int)i == g_menu_at ? accent : row);
        SDL_FColor label = r.control == Control::Disabled ? muted : r.id == M_QUIT ? rgba(1, 0.52f, 0.5f, 1) : text;
        const float label_width = r.control == Control::Switch ? r.r.w - 90*u :
            r.control == Control::Cycle || r.control == Control::Choice || r.control == Control::Slider ? r.r.w*.36f : r.r.w - 40*u;
        if (r.control == Control::Readout) {
            dev_text(r.r.x+14*u, r.r.y+12*u,10*u,r.r.w-28*u,menu_label(r.id),muted);
            dev_text(r.r.x+14*u, r.r.y+30*u,13*u,r.r.w-28*u,dev_value(r.id),text,true);
            continue;
        }
        dev_text(r.r.x + 14*u, cy, 14*u, label_width,menu_label(r.id),label,true);
        SDL_FRect a = control_area(l, r);
        switch (r.control) {
        case Control::Action:
            if (r.id == M_SAVES || r.id == M_GRAPHICS || r.id == M_EFFECTS || r.id == M_CONTROLLERS || r.id == M_AUDIO || r.id == M_MODS || r.id == M_DEV_DIAGNOSTICS)
                ui_text(r.r.x + r.r.w - 16 * u, cy, 24 * u, ">", ring, false, UiAlign::Right);
            break;
        case Control::Choice: {
            ui_fill_rounded(a, a.h / 2, well);
            int at = choice_at(r.id);
            for (int k = 0; k < choice_count(r.id); ++k) {
                SDL_FRect s = segment(l, r, k);
                if (k == at) ui_fill_rounded({s.x + 3 * u, s.y + 3 * u, s.w - 6 * u, s.h - 6 * u}, (s.h - 6 * u) / 2, accent);
                ui_text(s.x + s.w / 2, cy, 14 * u, choice_name(r.id, k), k == at ? text : muted, k == at);
            }
            break;
        }
        case Control::Cycle:
            ui_fill_rounded(a, a.h / 2, well);
            ui_text(a.x + 11 * u, cy, 16 * u, "<", muted, true);
            ui_text(a.x + a.w - 11 * u, cy, 16 * u, ">", muted, true);
            ui_text(a.x + a.w / 2, cy, 13 * u, choice_name(r.id, choice_at(r.id)), text, true);
            break;
        case Control::Slider: {
            int size = r.id == M_VOLUME ? g_settings.number("audio", "volume") : r.id == M_DEADZONE ? g_settings.number("gamepad", "stick") : g_settings.number("touch", "size");
            ui_text(a.x - 12*u, cy, 12*u, std::to_string(size) + "%", muted, false, UiAlign::Right);
            float b = a.h / 2;
            ui_fill_circle(a.x + b, cy, b, well);
            ui_fill_circle(a.x + a.w - b, cy, b, well);
            ui_text(a.x + b, cy, 18 * u, "−", text, true);
            ui_text(a.x + a.w - b, cy, 18 * u, "+", text, true);
            SDL_FRect t = slider_track(l, r);
            float f = r.id == M_VOLUME ? size / 100.0f : r.id == M_DEADZONE ? (size - 5) / 90.0f : (size - 50) / 150.0f, x = t.x + f * t.w;
            ui_fill_rounded({t.x, cy - 3 * u, t.w, 6 * u}, 3 * u, off);
            ui_fill_rounded({t.x, cy - 3 * u, x - t.x, 6 * u}, 3 * u, accent);
            ui_fill_circle(x, cy, 10 * u, knob);
            break;
        }
        case Control::Switch: {
            bool on = r.id >= M_MOD_FIRST ? sotn_mod_enabled(size_t(r.id - M_MOD_FIRST)) :
                      r.id == M_DEV_PROFILE ? VideoPerf::enabled() : r.id == M_FULLSCREEN ? fullscreen_on() : r.id == M_FSR ? g_settings.flag("display", "fsr") : r.id == M_INTEGER ? g_settings.flag("display", "integer_scale") : r.id == M_FPS ? g_settings.flag("display", "show_fps") : r.id == M_PAUSE_FOCUS ? g_settings.flag("display", "pause_unfocused") : r.id == M_MUTE ? g_settings.flag("audio", "muted") : r.id == M_PAD_ENABLED ? g_settings.flag("gamepad", "enabled") : g_settings.flag("touch", "menu_button");
            SDL_FRect p = {a.x, cy - 14 * u, a.w, 28 * u};
            ui_fill_rounded(p, 14 * u, on ? accent : off);
            ui_fill_circle(on ? p.x + p.w - 14 * u : p.x + 14 * u, cy, 11 * u, knob);
            break;
        }
        case Control::Readout: case Control::Disabled: break;
        }
    }
}


static void upload_picture(const Frame& f) {
    VideoPerf::Scope profile(VideoPerf::Stage::Upload);
    const int filter = std::clamp(g_settings.number("display", "filter"), 0, 2);
    const void* pixels = f.px.data();
    static std::vector<uint32_t> filtered;
    if (filter != 0) {
        filtered.resize(f.px.size());
        for (int y = 0; y < f.h; ++y)
            for (int x = 0; x < f.w; ++x) {
                const size_t i = size_t(y) * f.w + x;
                filtered[i] = SotnVideo::effect(f.px[i], x, y, SotnVideo::Effect(filter));
            }
        pixels = filtered.data();
    }
    SDL_SetTextureScaleMode(g_tex, g_settings.number("display", "sampling") == 0 ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
    if (!SDL_UpdateTexture(g_tex, nullptr, pixels, f.w * 4)) sat_note("picture upload: %s", SDL_GetError());
    g_picture_dirty = false;
}

// While --resume plays its presses: the game going by at speed, dimmed, under a card saying what the
// run is getting to and how far it has got.
static void draw_catching_up() {
    static uint64_t from = sat_vblanks();
    ui_fill_rounded({0, 0, (float)g_out_w, (float)g_out_h}, 0, rgba(0, 0, 0, 0.45f));
    float u = display_scale(), w = std::min(0.86f * g_out_w, 420 * u), h = 120 * u;
    SDL_FRect card = {(g_out_w - w) / 2, g_out_h - h - std::max(32 * u, 0.08f * g_out_h), w, h};
    ui_fill_rounded(card, 20 * u, rgba(0.09f, 0.1f, 0.13f, 0.92f));
    std::string label = g_cfg.resume_label.empty() ? "Getting back to where you were" : g_cfg.resume_label;
    ui_text(card.x + w / 2, card.y + 40 * u, 18 * u, label, rgba(0.95f, 0.96f, 0.98f, 1), true);
    float done = g_cfg.resume > from ? (float)(sat_vblanks() - from) / (float)(g_cfg.resume - from) : 1;
    SDL_FRect track = {card.x + 28 * u, card.y + 76 * u, w - 56 * u, 8 * u};
    ui_fill_rounded(track, 4 * u, rgba(0.25f, 0.27f, 0.32f, 1));
    ui_fill_rounded({track.x, track.y, std::max(track.h, track.w * std::clamp(done, 0.0f, 1.0f)), track.h}, 4 * u, rgba(0.27f, 0.47f, 0.95f, 1));
}

// The last picture and what goes over it.
static void draw() {
    VideoPerf::Scope profile(VideoPerf::Stage::Draw);
    SDL_GetCurrentRenderOutputSize(g_ren, &g_out_w, &g_out_h);
    if (g_tex && g_last) {
        if (g_picture_dirty) upload_picture(*g_last);
        const int tv_lines = bios_pal() ? std::max(256, g_tex_h) : g_tex_h;
        auto view = SotnVideo::viewport(g_out_w, g_out_h, g_tex_w, tv_lines, g_settings.flag("display", "integer_scale"), g_settings.number("display", "aspect"));
        const int height = view.h * g_tex_h / tv_lines;
        g_dst = {float(view.x), float(view.y + (view.h - height) / 2), float(view.w), float(height)};
    }
    SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
    SDL_RenderClear(g_ren);
    SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_BLEND);
    if (g_tex) {
        const int factor = SotnVideo::kHdFactors[std::clamp(g_settings.number("display", "hd_filter"), 0, int(SotnVideo::kHdFactors.size()) - 1)];
        SDL_Texture* picture = g_tex;
        const bool fsr_requested = g_settings.flag("display", "fsr");
        if (fsr_requested) {
            picture = g_fsr.render(g_tex, std::max(g_tex_w,int(g_dst.w)), std::max(g_tex_h,int(g_dst.h)), 0.2f);
            if (picture == g_tex && !g_fsr.enabled()) { g_settings.set("display", "fsr", "false"); host_settings_save(); }
        }
        else if (g_hd.resize(g_tex_w, g_tex_h, factor)) picture = g_hd.render(g_tex);
        if (SotnVideo::hd_failed(fsr_requested, factor, g_hd.factor(), picture == g_tex)) {
            sat_note("HD disabled: %s", g_hd.status());
            g_settings.set("display", "hd_filter", "0");
            host_settings_save();
        }
        if (picture != g_tex) SDL_SetTextureScaleMode(picture, g_settings.number("display", "sampling") == 0 ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
        const int shader = g_shader.enabled() ? g_settings.number("display", "shader") : 0;
        if (!g_shader.begin(shader, g_tex_w, g_tex_h)) sat_note("shader: %s", g_shader.status());
        SDL_RenderTexture(g_ren, picture, nullptr, &g_dst);
        g_shader.end(); // A interface fica legível, sem o efeito aplicado ao jogo.
    }
    if (g_live_map.open() && !g_menu_open) {
        const auto live_map=SotnMapOverlay::snapshot(ld8,ld16,ld32);
        g_live_map.sync(live_map.available);
        const SDL_FRect screen={0,0,float(g_out_w),float(g_out_h)};
        SotnMapOverlay::draw(g_ren,live_map,screen,SDL_GetTicks(),g_live_map.fullscreen(),g_live_map.pan_x(),g_live_map.pan_y());
        if (g_live_map.fullscreen())
            ui_text(g_out_w*.5f,24*display_scale(),12*display_scale(),
                "MAPA | Direcional: navegar | Select / Circulo / Esc: voltar | Vermelho: save | Verde: teleporte",
                rgba(1,.91f,.66f,1),true);
    }
    capture_bug_report(); // Antes do menu Esc: ele não deve esconder o defeito.
    if (g_open_report) {
        g_open_report = false;
        menu_open(); g_options_menu = 6; g_mod_menu = false; g_menu_at = 2;
        g_report_edit=1; SDL_StartTextInput(g_win);
    }
    if (sat_resuming()) draw_catching_up();
    if (g_settings.flag("display", "show_fps") && !g_menu_open) {
        const float u = display_scale();
        ui_fill_rounded({8 * u, 8 * u, 170 * u, 30 * u}, 6 * u, rgba(0, 0, 0, 0.75f));
        const auto label = std::to_string(int(g_display_fps + 0.5)) + " quadros/s | " + std::to_string(g_speed) + "x";
        ui_text(18 * u, 23 * u, 13 * u, label, rgba(1, 1, 1, 1), true, UiAlign::Left);
    }
    if (g_menu_open) draw_menu();
    else {
        if (touch_shown()) draw_touch_pad();
        if (menu_button_shown()) draw_menu_button();
    }
    if (SDL_GetTicks() < g_bug_notice_until) {
        const float u = display_scale();
        ui_fill_rounded({12*u, g_out_h - 58*u, 390*u, 42*u}, 7*u, rgba(0.06f, 0.07f, 0.10f, 0.96f));
        ui_text(26*u, g_out_h - 37*u, 13*u, g_bug_notice, rgba(0.95f, 0.85f, 0.66f, 1), false, UiAlign::Left);
    }
    if (g_quitting) {
        float u = display_scale();
        ui_fill_rounded({g_out_w / 2.0f - 70 * u, 24 * u, 140 * u, 40 * u}, 20 * u, rgba(0.09f, 0.1f, 0.13f, 0.9f));
        ui_text(g_out_w / 2.0f, 44 * u, 17 * u, "Saving…", rgba(1, 1, 1, 0.95f), true);
    }
    SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_NONE);
    HostCall in("SDL_RenderPresent");
    VideoPerf::Scope present_profile(VideoPerf::Stage::Present);
    if (!SDL_RenderPresent(g_ren) && (g_settings.number("display", "shader") != 0 || g_settings.number("display", "hd_filter") != 0 || g_settings.flag("display", "fsr"))) {
        sat_note("shader disabled after draw failure: %s", SDL_GetError());
        g_shader.disable("Shader desativado por falha no driver");
        g_settings.set("display", "shader", "0");
        g_settings.set("display", "hd_filter", "0");
        g_settings.set("display", "fsr", "false");
        g_hd.close();
        g_fsr.close();
        host_settings_save();
    }
}

// The menu, open until it is closed; the game waits.
static void menu_loop() {
    if (g_audio) SDL_PauseAudioStreamDevice(g_audio);
    ++g_host_paused;
    while (g_menu_open) {
        SDL_Event e;
        {
            HostCall in("SDL_PollEvent");
            while (SDL_PollEvent(&e)) on_event(e);
        }
        read_pads();
        draw();
        HostCall in("SDL_Delay");
        SDL_Delay(16);
    }
    g_base = Clock::now() - std::chrono::nanoseconds(sat_now() / g_speed);
    reset_sim_rate();
    VideoPerf::paused() = false;
    --g_host_paused;
    g_fps_from = Clock::now(); g_fps_frames = 0;
    if (g_audio) SDL_ResumeAudioStreamDevice(g_audio);
    read_pads();
}

static void present(const Frame& f) {
    g_last = &f;
    const auto now = Clock::now();
    if (g_fps_from == Clock::time_point{}) g_fps_from = now;
    ++g_fps_frames;
    const double elapsed = std::chrono::duration<double>(now - g_fps_from).count();
    if (elapsed >= 1.0) { g_display_fps = g_fps_frames / elapsed; g_fps_frames = 0; g_fps_from = now; }
    if (SDL_GetWindowFlags(g_win) & (SDL_WINDOW_MINIMIZED | SDL_WINDOW_OCCLUDED | SDL_WINDOW_HIDDEN)) {
        events();
        if (g_menu_open) menu_loop();
        return;
    }
    if (f.w != g_tex_w || f.h != g_tex_h) {
        if (g_tex) SDL_DestroyTexture(g_tex);
        g_tex = SDL_CreateTexture(g_ren, SDL_PIXELFORMAT_XRGB8888, SDL_TEXTUREACCESS_STREAMING, f.w, f.h);
        if (!g_tex) sat_fatal("SDL: no %dx%d texture: %s", f.w, f.h, SDL_GetError());
        SDL_SetTextureScaleMode(g_tex, SDL_SCALEMODE_LINEAR);
        g_tex_w = f.w;
        g_tex_h = f.h;
    }
    upload_picture(f);
    draw();
    VideoPerf::report(sat_vblanks(), f.w, f.h);
    if (g_dev_step) { g_dev_step = false; menu_open(); sat_note("dev step: paused at VBlank %llu", (unsigned long long)sat_vblanks()); }
    events();
    if (g_menu_open) menu_loop();
}

void host_present(const Frame& f) {
    if (!g_win) return;
    tasks_on_thread_stack([](void* f) { present(*static_cast<const Frame*>(f)); }, const_cast<Frame*>(&f));
}

void host_pace(uint64_t now) {
    if (!g_win || !g_paced) return;
    if (sat_resuming()) {
        g_base = Clock::now() - std::chrono::nanoseconds(now / g_speed);
        return;
    }
    // Mesmo quando o turbo não compõe um quadro, ler eventos e controles a cada VBlank.
    events();
    if (g_menu_open) menu_loop();
    if (g_sim_meter.update(now, SDL_GetTicksNS()))
        sat_note("dev speed: requested=%ux actual=%.3fx stage=%04X room=%04X vb=%llu",
                 g_speed, g_sim_meter.actual(), unsigned(ld16(0x0605D750)), unsigned(ld16(0x0605D754)),
                 static_cast<unsigned long long>(sat_vblanks()));
    auto wait = pace_wait(g_base, now, Clock::now(), g_speed);
    if (wait > Clock::duration{}) {
        VideoPerf::Scope profile(VideoPerf::Stage::Pace);
        HostCall in("SDL_DelayPrecise");
        SDL_DelayPrecise((Uint64)std::chrono::duration_cast<std::chrono::nanoseconds>(wait).count());
    }
}
