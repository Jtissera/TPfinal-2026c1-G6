
#include "AssetManager.h"
#include "GroupLabels.h"
#include "../Game.h"
#include "ECS/Components.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>


AssetManager::AssetManager(
    Manager* manager,
    Queue<std::shared_ptr<const Message>>& sendQueue,
    TextureManager& textureManager
)
    : manager(manager),
      sendQueue(sendQueue),
      textureManager(textureManager) {}

AssetManager::~AssetManager()
{}

void AssetManager::CreateProjectile(Vector2D pos, Vector2D vel, int range, int speed, std::string id)
{
    auto& projectile(manager->addEntity());

    projectile.addComponent<TransformComponent>(pos.x, pos.y, 32, 32, 1);

    std::map<std::string, Animation> projectileAnims;

    SpriteSheetConfig projectileConfig{
        32, // frameWidth
        32, // frameHeight
        1   // scale
    };
    projectile.addComponent<SpriteComponent>(
        *this,
        id,
        false,
        projectileAnims,
        projectileConfig
    );
    projectile.addComponent<ProjectileComponent>(range, speed, vel);
    projectile.addComponent<ColliderComponent>("projectile");
    projectile.addGroup(groupProjectiles);
}
Entity* AssetManager::CreateNpc(const NPCData& data) {
    auto& npc = manager->addEntity();

    npc.addComponent<TransformComponent>(data.x, data.y, 48, 48, 2);

    std::map<std::string, Animation> npcAnims;
    npcAnims.emplace("IdleDown", Animation(0, 1, 200));

    SpriteSheetConfig npcConfig{
        32, // frameWidth
        64, // frameHeight
        2   // scale
    };
    npc.addComponent<SpriteComponent>(
        *this,
        textureForNPC(data.type),
        true,
        npcAnims,
        npcConfig
    );
    npc.addComponent<ColliderComponent>("npc");
    npc.addGroup(groupNPC);

    return &npc;
}

Entity* AssetManager::CreateEnemy(const NPCData& data) {
    SpriteSheetConfig skeletonConfig {
        32,  // frameWidth
        64,  // frameHeight
        2    // scale
    };
    std::map<std::string, Animation> enemyAnims;
    enemyAnims.emplace("Idle",Animation(0,1,200));
    // enemyAnims.emplace("Walk",   Animation(0, 6,  100));
    // enemyAnims.emplace("Attack", Animation(0, 12, 80));
    // enemyAnims.emplace("Hurt",   Animation(0, 4,  100));

    auto& enemy = manager->addEntity();
    enemy.addComponent<TransformComponent>(data.x,data.y);
    enemy.addComponent<SpriteComponent>(*this,textureForNPC(data.type),true,enemyAnims,skeletonConfig);
    enemy.addComponent<ColliderComponent>("enemy");
    enemy.addGroup(groupEnemies);
    return &enemy;
}

Entity* AssetManager::CreatePlayer(const PlayerDto& data) {

    std::map<std::string, Animation> playerAnims;
    // Animaciones quietas.
    // Cada una usa 1 frame de la fila correspondiente.
    playerAnims.emplace("IdleDown",  Animation(0, 1, 150));
    playerAnims.emplace("IdleUp",    Animation(1, 1, 150));
    playerAnims.emplace("IdleRight", Animation(3, 1, 150));
    playerAnims.emplace("IdleLeft",  Animation(2, 1, 150));

    playerAnims.emplace("WalkDown",  Animation(0, 6, 100));
    playerAnims.emplace("WalkUp",    Animation(1, 6, 100));
    playerAnims.emplace("WalkRight", Animation(3, 5, 100));
    playerAnims.emplace("WalkLeft",  Animation(2, 5, 100));

    SpriteSheetConfig armorConfig {
        27,
        47,
        2
    };


    auto& player = manager->addEntity();
    player.addComponent<TransformComponent>(data.xpos, data.ypos);
    player.addComponent<SpriteComponent>(*this,"player", true, playerAnims, armorConfig);
    player.getComponent<SpriteComponent>().setHeadTexture("heads_elf", 2);
    player.addComponent<KeyboardController>(sendQueue);
    player.addComponent<ColliderComponent>("player");
    player.addGroup(groupPlayers);
    return &player;
}

void AssetManager::AddTexture(std::string id, const char* path) {
    if (textures.find(id) != textures.end()) {
        std::cerr << "Textura duplicada, se ignora id: "
                  << id << std::endl;
        return;
    }

    SDL_Texture* texture = textureManager.loadTexture(path);

    if (texture == nullptr) {
        std::cerr << "No se pudo cargar textura id="
                  << id << " path=" << path << std::endl;
        return;
    }
    textures.emplace(id, texture);
}

SDL_Texture* AssetManager::GetTexture(const std::string& id) {
    auto it = textures.find(id);

    if (it == textures.end()) {
        return nullptr;
    }

    return it->second;
}

void AssetManager::AddFont(std::string id, std::string path, int fontSize){

    fonts.emplace(id, TTF_OpenFont(path.c_str(), fontSize));
}
TTF_Font* AssetManager::GetFont(std::string id)
{
    return fonts[id];
}

std::string AssetManager::textureForNPC(NpcType type) {
    switch (type) {
        case NpcType::PRIEST:   return "priest";
        case NpcType::MERCHANT: return "merchant";
        case NpcType::BANKER:   return "banker";
        case NpcType::GOBLIN:   return "goblin";
        case NpcType::SKELETON: return "skeleton";
        case NpcType::ZOMBIE:   return "zombie";
        case NpcType::GUARD:    return "guard";
        default:                return "goblin";
    }
}

void AssetManager::LoadManifest(const std::string &manifestPath) {
    std::ifstream file(manifestPath);

    if (!file.is_open()) {
        std::cerr << "No se pudo abrir el manifest de assets: "
                  << manifestPath << std::endl;
        return;
    }

    nlohmann::json data;
    file >> data;

    if (!data.contains("textureFiles")) {
        std::cerr << "El manifest no contiene la clave 'textureFiles'."
                  << std::endl;
        return;
    }

    for (const auto& textureFile : data["textureFiles"]) {
        std::string path = textureFile.get<std::string>();

        std::cout << "[MANIFEST] cargando: " << path << std::endl;

        LoadTexturesFromJson(path);

        std::cout << "[MANIFEST] terminado: " << path << std::endl;
    }
}

void AssetManager::LoadTexturesFromJson(const std::string& jsonPath) {
    std::ifstream file(jsonPath);

    if (!file.is_open()) {
        std::cerr << "No se pudo abrir el archivo de texturas: "
                  << jsonPath << std::endl;
        return;
    }

    nlohmann::json data;
    file >> data;

    if (!data.contains("textures") || !data["textures"].is_array()) {
        std::cerr << "El archivo no contiene array 'textures': "
                  << jsonPath << std::endl;
        return;
    }

    for (const auto& texture : data["textures"]) {
        if (!texture.contains("id") || !texture.contains("path")) {
            std::cerr << "Textura inválida en "
                      << jsonPath
                      << ": falta id o path"
                      << std::endl;
            continue;
        }

        std::string id = texture.at("id").get<std::string>();
        std::string path = texture.at("path").get<std::string>();

        AddTexture(id, path.c_str());

        std::cout << "Textura cargada: "
                  << id << " -> " << path << std::endl;
    }
}
