#ifndef PRUEBA_SDL_GAME_H
#define PRUEBA_SDL_GAME_H

// ── Mensajes del servidor ─────────────────────────────────────────────────────
#include "../common/network/messages/server/player/playerStatsMessage.h"
#include "../common/network/messages/server/player/playerDiedMessage.h"
#include "../common/network/messages/server/player/EntityMoveMessage.h"
#include "../common/network/messages/server/world/entitySpawnMessage.h"
#include "../common/network/messages/server/world/entityDespawnMessage.h"
#include "../common/network/messages/server/npc/npcListMessage.h"
#include "../common/network/messages/server/inventory/inventoryUpdateMessage.h"

// ── Tipos comunes ─────────────────────────────────────────────────────────────
#include "../common/npcType.h"

// ── SDL ───────────────────────────────────────────────────────────────────────
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

// ── ECS y cliente ─────────────────────────────────────────────────────────────
#include "sdl/ECS/ECS.h"
#include "sdl/AssetManager.h"
#include "sdl/state/PlayerViewState.h"
#include "sdl/state/InventoryViewState.h"
#include "sdl/state/EquipmentViewState.h"
#include "sdl/state/PlayerViewStateMapper.h"
#include "sdl/items/ItemCatalog.h"
#include "sdl/Map.h"
#include "sdl/AttackSystem.h"
#include "sdl/ECS/KeyboardController.h"
#include "sdl/world/ClientGameWorld.h"

// ── Std ───────────────────────────────────────────────────────────────────────
#include <vector>
#include <memory>
#include <unordered_map>

#include "common/queue.h"

class Game {
public:
    Game();
    ~Game() = default;

    void init(
        SDL_Window* window,
        SDL_Renderer* renderer,
        Queue<std::shared_ptr<const Message>>& sendQueue,
        Queue<std::shared_ptr<const Message>>& receiveQueue,
        const PlayerDto& playerDto
    );

    void handleEvents();
    void update();
    void render();
    void clean();
    bool running() const;
    void renderHUD();

private:
    bool isRunning = false;

    SDL_Window*   window   = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Event     event{};
    Manager       manager;

    std::unique_ptr<TextureManager>  textureManager;
    std::unique_ptr<AssetManager>    assets;

    // Mundo visual: jugador local + jugadores remotos.
    std::unique_ptr<ClientGameWorld> clientWorld;

    Queue<std::shared_ptr<const Message>>* sendQueue    = nullptr;
    Queue<std::shared_ptr<const Message>>* receiveQueue = nullptr;

    Map*    map    = nullptr;
    Entity* player = nullptr;
    Entity* label  = nullptr;
    SDL_Rect camera{0, 0, 0, 0};

    PlayerDto playerDto;

    std::map<uint32_t, Entity*> enemies;

    AttackSystem       attackSystem;
    PlayerViewState    playerState;
    InventoryViewState inventoryState;
    EquipmentViewState equipmentState;
    ItemCatalog        itemCatalog;

    // ── Movimiento interpolado ────────────────────────────────────────────────
    float  targetX         = 0.0f;
    float  targetY         = 0.0f;
    bool   isMoving        = false;
    Uint32 moveAnimStartMs = 0;
    static constexpr Uint32 MOVE_ANIM_DURATION_MS = 50;

    // ── Estado de muerte/resurrección ─────────────────────────────────────────
    bool localGhostStateApplied      = false;
    bool hasReceivedValidPlayerStats = false;

    // ── HUD ───────────────────────────────────────────────────────────────────
    std::string statusMessage;
    Uint32      statusMessageTimer            = 0;
    static constexpr Uint32 STATUS_MESSAGE_DURATION_MS = 2500;
    TTF_Font*   statusFont                    = nullptr;

    // ── Cache de texturas de texto (evita TTF_Render cada frame) ─────────────
    struct CachedText {
        std::string  text;
        SDL_Color    color{0, 0, 0, 0};
        TTF_Font*    font    = nullptr;
        SDL_Texture* texture = nullptr;
        int w = 0;
        int h = 0;
    };
    std::unordered_map<std::string, CachedText> textCache;

    SDL_Texture* getOrCreateTextTexture(
        const std::string& key,
        const std::string& text,
        TTF_Font* font,
        SDL_Color color,
        int& outW,
        int& outH
    );
    void clearTextCache();
    bool sameColor(SDL_Color a, SDL_Color b) const;

    // ── Cheats ────────────────────────────────────────────────────────────────
    bool cheatGodMode = false;  // Ctrl+H: vida y mana al máximo
    bool cheatInfMana = false;  // Ctrl+M: mana siempre al máximo
    void handleCheatKeys();

    // ── Helpers generales ─────────────────────────────────────────────────────
    void showStatusMessage(const std::string& msg);

    void loadAssets();
    void loadInitialInventoryForCurrentClass();

    int  getInventorySlotIndexAt(int mouseX, int mouseY) const;
    void handleInventorySlotClick(int slotIndex);
    void equipItemFromInventory(int slotIndex);

    int  getEquipmentSlotIndexAt(int mouseX, int mouseY) const;
    void handleEquipmentSlotClick(int equipmentSlotIndex);
    bool addItemToFirstFreeInventorySlot(const ItemView& item);
    void consumePotion(int slotIndex);

    std::string       visualTextureForCurrentRace(const ItemView& item) const;
    void              renderEquippedArmor();
    void              renderEquippedWeapon();
    void              renderEquippedShield();
    void              refreshPlayerBodySprite();
    SpriteSheetConfig armorSpriteConfigForCurrentRace() const;
    SDL_Point         visualOffsetForCurrentRace(const ItemView& item) const;
    void              refreshPlayerEquipmentVisuals();
    void              renderEnemyHealthBars();

    // ── Muerte / fantasma / resurrección ──────────────────────────────────────
    bool isLocalPlayerDead() const;
    void applyLocalPlayerGhostState();
    void reviveLocalPlayer(int newHp);

    // ── Procesamiento de mensajes del servidor ────────────────────────────────
    void processServerMessage(const Message& msg);
    void handleEntityMove(const EntityMoveMessage& msg);
    void handlePlayerDied(const PlayerDiedMessage& msg);
    void handlePlayerStats(const PlayerStatsMessage& msg);
    // NOTA: usa EntitySpawnMessage de integracion (id, NpcType, x, y)
    void handleEntitySpawn(const EntitySpawnMessage& msg);
    void handleEntityDespawn(const EntityDespawnMessage& msg);
    void handleNpcList(const NpcListMessage& msg);
    void handleInventoryUpdate(const InventoryUpdateMessage& msg);
    void applyInventoryUpdate(const InventoryUpdateMessage& msg);
};

#endif // PRUEBA_SDL_GAME_H