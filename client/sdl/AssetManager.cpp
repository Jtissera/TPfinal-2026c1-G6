
#include "AssetManager.h"

#include "../Game.h"
#include "ECS/Components.h"


AssetManager::AssetManager(Manager* man, Queue<std::shared_ptr<const Message>>& sendQueue)
    : manager(man), sendQueue(sendQueue) {}

AssetManager::~AssetManager()
{}

void AssetManager::CreateProjectile(Vector2D pos, Vector2D vel, int range, int speed, std::string id)
{
    auto& projectile(manager->addEntity());
    projectile.addComponent<TransformComponent>(pos.x, pos.y, 32, 32, 1);
    projectile.addComponent<SpriteComponent>(id, false);
    projectile.addComponent<ProjectileComponent>(range, speed, vel);
    projectile.addComponent<ColliderComponent>("projectile");
    projectile.addGroup(Game::groupProjectiles);
}

Entity* AssetManager::CreateNpc(const NPCData& data) {
    auto& npc = manager->addEntity();
    npc.addComponent<TransformComponent>(data.x,data.y,48,48,2);
    npc.addComponent<SpriteComponent>(textureForNPC(data.type),true);
    npc.addComponent<ColliderComponent>("npc");
    npc.addGroup(Game::groupNPC);
    return &npc;

}

Entity* AssetManager::CreateEnemy(const NPCData& data) {
    std::map<std::string, Animation> enemyAnims;
    enemyAnims.emplace("Idle",Animation(0,6,200));
    // enemyAnims.emplace("Walk",   Animation(0, 6,  100));
    // enemyAnims.emplace("Attack", Animation(0, 12, 80));
    // enemyAnims.emplace("Hurt",   Animation(0, 4,  100));

    auto& enemy = manager->addEntity();
    enemy.addComponent<TransformComponent>(data.x,data.y,48,48,2);
    enemy.addComponent<SpriteComponent>(textureForNPC(data.type),true,enemyAnims);
    enemy.addComponent<ColliderComponent>("enemy");
    enemy.addGroup(Game::groupEnemies);
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
    playerAnims.emplace("WalkRight", Animation(3, 6, 100));
    playerAnims.emplace("WalkLeft",  Animation(2, 6, 100));

    SpriteSheetConfig warriorConfig {
        27, // frameWidth: ancho del frame en el spritesheet.
        47, // frameHeight: alto del frame en el spritesheet.
        2   // scale: tamaño visual en pantalla.
    };


    auto& player = manager->addEntity();
    player.addComponent<TransformComponent>(data.xpos, data.ypos);
    player.addComponent<SpriteComponent>("player", true, playerAnims, warriorConfig);
    player.getComponent<SpriteComponent>().setHeadTexture("heads_elf", 0);
    player.addComponent<KeyboardController>(sendQueue);
    player.addComponent<ColliderComponent>("player");
    player.addGroup(Game::groupPlayers);
    return &player;
}

void AssetManager::AddTexture(std::string id, const char* path)
{
    textures.emplace(id, TextureManager::loadTexture(path));
}

SDL_Texture* AssetManager::GetTexture(std::string id)
{
    if (textures.find(id) == textures.end()) {
        std::cerr << "No existe textura con id: " << id << std::endl;
        return nullptr;
    }

    return textures[id];
}

void AssetManager::AddFont(std::string id, std::string path, int fontSize)
{
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