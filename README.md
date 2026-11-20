# csotn-saturn-recomp

Port experimental de **Castlevania: Symphony of the Night, Sega Saturn JP**, para Windows x64.
O SH-2 é traduzido para C++ antes da compilação; o runtime reconstrói os dispositivos do Saturn.
O objetivo é tornar o motor, as ferramentas e as correções reproduzíveis e editáveis.

**Projeto em desenvolvimento. Campanha integral, estabilidade universal e 60 FPS constantes não estão certificados.**
Não inclui jogo, BIOS, executável do jogo, C++ traduzido do disco, extrações, áudio, imagens ou saves.
O usuário precisa fornecer sua própria mídia localmente. Nenhum download de ROM é feito.

## O que está salvo aqui

- `engine/`: fontes do motor, gerador SH-2 e testes, com as correções locais de overlays,
  salvamento normal, DMA/divisão, menu de desenvolvimento e composição VDP2 na GPU.
- `sotn-experiment/`: ferramentas de preparação/geração/build, launchers, configurações e testes sintéticos.
- `scripts/`: gate de publicação, testes sem janela e geração local dos marcadores privados.
- `docs/`: instalação, decisões, manual e limites das provas existentes.

VDP2 compute e apresentação suportam **Vulkan e DirectX 12**. Vulkan foi preferido nas medianas
dos ensaios offscreen locais; DirectX 12 continua disponível. Modos gráficos não cobertos pelo compute
usam o compositor CPU. O áudio ainda utiliza SCSP e o núcleo Musashi 68000 interpretado.
O menu de desenvolvimento oferece pausa, avanço de quadro, configuração gráfica, controles,
hacks opcionais e relatórios de bug. O salvamento normal continua nas salas do jogo.

## Começar

Windows 11, PowerShell e Python 3.11 ou posterior. LLVM/clang-cl, MSVC Build Tools, CMake,
Ninja e SDL3 são necessários para compilar e executar os testes nativos.

```powershell
git clone https://github.com/Neguchads/csotn-saturn-recomp.git
cd csotn-saturn-recomp
py -3 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements-dev.txt
$env:PYTHONPATH = "$PWD/engine/src"
```

Os comandos de [instalação e testes](docs/SETUP.md) usam dependências fornecidas localmente.
O provisionamento inteiro de uma máquina nova e a geração a partir de uma nova cópia do disco ainda
precisam de validação. Este repositório não é uma release pronta para jogar.

## Provas e diferenças da cópia local

O snapshot de código vem da auditoria local de 26/10/2026: perfil com 205 módulos e 11020 funções
configurados, build Windows x64 2870742E produzida localmente. Esses arquivos gerados e esse executável
não acompanham o repositório. A exportação pública é testada separadamente; resultados em
[VALIDATION](docs/VALIDATION.md). Build não demonstra uma campanha completa.

A tabela de marcadores de salas veio de um blueprint privado e **não está publicada**.
O mapa explorado e sua navegação continuam implementados; cores de save/teleporte exigem a tabela
local fornecida pelo usuário. O gerador aceita JSON e cria um header ignorado pelo Git.
Os testes dessa integração usam dois marcadores sintéticos, mantendo as verificações anteriores.

## Licença e origem

Código próprio: [GPL-3.0-or-later](LICENSE). O motor preserva [licença MIT](engine/LICENSE)
e [avisos de terceiros](engine/THIRD_PARTY.md); o rasterizador VDP1 opcional segue Mednafen sob GPL2+.
Veja [THIRD_PARTY](THIRD_PARTY.md) e [origem das alterações](docs/SOURCES.md).
Nenhuma licença das ferramentas concede direitos sobre arquivos do jogo.

Não afiliado à Konami ou à Sega. Trabalho desenvolvido com assistência de IA; testes e limitações
são registrados sem converter geração ou compilação em prova de gameplay.
