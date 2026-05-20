
#include "AssetManager.h"

#include "Game.h"
#include "ECS/Components.h"


AssetManager::AssetManager(Manager* man, Protocol* proto)
    : manager(man), protocol(proto) {}

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
    playerAnims.emplace("Idle", Animation(0, 6, 200));
    playerAnims.emplace("Walk", Animation(0, 6, 100));

    auto& player = manager->addEntity();
    player.addComponent<TransformComponent>(data.xpos, data.ypos, 48, 48, 2);
    player.addComponent<SpriteComponent>("player", true,playerAnims);
    player.addComponent<KeyboardController>(*protocol);
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