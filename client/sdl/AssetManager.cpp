
#include "AssetManager.h"
#include "GroupLabels.h"
#include "../Game.h"
#include "ECS/Components.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

AssetManager::AssetManager(
    Manager *manager,
    Queue<std::shared_ptr<const Message>> &sendQueue,
    TextureManager &textureManager)
    : manager(manager),
      sendQueue(sendQueue),
      textureManager(textureManager) {}

AssetManager::~AssetManager()
{
}

static std::map<std::string, Animation> animationsFromDefinition(const SpriteDefinition &def)
{
    std::map<std::string, Animation> anims;
    for (const auto &[name, animDef] : def.animations)
    {
        anims.emplace(name, Animation(animDef.row, animDef.frames, animDef.speed));
    }
    return anims;
}

void AssetManager::CreateProjectile(Vector2D pos, Vector2D vel, int range, int speed, std::string id)
{
    auto &projectile(manager->addEntity());

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
        projectileConfig);
    projectile.addComponent<ProjectileComponent>(range, speed, vel);
    projectile.addComponent<ColliderComponent>("projectile");
    projectile.addGroup(groupProjectiles);
}

// Devuelve la config de spritesheet correcta según el tipo de NPC/enemigo.
// 1024x1024, frame 128x128, escala 1: skeleton, zombie, orc, desert_spider.
// 1024x1024, frame 64x64,  escala 2: goblin y variantes de zona.
// 512x512,   frame 64x64,  escala 2: arañas, golems y skeletons de variante.
// Ciudad 256x256, frame 64x64, escala 2: priest, merchant, banker.
static SpriteSheetConfig configForNPC(NpcType type)
{
    switch (type)
    {
    case NpcType::SKELETON:
        return SpriteSheetConfig{100, 98, 1, 0, 0};
    case NpcType::ZOMBIE:
    case NpcType::ORC:
    case NpcType::SPIDER_DESERT:
        return SpriteSheetConfig{128, 128, 1, 0, 0};

    case NpcType::GOBLIN:
    case NpcType::GOBLIN_CAVE:
    case NpcType::GOBLIN_DUNGEON:
    case NpcType::GOBLIN_DESERT:
        return SpriteSheetConfig{64, 64, 2, 0, 0};

    case NpcType::SKELETON_CAVE:
    case NpcType::SKELETON_DUNGEON:
        return SpriteSheetConfig{100, 98, 1, 0, 0};
    case NpcType::SKELETON_DESERT:
    case NpcType::SPIDER_CAVE:
    case NpcType::SPIDER_DUNGEON:
    case NpcType::GOLEM_CAVE:
    case NpcType::GOLEM_DUNGEON:
    case NpcType::GOLEM_DESERT:
        return SpriteSheetConfig{64, 64, 2, 0, 0};

    case NpcType::PRIEST:
    case NpcType::MERCHANT:
    case NpcType::BANKER:
        return SpriteSheetConfig{27, 46, 2, 0, 0};

    default:
        return SpriteSheetConfig{64, 64, 2, 0, 0};
    }
}

static AttackConfig attackConfigForNPC(NpcType type)
{
    switch (type)
    {
    case NpcType::SKELETON:
        return {"skeleton_attack", 103, 104, 4, 4};
    case NpcType::SKELETON_DUNGEON:
        return {"dungeon_skeleton_attack", 124, 94, 2, 4};
    case NpcType::ZOMBIE:
        return {"zombie_attack", 128, 128, 4, 4};
    case NpcType::ORC:
        return {"orc_attack", 128, 128, 4, 4};
    default:
        return {"", 0, 0, 0, 0};  // sin ataque
    }
}

