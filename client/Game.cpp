#include "server/game/items/EquipSlot.h"
#include "Game.h"
#include "sdl/Map.h"
#include "sdl/TextureManager.h"
#include "sdl/ECS/Components.h"
#include "sdl/UpdateContext.h"
#include "sdl/RenderContext.h"
#include "sdl/GroupLabels.h"
#include "sdl/state/PlayerViewStateMapper.h"

#include "common/network/messages/client/combat/attackMessage.h"
#include "common/network/messages/client/combat/resurrectMessage.h"
#include "common/network/messages/client/cheat/cheatMessage.h"
#include "common/network/messages/client/cheat/cheatType.h"
#include "common/network/protocol/serverOpCode.h"

#include <sstream>
#include <iostream>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// init — firma de integracion: crea window/renderer internamente
// ─────────────────────────────────────────────────────────────────────────────
Game::Game() {}

void Game::init(SDL_Window* externalWindow, SDL_Renderer* externalRenderer,
                Queue<std::shared_ptr<const Message>>& sendQ,
                Queue<std::shared_ptr<const Message>>& receiveQ,
                const PlayerDto& pDto) {

    this->window       = externalWindow;
    this->renderer     = externalRenderer;
    this->sendQueue    = &sendQ;
    this->receiveQueue = &receiveQ;
    this->playerDto    = pDto;
    this->playerState  = toPlayerViewState(this->playerDto);

    if (TTF_Init() == -1) {
        std::cerr << "[Game::init] Error TTF_Init: " << TTF_GetError() << std::endl;
        return;
    }

    textureManager = std::make_unique<TextureManager>(renderer);
    assets         = std::make_unique<AssetManager>(&manager, *sendQueue, *textureManager);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    isRunning = true;

    loadAssets();

    try {
        itemCatalog.loadFromJson("assets/items/items.json");
        loadInitialInventoryForCurrentClass();
    } catch (const std::exception& e) {
        std::cerr << "[Game::init] Error cargando ítems: " << e.what() << std::endl;
        isRunning = false;
        return;
    }

    player = assets->CreatePlayer(playerDto);

    // ClientGameWorld: gestiona jugador local y jugadores remotos.
    clientWorld = std::make_unique<ClientGameWorld>(
        static_cast<uint32_t>(playerDto.playerID),
        player,
        *assets
    );

    refreshPlayerEquipmentVisuals();

    map = new Map(manager, *assets, "terrain", 3, 32);
    map->LoadMap("assets/sprites/MapAssets/mapa.argmap");

    // ── NO hay fakeEnemy. Los NPCs llegan vía MSG_NPC_LIST del servidor. ─────
}

// ─────────────────────────────────────────────────────────────────────────────
// handleEvents
// ─────────────────────────────────────────────────────────────────────────────
void Game::handleEvents() {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            isRunning = false;
        }

        // Cheats: solo en primera pulsación (no repeat)
        if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
            handleCheatKeys();
        }

        // Bloquear inputs de repeat para el movimiento
        if (event.type == SDL_KEYDOWN && event.key.repeat != 0) {
            event.type = SDL_USEREVENT;
        }

        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
            const int mouseX = event.button.x;
            const int mouseY = event.button.y;

            const int equipmentSlot = getEquipmentSlotIndexAt(mouseX, mouseY);
            if (equipmentSlot != -1) {
                handleEquipmentSlotClick(equipmentSlot);
                return;
            }

            const int inventorySlot = getInventorySlotIndexAt(mouseX, mouseY);
            if (inventorySlot != -1) {
                handleInventorySlotClick(inventorySlot);
                return;
            }

            // Bloquear ataque si el jugador está muerto
            if (isLocalPlayerDead()) {
                std::cout << "[PLAYER] No puede atacar: está muerto/fantasma." << std::endl;
                return;
            }

            const ItemView* equippedWeapon = nullptr;
            if (equipmentState.weapon.has_value()) {
                equippedWeapon = &equipmentState.weapon.value();
            }
            attackSystem.handleMouseClick(mouseX, mouseY, camera, enemies, sendQueue, player, equippedWeapon);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// update
// ─────────────────────────────────────────────────────────────────────────────
void Game::update() {
    // Procesar todos los mensajes del servidor pendientes
    std::shared_ptr<const Message> msg;
    while (receiveQueue->try_pop(msg)) {
        processServerMessage(*msg);
    }

    // Aplicar cheats DESPUÉS de procesar mensajes (para no ser sobreescritos)
    if (cheatGodMode) {
        playerState.hp   = playerState.maxHp;
        playerState.mana = playerState.maxMana;
    } else if (cheatInfMana) {
        playerState.mana = playerState.maxMana;
    }

    UpdateContext updateContext{
        SDL_GetKeyboardState(nullptr),
        sendQueue,
        camera
    };
    manager.refresh();
    manager.update(updateContext);

    // Actualizar sistema de ataque
    attackSystem.update();
    attackSystem.updateRespawns(enemies);

    // Lógica de muerte local
    if (isLocalPlayerDead()) {
        applyLocalPlayerGhostState();
    } else {
        attackSystem.updateEnemyChase(enemies, player, playerState.hp, sendQueue);
        if (hasReceivedValidPlayerStats && playerState.hp <= 0) {
            applyLocalPlayerGhostState();
        }
    }

    // Interpolación de movimiento del jugador local
    if (isMoving) {
        auto& transform = player->getComponent<TransformComponent>();
        float dx   = targetX - transform.position.x;
        float dy   = targetY - transform.position.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        Uint32 elapsed   = SDL_GetTicks() - moveAnimStartMs;
        Uint32 remaining = (elapsed >= MOVE_ANIM_DURATION_MS) ? 0 : MOVE_ANIM_DURATION_MS - elapsed;

        if (remaining == 0 || dist <= 1.0f) {
            transform.position.x = targetX;
            transform.position.y = targetY;
            isMoving = false;
        } else {
            float frames = static_cast<float>(remaining) / 16.0f;
            float step   = dist / frames;
            float factor = step / dist;
            transform.position.x += dx * factor;
            transform.position.y += dy * factor;
        }

    }

    // Cámara centrada en el jugador
    Vector2D playerPos = player->getComponent<TransformComponent>().position;
    camera.x = static_cast<int>(playerPos.x) - 450;
    camera.y = static_cast<int>(playerPos.y) - 343;
    if (camera.x < 0)              camera.x = 0;
    if (camera.y < 0)              camera.y = 0;
    if (camera.x > 20 * 96 - 900) camera.x = 20 * 96 - 900;
    if (camera.y > 15 * 96 - 687) camera.y = 15 * 96 - 687;
}

