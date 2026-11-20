# Licenças e origem

As ferramentas e alterações próprias são GPL-3.0-or-later, com texto integral em LICENSE.
O motor foi vendorizado com sua licença MIT e avisos preservados em engine/LICENSE e engine/THIRD_PARTY.md.
Revisão upstream: f20de940e8013fc84103a5b39c4069ed8071b1f9, mais alterações locais documentadas.

Componentes incluídos no motor:

- Musashi: MIT; SoftFloat 2b mantém seus próprios termos nos fontes.
- Dear ImGui: MIT; minicoro: MIT No Attribution com trechos MIT.
- stb_truetype: MIT/public domain; Inter: SIL Open Font License 1.1, duas fontes de build incluídas.
- SCSP: termos BSD-3-Clause dos trechos derivados de MAME preservados no aviso upstream.
- VDP1 opcional: derivado de Mednafen 1.32.1, GPL2 ou posterior; avisos e fontes mantidos.
- AMD FidelityFX FSR1: MIT, revisão a21ffb8f6c13233ba336352bdff293894c706575;
  licença, headers e manifesto em engine/runtime/third_party/fsr1/.

Dependências externas, não distribuídas: SDL3 3.4.14 (zlib), LLVM 21.1.8 (Apache2 com exceções LLVM),
CMake 3.31.6 (BSD3), Ninja 1.11.1.4 (Apache2), pytest 9.1.1 (MIT) e DXC1.9.2609
(avisos Microsoft/MIT/LLVM do pacote original).

sotn-decomp é uma referência externa AGPL3; não foi vendorizado. O usuário prepara sua cópia local
e seus símbolos. Ymir e outros emuladores foram referências de comportamento, não motores substitutos.

ROMs, BIOS, arquivos extraídos, código traduzido do disco, saves, capturas e blueprint privado não são
distribuídos. A redistribuição das ferramentas não concede direitos sobre o jogo ou marcas de terceiros.