Entity *AssetManager::CreateNpc(const NPCData &data)
{
    auto &npc = manager->addEntity();
    npc.addComponent<NpcTypeComponent>(data.type);

    const float centeredX = data.x + 48.0f;
    const float centeredY = data.y + 86.0f;
    npc.addComponent<TransformComponent>(centeredX, centeredY, 48, 48, 2);

    std::string textureId = textureForNPC(data.type);
    const SpriteDefinition *def = GetSpriteDefinition(textureId);

    std::map<std::string, Animation> npcAnims;
    SpriteSheetConfig npcConfig;

    if (def != nullptr)
    {
        npcConfig = def->config;
        npcAnims = animationsFromDefinition(*def);
        if (npcAnims.count("IdleDown") == 0)
            npcAnims.emplace("IdleDown", Animation(0, 1, 200));
    }
    else
    {
        npcConfig = configForNPC(data.type);
        npcAnims.emplace("IdleDown", Animation(0, 1, 200));
    }

    npc.addComponent<SpriteComponent>(*this,textureId,true,npcAnims,npcConfig);
    npc.addComponent<NameplateComponent>(data.nombre,"",0,"",NameplateType::PassiveNpc,"eagle_lake");
    npc.addComponent<ColliderComponent>("npc");
    npc.addGroup(groupNPC);

    return &npc;
}

Entity *AssetManager::CreateEnemy(const NPCData &data)
{
    std::string bodyTextureId = textureForNPC(data.type);
    const SpriteDefinition *bodyDef = GetSpriteDefinition(bodyTextureId);

    SpriteSheetConfig cfg = (bodyDef != nullptr) ? bodyDef->config : configForNPC(data.type);

    std::map<std::string, Animation> enemyAnims;
    if (bodyDef != nullptr && !bodyDef->animations.empty())
    {
        enemyAnims = animationsFromDefinition(*bodyDef);
    }
    else
    {
        enemyAnims.emplace("IdleDown",  Animation(0, 1, 150));
        enemyAnims.emplace("WalkDown",  Animation(4, 5, 100));
        enemyAnims.emplace("IdleUp",    Animation(0, 1, 150));
        enemyAnims.emplace("WalkUp",    Animation(5, 5, 100));
        enemyAnims.emplace("IdleRight", Animation(3, 1, 150));
        enemyAnims.emplace("WalkRight", Animation(7, 5, 100));
    }

    AttackConfig atkCfg = attackConfigForNPC(data.type);
    const SpriteDefinition *attackDef = atkCfg.textureId.empty()
        ? nullptr
        : GetSpriteDefinition(atkCfg.textureId);

    if (!atkCfg.textureId.empty())
    {
        if (attackDef != nullptr && !attackDef->animations.empty())
        {
            for (const auto &[name, animDef] : attackDef->animations)
            {
                enemyAnims.emplace(name, Animation(animDef.row, animDef.frames, animDef.speed));
            }
        }
        else
        {
            enemyAnims.emplace("AttackDown",  Animation(0, atkCfg.framesPerRow, 150));
            enemyAnims.emplace("AttackUp",    Animation(1, atkCfg.framesPerRow, 150));
            enemyAnims.emplace("AttackLeft",  Animation(2, atkCfg.framesPerRow, 150));
            enemyAnims.emplace("AttackRight", Animation(3, atkCfg.framesPerRow, 150));
        }
    }

    auto &enemy = manager->addEntity();
    enemy.addComponent<TransformComponent>(data.x, data.y);
    enemy.addComponent<SpriteComponent>(*this, bodyTextureId,
                                        true, enemyAnims, cfg);
    enemy.addComponent<NameplateComponent>(data.nombre,"",data.level,"",NameplateType::Enemy,"eagle_lake");
    enemy.addComponent<HealthBarComponent>(data.hp,data.hpMax,80,10,100);
    if (!atkCfg.textureId.empty())
    {
        SpriteSheetConfig attackSheetCfg = (attackDef != nullptr)
            ? attackDef->config
            : SpriteSheetConfig{atkCfg.frameWidth, atkCfg.frameHeight, cfg.scale, 0, 0};

        enemy.getComponent<SpriteComponent>()
             .setAttackTexture(atkCfg.textureId, attackSheetCfg);
    }

    enemy.addComponent<ColliderComponent>("enemy");
    enemy.addGroup(groupEnemies);
    return &enemy;
}