// ─────────────────────────────────────────────────────────────────────────────
// render
// ─────────────────────────────────────────────────────────────────────────────
void Game::render() {
    SDL_RenderClear(renderer);

    SDL_Rect mapArea = {0, 33, 900, 687};
    SDL_RenderSetClipRect(renderer, &mapArea);

    RenderContext renderContext{renderer, camera, mapArea, *textureManager, 133};

    // Mapa
    for (auto& t : manager.getGroup(groupMap)) {
        t->draw(renderContext);
    }

    // Escudo/arma detrás del personaje según dirección
    auto& sprite = player->getComponent<SpriteComponent>();
    bool weaponBehind = (sprite.getAnimationIndex() == 1 || sprite.getAnimationIndex() == 2);
    bool shieldBehind = (sprite.getAnimationIndex() == 1 || sprite.getAnimationIndex() == 3);

    if (shieldBehind)  renderEquippedShield();
    if (weaponBehind)  renderEquippedWeapon();

    // Jugador local y jugadores remotos
    for (auto& p : manager.getGroup(groupPlayers)) {
        p->draw(renderContext);
    }

    if (!shieldBehind) renderEquippedShield();
    if (!weaponBehind) renderEquippedWeapon();

    // Enemigos (filtra muertos)
    for (const auto& [enemyId, enemy] : enemies) {
        if (enemy == nullptr)                    continue;
        if (attackSystem.isEnemyDead(enemyId))   continue;
        enemy->draw(renderContext);
    }
    renderEnemyHealthBars();

    attackSystem.render(renderer, *assets, camera);
    SDL_RenderSetClipRect(renderer, nullptr);

    // Mensaje de estado temporal con fade-out
    if (!statusMessage.empty()) {
        const Uint32 elapsed = SDL_GetTicks() - statusMessageTimer;
        if (elapsed < STATUS_MESSAGE_DURATION_MS) {
            Uint8 alpha = 255;
            const Uint32 fadeStart = STATUS_MESSAGE_DURATION_MS - 500;
            if (elapsed > fadeStart) {
                alpha = static_cast<Uint8>(
                    255 * (1.0f - static_cast<float>(elapsed - fadeStart) / 500.0f)
                );
            }
            if (statusFont) {
                int tw = 0, th = 0;
                TTF_SizeText(statusFont, statusMessage.c_str(), &tw, &th);
                SDL_Color red = {255, 50, 50, alpha};
                SDL_Surface* surf = TTF_RenderText_Blended(statusFont, statusMessage.c_str(), red);
                if (surf) {
                    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
                    SDL_SetTextureAlphaMod(tex, alpha);
                    SDL_Rect dest = {(900 - tw) / 2, 350, tw, th};
                    SDL_RenderCopy(renderer, tex, nullptr, &dest);
                    SDL_FreeSurface(surf);
                    SDL_DestroyTexture(tex);
                }
            }
        } else {
            statusMessage.clear();
        }
    }

    renderHUD();
    SDL_RenderPresent(renderer);
}

// ─────────────────────────────────────────────────────────────────────────────
// clean
// ─────────────────────────────────────────────────────────────────────────────
void Game::clean() {
    clearTextCache();
    assets.reset();
    textureManager.reset();

    if (map != nullptr) {
        delete map;
        map = nullptr;
    }
    TTF_Quit();
    std::cout << "Game cleaned." << std::endl;
}

bool Game::running() const { return isRunning; }

