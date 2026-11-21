[English](#english) | [Português](#portugues)

<a id="english"></a>

> Historical manual from the local installation, exported on 20 November 2026. Referenced builds, captures and `validacao/` folders are not distributed. For this checkout, follow [SETUP](SETUP.md), [VALIDATION](VALIDATION.md) and [SOURCES](SOURCES.md).

# Public references

| Project | Consulted revision | Role and limit |
|---|---|---|
| [saturn-recomp](https://github.com/callumjenkins/saturn-recomp) | f20de940e8013fc84103a5b39c4069ed8071b1f9 | Active engine; MIT with optional GPL renderer |
| [pc-saturnbomberman](https://github.com/callumjenkins/pc-saturnbomberman) | c73bc117daec9f2dba4bf32cb08b5f0a91fb93a3 | Reference project and active submodule checkout |
| [sotn-decomp](https://github.com/Xeeynamo/sotn-decomp) | 2be55ee17221ee306e330edf2df2d49d48ba47ae | Game-specific addresses and structures; AGPL-3.0 |
| [saturnkit](https://github.com/vs-sr-dev/saturnkit) | 1b345ef949e1fa258ce3f693f03c0df8acdfb40d | Engine origin, useful for hypotheses; not independent |
| [satrecomp](https://github.com/tmcleroy/satrecomp) | c1e3bddf119180d77894eee1503b765cf0848efb | Another implementation; integrated Ymir test depends on an unpublished patch; GPL-3.0-only |
| [SaturnRecomp](https://github.com/sonsegajp/SaturnRecomp) | 26c9715e5493054b8a205aa31d73d8f125fdd8f5 | Additional reference; complete AOT generator absent from README; check license before reusing code |
| [Ymir](https://github.com/ymir-emu/Ymir) | 8cdf0a530ec1927f44bf3cd02c6ab51a586c4407 | Future external execution reference; GPL-3.0; not run in this step |
| [Serena](https://github.com/oraios/serena) | abb0a0f01b94bc8050086f0fb860077d17730324 | Local Python/C++ symbol MCP; not a recompilation engine |
| [GhidraMCP](https://github.com/LaurieWired/GhidraMCP) | 1e980252087b2479a21b9ca6d82d5829538651a7 | Local MCP; portable Ghidra12.1.4 and JDK21; STAGE_02 analyzed |
| [objdiff](https://github.com/encounter/objdiff) | 9aeec9cbab1a056ade7dc1f1625aacc41b0f8511 | Source consulted; binaries3.8.3 pinned separately; SH-2 object comparison tested |
| [asm-differ](https://github.com/simonlindholm/asm-differ) | 0dd09af8f8008f1f880327cf0aca3b26d2562ea2 | Assembly comparison; decomp Saturn configuration; six tests and SH-2 fixture verified |

Clones in `referencias/` are local and do not enter the experiment's Git repository. The private Discord study also stays local; its IDs, exports and messages are not included in this public selection.

In the local copy, the study entry point is `referencias/estudo-consolidado/INDICE.md`. It gathers lookup by problem, conclusions recovered from subagents and sources. Before repeating a difficult investigation, consult the corresponding study and check its current evidence. Private studies remain ignored by Git.

Cross-referencing documentation produces hypotheses. Validating behavior requires equivalent execution comparisons and, when possible, real hardware. Incorporating reference code is a different decision from consulting it and requires checking applicable terms.

---

<a id="portugues"></a>

> Manual histórico da instalação local, exportado em 20/11/2026. Builds, capturas e pastas `validacao/` citadas não são distribuídas. Para este checkout, siga [SETUP](SETUP.md), [VALIDATION](VALIDATION.md) e [SOURCES](SOURCES.md).

# Referências públicas

| Projeto | Revisão consultada | Papel e limite |
|---|---|---|
| [saturn-recomp](https://github.com/callumjenkins/saturn-recomp) | f20de940e8013fc84103a5b39c4069ed8071b1f9 | Motor ativo; MIT com renderizador opcional GPL |
| [pc-saturnbomberman](https://github.com/callumjenkins/pc-saturnbomberman) | c73bc117daec9f2dba4bf32cb08b5f0a91fb93a3 | Projeto de referência e checkout do submódulo ativo |
| [sotn-decomp](https://github.com/Xeeynamo/sotn-decomp) | 2be55ee17221ee306e330edf2df2d49d48ba47ae | Endereços e estruturas específicos do jogo; AGPL-3.0 |
| [saturnkit](https://github.com/vs-sr-dev/saturnkit) | 1b345ef949e1fa258ce3f693f03c0df8acdfb40d | Origem do motor, útil para hipóteses; não independente |
| [satrecomp](https://github.com/tmcleroy/satrecomp) | c1e3bddf119180d77894eee1503b765cf0848efb | Outra implementação; teste integrado com Ymir depende de patch não publicado; GPL-3.0-only |
| [SaturnRecomp](https://github.com/sonsegajp/SaturnRecomp) | 26c9715e5493054b8a205aa31d73d8f125fdd8f5 | Referência complementar; gerador AOT completo ausente no README; verificar licença antes de reutilizar código |
| [Ymir](https://github.com/ymir-emu/Ymir) | 8cdf0a530ec1927f44bf3cd02c6ab51a586c4407 | Futura referência externa de execução; GPL-3.0; não executado por esta etapa |
| [Serena](https://github.com/oraios/serena) | abb0a0f01b94bc8050086f0fb860077d17730324 | MCP local de símbolos Python/C++; não é motor de recompilação |
| [GhidraMCP](https://github.com/LaurieWired/GhidraMCP) | 1e980252087b2479a21b9ca6d82d5829538651a7 | MCP local; Ghidra 12.1.4 e JDK21 portáteis; STAGE_02 analisado |
| [objdiff](https://github.com/encounter/objdiff) | 9aeec9cbab1a056ade7dc1f1625aacc41b0f8511 | Fonte consultada; binários 3.8.3 fixados separadamente; comparação de objetos SH-2 testada |
| [asm-differ](https://github.com/simonlindholm/asm-differ) | 0dd09af8f8008f1f880327cf0aca3b26d2562ea2 | Comparação de assembly; configuração Saturn do decomp; seis testes e fixture SH-2 verificados |

Clones em `referencias/` são locais e não entram no Git do experimento. O estudo privado do Discord também permanece local; seus IDs, exportações e mensagens não acompanham esta seleção pública.

Na cópia local, a entrada para os estudos é `referencias/estudo-consolidado/INDICE.md`. Ela reúne consulta por problema, conclusões recuperadas dos subagentes e fontes. Antes de repetir uma investigação difícil, consultar o estudo correspondente e conferir sua evidência atual. Os estudos privados continuam ignorados pelo Git.

Cruzamento de documentação gera hipóteses. Validar comportamento exige comparar execuções equivalentes e, quando possível, hardware real. Incorporar código de uma referência é uma decisão diferente de consultá-la e exige conferir os termos aplicáveis.