Entity *AssetManager::CreatePlayer(const PlayerDto &data)
{

    std::map<std::string, Animation> playerAnims;
    // Animaciones quietas.
    // Cada una usa 1 frame de la fila correspondiente.
    playerAnims.emplace("IdleDown", Animation(0, 1, 150));
    playerAnims.emplace("IdleUp", Animation(1, 1, 150));
    playerAnims.emplace("IdleRight", Animation(3, 1, 150));
    playerAnims.emplace("IdleLeft", Animation(2, 1, 150));

    playerAnims.emplace("WalkDown", Animation(0, 6, 100));
    playerAnims.emplace("WalkUp", Animation(1, 6, 100));
    playerAnims.emplace("WalkRight", Animation(3, 5, 100));
    playerAnims.emplace("WalkLeft", Animation(2, 5, 100));

    SpriteSheetConfig bodyConfig = bodyConfigForRace(data.raza);

    std::string bodyTextureId = bodyTextureForRace(data.raza);
    std::string headTextureId = headTextureForRace(data.raza);

    auto &player = manager->addEntity();
    player.addComponent<TransformComponent>(data.xpos, data.ypos);
    player.addComponent<SpriteComponent>(*this, bodyTextureId, true, playerAnims, bodyConfig);
    player.getComponent<SpriteComponent>().setHeadTexture(headTextureId, data.headId);
    player.addComponent<NameplateComponent>(data.nombre,data.clase,data.level,"",NameplateType::LocalPlayer,"eagle_lake");
    player.addComponent<EquipmentComponent>(*this, data.raza);
    player.addComponent<KeyboardController>(sendQueue);
    player.addComponent<ColliderComponent>("player");

    player.addGroup(groupPlayers);

    return &player;
}

void AssetManager::AddTexture(std::string id, const char *path)
{
    if (textures.find(id) != textures.end())
    {
        std::cerr << "Textura duplicada, se ignora id: "
                  << id << std::endl;
        return;
    }

    SDL_Texture *texture = textureManager.loadTexture(path);

    if (texture == nullptr)
    {
        std::cerr << "No se pudo cargar textura id="
                  << id << " path=" << path << std::endl;
        return;
    }
    textures.emplace(id, texture);
}

SDL_Texture *AssetManager::GetTexture(const std::string &id)
{
    auto it = textures.find(id);

    if (it == textures.end())
    {
        return nullptr;
    }

    return it->second;
}

void AssetManager::AddFont(std::string id, std::string path, int fontSize)
{

    fonts.emplace(id, TTF_OpenFont(path.c_str(), fontSize));
}
TTF_Font *AssetManager::GetFont(std::string id)
{
    return fonts[id];
}

std::string AssetManager::textureForNPC(NpcType type)
{
    switch (type)
    {
    // Ciudad
    case NpcType::PRIEST:
        return "npc_priest";
    case NpcType::MERCHANT:
        return "npc_shop";
    case NpcType::BANKER:
        return "npc_bank";

    // Zona principal
    case NpcType::GOBLIN:
        return "goblin";
    case NpcType::SKELETON:
        return "skeleton";
    case NpcType::ZOMBIE:
        return "zombie";
    case NpcType::ORC:
        return "orc";

    // Caverna
    case NpcType::GOBLIN_CAVE:
        return "goblin";
    case NpcType::SKELETON_CAVE:
        return "dungeon_skeleton";
    case NpcType::SPIDER_CAVE:
        return "cavern_spider";
    case NpcType::GOLEM_CAVE:
        return "cavern_golem";

    // Mazmorra
    case NpcType::GOBLIN_DUNGEON:
        return "goblin";
    case NpcType::SKELETON_DUNGEON:
        return "dungeon_skeleton";
    case NpcType::SPIDER_DUNGEON:
        return "dungeon_spider";
    case NpcType::GOLEM_DUNGEON:
        return "dungeon_golem";

    // Desierto
    case NpcType::GOBLIN_DESERT:
        return "goblin";
    case NpcType::SKELETON_DESERT:
        return "dungeon_skeleton";
    case NpcType::SPIDER_DESERT:
        return "desert_spider";
    case NpcType::GOLEM_DESERT:
        return "desert_golem";

    default:
        return "goblin";
    }
}

