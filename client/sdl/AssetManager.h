
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
#include "ECS/SpriteSheetConfig.h"
#include "state/ItemView.h"
#include "state/PlayerViewState.h"

class AssetManager
{


public:
    AssetManager(Manager* manager,Queue<std::shared_ptr<const Message>>& sendQueue,TextureManager& textureManager);
    ~AssetManager();

    //gameobjects

    void CreateProjectile(Vector2D pos, Vector2D vel, int range, int speed, std::string id);
    Entity* CreateNpc(const NPCData& data);
    Entity* CreateEnemy(const NPCData& data);
    Entity* CreatePlayer(const PlayerDto& data);
    Entity* CreateRemotePlayer(const PlayerDto& data);
    Entity* CreateGroundItem(const ItemView& itemView, int worldX, int worldY);

    //texture management
    void AddTexture(std::string id, const char* path);
    SDL_Texture* GetTexture(const std::string& id);

    void AddFont(std::string id, std::string path, int fontSize);
    TTF_Font* GetFont(std::string id);

    // Carga el manifest principal.
    // Ese manifest contiene la lista de archivos JSON secundarios.
    void LoadManifest(const std::string& manifestPath);

    // Carga texturas desde un JSON específico.
    // Ejemplo: players.json, enemies.json, effects.json.
    void LoadTexturesFromJson(const std::string& jsonPath);


    SpriteSheetConfig bodyConfigForRace(const std::string& race) const;

    void applyGhostAppearance(Entity& entity);

    std::string ghostTextureId() const;

    void applyPlayerAppearance(Entity& entity, const PlayerViewState& playerState);

    void applyRemotePlayerAppearance(Entity& entity, const PlayerDto& dto);


private:

    Manager* manager;
    Queue<std::shared_ptr<const Message>>& sendQueue;
    TextureManager& textureManager;
    std::map<std::string, SDL_Texture*> textures;
    std::map<std::string, TTF_Font*> fonts;
    std::map<std::string, SpriteSheetConfig> bodyConfigs;

    std::string textureForNPC(NpcType type);
    std::string bodyTextureForRace(const std::string& race) const;
    std::string headTextureForRace(const std::string& race) const;
    void LoadBodiesFromJson(const std::string& path);

};

#endif //PRUEBA_SDL_ASSETMANAGER_H