// ─────────────────────────────────────────────────────────────────────────────
// Cheats
// ─────────────────────────────────────────────────────────────────────────────
void Game::handleCheatKeys() {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    const bool ctrl = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
    if (!ctrl) return;

    switch (event.key.keysym.sym) {

        // Ctrl+H — God mode (vida + mana infinitos)
        case SDLK_h:
            cheatGodMode = !cheatGodMode;
            if (cheatGodMode) cheatInfMana = false;
            showStatusMessage(cheatGodMode ? "[CHEAT] God mode ON" : "[CHEAT] God mode OFF");
            // También notificar al servidor
            sendQueue->try_push(std::make_shared<const CheatMessage>(CheatType::INFINITE_HP));
            break;

        // Ctrl+M — Mana infinito
        case SDLK_m:
            if (!cheatGodMode) {
                cheatInfMana = !cheatInfMana;
                showStatusMessage(cheatInfMana ? "[CHEAT] Mana infinito ON" : "[CHEAT] Mana infinito OFF");
                sendQueue->try_push(std::make_shared<const CheatMessage>(CheatType::INFINITE_MANA));
            }
            break;

        // Ctrl+K — Morir al instante
        case SDLK_k:
            if (!playerState.isDead) {
                showStatusMessage("[CHEAT] Muriendo...");
                cheatGodMode = false;
                cheatInfMana = false;
                playerState.hp = 0;
                applyLocalPlayerGhostState();
            }
            break;

        // Ctrl+L — Subir nivel visual (testeo de HUD)
        case SDLK_l:
            playerState.level = std::min(playerState.level + 1, 99);
            showStatusMessage("[CHEAT] Nivel: " + std::to_string(playerState.level));
            break;

        // Ctrl+R — Solicitar resurrección al servidor
        case SDLK_r:
            if (isLocalPlayerDead()) {
                sendQueue->try_push(std::make_shared<const ResurrectMessage>());
                showStatusMessage("Solicitando resurrección...");
            }
            break;

        default: break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Muerte / fantasma / resurrección
// ─────────────────────────────────────────────────────────────────────────────
bool Game::isLocalPlayerDead() const {
    if (playerState.isDead) return true;
    if (!hasReceivedValidPlayerStats) return false;
    return playerState.hp <= 0;
}

void Game::applyLocalPlayerGhostState() {
    if (localGhostStateApplied) return;
    localGhostStateApplied = true;
    playerState.isDead = true;
    playerState.hp = 0;
    assets->applyGhostAppearance(*player);
    attackSystem.clearEnemyAggro();
    showStatusMessage("Has muerto");
    std::cout << "[PLAYER] Jugador pasó a fantasma." << std::endl;
}

void Game::reviveLocalPlayer(int newHp) {
    playerState.isDead = false;
    localGhostStateApplied = false;
    playerState.hp = newHp;
    showStatusMessage("Has revivido");
    std::cout << "[PLAYER] Revivió. HP=" << playerState.hp << std::endl;
    assets->applyPlayerAppearance(*player, playerState);
    refreshPlayerEquipmentVisuals();
}

// ─────────────────────────────────────────────────────────────────────────────
// Procesamiento de mensajes del servidor
// ─────────────────────────────────────────────────────────────────────────────
void Game::processServerMessage(const Message& msg) {
    switch (static_cast<ServerOpCode>(msg.opCode())) {

        case ServerOpCode::MSG_ENTITY_MOVE:
            handleEntityMove(static_cast<const EntityMoveMessage&>(msg));
            return;

        case ServerOpCode::MSG_PLAYER_DIED:
            handlePlayerDied(static_cast<const PlayerDiedMessage&>(msg));
            return;

        case ServerOpCode::MSG_PLAYER_STATS:
            handlePlayerStats(static_cast<const PlayerStatsMessage&>(msg));
            return;

        case ServerOpCode::MSG_ENTITY_SPAWN:
            handleEntitySpawn(static_cast<const EntitySpawnMessage&>(msg));
            return;

        case ServerOpCode::MSG_ENTITY_DESPAWN:
            handleEntityDespawn(static_cast<const EntityDespawnMessage&>(msg));
            return;

        case ServerOpCode::MSG_NPC_LIST:
            handleNpcList(static_cast<const NpcListMessage&>(msg));
            return;

        case ServerOpCode::MSG_INVENTORY_UPDATE:
            handleInventoryUpdate(static_cast<const InventoryUpdateMessage&>(msg));
            return;

        default:
            std::cout << "[CLIENT] opcode no manejado: 0x"
                      << std::hex << static_cast<int>(msg.opCode())
                      << std::dec << std::endl;
            return;
    }
}

void Game::handleEntityMove(const EntityMoveMessage& moveMsg) {
    const uint32_t entityId = static_cast<uint32_t>(moveMsg.getId());
    const float serverX     = static_cast<float>(moveMsg.getX());
    const float serverY     = static_cast<float>(moveMsg.getY());

    // Si es un enemigo conocido, actualizarlo directamente
    auto enemyIt = enemies.find(entityId);
    if (enemyIt != enemies.end() && enemyIt->second != nullptr) {
        auto& enemyTransform = enemyIt->second->getComponent<TransformComponent>();
        enemyTransform.position.x = serverX;
        enemyTransform.position.y = serverY;
        return;
    }

    // Delegar al ClientGameWorld (jugador local o remoto)
    // NOTA: EntityMoveMessage de integracion no tiene getDirection()/isMoving().
    // Se usa Direction::DOWN y moving=true como fallback neutral.
    // TODO: actualizar EntityMoveMessage para incluir dirección y estado.
    if (clientWorld != nullptr) {
        clientWorld->updatePlayerPosition(entityId, serverX, serverY,
                                          Direction::DOWN, true);
    }

    // Si es el jugador local, actualizar también la interpolación visual
    if (entityId == static_cast<uint32_t>(playerDto.playerID)) {
        targetX        = serverX;
        targetY        = serverY;
        isMoving       = true;
        moveAnimStartMs = SDL_GetTicks();
    }
}

void Game::handlePlayerDied(const PlayerDiedMessage& /*diedMsg*/) {
    // NOTA: PlayerDiedMessage.h de integracion no tiene getId().
    // Hay dos opciones:
    //   (A) Agregar getId() a la .h (recomendado, ver abajo)
    //   (B) Asumir que cualquier MSG_PLAYER_DIED es del jugador local
    //
    // Por ahora usamos opción B hasta agregar getId():
    // const uint32_t deadPlayerId = diedMsg.getId();

    std::cout << "[SERVER] MSG_PLAYER_DIED recibido." << std::endl;
    playerState.hp = 0;
    applyLocalPlayerGhostState();
}

void Game::handlePlayerStats(const PlayerStatsMessage& stats) {
    const int serverHp = stats.getHp();

    if (serverHp > 0) hasReceivedValidPlayerStats = true;

    // Si estaba muerto y el servidor manda HP positivo → resurrección aceptada
    if (playerState.isDead && serverHp > 0) {
        reviveLocalPlayer(serverHp);
    } else if (!playerState.isDead) {
        playerState.hp = serverHp;
    } else {
        playerState.hp = 0;
    }

    playerState.maxHp          = stats.getMaxHp();
    playerState.mana           = stats.getMana();
    playerState.maxMana        = stats.getMaxMana();
    playerState.exp            = stats.getExp();
    playerState.expToNextLevel = stats.getExpLimit();
    playerState.level          = stats.getLevel();
    playerState.gold           = stats.getGold();
}

void Game::handleEntitySpawn(const EntitySpawnMessage& spawnMsg) {
    // EntitySpawnMessage de integracion manda NPC (id, type, x, y) — NO PlayerDto.
    // Esto es distinto al feat. Usamos la versión de integracion.
    std::cout << "[GAME] ENTITY_SPAWN id=" << spawnMsg.getId()
              << " x=" << spawnMsg.getX() << " y=" << spawnMsg.getY() << std::endl;

    NPCData data;
    data.npcID = spawnMsg.getId();
    data.type  = spawnMsg.getType();
    data.x     = spawnMsg.getX();
    data.y     = spawnMsg.getY();

    Entity* e = assets->CreateEnemy(data);
    enemies[spawnMsg.getId()] = e;
}

void Game::handleEntityDespawn(const EntityDespawnMessage& despawnMsg) {
    auto it = enemies.find(despawnMsg.getId());
    if (it != enemies.end()) {
        it->second->destroy();
        enemies.erase(it);
    }
}

void Game::handleNpcList(const NpcListMessage& npcList) {
    std::cout << "[GAME] MSG_NPC_LIST: " << npcList.getNpcs().size() << " NPCs" << std::endl;
    for (const auto& snap : npcList.getNpcs()) {
        NPCData data;
        data.npcID = snap.id;
        data.type  = snap.type;
        data.x     = snap.x;
        data.y     = snap.y;
        Entity* e  = assets->CreateEnemy(data);
        enemies[snap.id] = e;
    }
}

void Game::handleInventoryUpdate(const InventoryUpdateMessage& inventoryMsg) {
    applyInventoryUpdate(inventoryMsg);
    std::cout << "[CLIENT] MSG_INVENTORY_UPDATE: " << inventoryMsg.getItems().size() << " ítems" << std::endl;
}

// ─────────────────────────────────────────────────────────────────────────────
// HUD con cache de texto
// ─────────────────────────────────────────────────────────────────────────────
void Game::renderHUD() {
    SDL_Texture* texTop    = assets->GetTexture("hud_top");
    SDL_Texture* texLogo   = assets->GetTexture("hud_logo");
    SDL_Texture* texChat   = assets->GetTexture("hud_chat");
    SDL_Texture* texPjInfo = assets->GetTexture("hud_pj_info");
    SDL_Texture* texInv    = assets->GetTexture("hud_inv");
    SDL_Texture* texStats  = assets->GetTexture("hud_stats");

    SDL_Rect rTop    = {0,   0,   1280, 33};
    SDL_Rect rLogo   = {5,   0,   177,  33};
    SDL_Rect rChat   = {0,   33,  900,  100};
    SDL_Rect rPjInfo = {900, 33,  380,  100};
    SDL_Rect rInv    = {900, 133, 380, 442};
    SDL_Rect rStats  = {900, 575, 380, 145};

    if (texTop)    SDL_RenderCopy(renderer, texTop,    nullptr, &rTop);
    if (texLogo)   SDL_RenderCopy(renderer, texLogo,   nullptr, &rLogo);
    if (texChat)   SDL_RenderCopy(renderer, texChat,   nullptr, &rChat);
    if (texPjInfo) SDL_RenderCopy(renderer, texPjInfo, nullptr, &rPjInfo);
    if (texInv)    SDL_RenderCopy(renderer, texInv,    nullptr, &rInv);
    if (texStats)  SDL_RenderCopy(renderer, texStats,  nullptr, &rStats);

    SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
    SDL_RenderDrawLine(renderer, 0,   33,  1280, 33);
    SDL_RenderDrawLine(renderer, 0,   34,  1280, 34);
    SDL_RenderDrawLine(renderer, 0,   133, 900,  133);
    SDL_RenderDrawLine(renderer, 0,   134, 900,  134);
    SDL_RenderDrawLine(renderer, 900, 33,  900,  720);
    SDL_RenderDrawLine(renderer, 901, 33,  901,  720);
    SDL_RenderDrawLine(renderer, 900, 133, 1280, 133);
    SDL_RenderDrawLine(renderer, 900, 134, 1280, 134);
    SDL_RenderDrawLine(renderer, 900, 575, 1280, 575);
    SDL_RenderDrawLine(renderer, 900, 576, 1280, 576);

    TTF_Font* fontBold    = assets->GetFont("ao_bold");
    TTF_Font* fontRegular = assets->GetFont("ao_regular");
    if (!fontBold || !fontRegular) return;

    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color yellow = {255, 215, 0,   255};

    // Helpers con cache de texto
    auto drawTextCentered = [&](const std::string& key, const std::string& text,
                                TTF_Font* font, int x, int y, int w, int h, SDL_Color color) {
        int tw = 0, th = 0;
        SDL_Texture* tex = getOrCreateTextTexture(key, text, font, color, tw, th);
        if (!tex) return;
        SDL_Rect dest = {x + (w - tw) / 2, y + (h - th) / 2, tw, th};
        SDL_RenderCopy(renderer, tex, nullptr, &dest);
    };

    auto drawTextAt = [&](const std::string& key, const std::string& text,
                          TTF_Font* font, int x, int y, SDL_Color color) {
        int tw = 0, th = 0;
        SDL_Texture* tex = getOrCreateTextTexture(key, text, font, color, tw, th);
        if (!tex) return;
        SDL_Rect dest = {x, y, tw, th};
        SDL_RenderCopy(renderer, tex, nullptr, &dest);
    };

    // Caja de nivel
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_Rect nivelBox = {908, 38, 50, 50};
    SDL_RenderFillRect(renderer, &nivelBox);
    SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
    SDL_RenderDrawRect(renderer, &nivelBox);
    drawTextCentered("hud_level", std::to_string(playerState.level), fontBold, 908, 38, 50, 50, yellow);

    drawTextAt("hud_name",  playerState.name,                           fontBold,    968, 45, yellow);
    drawTextAt("hud_class", playerClassToString(playerState.playerClass), fontRegular, 968, 75, white);

    // Equipamiento
    drawTextCentered("hud_title_eq", "Equipamiento", fontRegular, 900, 142, 380, 20, white);
    SDL_Texture* texFrame = assets->GetTexture("hud_frame");
    const int eqSlotSize = 58, eqGap = 12, eqY = 168, eqStartX = 956;
    std::string eqLabels[] = {"Arma", "Casco", "Armadura", "Escudo"};

    for (int i = 0; i < 4; i++) {
        SDL_Rect slot = {eqStartX + i * (eqSlotSize + eqGap), eqY, eqSlotSize, eqSlotSize};
        if (texFrame) SDL_RenderCopy(renderer, texFrame, nullptr, &slot);

        const ItemView* item = nullptr;
        if (i == 0 && equipmentState.weapon.has_value())  item = &equipmentState.weapon.value();
        if (i == 1 && equipmentState.helmet.has_value())  item = &equipmentState.helmet.value();
        if (i == 2 && equipmentState.armor.has_value())   item = &equipmentState.armor.value();
        if (i == 3 && equipmentState.shield.has_value())  item = &equipmentState.shield.value();

        if (item) {
            SDL_Texture* itemTex = assets->GetTexture(item->textureId);
            if (itemTex) {
                SDL_Rect src  = {item->iconSrcX, item->iconSrcY, item->iconSrcW, item->iconSrcH};
                SDL_Rect dest = {slot.x + 7, slot.y + 7, slot.w - 14, slot.h - 14};
                SDL_RenderCopy(renderer, itemTex, &src, &dest);
            }
        }
        drawTextCentered("hud_eq_lbl_" + std::to_string(i), eqLabels[i], fontRegular,
                         slot.x - 8, slot.y + eqSlotSize + 4, eqSlotSize + 16, 14, white);
    }

    // Inventario
    drawTextCentered("hud_title_inv", "Inventario", fontRegular, 900, 255, 380, 20, white);
    const int invSlotSize = 44, invGapX = 8, invGapY = 7;
    const int invStartX = 964, invStartY = 280, invCols = 5, invRows = 4;

    for (int fila = 0; fila < invRows; fila++) {
        for (int col = 0; col < invCols; col++) {
            const int index = fila * invCols + col;
            SDL_Rect slot = {invStartX + col * (invSlotSize + invGapX),
                             invStartY + fila * (invSlotSize + invGapY),
                             invSlotSize, invSlotSize};
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderFillRect(renderer, &slot);
            SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
            SDL_RenderDrawRect(renderer, &slot);

            if (index < static_cast<int>(inventoryState.slots.size()) &&
                inventoryState.slots[index].has_value()) {
                const ItemView& item = inventoryState.slots[index].value();
                SDL_Texture* itemTex = assets->GetTexture(item.textureId);
                if (itemTex) {
                    SDL_Rect src  = {item.iconSrcX, item.iconSrcY, item.iconSrcW, item.iconSrcH};
                    SDL_Rect dest = {slot.x + 5, slot.y + 5, slot.w - 10, slot.h - 10};
                    SDL_RenderCopy(renderer, itemTex, &src, &dest);
                }
                if (item.quantity > 1) {
                    drawTextAt("hud_qty_" + std::to_string(index),
                               std::to_string(item.quantity), fontRegular,
                               slot.x + slot.w - 14, slot.y + slot.h - 16, white);
                }
            }
        }
    }

    // Barras
    auto drawBar = [&](const std::string& key, SDL_Texture* tex,
                       int x, int y, int w, int h, int actual, int max, TTF_Font* font) {
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_Rect bg = {x, y, w, h};
        SDL_RenderFillRect(renderer, &bg);
        int fillW = max > 0 ? (w * actual) / max : 0;
        if (tex && fillW > 0) {
            SDL_Rect src  = {0, 0, fillW, h};
            SDL_Rect fill = {x, y, fillW, h};
            SDL_RenderCopy(renderer, tex, &src, &fill);
        }
        drawTextCentered(key, std::to_string(actual) + "/" + std::to_string(max),
                         font, x, y, w, h, white);
    };

    SDL_Texture* texVida = assets->GetTexture("barra_vida");
    SDL_Texture* texMana = assets->GetTexture("barra_mana");
    SDL_Texture* texExp  = assets->GetTexture("barra_exp");

    drawTextCentered("hud_lbl_exp", "Experiencia", fontRegular, 910, 100, 350, 16, white);
    drawBar("hud_exp", texExp, 910, 118, 350, 16, playerState.exp, playerState.expToNextLevel, fontRegular);

    drawTextAt("hud_gold", "Oro: " + std::to_string(playerState.gold), fontRegular, 915, 585, yellow);

    const int statsX = 950, statsBarW = 260, statsBarH = 18;
    drawTextCentered("hud_lbl_hp",   "Vida", fontRegular, statsX, 610, statsBarW, 18, white);
    drawBar("hud_hp",   texVida, statsX, 630, statsBarW, statsBarH, playerState.hp,   playerState.maxHp,   fontRegular);
    drawTextCentered("hud_lbl_mana", "Mana", fontRegular, statsX, 665, statsBarW, 18, white);
    drawBar("hud_mana", texMana, statsX, 685, statsBarW, statsBarH, playerState.mana, playerState.maxMana, fontRegular);
}

// ─────────────────────────────────────────────────────────────────────────────
// Cache de texto
// ─────────────────────────────────────────────────────────────────────────────
bool Game::sameColor(SDL_Color a, SDL_Color b) const {
    return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a;
}

SDL_Texture* Game::getOrCreateTextTexture(const std::string& key, const std::string& text,
                                          TTF_Font* font, SDL_Color color, int& outW, int& outH) {
    auto it = textCache.find(key);
    if (it != textCache.end()) {
        CachedText& cached = it->second;
        if (cached.texture && cached.text == text && cached.font == font && sameColor(cached.color, color)) {
            outW = cached.w;
            outH = cached.h;
            return cached.texture;
        }
        if (cached.texture) {
            SDL_DestroyTexture(cached.texture);
            cached.texture = nullptr;
        }
    }
    if (text.empty() || !font) { outW = outH = 0; return nullptr; }
    SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), color);
    if (!surf) { outW = outH = 0; return nullptr; }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (!tex) { SDL_FreeSurface(surf); outW = outH = 0; return nullptr; }

    CachedText cached;
    cached.text = text; cached.color = color; cached.font = font;
    cached.texture = tex; cached.w = surf->w; cached.h = surf->h;
    SDL_FreeSurface(surf);
    textCache[key] = cached;
    outW = cached.w; outH = cached.h;
    return tex;
}