void AssetManager::LoadManifest(const std::string &manifestPath)
{
    std::ifstream file(manifestPath);

    if (!file.is_open())
    {
        std::cerr << "No se pudo abrir el manifest de assets: "
                  << manifestPath << std::endl;
        return;
    }

    nlohmann::json data;
    file >> data;

    if (!data.contains("textureFiles") || !data["textureFiles"].is_array())
    {
        std::cerr << "El manifest no contiene la clave 'textureFiles'."
                  << std::endl;
        return;
    }

    // 1. Cargar texturas.
    for (const auto &textureFile : data["textureFiles"])
    {
        std::string path = textureFile.get<std::string>();

        std::cout << "[MANIFEST] cargando texturas: "
                  << path
                  << std::endl;

        LoadTexturesFromJson(path);

        std::cout << "[MANIFEST] texturas cargadas: "
                  << path
                  << std::endl;
    }

    // 2. Cargar metadata de cuerpos.
    if (data.contains("bodyFiles") && data["bodyFiles"].is_array())
    {
        for (const auto &bodyFile : data["bodyFiles"])
        {
            std::string path = bodyFile.get<std::string>();

            std::cout << "[MANIFEST] cargando bodies: "
                      << path
                      << std::endl;

            LoadBodiesFromJson(path);

            std::cout << "[MANIFEST] bodies cargado: "
                      << path
                      << std::endl;
        }
    }
}

