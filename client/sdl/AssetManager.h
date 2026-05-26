
#ifndef PRUEBA_SDL_ASSETMANAGER_H
#define PRUEBA_SDL_ASSETMANAGER_H

#include <map>
#include <string>
#include "TextureManager.h"
#include "ECS/Vector2D.h"
#include "ECS/ECS.h"
#include "SDL_ttf.h"
#include "../../common/dtos/gameTypes.h"
#include "common/queue.h"
#include "common/network/messages/message.h"

class AssetManager
{


public:
    AssetManager(Manager* man, Queue<std::shared_ptr<const Message>>& sendQueue);
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

    // Carga el manifest principal.
    // Ese manifest contiene la lista de archivos JSON secundarios.
    void LoadManifest(const std::string& manifestPath);

    // Carga texturas desde un JSON específico.
    // Ejemplo: players.json, enemies.json, effects.json.
    void LoadTexturesFromJson(const std::string& jsonPath);



private:

    Manager* manager;
    Queue<std::shared_ptr<const Message>>& sendQueue;
    std::map<std::string, SDL_Texture*> textures;
    std::map<std::string, TTF_Font*> fonts;
    std::string textureForNPC(NpcType type);

};

#endif //PRUEBA_SDL_ASSETMANAGER_H
