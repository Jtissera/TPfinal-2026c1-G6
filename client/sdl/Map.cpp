#include "Map.h"

#include "../../editor/map/mapSerializer.h"
#include "../Game.h"
#include "ECS/Components.h"
#include "GroupLabels.h"
#include <cstdlib>
#include <vector>

Map::Map(Manager &manager, AssetManager &assets, const std::string &textID,
         int mapScale, int tileSize)
    : manager(manager), assets(assets), textID(textID), mapScale(mapScale),
      tileSize(tileSize)
{
    scaledSize = mapScale * tileSize;
}

std::string Map::GetRandomTextureForType(TileType type)
{
    switch (type)
    {
    case TileType::GRASS:
        return "tile_grass";
    case TileType::WATER:
        return "tile_water";
    case TileType::SAND:
        return "tile_sand";
    case TileType::CAVERN_ENTRANCE:
        return "tile_cavern_entrance";
    case TileType::DUNGEON_ENTRANCE:
        return "tile_dungeon_entrance";
    case TileType::CITY_FLOOR:
        return "tile_city_floor";
    case TileType::HOUSE:
        return "tile_house";
    case TileType::CHURCH:
        return "tile_church";
    case TileType::MILL:
        return "tile_mill";
    case TileType::CAVERN_FLOOR:
        return "tile_cavern_floor";
    case TileType::CAVERN_WALL_H:
        return "tile_cavern_horizontal_wall";
    case TileType::CAVERN_WALL_V:
        return "tile_cavern_vertical_wall";
    case TileType::DUNGEON_FLOOR:
        return "tile_dungeon_floor";
    case TileType::EXIT:
        return "tile_exit";

    case TileType::FOREST:
    {
        std::vector<std::string> trees = {"tile_tree", "tile_tree2", "tile_tree3"};
        int randomIndex = rand() % trees.size();
        return trees[randomIndex];
    }

    case TileType::CACTUS:
    {
        std::vector<std::string> cacti = {"tile_cactus", "tile_cactus2",
                                          "tile_cactus3"};
        return cacti[rand() % cacti.size()];
    }

    case TileType::STONE:
    {
        std::vector<std::string> stones = {"tile_stone", "tile_stone2",
                                           "tile_stone3"};
        return stones[rand() % stones.size()];
    }

    default:
        return "grass";
    }
}

void Map::LoadMap(const std::string &path)
{
    MapData mapData = MapSerializer::load(path);

    width  = mapData.width();
    height = mapData.height();
    tiles.clear();
    tiles.reserve(width * height * 2);  // 2 capas por tile como maximo

    srand(123456);

    for (int y = 0; y < mapData.height(); y++)
    {
        for (int x = 0; x < mapData.width(); x++)
        {
            const Tile &t = mapData.at(x, y);

            if (t.type == TileType::GRASS || t.type == TileType::WATER ||
                t.type == TileType::SAND || t.type == TileType::CITY_FLOOR || t.type == TileType::CAVERN_FLOOR || t.type == TileType::DUNGEON_FLOOR)
            {
                AddTile(GetRandomTextureForType(t.type), x * scaledSize, y * scaledSize, t.type);
            }
            else if (t.type == TileType::FOREST || t.type == TileType::STONE ||
                     t.type == TileType::DUNGEON_ENTRANCE || t.type == TileType::CAVERN_ENTRANCE || t.type == TileType::EXIT)
            {
                AddTile(GetRandomTextureForType(TileType::GRASS), x * scaledSize, y * scaledSize, TileType::GRASS);
            }
            else if (t.type == TileType::CACTUS)
            {
                AddTile(GetRandomTextureForType(TileType::SAND), x * scaledSize, y * scaledSize, TileType::SAND);
            }
            else if (t.type == TileType::HOUSE || t.type == TileType::CHURCH || t.type == TileType::MILL)
            {
                AddTile(GetRandomTextureForType(TileType::CITY_FLOOR), x * scaledSize, y * scaledSize, TileType::CITY_FLOOR);
            }
            else if (t.type == TileType::CAVERN_WALL_H || t.type == TileType::CAVERN_WALL_V)
            {
                AddTile(GetRandomTextureForType(TileType::CAVERN_FLOOR), x * scaledSize, y * scaledSize, TileType::CAVERN_FLOOR);
            }
        }
    }

    for (int y = 0; y < mapData.height(); y++)
    {
        for (int x = 0; x < mapData.width(); x++)
        {
            const Tile &t = mapData.at(x, y);

            if (t.type == TileType::FOREST || t.type == TileType::CACTUS ||
                t.type == TileType::STONE || t.type == TileType::DUNGEON_ENTRANCE ||
                t.type == TileType::CAVERN_ENTRANCE || t.type == TileType::HOUSE ||
                t.type == TileType::CHURCH || t.type == TileType::MILL ||
                t.type == TileType::CAVERN_WALL_H || t.type == TileType::CAVERN_WALL_V ||
                t.type == TileType::EXIT)
            {
                std::string randomTexId = GetRandomTextureForType(t.type);
                AddTile(randomTexId, x * scaledSize, y * scaledSize, t.type);
            }
        }
    }
}

