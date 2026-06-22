#ifndef PRUEBA_SDL_GAME_H
#define PRUEBA_SDL_GAME_H

#include <SDL2/SDL.h>

#include <SDL2/SDL_ttf.h>
#include "sdl/ECS/ECS.h"
#include "sdl/AssetManager.h"
#include "sdl/state/PlayerViewState.h"
#include "sdl/state/InventoryViewState.h"
#include "sdl/state/EquipmentViewState.h"
#include "sdl/state/PlayerViewStateMapper.h"
#include "sdl/items/ItemCatalog.h"
#include "sdl/Map.h"
#include "common/queue.h"
#include "sdl/AttackSystem.h"
#include "sdl/AudioManager.h"
#include <memory>
#include "sdl/world/ClientGameWorld.h"

#include "common/network/messages/client/combat/attackMessage.h"
#include "common/network/messages/client/combat/resurrectMessage.h"
#include "common/network/messages/client/inventory/equipItemMessage.h"

#include "common/network/messages/client/cheat/cheatMessage.h"
#include "common/network/messages/server/player/EntityMoveMessage.h"
#include "common/network/messages/server/player/EntityDespawnMessage.h"
#include "common/network/messages/server/player/playerDiedMessage.h"
#include "common/network/messages/server/world/EntitySpawnMessage.h"
#include "common/network/messages/server/inventory/inventoryUpdateMessage.h"
#include "common/network/messages/server/npc/npcHealthMessage.h"
#include "common/network/messages/server/npc/npcMoveMessage.h"
#include "common/network/messages/server/player/levelUpMessage.h"
#include "common/network/messages/server/player/resurrectionStartedMessage.h"
#include "common/network/messages/server/player/playerEquipmentUpdateMessage.h"
#include "common/network/messages/server/player/playerStatsMessage.h"
#include "common/network/messages/server/npc/npcSpawnMessage.h"
#include "common/network/messages/server/player/playerResurrectedMessage.h"
#include "common/network/messages/client/inventory/unequipSlotMessage.h"
#include "MiniChat.h"
#include "common/network/messages/server/chat/chatNotificationMessage.h"
#include "common/network/messages/client/chat/chatMessage.h"
#include "common/network/messages/client/inventory/useItemMessage.h"
#include "common/network/messages/server/clan/clanUpdateMessage.h"
#include "common/network/messages/server/inventory/goldOnGroundMessage.h"
#include "common/network/messages/server/inventory/itemOnGroundMessage.h"
#include "common/network/messages/server/inventory/itemPickedMessage.h"
#include "common/network/messages/server/system/mapChangedMessage.h"
#include "common/network/messages/server/error/errorMessage.h"
#include "common/network/messages/server/combat/combatLogMessage.h"
#include "common/network/messages/server/npc/npcAttackMessage.h"
#include "common/network/messages/server/player/playerAttackVisualMessage.h"
#include "common/network/messages/server/player/playerHealthMessage.h"

#include "common/network/protocol/serverOpCode.h"
#include "sdl/state/PlayerViewStateMapper.h"
#include "sdl/GroupLabels.h"
#include "sdl/pickUpSystem.h"

class Game
{
public:
    Game();
    ~Game() = default;

    void init(SDL_Window *existingWindow, SDL_Renderer *existingRenderer,
              Queue<std::shared_ptr<const Message>> &sendQ,
              Queue<std::shared_ptr<const Message>> &receiveQ,
              const PlayerDto &pDto,
              const std::string &mapPath);

    void handleEvents();
    void update();
    void render();
    void clean();
    bool running() const;
    void renderHUD();
    AudioManager &getAudioManager() { return audioManager; }

private:
    bool isRunning = false;

    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Event event{};
    Manager manager;

    Uint32 resurrectionEndTime = 0;
    bool pendingGhostReapply = false;

    std::unique_ptr<TextureManager> textureManager;
    std::unique_ptr<AssetManager> assets;

    Queue<std::shared_ptr<const Message>> *sendQueue = nullptr;
    Queue<std::shared_ptr<const Message>> *receiveQueue = nullptr;
    // Mundo visual del cliente.
    // Maneja jugador local y jugadores remotos.
    std::unique_ptr<ClientGameWorld> clientWorld;
    Map *map = nullptr;
    Entity *player = nullptr;
    Entity *label = nullptr;
    SDL_Rect camera{0, 0, 0, 0};

    // PERF: cámara del frame anterior — detecta si se movió para evitar
    // 300 recálculos de TileComponent cuando el jugador está quieto.
    SDL_Rect prevCamera{-1, -1, 0, 0};

    PlayerDto playerDto;

    std::map<uint32_t, Entity *> enemies;
    std::map<uint32_t, Entity *> groundItems;
    std::map<uint32_t, Entity *> groundGold;
    AttackSystem attackSystem;
    PickupSystem pickUpSystem;
    PlayerViewState playerState;
    InventoryViewState inventoryState;
    EquipmentViewState equipmentState;
    ItemCatalog itemCatalog;
    MiniChat miniChat;

    std::string statusMessage;
    Uint32 statusMessageTimer = 0;
    static constexpr Uint32 STATUS_MESSAGE_DURATION_MS = 2500;
    TTF_Font *statusFont = nullptr; // se asigna en loadAssets()

