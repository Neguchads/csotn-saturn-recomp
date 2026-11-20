#pragma once
#include <array>

namespace SotnMapOverlay {
struct Marker { bool inverted; int x, y, kind; };
// A tabela do jogo é gerada localmente. O código público não contém o blueprint.
#if __has_include("sotn_map_markers.local.h")
#include "sotn_map_markers.local.h"
inline constexpr bool kMapMarkerDataAvailable = true;
#else
inline constexpr std::array<Marker, 0> kMarkers{};
inline constexpr bool kMapMarkerDataAvailable = false;
#endif
}
