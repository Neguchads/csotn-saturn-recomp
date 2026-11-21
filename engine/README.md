[English](#english) | [Português](#portugues)

<a id="english"></a>

# saturn-recomp

Static recompilation of Sega Saturn games to native PC. It reads a disc, translates the games'
SH-2 code to C++, and builds it against a runtime that replaces the Saturn's hardware: both
SH-2s, VDP1 and VDP2, the SCU, the SMPC, the CD block, and the sound side (the 68000 and the SCSP).

saturn-recomp began as a fork of Samuele Voltan's
[saturnkit](https://github.com/vs-sr-dev/saturnkit), whose history it keeps, and is now developed
separately. It holds what every Saturn game shares. What one game needs lives in that game's own
repository, which takes saturn-recomp as a submodule.

## Games built on it

| Game | Repository | What it added |
|---|---|---|
| Virtual Hydlide (1995) | [vs-sr-dev/pc-virtualhydlide](https://github.com/vs-sr-dev/pc-virtualhydlide) (on saturnkit) | the disc, the SH-2 decoder, the address map, function discovery, cross-program matching, the interpreter, the recompiler, the runtime core (HLE boot and BIOS, SCU, SMPC, the slave as a coroutine, the CD block), VDP1 and VDP2 in software in a window with the pad, the 68000 and the SCSP (sound), recompiler hooks for a game layer, and the fields between a game's frames drawn moving (`--interp`) |
| Deep Fear (1998) | [vs-sr-dev/pc-deepfear](https://github.com/vs-sr-dev/pc-deepfear) (on saturnkit) | function discovery for GCC and SGL code (GCC's switches, `mova` tables and pointers, tables of records, computed jumps into unrolled code, SGL's hand-written handlers), the SCU DSP (a disassembler, and an interpreter in the runtime), `SYS_CHGUIPR` and the per-interrupt SCU masks, literal-pool slots that the code writes |
| X JAPAN Virtual Shock 001 (1995) | [vs-sr-dev/pc-xjapan](https://github.com/vs-sr-dev/pc-xjapan) (on saturnkit) | a file system over a Mode 1 and a CD-ROM XA track, interleaved files and records of CD-DA tracks, function discovery through far jumps and callbacks that do nothing, TVSTAT's HBLANK, the pad read directly through the SMPC's ports, the disc's area code, recorded play (`--record-input`, `--input @FILE`) |
| Saturn Bomberman (1996) | [callumjenkins/pc-saturnbomberman](https://github.com/callumjenkins/pc-saturnbomberman) | a game's own tasks switched with setjmp and longjmp (`--tasks`), 6-player multitaps, the 8-bit VDP1 framebuffer, VDP2 hi-res, interlace and windows, per-character special priority and colour calculation, the SMPC clock (`--clock`), discovery of jump tables reached over a `bra` |

## Using it

A game's repository takes saturn-recomp as a submodule and installs it into its Python
environment:

```sh
git submodule add https://github.com/callumjenkins/saturn-recomp.git saturn-recomp
uv add --editable ./saturn-recomp      # or: pip install -e saturn-recomp
```

The install has to be editable: the recompiler points the build at the C++ runtime in the checkout.
Each game pins a saturn-recomp commit and moves it forward deliberately.

A game describes itself in a `game.toml`: the programs to recompile and where they load, names for
addresses, the game's own task switch if it has one, hook points, and the file where learned seeds
go. `src/saturnrecomp/config.py` documents the format. From it:

```sh
python -m saturnrecomp.build game.toml [--recompile]    # recompile if needed, then build
python -m saturnrecomp.learn game.toml -- --cue GAME.cue --headless --vblanks 3600
    # run; while it stops at code discovery missed, add the seed, recompile and run again
python -m saturnrecomp.analysis.resume game.toml --check   # where the game's tasks can resume
python -m saturnrecomp.inventory game.toml      # every disc file and what became of it, each module's
    # bytes by class, and a check that every discovered instruction has a generated line; each 68000
    # program in [[inventory.sound]] gets the same split from a descent from its vectors
python -m saturnrecomp.m68k FILE.BIN --offset 44   # one 68000 program's descent, or --at ADDR to disassemble
python -m saturnrecomp.native game.toml         # each instruction's native address ranges, from line
    # tables of a second build whose code must equal the built executable's
python -m saturnrecomp.executed game.toml RUN.tsv ...   # whether every instruction a reference run
    # executed (CPU, address, opcode a line) is translated; fails if one a module holds is not, if a
    # 68000 program holds one its descent did not find, or if no note explains one nothing holds
```

A program can also play a run, which is how a bot or a test plays a game it cannot script ahead of
time. `saturnrecomp.agent` starts the executable with `--agent`, pauses it at VBlank 1, and from
there steps VBlanks, presses the pads, reads and writes memory and takes frames:

```python
from saturnrecomp import agent

with agent.start("build/recomp-build/saturn", ["--cue", "GAME.cue", "--headless"], "build/run/bot") as run:
    run.step(1900)
    run.pad("START")
    run.step(10)
    run.pad("")
    work_ram = run.read(0x06000000, 64)    # bytes, as the CPU reads them
    run.frame().save_png("build/run/bot/now.png")
    print(",".join(run.presses))    # the presses as an --input script, to replay the run
```

A pause takes no time on the Saturn's clock, so the same presses at the same VBlanks play the same
whichever way they come. The protocol is a line of text a command, documented in
`runtime/src/machine/agent.cpp`.

The lower-level tools below take their inputs as arguments instead.

```sh
python -m saturnrecomp.disc GAME.cue --info              # IP.BIN, ISO 9660, tracks
python -m saturnrecomp.disc GAME.cue --list
python -m saturnrecomp.disc GAME.cue --extract build/extract   # files + IP.BIN
python -m saturnrecomp.disc GAME.cue --audio build/audio       # CD-DA as WAV
python -m saturnrecomp.sh2 FILE.BIN --find-base               # where it loads
python -m saturnrecomp.sh2 FILE.BIN --base 0600B000 --at 0600B000 --count 64
python -m saturnrecomp.sh2 FILE.BIN --base 0600B000 --refs 25D00000:25D00018
python -m saturnrecomp.sh2 FILE.BIN --base 0600B000 --census
python -m saturnrecomp.hw 25D00002 25F80114 06000310
python -m saturnrecomp.recomp.discover FILE.BIN --base 0600B000 --report     # functions, code/data
python -m saturnrecomp.recomp.match A.BIN@0600B000 B.BIN@0600B000 --names a.tsv --out b.tsv
python -m saturnrecomp.sh2emu FILE.BIN --base 0600B000 --call 060224DC --regs r1=100,r0=7
python -m saturnrecomp.recomp --out build/recomp A=A.BIN@0600B000 B=B.BIN@0600B000 --optest   # to C++
python -m saturnrecomp.recomp --out build/recomp A=A.BIN@0600B000 --hook A:0600B6F4   # sh2_hook after that instruction
python -m saturnrecomp.recomp.selftest --out a.txt --image A=A.BIN@0600B000 --test A --auto  # vectors
cmake -S build/recomp -B build/recomp-build -G Ninja -DCMAKE_CXX_COMPILER=clang++
ninja -C build/recomp-build && build/recomp-build/selftest build/recomp/selftest/optest.txt a.txt
build/recomp-build/saturn --cue GAME.cue                              # the game in a window, with the pad and sound
build/recomp-build/saturn --cue GAME.cue --interp                     # the fields between the game's frames drawn moving
build/recomp-build/saturn --cue GAME.cue --headless --vblanks 3600 --wav run.wav   # the run's sound as a WAV
build/recomp-build/saturn --cue GAME.cue --hook 0600B6F4:r5=1         # a register set at a hooked instruction
build/recomp-build/saturn --cue GAME.cue --headless --vblanks 600 --trace --shot 300,600   # headless, pictures
build/recomp-build/saturn --cue GAME.cue --headless --input 1200:START,1210: --peek 25F80000:8 --watch 05A00000:05A01000
build/recomp-build/saturn --cue GAME.cue --record-input play.txt          # the pad as played in the window, as a script
build/recomp-build/saturn --cue GAME.cue --headless --input @play.txt --vblanks 9000 --shot 7760   # that game again, headless
build/recomp-build/saturn --cue GAME.cue --headless --watch 06058D6C:06058DCF --watch-vblanks 7300:7310   # every store, by function
build/recomp-build/saturn --cue GAME.cue --headless --write 4250:060C179A=0201   # memory set at a VBlank, such as a stage select
build/recomp-build/saturn --cue GAME.cue --headless --save -        # saves kept for this run alone, starting empty
build/recomp-build/saturn --cue GAME.cue --coverage cov.txt --checkpoint 600   # which functions ran, rewritten every 10 minutes
build/recomp-build/saturn --cue GAME.cue --state-out game.state --state-every 60   # the machine dumped every minute and at a quit (a game with --tasks)
build/recomp-build/saturn --cue GAME.cue --state-in game.state       # the run going on from that dump
build/recomp-build/saturn --cue GAME.cue --progress p.txt --progress-keep 060C029C:2,060C0690:8   # both ranges noted whenever the first changes
```

The game's saves go to one file per game in the user's data directory unless `--save` names
another, as `saturn-recomp/PRODUCT_VERSION/backup.bin` under `$XDG_DATA_HOME` (or
`~/.local/share`), `~/Library/Application Support` or `%APPDATA%`. A test run should pass
`--save -` or a file of its own, so it starts from no saves and leaves the player's alone.

## Playtesting

`playtest/` builds a game for other people to play and brings their sessions back for review: a
launcher for testers, a Cloudflare Worker that keeps the sessions, a release workflow that compiles
for Linux, Windows and macOS, and `python -m saturnrecomp.playtest` for the maintainer.
`playtest/README.md` says how to set it up.

## Licence

saturn-recomp is MIT-licensed, apart from one optional file under the GPL
(`THIRD_PARTY.md`). With `-DSATURN_VDP1_GPL=ON`, or `cmake = ["SATURN_VDP1_GPL=ON"]` under
`[game]` in a game's `game.toml`, VDP1 draws its shapes by Mednafen's rules: a scaled sprite's
edges then match Mednafen's to the pixel, and the build is a GPL work. Without it the rules are
saturn-recomp's own, from Sega's manual, and the build stays MIT.

## Comparing with a reference emulator

`python -m saturnrecomp.reference` plays a disc in a libretro core, headless, a frame at a time,
with presses in `--input`'s syntax, and saves its pictures as `shot-N.png`. It is written for
Beetle Saturn (Mednafen's Saturn), which needs the real BIOS: `--bios` names the folder holding
`mpr-17933.bin` for a US or European disc. Build the core from
github.com/libretro/beetle-saturn-libretro with `-D_LOW_ACCURACY_` taken out of its Makefile's
`FLAGS`, which otherwise turns on a faster branch path upstream Mednafen does not have.

```sh
python -m saturnrecomp.reference mednafen_saturn_libretro.so --bios BIOS_DIR --cue GAME.cue \
    --frames 3600 --input 3000:START,3010: --shot 3300,3400 --out ref/
```

The core's pictures are 330x240 for a 320x224 screen, with ours at x+5, y+8 inside them. Only pad 1
is plugged in unless `--pads 2` says otherwise, as in our runtime: a game can ask different
questions with a second pad connected.

The core's frames do not line up with our VBlanks. The real BIOS boots first, and its CD reads take
a drive's time, so the gap grows each time a game reads the disc. A game's own count of its frames
does line up, because it stops while the game loads. `reference.play_synced` places each press and
shot of our run at the same value of that count, and as many frames into it as in ours, and
`reference.difference` compares a frame with ours. A game repository traces its count through our
run with the agent and gives the address; pc-saturnbomberman's `bomberman compare` does this.

What is left after that is the game itself running differently. Some games run tasks on every
VBlank while they load, so a longer load leaves their state a few frames further on.

`saturnrecomp.audio` compares sound the same way, a window at a time. The runtime's `--wav` records
a run's samples and the core keeps its own in `Core.audio`. `audio.window` takes a window of ours
and the reference's sound around the same point in the game. It finds the offset where the two
correlate best and reports that correlation, both levels, the clipped samples, and whether the
channels are swapped. Two sound chips emulated separately never give the same bytes, so a window
counts as a match when its correlation is near 1, not when its samples are equal. In a window that
matches, `audio.parts` compares each 10 ms part at the window's offset, so a click or a dropout too
short to move a whole second shows.

`audio.live` checks the other end: what a sound device was given, against the run's own `--wav`. SDL's
disk driver (`SDL_AUDIO_DRIVER=disk`) writes the device's samples to a file. `audio.live` lines the two
up and reports where the device went silent for a while, where it skipped samples, and where it played
anything else. Those samples should be equal, so they are compared exactly.

## Checking a change

`tools/check.sh` runs every check that needs no game data. It's the same thing CI runs on every
push:
- the Python tests
- the recompiler's instruction self-test
- the runtime's unit tests

All of these build without SDL. With `SATURN_SDL` set to an SDL3 install, it also builds the
self-test's program on the SDL host. CI runs it so on Linux with Clang and with GCC, on Windows with
Clang, and on macOS. On Windows it also checks that `saturn.exe` needs no DLL a fresh Windows lacks.
The tests of what runs only on Linux skip elsewhere: building from a disc, the agent and the research
tools. A change to the runtime should also pass each game's own frame tests, which need the game's
disc.

## Layers

| Layer | Question it answers | Now | Next |
|---|---|---|---|
| 1. Recognise | What is on this disc? | `disc --info`: IP.BIN (product, areas, peripherals, stacks, 1st read), ISO 9660 volume, track list with pregaps | a `fingerprint`: SBL/SGL and their versions by code signature, CRI middleware, sound driver versions |
| 2. Extract | Turn standard formats into standard files | `disc --extract` (a file system over MODE1 and MODE2 tracks, interleaved files, records of CD-DA tracks left out), `disc --audio` (CD-DA to WAV) | VDP1/VDP2 image decoders (4/8/16 bpp, CLUT, CRAM), Sega FILM/Cinepak, SCSP tone banks |
| 3. Map code | What does the code do, where? | `sh2` (SH7604 decoder, disassembly with literal pools resolved and hardware registers named, `--refs`, `--census`, `--find-base`), `hw` (address map, register names, BIOS service pointers, SCU vectors), `sh2emu` (an SH-2 interpreter for isolated functions: the recompiler's oracle) | executable map (crt0, BSS, programs swapped at one address); `fingerprint` (SBL by signature) |
| 4. Translate | Turn SH-2 code into C++ | `recomp.discover` (functions and code/data in stripped SHC code: recursive descent, constant propagation for register calls, four switch forms, pointer and prologue seeds), `recomp.match` (the same function across programs linked at other addresses; names carried), `recomp` (one C++ function per entry: delay slots, calls checked on return, guarded dispatch through registers, switches, safe points; one module per program, recognised in memory by its crc32), `recomp.selftest` (vectors from `sh2emu`: every instruction form, and a program's functions that run alone), hooks (`--hook`: a call to `sh2_hook` after chosen instructions, where the runtime sets a register or a port's game layer runs) | replacing a whole function by a hand-written one |
| 5. Runtime | Replace the hardware | `runtime/` (C++20): the SH-2 context, the work RAMs with the cache-through addresses, dispatch over the active modules, the self-test harness; the Saturn (`saturn` executable): virtual or host time, program starts with the host stack unwound, HLE boot and BIOS services (interrupt dispatch, SCU mask, semaphores, BUP in a host file), SCU (interrupts, timers, DMA), SMPC (INTBACK, the pad read directly through PDR1/DDR1, the disc's area, a scripted pad, a played pad recorded and given back, slave and sound on/off), the slave SH-2 as a deterministic coroutine, the CD block at its registers over .cue/.bin (with `--accurate-drive`, the drive's seeks, double-speed reads and directory reads as Mednafen times them), the SH7604's division unit, FRT and DMAC (the FRT and the watchdog count at φ: the master clock over 65 until SMPC's CKCHG352, then over 61), the raster timing (TVSTAT's VBLANK and HBLANK), VDP1 drawn in software (every command, colour mode and colour calculation, the chip's way of walking quadrilaterals, and each draw's end once the cycles its commands cost the chip have gone by: Mednafen's rules for the costs with `SATURN_VDP1_GPL`, a fit to them without), VDP2 composed in software (NBG0–3 in cell and bitmap modes with zoom, the sprite layer, priorities, colour calculation and offsets, the back screen), an SDL3 window with the keyboard, a gamepad or a pad drawn on the touchscreen as the pad, and a menu that pauses the game; the 68000 (Musashi) and the SCSP (slots, envelopes, LFOs, FM, timers, interrupts, DMA, the DSP, CD-DA at its external input) in virtual time, out through an SDL3 audio stream or a WAV; `--interp`, the fields between a game's frames drawn by VDP1 with every command moved part of the way (matched by keys a game layer gives); a log of every register touched and a store watch over the work RAMs | VDP1 on the GPU, the rest of VDP2 (rotation, windows, line scroll…), `--interp` without its frame of latency |

## Principles

* Pure Python, no dependencies, for layers 1–4; the runtime (layer 5) is
  C++20 with SDL3, built with CMake, Ninja and clang.
* Every claim is checked on a real disc before it goes in.
* Game knowledge stays out.
* Every change is checked on every game built on it before it goes in.

## Checks behind each module

| Module | Checked by |
|---|---|
| `disc` | Virtual Hydlide (Europe), a Redump .cue with 28 tracks: IP.BIN parsed (MK-81380, area E), 458 files extracted from ISO 9660, the 1st read file found (`A.BIN`, identical to the game's own `OPEN.BIN`), track LBAs with their pregaps. A disc whose ISO 9660 spans a MODE1/2352 track and a MODE2/2352 one (Form 1): 270 files extracted, five of them interleaved (file unit 4 or 3, gap 15 or 16), every one of their 45 546 sectors carrying the XA file number of its record; the other files byte-identical to before on Virtual Hydlide and on a two-disc SGL game |
| `sh2` | every one of the 65 536 16-bit words against capstone 5 (SH-2 mode): 53 752 decode to the same mnemonic and operands; the 452 capstone alone decodes are SH-2A extensions (`movua`, `divs`, `jsr/n`, `clips`…) that the SH7604 does not have. `--find-base` puts 14 of 15 Virtual Hydlide executables at the address their crt0 implies |
| `recomp.discover` | Virtual Hydlide, 15 programs: no function whose descent meets data or an undefined opcode; every resolved switch target is code. Against Ghidra 12's auto-analysis of the field program: 571 of its 576 functions found; the other 5 are shared tails (common epilogues several functions branch to), which discovery follows as part of each caller |
| `recomp.match` | Virtual Hydlide: the frame limiter, found in each program by its bytes, is where the match carries its name in 11 of 11 programs |
| `sh2emu` | the SHC runtime's four division helpers run on 3 000 random operands each: quotient and remainder, signed and unsigned, 12 000 of 12 000 equal to Python |
| `recomp` (emit) | Virtual Hydlide, 15 programs: 10 259 functions, 1.5 million instructions, compiled with clang 22 and linked; no static target outside the modules. The self-test: 775 functions of the instruction test (126 forms, alone and in a delay slot, and the control flow) and 2 653 of the game's, 51 532 vectors, 0 differences in registers or memory |
| `runtime` (the Saturn) | Virtual Hydlide from the boot of its 1st read file to its first field: the opening movie through the CD block and the PCM handshake, three program starts (HYDSYS, the title menu, the field), both CPUs, the field's frame loop at its 12 fps cap; 120 s of game time with no wrong return and no call to a non-entry, the same run every time |
| `runtime` (the pad, HBLANK, recorded play) | a 1995 FMV game on SBL's Cinepak player: it waits for TVSTAT's HBLANK before writing TVMD and reads its pad only through PDR1 (the ID 0xB, START, the directions, A); from the boot through its movies, the title and the backstage to its first item, played in the window by the user, then given back headless with `--input @FILE` along the same route. Virtual Hydlide and a two-disc SGL game give byte-identical pictures and sound before and after |
| `runtime` (sound) | Virtual Hydlide against Beetle Saturn's recording: the opening movie's sound correlates at 0.99 with Beetle's, same stereo image, at a constant offset (1.6 dB louder before the output's 27/32 scale); the title silent in both; the menus' and the field's music and effects driven by the game's own sound drivers |
| `runtime` (`--interp`) | Virtual Hydlide's field, with its game layer's keys: 500 fields of walking and turning, no cracks beyond the game's own, trees whole through a turn; the movie, the title and the menus the same picture as without it, pixel for pixel; played by the user |
| `runtime` (VDP1, VDP2, the window) | Virtual Hydlide against Beetle Saturn: the title and the opening movie's frames the same pictures, the menus and the first field the same layout and look; the movie at its own 15 fps, paced by the PCM play position |
| `sh2.encode` | every word that decodes (53 752 of the 65 536): encoded back from its format and fields, 53 752 of 53 752 equal |
| `hw` | the register names Virtual Hydlide's code uses, read in context (VDP1 FBCR/PTMR/EDSR, VDP2 TVSTAT and colour offset, SMPC COMREG, FRT FTCSR in the slave's wait loop) |

## Known gaps

* Discovery leaves unreached what nothing references: leaf functions with
  no prologue called only by dead code (about 7% of the opening program's
  text). The runtime stops at a call to an address that is not an entry
  and logs it, to be given as a seed; Virtual Hydlide made none from the
  boot to its first field.
* A computed jump that fits none of the four switch forms stays
  unresolved; the recompiled code tries every instruction of its own
  function, then dispatches. In Virtual Hydlide all 36 left are tail calls
  through tables of function pointers, dispatched at run time.
* The recompiled code's semantics are `sh2emu`'s. Where the interpreter
  could differ from the hardware (`mac.w` saturation, `div1` corner cases)
  the self-test cannot tell; the running game is the check.
* Interrupts reach recompiled code at safe points only (back-edges, calls,
  `ldc …,sr`); there are no cycle counts. An interrupt raised twice before
  a poll takes it is taken once (an SCU timer on every line comes about
  once a poll).
* Sound: MIDI is not done (its input reads empty); CD-DA reaches the
  SCSP's external input but no game has played it yet. VDP1 draws in
  software at the Saturn's resolution; VDP2's rotation planes, the sprite window, line and
  cell scroll, mosaic, line colour screen and shadows are not done (the
  runtime notes a register that asks for one). Not emulated: the
  SCU DSP, the SH-2's on-chip interrupts, the caches.
* `match` needs a fingerprint unique on both sides; identical small
  functions (library copies) are matched only through their callers.
* `--find-base` scores literals pointing at function prologues. A small
  program that calls into a large resident one can score higher at a
  wrong base (Virtual Hydlide's MENU.BIN): check against the crt0, whose
  BSS starts where the file ends.
* `hw.BIOS`: the service-pointer names come from the SBL headers; those not
  marked as checked by a game are unconfirmed.
* `sh2.Image.literal_refs` reads every `mov.l @(disp,PC)` in the file,
  data included; a few hits in data areas are noise.

## Licence

MIT — see [LICENSE](LICENSE). saturn-recomp contains no game data and no Sega
code; it reads and replaces, it does not include. The runtime includes
Musashi (the 68000, MIT) with SoftFloat, and its SCSP is in part derived
from MAME's (BSD-3-Clause): their terms are in
[THIRD_PARTY.md](THIRD_PARTY.md).

Its interfaces will still change as the next game asks things of it.

---

<a id="portugues"></a>

# saturn-recomp

Recompilação estática de jogos Sega Saturn para PC nativo. Lê um disco, traduz o código SH-2
dos jogos para C++ e o compila com um runtime que substitui o hardware do Saturn: os dois SH-2,
VDP1 e VDP2, SCU, SMPC, bloco de CD e subsistema de áudio (68000 e SCSP).

saturn-recomp começou como fork do [saturnkit](https://github.com/vs-sr-dev/saturnkit) de
Samuele Voltan, cujo histórico preserva, e hoje é desenvolvido separadamente. Contém o que todos
os jogos Saturn compartilham. As necessidades específicas de cada jogo ficam no repositório
do próprio jogo, que utiliza saturn-recomp como submódulo.

## Jogos que o utilizam

| Jogo | Repositório | O que acrescentou |
|---|---|---|
| Virtual Hydlide (1995) | [vs-sr-dev/pc-virtualhydlide](https://github.com/vs-sr-dev/pc-virtualhydlide) (sobre saturnkit) | disco, decodificador SH-2, mapa de endereços, descoberta de funções, correspondência entre programas, interpretador, recompilador, núcleo do runtime (boot HLE e BIOS, SCU, SMPC, slave como corrotina, bloco de CD), VDP1 e VDP2 em software em janela com controle, 68000 e SCSP (áudio), hooks do recompilador para a camada do jogo e campos intermediários entre quadros desenhados em movimento (`--interp`) |
| Deep Fear (1998) | [vs-sr-dev/pc-deepfear](https://github.com/vs-sr-dev/pc-deepfear) (sobre saturnkit) | descoberta de funções GCC/SGL (switches GCC, tabelas e ponteiros `mova`, tabelas de registros, saltos calculados para código desenrolado, handlers SGL escritos à mão), SCU DSP (desmontador e interpretador no runtime), `SYS_CHGUIPR` e máscaras SCU por interrupção, slots de literal pool gravados pelo código |
| X JAPAN Virtual Shock 001 (1995) | [vs-sr-dev/pc-xjapan](https://github.com/vs-sr-dev/pc-xjapan) (sobre saturnkit) | sistema de arquivos sobre faixa Mode 1 e CD-ROM XA, arquivos intercalados e registros de faixas CD-DA, descoberta por saltos distantes e callbacks vazios, HBLANK de TVSTAT, leitura direta do controle pelas portas SMPC, código de região do disco e gravação da partida (`--record-input`, `--input @FILE`) |
| Saturn Bomberman (1996) | [callumjenkins/pc-saturnbomberman](https://github.com/callumjenkins/pc-saturnbomberman) | tarefas próprias do jogo trocadas por setjmp/longjmp (`--tasks`), multitaps para seis jogadores, framebuffer VDP1 de 8 bits, alta resolução/interlace/janelas VDP2, prioridade especial e cálculo de cor por caractere, relógio SMPC (`--clock`) e descoberta de jump tables acessadas por `bra` |

## Uso

O repositório do jogo incorpora saturn-recomp como submódulo e o instala no ambiente Python:

```sh
git submodule add https://github.com/callumjenkins/saturn-recomp.git saturn-recomp
uv add --editable ./saturn-recomp      # or: pip install -e saturn-recomp
```

A instalação precisa ser editável: o recompilador aponta o build para o runtime C++ no checkout.
Cada jogo fixa um commit de saturn-recomp e o atualiza deliberadamente.

O jogo se descreve em `game.toml`: programas a recompilar e seus endereços de carga, nomes de
endereços, troca de tarefas própria quando existe, pontos de hook e arquivo de sementes aprendidas.
`src/saturnrecomp/config.py` documenta o formato. A partir dele:

```sh
python -m saturnrecomp.build game.toml [--recompile]    # recompile if needed, then build
python -m saturnrecomp.learn game.toml -- --cue GAME.cue --headless --vblanks 3600
    # run; while it stops at code discovery missed, add the seed, recompile and run again
python -m saturnrecomp.analysis.resume game.toml --check   # where the game's tasks can resume
python -m saturnrecomp.inventory game.toml      # every disc file and what became of it, each module's
    # bytes by class, and a check that every discovered instruction has a generated line; each 68000
    # program in [[inventory.sound]] gets the same split from a descent from its vectors
python -m saturnrecomp.m68k FILE.BIN --offset 44   # one 68000 program's descent, or --at ADDR to disassemble
python -m saturnrecomp.native game.toml         # each instruction's native address ranges, from line
    # tables of a second build whose code must equal the built executable's
python -m saturnrecomp.executed game.toml RUN.tsv ...   # whether every instruction a reference run
    # executed (CPU, address, opcode a line) is translated; fails if one a module holds is not, if a
    # 68000 program holds one its descent did not find, or if no note explains one nothing holds
```

Nos comandos acima, `build` recompila quando necessário e compila; `learn` executa e acrescenta
sementes quando a descoberta deixa código de fora; `analysis.resume` identifica retomadas de tarefas.
`inventory` inventaria arquivos e classes de bytes, verifica instruções com linhas geradas e aplica
a mesma divisão aos programas 68000 de `[[inventory.sound]]` por descida a partir dos vetores.
`m68k` analisa um programa ou desmonta em `--at ADDR`. `native` obtém intervalos de endereços nativos
pelas tabelas de linhas de um segundo build cujo código deve ser igual ao executável existente.
`executed` verifica se cada instrução executada numa referência (CPU, endereço, opcode por linha)
foi traduzida: falha se um módulo a contém mas não a traduz, se a análise 68000 não a encontrou,
ou se uma instrução não pertence a nada e não tem nota explicativa.

Um programa também pode controlar uma execução, permitindo que um bot ou teste jogue sem
roteiro totalmente definido de antemão. `saturnrecomp.agent` inicia o executável com `--agent`,
pausa no VBlank 1 e então avança VBlanks, pressiona controles, lê/escreve memória e captura quadros:

```python
from saturnrecomp import agent

with agent.start("build/recomp-build/saturn", ["--cue", "GAME.cue", "--headless"], "build/run/bot") as run:
    run.step(1900)
    run.pad("START")
    run.step(10)
    run.pad("")
    work_ram = run.read(0x06000000, 64)    # bytes, as the CPU reads them
    run.frame().save_png("build/run/bot/now.png")
    print(",".join(run.presses))    # the presses as an --input script, to replay the run
```

A pausa não consome tempo no relógio do Saturn; as mesmas entradas nos mesmos VBlanks produzem
a mesma execução, independentemente de como foram enviadas. O protocolo usa uma linha de texto
por comando e está documentado em `runtime/src/machine/agent.cpp`.

As ferramentas de nível mais baixo recebem suas entradas como argumentos:

```sh
python -m saturnrecomp.disc GAME.cue --info              # IP.BIN, ISO 9660, tracks
python -m saturnrecomp.disc GAME.cue --list
python -m saturnrecomp.disc GAME.cue --extract build/extract   # files + IP.BIN
python -m saturnrecomp.disc GAME.cue --audio build/audio       # CD-DA as WAV
python -m saturnrecomp.sh2 FILE.BIN --find-base               # where it loads
python -m saturnrecomp.sh2 FILE.BIN --base 0600B000 --at 0600B000 --count 64
python -m saturnrecomp.sh2 FILE.BIN --base 0600B000 --refs 25D00000:25D00018
python -m saturnrecomp.sh2 FILE.BIN --base 0600B000 --census
python -m saturnrecomp.hw 25D00002 25F80114 06000310
python -m saturnrecomp.recomp.discover FILE.BIN --base 0600B000 --report     # functions, code/data
python -m saturnrecomp.recomp.match A.BIN@0600B000 B.BIN@0600B000 --names a.tsv --out b.tsv
python -m saturnrecomp.sh2emu FILE.BIN --base 0600B000 --call 060224DC --regs r1=100,r0=7
python -m saturnrecomp.recomp --out build/recomp A=A.BIN@0600B000 B=B.BIN@0600B000 --optest   # to C++
python -m saturnrecomp.recomp --out build/recomp A=A.BIN@0600B000 --hook A:0600B6F4   # sh2_hook after that instruction
python -m saturnrecomp.recomp.selftest --out a.txt --image A=A.BIN@0600B000 --test A --auto  # vectors
cmake -S build/recomp -B build/recomp-build -G Ninja -DCMAKE_CXX_COMPILER=clang++
ninja -C build/recomp-build && build/recomp-build/selftest build/recomp/selftest/optest.txt a.txt
build/recomp-build/saturn --cue GAME.cue                              # the game in a window, with the pad and sound
build/recomp-build/saturn --cue GAME.cue --interp                     # the fields between the game's frames drawn moving
build/recomp-build/saturn --cue GAME.cue --headless --vblanks 3600 --wav run.wav   # the run's sound as a WAV
build/recomp-build/saturn --cue GAME.cue --hook 0600B6F4:r5=1         # a register set at a hooked instruction
build/recomp-build/saturn --cue GAME.cue --headless --vblanks 600 --trace --shot 300,600   # headless, pictures
build/recomp-build/saturn --cue GAME.cue --headless --input 1200:START,1210: --peek 25F80000:8 --watch 05A00000:05A01000
build/recomp-build/saturn --cue GAME.cue --record-input play.txt          # the pad as played in the window, as a script
build/recomp-build/saturn --cue GAME.cue --headless --input @play.txt --vblanks 9000 --shot 7760   # that game again, headless
build/recomp-build/saturn --cue GAME.cue --headless --watch 06058D6C:06058DCF --watch-vblanks 7300:7310   # every store, by function
build/recomp-build/saturn --cue GAME.cue --headless --write 4250:060C179A=0201   # memory set at a VBlank, such as a stage select
build/recomp-build/saturn --cue GAME.cue --headless --save -        # saves kept for this run alone, starting empty
build/recomp-build/saturn --cue GAME.cue --coverage cov.txt --checkpoint 600   # which functions ran, rewritten every 10 minutes
build/recomp-build/saturn --cue GAME.cue --state-out game.state --state-every 60   # the machine dumped every minute and at a quit (a game with --tasks)
build/recomp-build/saturn --cue GAME.cue --state-in game.state       # the run going on from that dump
build/recomp-build/saturn --cue GAME.cue --progress p.txt --progress-keep 060C029C:2,060C0690:8   # both ranges noted whenever the first changes
```

Os comentários dos exemplos originais foram preservados. Os comandos cobrem informações e extração
do disco e áudio, análise SH-2/base/referências, mapa de hardware, descoberta e correspondência de
funções, interpretador, geração C++, hooks e vetores de autoteste. Também mostram build CMake/Ninja,
execução com janela/controle/áudio ou headless, interpolação, WAV, hooks de registradores, traces e
capturas, leitura/monitoramento/escrita de memória, gravação/replay de entradas, save descartável,
cobertura com checkpoint, dumps de estado para jogos com `--tasks` e registro de progresso.

Saves do jogo vão para um arquivo por jogo no diretório de dados do usuário, a menos que `--save`
indique outro: `saturn-recomp/PRODUCT_VERSION/backup.bin` em `$XDG_DATA_HOME` (ou `~/.local/share`),
`~/Library/Application Support` ou `%APPDATA%`. Testes devem usar `--save -` ou arquivo próprio,
começando sem saves e preservando os do jogador.

## Testes com jogadores

`playtest/` prepara o jogo para outras pessoas jogarem e devolve sessões para revisão: launcher
para testadores, Cloudflare Worker para armazenar sessões, workflow de release para Linux/Windows/macOS
e `python -m saturnrecomp.playtest` para o mantenedor. `playtest/README.md` explica a configuração.

## Licença

saturn-recomp usa MIT, exceto um arquivo opcional GPL (`THIRD_PARTY.md`). Com `-DSATURN_VDP1_GPL=ON`,
ou `cmake = ["SATURN_VDP1_GPL=ON"]` em `[game]` do `game.toml`, o VDP1 desenha segundo as regras do
Mednafen: as bordas de sprites escalados passam a coincidir pixel a pixel com Mednafen e o build
é uma obra GPL. Sem essa opção, as regras são próprias, baseadas no manual Sega, e o build continua MIT.

## Comparação com emulador de referência

`python -m saturnrecomp.reference` executa um disco em núcleo libretro headless, quadro a quadro,
com entradas na sintaxe `--input`, e salva imagens `shot-N.png`. Foi escrito para Beetle Saturn
(Mednafen Saturn), que exige BIOS real: `--bios` indica a pasta com `mpr-17933.bin` para disco US
ou europeu. Compile github.com/libretro/beetle-saturn-libretro removendo `-D_LOW_ACCURACY_` de
`FLAGS` no Makefile; essa opção ativa um caminho de branch mais rápido inexistente no Mednafen upstream.

```sh
python -m saturnrecomp.reference mednafen_saturn_libretro.so --bios BIOS_DIR --cue GAME.cue \
    --frames 3600 --input 3000:START,3010: --shot 3300,3400 --out ref/
```

As imagens do núcleo são 330x240 para uma tela 320x224, com nossa imagem em x+5, y+8 dentro delas.
Só o controle 1 fica conectado, salvo `--pads 2`, como no runtime: o jogo pode consultar coisas
diferentes quando há um segundo controle conectado.

Os quadros do núcleo não coincidem com nossos VBlanks. A BIOS real inicia primeiro e as leituras
CD levam o tempo de um drive, aumentando a diferença a cada leitura. O contador de quadros do próprio
jogo coincide, pois para durante cargas. `reference.play_synced` posiciona cada entrada e captura
no mesmo valor desse contador e no mesmo deslocamento de quadros; `reference.difference` compara
um quadro ao nosso. O repositório do jogo rastreia o contador via agente e informa o endereço;
`bomberman compare` de pc-saturnbomberman faz isso.

O restante é o jogo executando de forma diferente. Alguns jogos executam tarefas em cada VBlank
durante cargas; uma carga mais longa deixa seu estado alguns quadros adiante.

`saturnrecomp.audio` compara áudio do mesmo modo, por janelas. `--wav` grava amostras do runtime;
o núcleo conserva as próprias em `Core.audio`. `audio.window` compara uma janela nossa e da referência
no mesmo ponto do jogo, acha o deslocamento de maior correlação e informa correlação, níveis,
amostras saturadas e canais trocados. Dois chips de áudio emulados separadamente não geram bytes
idênticos: uma janela corresponde quando a correlação se aproxima de 1, não quando as amostras são
iguais. Numa janela correspondente, `audio.parts` compara trechos de 10 ms no deslocamento encontrado,
mostrando estalos ou falhas curtas demais para alterar um segundo inteiro.

`audio.live` verifica a outra ponta: áudio entregue ao dispositivo versus `--wav` da própria execução.
O driver SDL de disco (`SDL_AUDIO_DRIVER=disk`) grava as amostras do dispositivo. `audio.live` alinha
as duas gravações e identifica silêncio temporário, amostras puladas e qualquer outra diferença.
Essas amostras devem ser idênticas e são comparadas exatamente.

## Verificação de mudanças

`tools/check.sh` executa todas as verificações sem dados de jogo, as mesmas do CI em cada push:

- Testes Python.
- Autoteste de instruções do recompilador.
- Testes unitários do runtime.

Todos compilam sem SDL. Com `SATURN_SDL` apontando à instalação SDL3, também compila o programa de
autoteste no host SDL. O CI o executa assim no Linux com Clang e GCC, Windows com Clang e macOS.
No Windows também verifica se `saturn.exe` dispensa DLLs ausentes numa instalação nova. Testes de
funcionalidades exclusivas de Linux são pulados nas outras plataformas: build a partir do disco,
agente e ferramentas de pesquisa. Uma mudança de runtime também deve passar nos testes de quadros
de cada jogo, que exigem o disco daquele jogo.

## Camadas

| Camada | Pergunta respondida | Agora | Próximo |
|---|---|---|---|
| 1. Reconhecer | O que existe neste disco? | `disc --info`: IP.BIN (produto, regiões, periféricos, pilhas, primeira leitura), volume ISO9660 e faixas com pregaps | `fingerprint`: SBL/SGL e versões por assinatura, middleware CRI, versões de drivers de áudio |
| 2. Extrair | Transformar formatos padrão em arquivos padrão | `disc --extract` (sistema sobre MODE1/MODE2, arquivos intercalados, registros CD-DA omitidos), `disc --audio` (CD-DA para WAV) | decodificadores VDP1/VDP2 (4/8/16bpp, CLUT, CRAM), Sega FILM/Cinepak, bancos SCSP |
| 3. Mapear código | O que faz o código e onde? | `sh2` (SH7604, desmontagem com literal pools resolvidos/registradores nomeados, `--refs`, `--census`, `--find-base`), `hw` (mapa, registradores, ponteiros de serviços BIOS, vetores SCU), `sh2emu` (interpretador SH-2 para funções isoladas, oráculo do recompilador) | mapa executável (crt0, BSS, programas trocados no mesmo endereço), `fingerprint` (SBL por assinatura) |
| 4. Traduzir | Transformar SH-2 em C++ | `recomp.discover` (descida recursiva em SHC sem símbolos, propagação de constantes para chamadas por registrador, quatro formas de switch, sementes por ponteiros/prólogos), `recomp.match` (funções equivalentes em programas relocados com nomes preservados), `recomp` (função C++ por entrada, delay slots, retorno de chamadas verificado, dispatch protegido por registrador, switches, pontos seguros; módulo por programa reconhecido por crc32), `recomp.selftest` (vetores de `sh2emu` para cada forma de instrução e funções isoladas), hooks (`--hook`, chamada `sh2_hook` após instruções escolhidas, para registradores ou camada específica do port) | substituir função inteira por implementação manual |
| 5. Runtime | Substituir hardware | `runtime/` C++20: contexto SH-2, Work RAM e endereços cache-through, dispatch por módulos ativos, harness de autoteste; executável `saturn` com tempo virtual/host, entrada de programa desenrolando pilha host, boot/BIOS HLE (interrupções, máscaras SCU, semáforos, BUP em arquivo), SCU (interrupções/timers/DMA), SMPC (INTBACK, PDR1/DDR1, região, controles roteirizados e gravados, slave/áudio on/off), slave SH-2 como corrotina determinista, CD nos registradores sobre .cue/.bin (`--accurate-drive` com seeks/leituras duplas/diretórios nos tempos Mednafen), divisão/FRT/DMAC SH7604 (FRT/watchdog em φ: clock mestre/65 até CKCHG352, depois /61), raster TVSTAT VBLANK/HBLANK, VDP1 software com todos comandos/modos/cálculos e quadriláteros, conclusão após ciclos de comandos (custos Mednafen com SATURN_VDP1_GPL, aproximação sem), VDP2 software NBG0–3 célula/bitmap com zoom, sprites/prioridades/cores/offsets/back screen, janela SDL3 com teclado/gamepad/controle de toque e menu de pausa; 68000 Musashi e SCSP (slots/envelopes/LFO/FM/timers/interrupções/DMA/DSP/CD-DA externo) em tempo virtual, saída SDL3 ou WAV; `--interp` move comandos VDP1 parcialmente entre quadros por chaves da camada do jogo; logs de registradores e monitor de escritas Work RAM | VDP1 GPU, restante do VDP2 (rotação/janelas/line scroll…), `--interp` sem um quadro de latência |

## Princípios

- Python puro, sem dependências, nas camadas 1–4; runtime (camada 5) C++20/SDL3, compilado com CMake, Ninja e clang.
- Cada afirmação é conferida num disco real antes de entrar.
- Conhecimento específico de jogo fica de fora.
- Cada mudança é verificada em todos os jogos que utilizam o motor antes de entrar.

## Verificações de cada módulo

| Módulo | Verificado por |
|---|---|
| `disc` | Virtual Hydlide europeu, .cue Redump de28 faixas: IP.BIN MK-81380/regiãoE,458 arquivos ISO9660 extraídos, primeira leitura A.BIN idêntica a OPEN.BIN, LBAs/pregaps. ISO9660 atravessando MODE1/2352 e MODE2/2352 Form1:270 arquivos, cinco intercalados (unidade4/3, gap15/16),45546 setores com número XA correto; demais arquivos idênticos em Virtual Hydlide e jogo SGL de dois discos |
| `sh2` | todas65536 palavras16bits contra capstone5 SH-2:53752 com mesmos mnemônicos/operandos;452 decodificadas só por capstone são extensões SH-2A ausentes no SH7604 (`movua`, `divs`, `jsr/n`, `clips`…). `--find-base` situa14 de15 executáveis Virtual Hydlide na base indicada pelo crt0 |
| `recomp.discover` |15 programas Virtual Hydlide: nenhuma descida encontra dados/opcode indefinido; todo alvo switch resolvido é código. Comparação com Ghidra12 no programa de campo:571/576 funções; outras cinco são caudas compartilhadas seguidas como parte dos chamadores |
| `recomp.match` | limitador de quadros Virtual Hydlide, localizado pelos bytes: nome propagado corretamente em11/11 programas |
| `sh2emu` | quatro helpers de divisão SHC com3000 operandos aleatórios cada: quociente/resto, com/sem sinal,12000/12000 iguais ao Python |
| `recomp` (emissão) | Virtual Hydlide:15 programas,10259 funções,1,5milhão de instruções compiladas/ligadas com clang22; nenhum alvo estático fora dos módulos. Autoteste:775 funções de teste (126 formas, isoladas/em delay slot, fluxo) e2653 do jogo,51532 vetores, zero diferenças de registradores/memória |
| `runtime` (Saturn) | boot da primeira leitura até primeiro campo Virtual Hydlide: filme pelo CD e handshake PCM, três entradas de programa (HYDSYS, título, campo), ambas CPUs, laço limitado a12fps;120s de tempo de jogo sem retorno errado/chamada não entrada, execução repetível |
| `runtime` (controle/HBLANK/gravação) | jogo FMV1995 com Cinepak SBL espera HBLANK TVSTAT para escrever TVMD e lê controle só PDR1 (ID0xB, START, direções, A); boot/filmes/título/backstage/primeiro item jogados pelo usuário em janela e repetidos headless por `--input @FILE`. Virtual Hydlide e jogo SGL dois discos com imagens/áudio byte a byte iguais antes/depois |
| `runtime` (áudio) | Virtual Hydlide versus Beetle Saturn: correlação0,99 no filme, mesma imagem estéreo/deslocamento constante (1,6dB acima antes da escala27/32); título silencioso em ambos; menus/campo com música/efeitos dos drivers próprios do jogo |
| `runtime` (`--interp`) | campo Virtual Hydlide com chaves da camada do jogo:500 campos andando/girando, sem rachaduras além das do jogo, árvores inteiras na curva; filme/título/menus pixel a pixel iguais sem interpolação; jogado pelo usuário |
| `runtime` (VDP1/VDP2/janela) | Virtual Hydlide versus Beetle Saturn: título/quadros do filme iguais, menus/primeiro campo com mesmo layout/aspecto; filme15fps controlado pela posição PCM |
| `sh2.encode` | todas53752 palavras decodificáveis entre65536 recodificadas pelo formato/campos:53752/53752 iguais |
| `hw` | nomes de registradores usados no código Virtual Hydlide lidos no contexto (VDP1 FBCR/PTMR/EDSR, VDP2 TVSTAT/offset de cor, SMPC COMREG, FRT FTCSR na espera slave) |

## Lacunas conhecidas

- Descoberta não alcança o que nada referencia: funções folha sem prólogo chamadas só por código morto (cerca de7% do texto do programa inicial). Runtime para e registra chamada a endereço sem entrada para acrescentá-lo como semente; Virtual Hydlide não fez nenhuma do boot ao primeiro campo.
- Salto calculado fora das quatro formas switch permanece não resolvido; código recompilado tenta cada instrução da própria função e então faz dispatch. Em Virtual Hydlide, os36 restantes são chamadas de cauda por tabelas de ponteiros resolvidas em execução.
- A semântica do recompilado é a de `sh2emu`. Onde o interpretador pode divergir do hardware (saturação `mac.w`, casos limite `div1`), autoteste não distingue; o jogo em execução é a verificação.
- Interrupções chegam só em pontos seguros (back-edges, chamadas, `ldc …,sr`); não há contagem de ciclos. Interrupção levantada duas vezes antes de um poll é atendida uma vez (timer SCU por linha chega aproximadamente uma vez por poll).
- Áudio: MIDI ausente (entrada vazia); CD-DA chega à entrada externa SCSP, mas nenhum jogo ainda o reproduziu. VDP1 software na resolução Saturn; planos de rotação, janela de sprites, scroll por linha/célula, mosaico, tela de cor por linha e sombras VDP2 ausentes (runtime registra a solicitação). Não emulados: SCU DSP, interrupções internas SH-2, caches.
- `match` exige fingerprint único em ambos lados; funções pequenas idênticas (cópias de biblioteca) só correspondem pelos chamadores.
- `--find-base` pontua literais apontando a prólogos. Programa pequeno que chama residente grande pode obter pontuação maior na base errada (MENU.BIN Virtual Hydlide): conferir crt0, cujo BSS começa no fim do arquivo.
- `hw.BIOS`: nomes de ponteiros de serviço vêm dos headers SBL; os não marcados como conferidos por jogo permanecem não confirmados.
- `sh2.Image.literal_refs` lê cada `mov.l @(disp,PC)` do arquivo, inclusive dados; alguns resultados em dados são ruído.

## Licença

MIT: veja [LICENSE](LICENSE). saturn-recomp não contém dados de jogo nem código Sega: lê e substitui,
sem incluí-los. O runtime inclui Musashi (68000, MIT) com SoftFloat; o SCSP deriva parcialmente do
MAME (BSD-3-Clause). Os termos ficam em [THIRD_PARTY.md](THIRD_PARTY.md).

As interfaces ainda mudarão conforme novos jogos exigirem recursos.