    // PERF: textura cacheada del statusMessage.
    // Se crea una vez en showStatusMessage() y se reutiliza con SetTextureAlphaMod.
    SDL_Texture *statusMessageTexture = nullptr;
    int statusMessageTexW = 0;
    int statusMessageTexH = 0;

    bool localGhostStateApplied = false;
    bool hasReceivedValidPlayerStats = false;

    struct CachedText
    {
        // Texto actual asociado a esta textura.
        std::string text;

        // Color actual asociado a esta textura.
        SDL_Color color{0, 0, 0, 0};

        // Font usado para crear la textura.
        // No destruimos la fuente acá, solo guardamos el puntero para comparar.
        TTF_Font *font = nullptr;

        // Textura cacheada.
        SDL_Texture *texture = nullptr;

        // Dimensiones de la textura.
        int w = 0;
        int h = 0;
    };

    // Cache de textos reutilizables.
    // La key representa un lugar lógico del HUD, por ejemplo:
    // "hud_name", "hud_gold", "hud_hp_bar_text".
    std::unordered_map<std::string, CachedText> textCache;

    SDL_Texture *getOrCreateTextTexture(
        const std::string &key,
        const std::string &text,
        TTF_Font *font,
        SDL_Color color,
        int &outW,
        int &outH);

    void clearTextCache();

    bool sameColor(SDL_Color a, SDL_Color b) const;
    void drawEquippedEntity(Entity *entity, RenderContext &context);
    // --- Cheats ---
    AudioManager audioManager;

    bool cheatGodMode = false; // Ctrl+H: vida y mana siempre al maximo
    bool cheatInfMana = false; // Ctrl+M: mana siempre al maximo
    void handleCheatKeys();

    void showStatusMessage(const std::string &msg);

    void loadAssets();
    int getInventorySlotIndexAt(int mouseX, int mouseY) const;
    void handleInventorySlotClick(int slotIndex);
    void equipItemFromInventory(int slotIndex);

    int getEquipmentSlotIndexAt(int mouseX, int mouseY) const;
    void handleEquipmentSlotClick(int equipmentSlotIndex);
    bool addItemToFirstFreeInventorySlot(const ItemView &item);
    void consumePotion(int slotIndex);
    std::string visualTextureForCurrentRace(const ItemView &item) const;
    void renderEquippedArmor();
    void renderEquippedWeapon();
    void renderEquippedShield();
    void refreshPlayerBodySprite();
    SpriteSheetConfig armorSpriteConfigForCurrentRace() const;
    SDL_Point visualOffsetForCurrentRace(const ItemView &item) const;
    void refreshPlayerEquipmentVisuals();
    void renderEnemyHealthBars();
    bool isLocalPlayerDead() const;
    void applyLocalPlayerGhostState(bool showMessage = true);
    void reviveLocalPlayer(int newHp);
    std::optional<ClientEquipmentSlot> toClientEquipmentSlot(int index) const;
    EquipSlot toServerEquipSlot(ClientEquipmentSlot slot) const;

    void handleChatNotification(const ChatNotificationMessage &msg);
    void processServerMessage(const Message &msg);
    void handleEntityMove(const EntityMoveMessage &msg);
    void handlePlayerDied(const PlayerDiedMessage &msg);
    void handlePlayerStats(const PlayerStatsMessage &msg);
    void handleEntitySpawn(const EntitySpawnMessage &msg);
    void handleInventoryUpdate(const InventoryUpdateMessage &msg);
    void applyInventoryUpdate(const InventoryUpdateMessage &msg);
    void handlePlayerEquipmentUpdate(const PlayerEquipmentUpdateMessage &msg);
    void handleLevelUp(const LevelUpMessage &msg);
    void handleNpcSpawn(const NpcSpawnMessage &msg);
    void handleNpcHealth(const NpcHealthMessage &msg);
    void handleNpcMove(const NpcMoveMessage &msg);
    void handlePlayerResurrected(const PlayerResurrectedMessage &msg);
    void handleNpcAttack(const NpcAttackMessage &msg);

    void handleEntityDespawn(const EntityDespawnMessage &msg);
    void clearCurrentScene();
    void handleMapChanged(const MapChangedMessage &msg);

    void handleItemOnGround(const ItemOnGroundMessage &msg);
    void handleGoldOnGround(const GoldOnGroundMessage &msg);
    void handleItemPicked(const ItemPickedMessage &msg);
    void handlePlayerAttackVisual(const PlayerAttackVisualMessage &msg);
    void handlePlayerHeathVisual(const PlayerHealthMessage &msg);
    void  handleClanUpdate(const ClanUpdateMessage &msg);

    struct EnemyMoveInterp
    {
        float startX, startY;
        float targetX, targetY;
        Uint32 startTime;
        Uint32 durationMs;
    };

    std::unordered_map<uint32_t, EnemyMoveInterp> enemyMoveInterp;
    std::unordered_map<uint32_t, NpcType> enemyNpcTypes; // para saber la duración de cada uno

    std::unordered_map<uint32_t, FacingDirection> enemyFacing;
};

#endif // PRUEBA_SDL_GAME_H