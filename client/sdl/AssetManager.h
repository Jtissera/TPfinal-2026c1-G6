
#ifndef PRUEBA_SDL_ASSETMANAGER_H
#define PRUEBA_SDL_ASSETMANAGER_H

#include <map>
#include <string>
#include "TextureManager.h"
#include "ECS/Vector2D.h"
#include "ECS/ECS.h"
#include "SDL_ttf.h"
#include "../../common/dtos/gameTypes.h"
#include "common/network/protocol/protocol.h"

class AssetManager
{


public:
    AssetManager(Manager* man, Protocol* protocol);
    ~AssetManager();

    //gameobjects

    void CreateProjectile(Vector2D pos, Vector2D vel, int range, int speed, std::string id);
    Entity* CreateNpc(const NPCData& data);
    Entity* CreateEnemy(const NPCData& data);
    Entity* CreatePlayer(const PlayerDto& data);

    //texture management
    void AddTexture(std::string id, const char* path);
    SDL_Texture* GetTexture(std::string id);

    void AddFont(std::string id, std::string path, int fontSize);
    TTF_Font* GetFont(std::string id);


private:

    Manager* manager;
    Protocol* protocol;
    std::map<std::string, SDL_Texture*> textures;
    std::map<std::string, TTF_Font*> fonts;
    std::string textureForNPC(NpcType type);

};

#endif //PRUEBA_SDL_ASSETMANAGER_H
