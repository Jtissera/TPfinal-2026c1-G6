#pragma once
#include "../../editor/map/mapData.h"
#include <cstdint>

struct Rect {
    float x, y, w, h;
};

class CollisionSystem {
public:
    explicit CollisionSystem(const MapData& mapData);

    // Colision con el mapa — recibe pies del sprite en pixeles
    // Internamente convierte a tile para lookup en MapData
    bool isWalkable(float pixelX, float pixelY) const;

    // Bounds del mapa en pixeles
    bool isInBounds(float pixelX, float pixelY) const;

    // Colision AABB entre dos hitboxes
    bool overlaps(const Rect& a, const Rect& b) const;

    // Adyacencia — true si los centros estan a <= threshold pixeles
    // (distancia Chebyshev: max(|dx|, |dy|) <= threshold)
    // Usada para: ataque melee, interaccion NPC, spawn cerca de un punto
    bool isAdjacent(float ax, float ay,
                    float bx, float by,
                    float threshold) const;

    // Helpers de conversion
    int toTileX(float pixelX) const;
    int toTileY(float pixelY) const;
    float tileCenterX(int tileX) const;
    float tileCenterY(int tileY) const;

    int mapWidthTiles()  const;
    int mapHeightTiles() const;

    // Version en tiles (para NPCs, spawn checks internos)
    bool isWalkableTile(int tileX, int tileY) const;
    bool isInBoundsTile(int tileX, int tileY) const;

private:
    const MapData& mapData;
    int tileSize;  // leido de mapData o hardcodeado 96
};