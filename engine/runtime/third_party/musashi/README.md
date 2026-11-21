[English](#english) | [Português](#portugues)

<a id="english"></a>

# Musashi 4.60

The 68000 of saturnkit's sound side (`../../sound.cpp`): Karl Stenerud's
Musashi, MIT licence (the notice is at the top of `m68kcpu.c` and in
`readme.txt`), with MAME's repackaging of John R. Hauser's SoftFloat 2b in
`softfloat/` (its own permissive notice, `softfloat/README.txt` and the
source headers), which the FPU code compiled into `m68kcpu.c` needs.

Taken as released, with `m68kops.c`/`m68kops.h` as `m68kmake` generates
them from 4.60's `m68k_in.c`. One change, in `m68kconf.h`: the 68010,
68EC020, 68020, 68030 and 68040 and the PMMU are switched off
(`M68K_EMULATE_* M68K_OPT_OFF`); the Saturn's sound CPU is a plain 68000
(68EC000). The memory callbacks (`m68k_read_memory_*`,
`m68k_write_memory_*`, `m68k_read_disassembler_*`) are `sound.cpp`'s.

---

<a id="portugues"></a>

# Musashi 4.60

O 68000 do subsistema de áudio do saturnkit (`../../sound.cpp`): Musashi de Karl Stenerud,
licença MIT (o aviso fica no início de `m68kcpu.c` e em `readme.txt`), com o SoftFloat 2b
 de John R. Hauser reempacotado pelo MAME em `softfloat/` (aviso permissivo próprio,
`softfloat/README.txt` e cabeçalhos dos fontes), necessário ao código de FPU compilado em `m68kcpu.c`.

Incorporado como lançado, com `m68kops.c`/`m68kops.h` gerados por `m68kmake` a partir de
`m68k_in.c` da versão 4.60. Uma mudança em `m68kconf.h`: 68010, 68EC020, 68020, 68030,
68040 e PMMU ficam desativados (`M68K_EMULATE_* M68K_OPT_OFF`); a CPU de áudio do Saturn
é um 68000 simples (68EC000). Os callbacks de memória (`m68k_read_memory_*`,
`m68k_write_memory_*`, `m68k_read_disassembler_*`) pertencem a `sound.cpp`.