void AssetManager::LoadTexturesFromJson(const std::string &jsonPath)
{
    std::ifstream file(jsonPath);

    if (!file.is_open())
    {
        std::cerr << "No se pudo abrir el archivo de texturas: "
                  << jsonPath << std::endl;
        return;
    }

    nlohmann::json data;
    file >> data;

    if (!data.contains("textures") || !data["textures"].is_array())
    {
        std::cerr << "El archivo no contiene array 'textures': "
                  << jsonPath << std::endl;
        return;
    }

    for (const auto &texture : data["textures"])
    {
        if (!texture.contains("id") || !texture.contains("path"))
        {
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

    // Geometría y animaciones por textura. Si una textura no aparece acá,
    // queda sin SpriteDefinition y el caller debe hacer fallback al switch viejo.
    if (data.contains("_sprite_info") && data["_sprite_info"].is_object())
    {
        for (auto it = data["_sprite_info"].begin(); it != data["_sprite_info"].end(); ++it)
        {
            const std::string &id = it.key();
            const auto &info = it.value();

            if (!info.contains("frame_width") || !info.contains("frame_height"))
            {
                std::cerr << "[SPRITE_INFO] '" << id
                          << "' sin frame_width/frame_height, se ignora." << std::endl;
                continue;
            }

            SpriteDefinition def;
            def.config.frameWidth = info.at("frame_width").get<int>();
            def.config.frameHeight = info.at("frame_height").get<int>();
            def.config.scale = info.value("scale", 1);
            def.config.startX = info.value("start_x", 0);
            def.config.startY = info.value("start_y", 0);
            def.config.renderOffsetX = info.value("render_offset_x", 0);
            def.config.renderOffsetY = info.value("render_offset_y", 0);

            if (info.contains("animations") && info["animations"].is_object())
            {
                for (auto animIt = info["animations"].begin();
                     animIt != info["animations"].end(); ++animIt)
                {
                    const auto &animJson = animIt.value();
                    AnimationDef anim;
                    anim.row = animJson.value("row", 0);
                    anim.frames = animJson.value("frames", 1);
                    anim.speed = animJson.value("speed_ms", 150);
                    def.animations.emplace(animIt.key(), anim);
                }
            }

            spriteDefinitions[id] = def;

            std::cout << "[SPRITE_INFO] cargado: " << id
                      << " frame=(" << def.config.frameWidth << "x"
                      << def.config.frameHeight << ") scale="
                      << def.config.scale << " anims="
                      << def.animations.size() << std::endl;
        }
    }
}

const SpriteDefinition *AssetManager::GetSpriteDefinition(const std::string &id) const
{
    auto it = spriteDefinitions.find(id);
    if (it == spriteDefinitions.end())
        return nullptr;
    return &it->second;
}


void AssetManager::LoadBodiesFromJson(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "No se pudo abrir bodies.json: " << path << std::endl;
        return;
    }
    nlohmann::json data;
    file >> data;

    for (const auto &body : data["bodies"])
    {
        std::string race = body["race"];
        SpriteSheetConfig config{
            body["frameWidth"],
            body["frameHeight"],
            body["scale"],
            body["srcX"],
            body["srcY"]};
        bodyConfigs[race] = config;
    }
}

std::string AssetManager::headTextureForRace(const std::string &race) const
{
    if (race == "Human")
    {
        return "heads_human_man";
    }

    if (race == "Elf")
    {
        return "heads_elf";
    }

    if (race == "Dwarf")
    {
        return "heads_dwarf";
    }

    if (race == "Gnome")
    {
        return "heads_elf";
    }

    return "heads_human_man";
}

SpriteSheetConfig AssetManager::bodyConfigForRace(const std::string &race) const
{
    auto it = bodyConfigs.find(race);
    if (it != bodyConfigs.end())
        return it->second;
    return SpriteSheetConfig{27, 47, 2, 0, 0};
}

std::string AssetManager::bodyTextureForRace([[maybe_unused]] const std::string &race) const
{
    return "body_sheet";
}

std::string AssetManager::ghostTextureId() const
{
    return "ghost";
}

void AssetManager::applyGhostAppearance(Entity &entity)
{
    auto &sprite = entity.getComponent<SpriteComponent>();

    // Configuración del sprite fantasma cargada desde bodies.json.
    SpriteSheetConfig ghostConfig = bodyConfigForRace("ghost");

    std::cout << "[GHOST] applyGhostAppearance texture=" << ghostTextureId() << " frameW=" << ghostConfig.frameWidth << " frameH="
              << ghostConfig.frameHeight << " scale=" << ghostConfig.scale << std::endl;

    // Cambia textura, tamaño de frame, escala, offsets y srcRect.
    sprite.setSpriteTextureAndConfig(ghostTextureId(), ghostConfig);

    sprite.clearHead();
    sprite.clearHelmet();
    sprite.Play("IdleDown");
}

void AssetManager::applyPlayerAppearance(Entity &entity, const PlayerViewState &playerState)
{
    auto &sprite = entity.getComponent<SpriteComponent>();

    SpriteSheetConfig bodyConfig = bodyConfigForRace(playerState.race);
    std::string bodyTextureId = bodyTextureForRace(playerState.race);

    sprite.setBody(bodyTextureId, bodyConfig);

    std::string headTextureId = headTextureForRace(playerState.race);
    sprite.setHeadTexture(headTextureId, 0);
}

Entity *AssetManager::CreateRemotePlayer(const PlayerDto &data)
{
    // Animaciones básicas del jugador remoto.
    std::map<std::string, Animation> playerAnims;

    // Animaciones quietas.
    playerAnims.emplace("IdleDown", Animation(0, 1, 150));
    playerAnims.emplace("IdleUp", Animation(1, 1, 150));
    playerAnims.emplace("IdleRight", Animation(3, 1, 150));
    playerAnims.emplace("IdleLeft", Animation(2, 1, 150));

    // Animaciones de caminata.
    playerAnims.emplace("WalkDown", Animation(0, 6, 100));
    playerAnims.emplace("WalkUp", Animation(1, 6, 100));
    playerAnims.emplace("WalkRight", Animation(3, 5, 100));
    playerAnims.emplace("WalkLeft", Animation(2, 5, 100));

    // Buscamos configuración de sprites según raza.
    SpriteSheetConfig bodyConfig = bodyConfigForRace(data.raza);

    // Buscamos textura de cuerpo y cabeza según raza.
    std::string bodyTextureId = bodyTextureForRace(data.raza);
    std::string headTextureId = headTextureForRace(data.raza);

    auto &remotePlayer = manager->addEntity();

    remotePlayer.addComponent<TransformComponent>(data.xpos, data.ypos);

    // Sprite principal del cuerpo.
    remotePlayer.addComponent<SpriteComponent>(
        *this,
        bodyTextureId,
        true,
        playerAnims,
        bodyConfig);

    // Cabeza del jugador remoto.
    remotePlayer.getComponent<SpriteComponent>().setHeadTexture(headTextureId, data.headId);
    remotePlayer.addComponent<NameplateComponent>(data.nombre,data.clase,data.level,"",NameplateType::RemotePlayer,"eagle_lake");
    remotePlayer.addComponent<HealthBarComponent>(data.hp,data.hpMax,80,10,100);
    remotePlayer.addComponent<EquipmentComponent>(*this, data.raza);
    remotePlayer.addComponent<ColliderComponent>("remote_player");
    remotePlayer.addGroup(groupPlayers);

    std::cout << "[REMOTE_PLAYER] creado id="
              << static_cast<int>(data.playerID)
              << " race=" << data.raza
              << " pos=(" << data.xpos << ", " << data.ypos << ")"
              << std::endl;

    return &remotePlayer;
}

void AssetManager::applyRemotePlayerAppearance(Entity &entity, const PlayerDto &dto)
{
    auto &sprite = entity.getComponent<SpriteComponent>();

    // Restauramos cuerpo normal según raza.
    SpriteSheetConfig bodyConfig = bodyConfigForRace(dto.raza);

    sprite.setSpriteTextureAndConfig(
        bodyTextureForRace(dto.raza),
        bodyConfig);

    // Restauramos cabeza normal.
    sprite.setHeadTexture(
        headTextureForRace(dto.raza),
        dto.headId);

    // Estado inicial razonable. Luego los EntityMoveMessage corrigen dirección.
    sprite.Play("IdleDown");

    std::cout << "[REMOTE_PLAYER] apply normal appearance id="
              << static_cast<int>(dto.playerID)
              << " race="
              << dto.raza
              << " headId="
              << dto.headId
              << std::endl;
}

Entity * AssetManager::CreateGroundItem(const ItemView &itemView, int worldX, int worldY) {

    auto &groundItem = manager->addEntity();

    groundItem.addComponent<TransformComponent>(worldX,worldY,itemView.iconSrcH,itemView.iconSrcW,1);

    std::map<std::string, Animation> itemAnimatation;
    itemAnimatation.emplace("Idle", Animation(0, 1, 1));

    SpriteSheetConfig itemConfig{};
    itemConfig.frameWidth = itemView.iconSrcW;
    itemConfig.frameHeight = itemView.iconSrcH;
    itemConfig.scale = 1;
    itemConfig.startX = itemView.iconSrcX;
    itemConfig.startY = itemView.iconSrcY;

    groundItem.addComponent<SpriteComponent>(*this,itemView.textureId,false,itemAnimatation,itemConfig);
    groundItem.addGroup(groupItems);

    return &groundItem;
}