void Map::AddTile(const std::string &texId, int x, int y, TileType type)
{
    SDL_Texture *tex = assets.GetTexture(texId);
    if (!tex)
    {
        std::cerr
            << "ERROR FATAL: Textura no encontrada en AssetManager para el ID: '"
            << texId << "'\n";
        return;
    }

    int srcW = 32;
    int srcH = 32;

    if (type == TileType::GRASS || type == TileType::WATER ||
        type == TileType::SAND)
    {
        srcW = 32;
        srcH = 32;
    }
    else
    {
        SDL_QueryTexture(tex, NULL, NULL, &srcW, &srcH);
    }

    int xpos = x;
    int ypos = y;

    // Offset Vertical: Desplaza hacia arriba si el objeto es alto
    if (srcH > 32)
    {
        int pixelDiffY = srcH - 32;
        ypos -= (pixelDiffY * mapScale);
    }

    if (srcW > 32)
    {
        int pixelDiffX = srcW - 32;
        xpos -= (pixelDiffX * mapScale) / 2;
    }

    const bool isTop = (type == TileType::FOREST || type == TileType::STONE ||
                        type == TileType::CACTUS || type == TileType::DUNGEON_ENTRANCE ||
                        type == TileType::CAVERN_ENTRANCE || type == TileType::HOUSE ||
                        type == TileType::CHURCH || type == TileType::MILL ||
                        type == TileType::CAVERN_WALL_H || type == TileType::CAVERN_WALL_V ||
                        type == TileType::EXIT);

    TileEntry entry;
    entry.texture  = tex;
    entry.srcRect  = {0, 0, srcW, srcH};
    entry.destRect = {xpos, ypos, srcW * mapScale, srcH * mapScale};
    entry.isTop    = isTop;
    tiles.push_back(entry);
}

void Map::renderLayer(SDL_Renderer *renderer, const SDL_Rect &camera,
                      const SDL_Rect &viewport, bool top) const
{
    // Coordenadas del viewport en world-space
    const int worldLeft   = camera.x;
    const int worldTop    = camera.y;
    const int worldRight  = camera.x + viewport.w;
    const int worldBottom = camera.y + viewport.h;

    // Offset vertical del HUD (33px de barra superior)
    const int hudOffsetY = 133;

    for (const TileEntry &t : tiles)
    {
        if (t.isTop != top || t.texture == nullptr)
            continue;

        // Posición en pantalla
        const int screenX = t.destRect.x - camera.x;
        const int screenY = t.destRect.y - camera.y + hudOffsetY;

        // Culling: saltar si está completamente fuera del viewport
        if (screenX + t.destRect.w < 0 || screenX > viewport.w ||
            screenY + t.destRect.h < 0 || screenY > viewport.h + hudOffsetY)
            continue;

        SDL_Rect dst = {screenX, screenY, t.destRect.w, t.destRect.h};
        SDL_RenderCopy(renderer, t.texture,
                       const_cast<SDL_Rect *>(&t.srcRect), &dst);
    }
    (void)worldLeft; (void)worldTop; (void)worldRight; (void)worldBottom;
}