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