void Game::clearTextCache() {
    for (auto& [key, cached] : textCache) {
        if (cached.texture) { SDL_DestroyTexture(cached.texture); cached.texture = nullptr; }
    }
    textCache.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
// Barras de vida de enemigos
// ─────────────────────────────────────────────────────────────────────────────
void Game::renderEnemyHealthBars() {
    for (const auto& [enemyId, enemy] : enemies) {
        if (!enemy || attackSystem.isEnemyDead(enemyId)) continue;
        auto& transform = enemy->getComponent<TransformComponent>();
        int currentHp = attackSystem.getEnemyHealth(enemyId);
        int maxHp     = attackSystem.getEnemyMaxHealth(enemyId);
        if (maxHp <= 0) continue;

        float ratio = std::max(0.0f, std::min(1.0f, static_cast<float>(currentHp) / maxHp));
        int screenX = static_cast<int>(transform.position.x) - camera.x;
        int screenY = static_cast<int>(transform.position.y) - camera.y + 133;

        const int barW = 50, barH = 6, offsetY = -4;
        SDL_Rect bg = {screenX, screenY + offsetY, barW, barH};
        SDL_Rect hp = {screenX, screenY + offsetY, static_cast<int>(barW * ratio), barH};

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &bg);
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        SDL_RenderFillRect(renderer, &hp);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &bg);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// showStatusMessage
// ─────────────────────────────────────────────────────────────────────────────
void Game::showStatusMessage(const std::string& msg) {
    statusMessage      = msg;
    statusMessageTimer = SDL_GetTicks();
}

// ─────────────────────────────────────────────────────────────────────────────
// loadAssets
// ─────────────────────────────────────────────────────────────────────────────
void Game::loadAssets() {
    assets->LoadManifest("assets/manifest.json");

    assets->AddTexture("hud_top",       "assets/Recursos/BabelUI/static/media/main_top..png");
    assets->AddTexture("hud_chat",      "assets/Recursos/BabelUI/static/media/main_chat..png");
    assets->AddTexture("hud_pj_info",   "assets/Recursos/BabelUI/static/media/main_pj_info..png");
    assets->AddTexture("hud_inv",       "assets/Recursos/BabelUI/static/media/inventory-bg..png");
    assets->AddTexture("hud_stats",     "assets/Recursos/BabelUI/static/media/stats-bg..png");
    assets->AddTexture("hud_logo",      "assets/Recursos/BabelUI/static/media/ao20_logo_med..png");
    assets->AddTexture("hud_pergamino", "assets/Recursos/BabelUI/static/media/titulo_pergamino..png");

    assets->AddFont("ao_bold",    "assets/Recursos/BabelUI/static/media/Alegreya-Sans-AO-Bold..ttf",    18);
    assets->AddFont("ao_regular", "assets/Recursos/BabelUI/static/media/Alegreya-Sans-AO-Regular..ttf", 14);
    assets->AddFont("cardo",      "assets/Recursos/BabelUI/static/media/Cardo-Regular..ttf",            14);

    statusFont = assets->GetFont("ao_bold");
    if (!statusFont)
        statusFont = TTF_OpenFont("assets/Recursos/BabelUI/static/media/Alegreya-Sans-AO-Bold..ttf", 24);
    if (!statusFont)
        statusFont = TTF_OpenFont("assets/sprites/MapAssets/arial.ttf", 24);

    assets->AddTexture("barra_vida", "assets/Recursos/interface/es_barradevida.bmp");
    assets->AddTexture("barra_mana", "assets/Recursos/interface/es_barrademana.bmp");
    assets->AddTexture("barra_exp",  "assets/Recursos/interface/es_barraexperiencia.bmp");
    assets->AddTexture("hud_frame",  "assets/Recursos/BabelUI/static/media/frame..png");

    assets->AddFont("arial",      "assets/sprites/MapAssets/arial.ttf", 16);
    assets->AddTexture("tile_grass", "assets/sprites/MapAssets/tile_grass.png");
    assets->AddTexture("tile_water", "assets/sprites/MapAssets/tile_water.png");
    assets->AddTexture("tile_floor", "assets/sprites/MapAssets/tile_floor.png");
}

// ─────────────────────────────────────────────────────────────────────────────
// loadInitialInventoryForCurrentClass (de integracion, no está en feat)
// ─────────────────────────────────────────────────────────────────────────────
void Game::loadInitialInventoryForCurrentClass() {
    for (auto& slot : inventoryState.slots) slot = std::nullopt;

    switch (playerState.playerClass) {
        case PlayerClass::Cleric:
            inventoryState.slots[0] = itemCatalog.requireById(2);
            inventoryState.slots[1] = itemCatalog.requireById(4);
            inventoryState.slots[2] = itemCatalog.requireById(6);
            inventoryState.slots[3] = itemCatalog.requireById(7);
            break;
        case PlayerClass::Mage:
            inventoryState.slots[0] = itemCatalog.requireById(2);
            inventoryState.slots[1] = itemCatalog.requireById(4);
            inventoryState.slots[2] = itemCatalog.requireById(7);
            inventoryState.slots[3] = itemCatalog.requireById(6);
            break;
        case PlayerClass::Paladin:
            inventoryState.slots[0] = itemCatalog.requireById(1);
            inventoryState.slots[1] = itemCatalog.requireById(3);
            inventoryState.slots[2] = itemCatalog.requireById(5);
            inventoryState.slots[3] = itemCatalog.requireById(6);
            inventoryState.slots[4] = itemCatalog.requireById(4);
            inventoryState.slots[5] = itemCatalog.requireById(7);
            break;
        case PlayerClass::Warrior:
            inventoryState.slots[0] = itemCatalog.requireById(1);
            inventoryState.slots[1] = itemCatalog.requireById(3);
            inventoryState.slots[2] = itemCatalog.requireById(5);
            inventoryState.slots[3] = itemCatalog.requireById(6);
            break;
        default:
            inventoryState.slots[0] = itemCatalog.requireById(1);
            inventoryState.slots[1] = itemCatalog.requireById(6);
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Inventario / Equipamiento
// ─────────────────────────────────────────────────────────────────────────────
int Game::getInventorySlotIndexAt(int mouseX, int mouseY) const {
    const int invSlotSize = 44, invGapX = 8, invGapY = 7;
    const int invStartX = 964, invStartY = 280, invCols = 5, invRows = 4;
    for (int fila = 0; fila < invRows; fila++) {
        for (int col = 0; col < invCols; col++) {
            SDL_Rect slot = {invStartX + col * (invSlotSize + invGapX),
                             invStartY + fila * (invSlotSize + invGapY),
                             invSlotSize, invSlotSize};
            if (mouseX >= slot.x && mouseX < slot.x + slot.w &&
                mouseY >= slot.y && mouseY < slot.y + slot.h)
                return fila * invCols + col;
        }
    }
    return -1;
}

void Game::handleInventorySlotClick(int slotIndex) {
    if (slotIndex < 0 || slotIndex >= static_cast<int>(inventoryState.slots.size())) return;
    if (!inventoryState.slots[slotIndex].has_value()) return;
    const ItemView& item = inventoryState.slots[slotIndex].value();
    if (item.type == ClientItemType::HealthPotion || item.type == ClientItemType::ManaPotion) {
        consumePotion(slotIndex);
        return;
    }
    equipItemFromInventory(slotIndex);
}

void Game::equipItemFromInventory(int slotIndex) {
    if (isLocalPlayerDead()) { showStatusMessage("No puedes usar objetos estando muerto"); return; }
    if (slotIndex < 0 || slotIndex >= static_cast<int>(inventoryState.slots.size())) return;
    if (!inventoryState.slots[slotIndex].has_value()) return;

    ItemView itemToEquip = inventoryState.slots[slotIndex].value();
    std::optional<ItemView>* targetSlot = nullptr;

    if (itemToEquip.type == ClientItemType::MeleeWeapon  ||
        itemToEquip.type == ClientItemType::RangedWeapon ||
        itemToEquip.type == ClientItemType::MagicWeapon) {
        const bool isMagic = itemToEquip.type == ClientItemType::MagicWeapon;
        const bool isMelee = !isMagic;
        const PlayerClass pc = playerState.playerClass;
        if (isMagic && (pc == PlayerClass::Warrior || pc == PlayerClass::Paladin)) {
            showStatusMessage("Tu clase no puede usar armas magicas."); return;
        }
        if (isMelee && (pc == PlayerClass::Mage || pc == PlayerClass::Cleric)) {
            showStatusMessage("Tu clase no puede usar ese tipo de arma."); return;
        }
        targetSlot = &equipmentState.weapon;
    } else if (itemToEquip.type == ClientItemType::Armor)   { targetSlot = &equipmentState.armor;
    } else if (itemToEquip.type == ClientItemType::Helmet)  { targetSlot = &equipmentState.helmet;
    } else if (itemToEquip.type == ClientItemType::Shield)  { targetSlot = &equipmentState.shield;
    } else { return; }

    if (targetSlot->has_value()) inventoryState.slots[slotIndex] = targetSlot->value();
    else                         inventoryState.slots[slotIndex] = std::nullopt;

    *targetSlot = itemToEquip;
    if (itemToEquip.type == ClientItemType::Armor) refreshPlayerBodySprite();
    else                                            refreshPlayerEquipmentVisuals();
}

int Game::getEquipmentSlotIndexAt(int mouseX, int mouseY) const {
    const int eqSlotSize = 58, eqGap = 12, eqY = 168, eqStartX = 956;
    for (int i = 0; i < 4; i++) {
        SDL_Rect slot = {eqStartX + i * (eqSlotSize + eqGap), eqY, eqSlotSize, eqSlotSize};
        if (mouseX >= slot.x && mouseX < slot.x + slot.w &&
            mouseY >= slot.y && mouseY < slot.y + slot.h)
            return i;
    }
    return -1;
}

bool Game::addItemToFirstFreeInventorySlot(const ItemView& item) {
    for (auto& slot : inventoryState.slots) {
        if (!slot.has_value()) { slot = item; return true; }
    }
    return false;
}

void Game::handleEquipmentSlotClick(int idx) {
    if (isLocalPlayerDead()) { showStatusMessage("No puedes usar objetos estando muerto"); return; }
    std::optional<ItemView>* sel = nullptr;
    if (idx == 0) sel = &equipmentState.weapon;
    else if (idx == 1) sel = &equipmentState.helmet;
    else if (idx == 2) sel = &equipmentState.armor;
    else if (idx == 3) sel = &equipmentState.shield;
    else return;
    if (!sel->has_value()) return;
    ItemView item = sel->value();
    if (!addItemToFirstFreeInventorySlot(item)) { showStatusMessage("Inventario lleno."); return; }
    sel->reset();
    if (item.type == ClientItemType::Armor) refreshPlayerBodySprite();
    else                                    refreshPlayerEquipmentVisuals();
}

void Game::consumePotion(int slotIndex) {
    if (isLocalPlayerDead()) { showStatusMessage("No puedes usar objetos estando muerto"); return; }
    if (slotIndex < 0 || slotIndex >= static_cast<int>(inventoryState.slots.size())) return;
    if (!inventoryState.slots[slotIndex].has_value()) return;
    ItemView item = inventoryState.slots[slotIndex].value();
    if (item.type == ClientItemType::HealthPotion) {
        playerState.hp = std::min(playerState.hp + item.healAmount, playerState.maxHp);
    } else if (item.type == ClientItemType::ManaPotion) {
        playerState.mana = std::min(playerState.mana + item.manaAmount, playerState.maxMana);
    } else { return; }
    item.quantity--;
    if (item.quantity <= 0) inventoryState.slots[slotIndex] = std::nullopt;
    else                    inventoryState.slots[slotIndex] = item;
}

void Game::applyInventoryUpdate(const InventoryUpdateMessage& msg) {
    for (auto& slot : inventoryState.slots) slot.reset();
    equipmentState.weapon.reset();
    equipmentState.helmet.reset();
    equipmentState.armor.reset();
    equipmentState.shield.reset();

    const auto& serverItems = msg.getItems();
    for (std::size_t i = 0; i < serverItems.size() && i < inventoryState.slots.size(); ++i) {
        const auto& serverItem = serverItems[i];
        try {
            ItemView view = itemCatalog.requireById(static_cast<int>(serverItem.id));
            
            inventoryState.slots[i] = view;
        } catch (const std::exception& e) {
            std::cerr << "[INV] catalogId desconocido=" << serverItem.id << ": " << e.what() << std::endl;
        }
    }

    const auto& equipped = msg.getEquipped();
    auto applyEquipped = [&](EquipSlot slot, std::optional<ItemView>& target) {
        const auto idx = static_cast<std::size_t>(slot);
        if (idx >= equipped.size()) return;
        const uint32_t equippedId = equipped[idx];
        if (equippedId == 0) { target.reset(); return; }
        auto it = std::find_if(serverItems.begin(), serverItems.end(),
                               [equippedId](const auto& i){ return i.id == equippedId; });
        if (it == serverItems.end()) { target.reset(); return; }
        try {
            ItemView view = itemCatalog.requireById(static_cast<int>(it->id));
            
            target = view;
        } catch (...) { target.reset(); }
    };

    applyEquipped(EquipSlot::HAND,   equipmentState.weapon);
    applyEquipped(EquipSlot::HELMET, equipmentState.helmet);
    applyEquipped(EquipSlot::ARMOR,  equipmentState.armor);
    applyEquipped(EquipSlot::SHIELD, equipmentState.shield);
    refreshPlayerEquipmentVisuals();
}

// ─────────────────────────────────────────────────────────────────────────────
// Visuales de equipo / raza
// ─────────────────────────────────────────────────────────────────────────────
std::string Game::visualTextureForCurrentRace(const ItemView& item) const {
    // Unificado: acepta mayúsculas y minúsculas (servidor usa mayúscula)
    const std::string& r = playerState.race;
    bool isShort = (r == "Dwarf" || r == "dwarf" || r == "Gnome" || r == "gnome");
    if (isShort && !item.visualTextureIdShort.empty()) return item.visualTextureIdShort;
    if (!item.visualTextureIdTall.empty())             return item.visualTextureIdTall;
    return item.visualTextureId;
}

SDL_Point Game::visualOffsetForCurrentRace(const ItemView& item) const {
    const std::string& r = playerState.race;
    bool isShort = (r == "Dwarf" || r == "dwarf" || r == "Gnome" || r == "gnome");
    if (isShort) return {item.visualShortOffsetX, item.visualShortOffsetY};
    return {item.visualTallOffsetX, item.visualTallOffsetY};
}

SpriteSheetConfig Game::armorSpriteConfigForCurrentRace() const {
    if (!equipmentState.armor.has_value())
        return SpriteSheetConfig{27, 47, 2, 0, 0, 0, 0};
    const ItemView& armor = equipmentState.armor.value();
    const std::string& r = playerState.race;
    bool isShort = (r == "Dwarf" || r == "dwarf" || r == "Gnome" || r == "gnome");
    return SpriteSheetConfig{27, 47, 2, 0, 0,
        isShort ? armor.visualShortOffsetX : armor.visualTallOffsetX,
        isShort ? armor.visualShortOffsetY : armor.visualTallOffsetY};
}

void Game::renderEquippedArmor() {
    if (!equipmentState.armor.has_value()) return;
    const ItemView& armor = equipmentState.armor.value();
    SDL_Texture* tex = assets->GetTexture(visualTextureForCurrentRace(armor));
    if (!tex) return;
    auto& sprite = player->getComponent<SpriteComponent>();
    const SDL_Rect& pSrc  = sprite.getSrcRect();
    const SDL_Rect& pDest = sprite.getDestRect();
    SDL_Rect armorSrc  = {pSrc.x - sprite.getStartX(), pSrc.y - sprite.getStartY(), pSrc.w, pSrc.h};
    SDL_Rect armorDest = pDest;
    SDL_Point off = visualOffsetForCurrentRace(armor);
    armorDest.x += off.x * armorSpriteConfigForCurrentRace().scale;
    armorDest.y += off.y * armorSpriteConfigForCurrentRace().scale;
    SDL_RenderCopy(renderer, tex, &armorSrc, &armorDest);
}

void Game::renderEquippedWeapon() {
    if (!equipmentState.weapon.has_value()) return;
    const ItemView& weapon = equipmentState.weapon.value();
    if (weapon.visualTextureId.empty()) return;
    SDL_Texture* tex = assets->GetTexture(weapon.visualTextureId);
    if (!tex) return;
    auto& sprite = player->getComponent<SpriteComponent>();
    const SDL_Rect& pSrc  = sprite.getSrcRect();
    const SDL_Rect& pDest = sprite.getDestRect();
    SDL_Rect wSrc  = {pSrc.x - sprite.getStartX(), pSrc.y - sprite.getStartY(), pSrc.w, pSrc.h};
    SDL_Point off  = visualOffsetForCurrentRace(weapon);
    const SpriteSheetConfig cfg = armorSpriteConfigForCurrentRace();
    SDL_Rect wDest = {pDest.x + off.x, pDest.y + off.y, pSrc.w * cfg.scale, pSrc.h * cfg.scale};
    SDL_RenderCopyEx(renderer, tex, &wSrc, &wDest, 0, nullptr, sprite.spriteFlip);
}

void Game::renderEquippedShield() {
    if (!equipmentState.shield.has_value()) return;
    const ItemView& shield = equipmentState.shield.value();
    if (shield.visualTextureId.empty()) return;
    SDL_Texture* tex = assets->GetTexture(shield.visualTextureId);
    if (!tex) return;
    auto& sprite = player->getComponent<SpriteComponent>();
    const SDL_Rect& pSrc  = sprite.getSrcRect();
    const SDL_Rect& pDest = sprite.getDestRect();
    SDL_Rect sSrc  = {pSrc.x - sprite.getStartX(), pSrc.y - sprite.getStartY(), pSrc.w, pSrc.h};
    SDL_Point off  = visualOffsetForCurrentRace(shield);
    const SpriteSheetConfig cfg = armorSpriteConfigForCurrentRace();
    SDL_Rect sDest = {pDest.x + off.x, pDest.y + off.y, pSrc.w * cfg.scale, pSrc.h * cfg.scale};
    SDL_RenderCopyEx(renderer, tex, &sSrc, &sDest, 0, nullptr, sprite.spriteFlip);
}

void Game::refreshPlayerBodySprite() {
    auto& sprite = player->getComponent<SpriteComponent>();
    if (equipmentState.armor.has_value()) {
        const ItemView& armor = equipmentState.armor.value();
        sprite.setSpriteTextureAndConfig(visualTextureForCurrentRace(armor), armorSpriteConfigForCurrentRace());
        return;
    }
    sprite.setSpriteTextureAndConfig("body_sheet", assets->bodyConfigForRace(playerState.race));
}

void Game::refreshPlayerEquipmentVisuals() {
    auto& sprite = player->getComponent<SpriteComponent>();
    if (equipmentState.helmet.has_value()) {
        const ItemView& h = equipmentState.helmet.value();
        sprite.setHelmetTexture(h.visualTextureId, h.visualOffsetX, h.visualOffsetY,
                                h.iconSrcW, h.iconSrcH,
                                h.visualDownSrcX,  h.visualDownSrcY,
                                h.visualLeftSrcX,  h.visualLeftSrcY,
                                h.visualRightSrcX, h.visualRightSrcY,
                                h.visualUpSrcX,    h.visualUpSrcY);
    } else {
        sprite.clearHelmet();
    }
}