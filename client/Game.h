#ifndef PRUEBA_SDL_GAME_H
#define PRUEBA_SDL_GAME_H


#include "../common/network/messages/server/player/playerStatsMessage.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include "sdl/ECS/ECS.h"
#include "sdl/AssetManager.h"
#include <vector>
#include "sdl/state/PlayerViewState.h"
#include "sdl/state/InventoryViewState.h"
#include "sdl/state/EquipmentViewState.h"
#include "sdl/state/PlayerViewStateMapper.h"
#include "sdl/items/ItemCatalog.h"
#include "sdl/Map.h"
#include "common/queue.h"
#include "sdl/AttackSystem.h"

class Game {
public:
    Game();
    ~Game() = default;

    void init(
        const char* title,
        int width,
        int height,
        bool fullscreen,
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

    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    SDL_Event event{};
    Manager manager;

    std::unique_ptr<TextureManager> textureManager;
    std::unique_ptr<AssetManager> assets;

    Queue<std::shared_ptr<const Message>>* sendQueue = nullptr;
    Queue<std::shared_ptr<const Message>>* receiveQueue = nullptr;

    Map* map = nullptr;
    Entity* player = nullptr;
    Entity* label = nullptr;
    SDL_Rect camera{0, 0, 0, 0};

    PlayerDto playerDto;

    std::map<uint32_t, Entity*> enemies;

    AttackSystem attackSystem;
    PlayerViewState playerState;
    InventoryViewState inventoryState;
    EquipmentViewState equipmentState;
    ItemCatalog itemCatalog;

    std::string statusMessage;
    Uint32 statusMessageTimer = 0;
    static constexpr Uint32 STATUS_MESSAGE_DURATION_MS = 2500;
    TTF_Font* statusFont = nullptr;  // se asigna en loadAssets()
    bool localGhostStateApplied = false;
    bool hasReceivedValidPlayerStats = false;

    void showStatusMessage(const std::string& msg);

    void loadAssets();
    void loadInitialInventoryForCurrentClass();
    int getInventorySlotIndexAt(int mouseX, int mouseY) const;
    void handleInventorySlotClick(int slotIndex);
    void equipItemFromInventory(int slotIndex);

    int getEquipmentSlotIndexAt(int mouseX, int mouseY) const;
    void handleEquipmentSlotClick(int equipmentSlotIndex);
    bool addItemToFirstFreeInventorySlot(const ItemView& item);
    void consumePotion(int slotIndex);
    std::string visualTextureForCurrentRace(const ItemView& item) const;
    void renderEquippedArmor();
    void renderEquippedWeapon();
    void renderEquippedShield();
    void refreshPlayerBodySprite();
    SpriteSheetConfig armorSpriteConfigForCurrentRace() const;
    SDL_Point visualOffsetForCurrentRace(const ItemView& item) const;
    void refreshPlayerEquipmentVisuals();
    void renderEnemyHealthBars();
    bool isLocalPlayerDead() const;
    void applyLocalPlayerGhostState();
    void reviveLocalPlayer(int newHp);
};

#endif //PRUEBA_SDL_GAME_H
