#include "Game.h"
#include "sdl/Map.h"
#include "sdl/TextureManager.h"
#include "sdl/ECS/Components.h"
#include <sstream>
#include <iostream>
#include "sdl/UpdateContext.h"
#include "sdl/RenderContext.h"
#include <unordered_set>


#include "common/network/messages/client/inventory/unequipSlotMessage.h"
#include "common/network/protocol/serverOpCode.h"

#include "sdl/state/PlayerViewStateMapper.h"
#include "sdl/GroupLabels.h"


Game::Game() {
}

void Game::init(SDL_Window* existingWindow,
                SDL_Renderer* existingRenderer,
                Queue<std::shared_ptr<const Message>>& sendQ,
                Queue<std::shared_ptr<const Message>>& receiveQ,
                const PlayerDto& pDto) {

    this->sendQueue = &sendQ;
    this->receiveQueue = &receiveQ;
    this->playerDto = pDto;
    this->playerState = toPlayerViewState(this->playerDto);

    this->window = existingWindow;
    this->renderer = existingRenderer;

    if (this->window == nullptr || this->renderer == nullptr) {
        std::cerr << "[Game::init] window o renderer inválidos" << std::endl;
        isRunning = false;
        return;
    }

    textureManager = std::make_unique<TextureManager>(renderer);
    assets = std::make_unique<AssetManager>(&manager, *sendQueue, *textureManager);

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    isRunning = true;

    loadAssets();

    try {
        itemCatalog.loadFromJson("assets/items/items.json");
    } catch (const std::exception& e) {
        std::cerr << "Error cargando catálogo de ítems: " << e.what() << std::endl;
        isRunning = false;
        return;
    }

    player = assets->CreatePlayer(playerDto);

    clientWorld = std::make_unique<ClientGameWorld>(
        static_cast<uint32_t>(playerDto.playerID),
        player,
        *assets
    );

    refreshPlayerEquipmentVisuals();

    map = new Map(manager, *assets, "terrain", 3, 32);
    map->LoadMap("assets/sprites/MapAssets/mapa.argmap");

    NPCData fakeEnemy;
    fakeEnemy.npcID = 99;
    fakeEnemy.type = NpcType::SKELETON;
    fakeEnemy.x = 600;
    fakeEnemy.y = 400;

    Entity* e = assets->CreateEnemy(fakeEnemy);
    enemies[fakeEnemy.npcID] = e;
}

void Game::handleEvents() {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT){
            isRunning = false;
        }

        if (event.type == SDL_KEYDOWN && event.key.repeat != 0){
            event.type = SDL_USEREVENT;
        }
        if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
            handleCheatKeys();
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
            const ItemView* equippedWeapon = nullptr;
            if (equipmentState.weapon.has_value()) {
                equippedWeapon = &equipmentState.weapon.value();
            }

            if (isLocalPlayerDead()) {
                std::cout << "[PLAYER] No puede atacar porque está muerto/fantasma." << std::endl;
                return;
            }

            attackSystem.handleMouseClick(mouseX, mouseY, camera, enemies, sendQueue,player,equippedWeapon);
        }
    }

}

void Game::update() {
    std::shared_ptr<const Message> msg;
    while (receiveQueue->try_pop(msg)) {
        processServerMessage(*msg);
    }


    Vector2D playerPos = player->getComponent<TransformComponent>().position;
    camera.x = static_cast<int>(playerPos.x) - 450;
    camera.y = static_cast<int>(playerPos.y) - 343;
    if (camera.x < 0) camera.x = 0;
    if (camera.y < 0) camera.y = 0;
    if (camera.x > 20 * 96 - 900) camera.x = 20 * 96 - 900;
    if (camera.y > 15 * 96 - 687) camera.y = 15 * 96 - 687;

    UpdateContext updateContext{
        SDL_GetKeyboardState(nullptr),
        sendQueue,
        camera
    };
    manager.refresh();
    manager.update(updateContext);

    attackSystem.update();
    attackSystem.updateRespawns(enemies);
    if (isLocalPlayerDead()) {
        applyLocalPlayerGhostState();
    } else {
        attackSystem.updateEnemyChase(enemies, player, playerState.hp,sendQueue);

        if (hasReceivedValidPlayerStats && playerState.hp <= 0) {
            applyLocalPlayerGhostState();
        }
    }
}


void Game::render() {

    // Limpia la pantalla antes de dibujar el nuevo frame.
    SDL_RenderClear(renderer);

    // Limita el dibujado al área del mapa, para que no invada el HUD.
    SDL_Rect mapArea = {0, 33, 900, 687};
    SDL_RenderSetClipRect(renderer, &mapArea);

    RenderContext renderContext{
        renderer,
        camera,
        mapArea,
        *textureManager,
        133
    };
    // Dibuja el mapa.
    for (auto& t : manager.getGroup(groupMap)) {
        t->draw(renderContext);
    }


    // Escudo detrás del personaje cuando mira arriba (animIndex 3)
    // o derecha (animIndex 2).
    auto& sprite = player->getComponent<SpriteComponent>();
    bool weaponBehind = (sprite.getAnimationIndex() == 1 || sprite.getAnimationIndex() == 2);
    bool shieldBehind = (sprite.getAnimationIndex() == 1 || sprite.getAnimationIndex() == 3);

    if (shieldBehind) {
        renderEquippedShield();
    }
    if (weaponBehind) {
        renderEquippedWeapon();
    }

    for (auto& p : manager.getGroup(groupPlayers)) {
        p->draw(renderContext);
    }

    if (!shieldBehind) {
        renderEquippedShield();
    }
    if (!weaponBehind) {
        renderEquippedWeapon();
    }

    // Dibuja enemigos vivos.
    for (const auto& [enemyId, enemy] : enemies) {
        if (enemy == nullptr) {
            continue;
        }

        if (attackSystem.isEnemyDead(enemyId)) {
            continue;
        }

        enemy->draw(renderContext);
    }

    renderEnemyHealthBars();

    attackSystem.render(renderer, *assets, camera);
    SDL_RenderSetClipRect(renderer, nullptr);

    // Mensaje de estado temporal — se dibuja sobre el mapa, antes del HUD, para que nada lo tape
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

void Game::clean() {
    clearTextCache();
    assets.reset();
    textureManager.reset();

    if (map != nullptr) {
        delete map;
        map = nullptr;
    }

    // No destruimos renderer ni window acá.
    // Son propiedad de main().
    renderer = nullptr;
    window = nullptr;

    std::cout << "Game cleaned." << std::endl;
}

void Game::showStatusMessage(const std::string& msg) {
    statusMessage      = msg;
    statusMessageTimer = SDL_GetTicks();
}

// ---------------------------------------------------------------------------
// Cheats — combinaciones Ctrl+tecla, sin repeat
// ---------------------------------------------------------------------------
void Game::handleCheatKeys() {
    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    const bool ctrl = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
    if (!ctrl) return;

    switch (event.key.keysym.sym) {

        // Ctrl+H — God mode (vida + mana infinitos)
        case SDLK_h:
            cheatGodMode = !cheatGodMode;
            if (cheatGodMode) cheatInfMana = false; // god mode incluye mana
            showStatusMessage(cheatGodMode
                ? "[CHEAT] God mode ON"
                : "[CHEAT] God mode OFF");
            break;

        // Ctrl+M — Mana infinito (solo mana)
        case SDLK_m:
            if (!cheatGodMode) {
                cheatInfMana = !cheatInfMana;
                showStatusMessage(cheatInfMana
                    ? "[CHEAT] Mana infinito ON"
                    : "[CHEAT] Mana infinito OFF");
            }
            break;

        // Ctrl+K — Morir
        case SDLK_k:
            if (!playerState.isDead) {
                showStatusMessage("[CHEAT] Muriendo...");
                cheatGodMode = false;
                cheatInfMana = false;
                playerState.hp = 0;
                applyLocalPlayerGhostState();
            }
            break;

        // Ctrl+L — Subir nivel local (solo visual, para testear HUD)
        case SDLK_l:
            playerState.level = std::min(playerState.level + 1, 99);
            showStatusMessage("[CHEAT] Nivel: " + std::to_string(playerState.level));
            break;
        case SDLK_r:
            if (isLocalPlayerDead()) {
                sendQueue->try_push(std::make_shared<const ResurrectMessage>());
                showStatusMessage("Solicitando resurrección...");
            }
            break;

        default: break;
    }
}

bool Game::running() const { return isRunning; }


void Game::renderHUD() {
    // === FONDOS ===
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

    // === BORDES ===
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

    // === FUENTES Y COLORES ===
    TTF_Font* fontBold    = assets->GetFont("ao_bold");
    TTF_Font* fontRegular = assets->GetFont("ao_regular");

    if (fontBold == nullptr || fontRegular == nullptr) {
        return;
    }

    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color yellow = {255, 215, 0,   255};

    // === HELPERS DE TEXTO CACHEADO ===
    auto drawTextCentered = [&](const std::string& key,
                                const std::string& text,
                                TTF_Font* font,
                                int x,
                                int y,
                                int w,
                                int h,
                                SDL_Color color) {
        int textW = 0;
        int textH = 0;

        SDL_Texture* texture = getOrCreateTextTexture(
            key,
            text,
            font,
            color,
            textW,
            textH
        );

        if (texture == nullptr) {
            return;
        }

        SDL_Rect dest = {
            x + (w - textW) / 2,
            y + (h - textH) / 2,
            textW,
            textH
        };

        SDL_RenderCopy(renderer, texture, nullptr, &dest);
    };

    auto drawTextAt = [&](const std::string& key,
                          const std::string& text,
                          TTF_Font* font,
                          int x,
                          int y,
                          SDL_Color color) {
        int textW = 0;
        int textH = 0;

        SDL_Texture* texture = getOrCreateTextTexture(
            key,
            text,
            font,
            color,
            textW,
            textH
        );

        if (texture == nullptr) {
            return;
        }

        SDL_Rect dest = {x, y, textW, textH};
        SDL_RenderCopy(renderer, texture, nullptr, &dest);
    };

    // === CAJA DE NIVEL ===
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_Rect nivelBox = {908, 38, 50, 50};
    SDL_RenderFillRect(renderer, &nivelBox);

    SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
    SDL_RenderDrawRect(renderer, &nivelBox);

    drawTextCentered(
        "hud_level",
        std::to_string(playerState.level),
        fontBold,
        908,
        38,
        50,
        50,
        yellow
    );

    // === NOMBRE Y CLASE ===
    drawTextAt(
        "hud_name",
        playerState.name,
        fontBold,
        968,
        45,
        yellow
    );

    drawTextAt(
        "hud_class",
        playerClassToString(playerState.playerClass),
        fontRegular,
        968,
        75,
        white
    );

    // === EQUIPAMIENTO ===
    drawTextCentered(
        "hud_title_equipment",
        "Equipamiento",
        fontRegular,
        900,
        142,
        380,
        20,
        white
    );

    SDL_Texture* texFrame = assets->GetTexture("hud_frame");

    const int eqSlotSize = 58;
    const int eqGap = 12;
    const int eqY = 168;
    const int eqStartX = 956;

    std::string eqLabels[] = {"Arma", "Casco", "Armadura", "Escudo"};

    for (int i = 0; i < 4; i++) {
        SDL_Rect slot = {
            eqStartX + i * (eqSlotSize + eqGap),
            eqY,
            eqSlotSize,
            eqSlotSize
        };

        if (texFrame != nullptr) {
            SDL_RenderCopy(renderer, texFrame, nullptr, &slot);
        }

        const ItemView* equippedItem = nullptr;

        if (i == 0 && equipmentState.weapon.has_value()) {
            equippedItem = &equipmentState.weapon.value();
        } else if (i == 1 && equipmentState.helmet.has_value()) {
            equippedItem = &equipmentState.helmet.value();
        } else if (i == 2 && equipmentState.armor.has_value()) {
            equippedItem = &equipmentState.armor.value();
        } else if (i == 3 && equipmentState.shield.has_value()) {
            equippedItem = &equipmentState.shield.value();
        }

        if (equippedItem != nullptr) {
            SDL_Texture* itemTexture = assets->GetTexture(equippedItem->textureId);

            if (itemTexture != nullptr) {
                SDL_Rect itemSrc = {
                    equippedItem->iconSrcX,
                    equippedItem->iconSrcY,
                    equippedItem->iconSrcW,
                    equippedItem->iconSrcH
                };

                SDL_Rect itemDest = {
                    slot.x + 7,
                    slot.y + 7,
                    slot.w - 14,
                    slot.h - 14
                };

                SDL_RenderCopy(renderer, itemTexture, &itemSrc, &itemDest);
            }
        }

        drawTextCentered(
            "hud_eq_label_" + std::to_string(i),
            eqLabels[i],
            fontRegular,
            slot.x - 8,
            slot.y + eqSlotSize + 4,
            eqSlotSize + 16,
            14,
            white
        );
    }

    // === INVENTARIO ===
    const int inventoryTitleY = 255;

    drawTextCentered(
        "hud_title_inventory",
        "Inventario",
        fontRegular,
        900,
        inventoryTitleY,
        380,
        20,
        white
    );

    const int invSlotSize = 44;
    const int invGapX = 8;
    const int invGapY = 7;

    const int invStartX = 964;
    const int invStartY = 280;
    const int invCols = 5;
    const int invRows = 4;

    for (int fila = 0; fila < invRows; fila++) {
        for (int col = 0; col < invCols; col++) {
            const int index = fila * invCols + col;

            SDL_Rect slot = {
                invStartX + col * (invSlotSize + invGapX),
                invStartY + fila * (invSlotSize + invGapY),
                invSlotSize,
                invSlotSize
            };

            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderFillRect(renderer, &slot);

            SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
            SDL_RenderDrawRect(renderer, &slot);

            if (index < static_cast<int>(inventoryState.slots.size()) &&
                inventoryState.slots[index].has_value()) {

                const ItemView& item = inventoryState.slots[index].value();
                SDL_Texture* itemTexture = assets->GetTexture(item.textureId);

                if (itemTexture != nullptr) {
                    SDL_Rect itemDest = {
                        slot.x + 5,
                        slot.y + 5,
                        slot.w - 10,
                        slot.h - 10
                    };

                    SDL_Rect itemSrc = {
                        item.iconSrcX,
                        item.iconSrcY,
                        item.iconSrcW,
                        item.iconSrcH
                    };

                    SDL_RenderCopy(renderer, itemTexture, &itemSrc, &itemDest);
                }

                if (item.quantity > 1) {
                    drawTextAt(
                        "hud_item_qty_" + std::to_string(index),
                        std::to_string(item.quantity),
                        fontRegular,
                        slot.x + slot.w - 14,
                        slot.y + slot.h - 16,
                        white
                    );
                }
            }
        }
    }

    // === BARRAS ===
    const int hpActual   = playerState.hp;
    const int hpMax      = playerState.maxHp;

    const int manaActual = playerState.mana;
    const int manaMax    = playerState.maxMana;

    const int expActual  = playerState.exp;
    const int expMax     = playerState.expToNextLevel;

    auto drawBar = [&](const std::string& key,
                       SDL_Texture* tex,
                       int x,
                       int y,
                       int w,
                       int h,
                       int actual,
                       int max,
                       TTF_Font* font) {
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);

        SDL_Rect bgRect = {x, y, w, h};
        SDL_RenderFillRect(renderer, &bgRect);

        const int fillW = max > 0 ? (w * actual) / max : 0;

        if (tex != nullptr && fillW > 0) {
            SDL_Rect srcR  = {0, 0, fillW, h};
            SDL_Rect fillR = {x, y, fillW, h};
            SDL_RenderCopy(renderer, tex, &srcR, &fillR);
        }

        const std::string text =
            std::to_string(actual) + "/" + std::to_string(max);

        drawTextCentered(
            key,
            text,
            font,
            x,
            y,
            w,
            h,
            white
        );
    };

    SDL_Texture* texVida = assets->GetTexture("barra_vida");
    SDL_Texture* texMana = assets->GetTexture("barra_mana");
    SDL_Texture* texExp  = assets->GetTexture("barra_exp");

    // Experiencia
    drawTextCentered(
        "hud_label_exp",
        "Experiencia",
        fontRegular,
        910,
        100,
        350,
        16,
        white
    );

    drawBar(
        "hud_exp_bar_text",
        texExp,
        910,
        118,
        350,
        16,
        expActual,
        expMax,
        fontRegular
    );

    // Oro
    drawTextAt(
        "hud_gold",
        "Oro: " + std::to_string(playerState.gold),
        fontRegular,
        915,
        585,
        yellow
    );

    // Vida
    const int statsX = 950;
    const int statsBarW = 260;
    const int statsBarH = 18;

    drawTextCentered(
        "hud_label_hp",
        "Vida",
        fontRegular,
        statsX,
        610,
        statsBarW,
        18,
        white
    );

    drawBar(
        "hud_hp_bar_text",
        texVida,
        statsX,
        630,
        statsBarW,
        statsBarH,
        hpActual,
        hpMax,
        fontRegular
    );

    // Mana
    drawTextCentered(
        "hud_label_mana",
        "Mana",
        fontRegular,
        statsX,
        665,
        statsBarW,
        18,
        white
    );

    drawBar(
        "hud_mana_bar_text",
        texMana,
        statsX,
        685,
        statsBarW,
        statsBarH,
        manaActual,
        manaMax,
        fontRegular
    );
}
void Game::loadAssets() {
    assets->LoadManifest("assets/manifest.json");
    // HUD - fondos
    assets->AddTexture("hud_top",      "assets/Recursos/BabelUI/static/media/main_top..png");
    assets->AddTexture("hud_chat",     "assets/Recursos/BabelUI/static/media/main_chat..png");
    assets->AddTexture("hud_pj_info",  "assets/Recursos/BabelUI/static/media/main_pj_info..png");
    assets->AddTexture("hud_inv",      "assets/Recursos/BabelUI/static/media/inventory-bg..png");
    assets->AddTexture("hud_stats",    "assets/Recursos/BabelUI/static/media/stats-bg..png");
    assets->AddTexture("hud_logo",     "assets/Recursos/BabelUI/static/media/ao20_logo_med..png");
    assets->AddTexture("hud_pergamino","assets/Recursos/BabelUI/static/media/titulo_pergamino..png");

    //Textos

    assets->AddFont("ao_bold",    "assets/Recursos/BabelUI/static/media/Alegreya-Sans-AO-Bold..ttf",    18);
    assets->AddFont("ao_regular", "assets/Recursos/BabelUI/static/media/Alegreya-Sans-AO-Regular..ttf", 14);
    assets->AddFont("cardo",      "assets/Recursos/BabelUI/static/media/Cardo-Regular..ttf",            14);

    statusFont = assets->GetFont("ao_bold");
    if (!statusFont) {
        statusFont = TTF_OpenFont(
            "assets/Recursos/BabelUI/static/media/Alegreya-Sans-AO-Bold..ttf", 24
        );
    }
    if (!statusFont) {
        statusFont = TTF_OpenFont("assets/sprites/MapAssets/arial.ttf", 24);
    }


    // HUD
    assets->AddTexture("barra_vida", "assets/Recursos/interface/es_barradevida.bmp");
    assets->AddTexture("barra_mana", "assets/Recursos/interface/es_barrademana.bmp");
    assets->AddTexture("barra_exp",  "assets/Recursos/interface/es_barraexperiencia.bmp");
    assets->AddTexture("hud_frame", "assets/Recursos/BabelUI/static/media/frame..png");

    // Fuentes
    assets->AddFont("arial", "assets/sprites/MapAssets/arial.ttf", 16);

    assets->AddTexture("tile_grass", "assets/sprites/MapAssets/tile_grass.png");
    assets->AddTexture("tile_water", "assets/sprites/MapAssets/tile_water.png");
    assets->AddTexture("tile_floor", "assets/sprites/MapAssets/tile_floor.png");


}

int Game::getInventorySlotIndexAt(int mouseX, int mouseY) const {
    //deben ir los mismos valores que el inventario del renderhud.
    const int invSlotSize = 44;
    const int invGapX = 8;
    const int invGapY = 7;
    const int invStartX = 964;
    const int invStartY = 280;
    const int invCols = 5;
    const int invRows = 4;

    for (int fila = 0; fila < invRows; fila++) {
        for (int col = 0; col < invCols; col++) {
            SDL_Rect slot = {
                invStartX + col * (invSlotSize + invGapX),
                invStartY + fila * (invSlotSize + invGapY),
                invSlotSize,
                invSlotSize
            };

            const bool inside =
                mouseX >= slot.x &&
                mouseX < slot.x + slot.w &&
                mouseY >= slot.y &&
                mouseY < slot.y + slot.h;

            if (inside) {
                return fila * invCols + col;
            }
        }
    }

    return -1;
}
void Game::handleInventorySlotClick(int slotIndex) {
    // Valida que el índice sea válido.
    if (slotIndex < 0 ||
        slotIndex >= static_cast<int>(inventoryState.slots.size())) {
        return;
        }

    // Si el slot está vacío, no hacemos nada.
    if (!inventoryState.slots[slotIndex].has_value()) {
        std::cout << "[INVENTORY] slot vacío: "
                  << slotIndex
                  << std::endl;
        return;
    }

    // Obtenemos el ítem del slot clickeado.
    const ItemView& item = inventoryState.slots[slotIndex].value();

    std::cout << "[INVENTORY] click slot "
              << slotIndex
              << " item="
              << item.itemName
              << " instanceId="
              << item.instanceId
              << std::endl;

    if (item.type == ClientItemType::HealthPotion ||
    item.type == ClientItemType::ManaPotion) {
        std::cout << "[INVENTORY] poción pendiente de UseItemMessage. instanceId="
                  << item.instanceId
                  << std::endl;
        return;
    }
    if (isLocalPlayerDead()) {
        showStatusMessage("No puedes usar objetos estando muerto");
        return;
    }

    if (sendQueue == nullptr) {
        std::cerr << "[INVENTORY] sendQueue nullptr. No se puede enviar acción."
                  << std::endl;
        return;
    }

    // Por ahora, todo click sobre item equipable se manda al server.
    // El cliente NO equipa localmente.
    sendQueue->try_push(
        std::make_shared<const EquipItemMessage>(item.instanceId)
    );
}



int Game::getEquipmentSlotIndexAt(int mouseX, int mouseY) const {
    const int eqSlotSize = 58;
    const int eqGap = 12;
    const int eqY = 168;
    const int eqStartX = 956;

    for (int i = 0; i < 4; i++) {
        SDL_Rect slot = {
            eqStartX + i * (eqSlotSize + eqGap),
            eqY,
            eqSlotSize,
            eqSlotSize
        };

        const bool inside =
            mouseX >= slot.x &&
            mouseX < slot.x + slot.w &&
            mouseY >= slot.y &&
            mouseY < slot.y + slot.h;

        if (inside) {
            return i;
        }
    }

    return -1;
}

bool Game::addItemToFirstFreeInventorySlot(const ItemView& item) {
    for (auto& slot : inventoryState.slots) {
        if (!slot.has_value()) {
            slot = item;
            return true;
        }
    }

    return false;
}

void Game::handleEquipmentSlotClick(int equipmentSlotIndex) {
    if (isLocalPlayerDead()) {
        showStatusMessage("No puedes usar objetos estando muerto");
        return;
    }

    if (sendQueue == nullptr) {
        std::cerr << "[EQUIPMENT] sendQueue nullptr. No se puede desequipar."
                  << std::endl;
        return;
    }

    const auto maybeSlot = toClientEquipmentSlot(equipmentSlotIndex);

    if (!maybeSlot.has_value()) {
        return;
    }

    const ClientEquipmentSlot visualSlot = maybeSlot.value();

    const std::optional<ItemView>* selectedSlot = nullptr;

    switch (visualSlot) {
        case ClientEquipmentSlot::Weapon:
            selectedSlot = &equipmentState.weapon;
            break;

        case ClientEquipmentSlot::Helmet:
            selectedSlot = &equipmentState.helmet;
            break;

        case ClientEquipmentSlot::Armor:
            selectedSlot = &equipmentState.armor;
            break;

        case ClientEquipmentSlot::Shield:
            selectedSlot = &equipmentState.shield;
            break;
    }

    if (selectedSlot == nullptr || !selectedSlot->has_value()) {
        std::cout << "[EQUIPMENT] slot vacío visual="
                  << equipmentSlotIndex
                  << std::endl;
        return;
    }

    const EquipSlot serverSlot = toServerEquipSlot(visualSlot);

    std::cout << "[EQUIPMENT] pedido desequipar visualSlot="
              << equipmentSlotIndex
              << " serverSlot="
              << static_cast<int>(serverSlot)
              << " item="
              << selectedSlot->value().itemName
              << std::endl;

    sendQueue->try_push(
        std::make_shared<const UnequipSlotMessage>(serverSlot)
    );
}

void Game::consumePotion(int slotIndex) {
    if (isLocalPlayerDead()) {
        showStatusMessage("No puedes usar objetos estando muerto");
        return;
    }
    // Validamos que el índice sea válido.
    if (slotIndex < 0 ||
        slotIndex >= static_cast<int>(inventoryState.slots.size())) {
        return;
        }

    // Si el slot está vacío, no hay nada para consumir.
    if (!inventoryState.slots[slotIndex].has_value()) {
        return;
    }

    // Tomamos una copia modificable del ítem.
    ItemView item = inventoryState.slots[slotIndex].value();

    if (item.type == ClientItemType::HealthPotion) {
        // Calculamos nueva vida sin superar el máximo.
        playerState.hp += item.healAmount;

        if (playerState.hp > playerState.maxHp) {
            playerState.hp = playerState.maxHp;
        }

        std::cout << "[POTION] consumida vida: "
                  << item.itemName
                  << " hp="
                  << playerState.hp
                  << "/"
                  << playerState.maxHp
                  << std::endl;

    } else if (item.type == ClientItemType::ManaPotion) {
        // Calculamos nuevo maná sin superar el máximo.
        playerState.mana += item.manaAmount;

        if (playerState.mana > playerState.maxMana) {
            playerState.mana = playerState.maxMana;
        }

        std::cout << "[POTION] consumida maná: "
                  << item.itemName
                  << " mana="
                  << playerState.mana
                  << "/"
                  << playerState.maxMana
                  << std::endl;

    } else {
        // Si no era poción, no hacemos nada.
        return;
    }

    // Reducimos la cantidad.
    item.quantity--;

    // Si se terminó, vaciamos el slot.
    if (item.quantity <= 0) {
        inventoryState.slots[slotIndex] = std::nullopt;
    } else {
        inventoryState.slots[slotIndex] = item;
    }
}

std::string Game::visualTextureForCurrentRace(const ItemView& item) const {
    if (playerState.race == "Dwarf" || playerState.race == "Gnome") {
        if (!item.visualTextureIdShort.empty()) {
            return item.visualTextureIdShort;
        }
    }

    if (!item.visualTextureIdTall.empty()) {
        return item.visualTextureIdTall;
    }

    return item.visualTextureId;
}

void Game::renderEquippedArmor() {
    if (isLocalPlayerDead()) {
        showStatusMessage("No puedes usar objetos estando muerto");
        return;
    }
    // Si no hay armadura equipada, no dibujamos nada.
    if (!equipmentState.armor.has_value()) {
        return;
    }

    // Tomamos la armadura equipada.
    const ItemView& armor = equipmentState.armor.value();

    // Elegimos la textura visual correcta según la raza:
    // human/elf -> tall
    // dwarf/gnome -> short
    const std::string visualTextureId = visualTextureForCurrentRace(armor);

    SDL_Texture* armorTexture = assets->GetTexture(visualTextureId);

    if (armorTexture == nullptr) {
        std::cout << "[EQUIPMENT RENDER] No existe textura: "
                  << visualTextureId
                  << std::endl;
        return;
    }

    // Obtenemos el SpriteComponent del player para copiar su frame y posición.
    auto& sprite = player->getComponent<SpriteComponent>();

    const SDL_Rect& playerSrc = sprite.getSrcRect();
    const SDL_Rect& playerDest = sprite.getDestRect();

    // La armadura debe usar el mismo frame/dirección del cuerpo.
    SDL_Rect armorSrc = {
        playerSrc.x - sprite.getStartX(),
        playerSrc.y - sprite.getStartY(),
        playerSrc.w,
        playerSrc.h
    };

    // Copiamos la posición actual del jugador en pantalla.
    SDL_Rect armorDest = playerDest;


    SDL_Point armorOffset = visualOffsetForCurrentRace(armor);
    armorDest.x += armorOffset.x * armorSpriteConfigForCurrentRace().scale;
    armorDest.y += armorOffset.y * armorSpriteConfigForCurrentRace().scale;
    SDL_RenderCopy(renderer, armorTexture, &armorSrc, &armorDest);

}

SpriteSheetConfig Game::armorSpriteConfigForCurrentRace() const {
    // Si no hay armadura equipada, devolvemos una config neutra.
    // En la práctica casi no debería entrar acá, porque este método
    // se llama cuando ya hay armadura.
    if (!equipmentState.armor.has_value()) {
        return SpriteSheetConfig{
            27,  // ancho de cada frame
            47,  // alto de cada frame
            2,   // escala visual
            0,   // startX dentro del spritesheet de armadura
            0,   // startY dentro del spritesheet de armadura
            0,   // offset X
            0    // offset Y
        };
    }

    // Tomamos la armadura actualmente equipada.
    const ItemView& armor = equipmentState.armor.value();

    // Las razas bajas necesitan usar los offsets short.
    const bool isShortRace =
        playerState.race == "Dwarf" || playerState.race == "Gnome";

    // Devolvemos la config de la armadura, incluyendo offsets visuales.
    return SpriteSheetConfig{
        27,  // ancho de cada frame
        47,  // alto de cada frame
        2,   // escala visual
        0,   // startX: el spritesheet de armadura arranca en 0
        0,   // startY: el spritesheet de armadura arranca en 0

        // Si es dwarf/gnome, usa shortOffsetX.
        // Si no, usa tallOffsetX.
        isShortRace ? armor.visualShortOffsetX : armor.visualTallOffsetX,

        // Si es dwarf/gnome, usa shortOffsetY.
        // Si no, usa tallOffsetY.
        isShortRace ? armor.visualShortOffsetY : armor.visualTallOffsetY
    };
}

void Game::refreshPlayerBodySprite() {
    if (isLocalPlayerDead()) {
        showStatusMessage("No puedes usar objetos estando muerto");
        return;
    }
    // Obtenemos el SpriteComponent del jugador local.
    auto& sprite = player->getComponent<SpriteComponent>();

    // Si hay armadura equipada, reemplazamos el cuerpo desnudo
    // por la textura visual de la armadura.
    if (equipmentState.armor.has_value()) {
        const ItemView& armor = equipmentState.armor.value();

        // Elige armor_iron_tall para human/elf
        // y armor_iron_short para dwarf/gnome.
        const std::string armorTextureId = visualTextureForCurrentRace(armor);

        sprite.setSpriteTextureAndConfig(
            armorTextureId,
            armorSpriteConfigForCurrentRace()
        );

        return;
    }

    sprite.setSpriteTextureAndConfig(
        "body_sheet",
        assets->bodyConfigForRace(playerState.race)
    );
}

// helpér
SDL_Point Game::visualOffsetForCurrentRace(const ItemView& item) const {
    if (playerState.race == "Dwarf" || playerState.race == "Gnome") {
        return SDL_Point{
            item.visualShortOffsetX,
            item.visualShortOffsetY
        };
    }

    return SDL_Point{
        item.visualTallOffsetX,
        item.visualTallOffsetY
    };
}

void Game::refreshPlayerEquipmentVisuals() {
    if (isLocalPlayerDead()) {
        showStatusMessage("No puedes usar objetos estando muerto");
        return;
    }
    // Obtenemos el SpriteComponent del jugador local.
    auto& sprite = player->getComponent<SpriteComponent>();

    // Casco / capucha.
    // Si hay casco equipado, usamos su textura visual y sus offsets.
    if (equipmentState.helmet.has_value()) {
        const ItemView& helmet = equipmentState.helmet.value();

        sprite.setHelmetTexture(
            helmet.visualTextureId,
            helmet.visualOffsetX,
            helmet.visualOffsetY,
            helmet.iconSrcW,
            helmet.iconSrcH,
            helmet.visualDownSrcX,
            helmet.visualDownSrcY,
            helmet.visualLeftSrcX,
            helmet.visualLeftSrcY,
            helmet.visualRightSrcX,
            helmet.visualRightSrcY,
            helmet.visualUpSrcX,
            helmet.visualUpSrcY
        );
    } else {
        // Si no hay casco equipado, limpiamos el visual.
        sprite.clearHelmet();
    }
    // Arma y escudo: no necesitan limpiar nada en el sprite porque se
    // renderizan en render() chequeando equipmentState directamente.
    // Con que el slot esté vacío alcanza para que no se dibujen.
}

void Game::renderEquippedWeapon() {
    if (isLocalPlayerDead()) {
        showStatusMessage("No puedes usar objetos estando muerto");
        return;
    }

    if (!equipmentState.weapon.has_value()) {
        return;
    }

    const ItemView& weapon = equipmentState.weapon.value();

    const std::string& textureId = weapon.visualTextureId;
    if (textureId.empty()) {
        return;
    }

    SDL_Texture* weaponTexture = assets->GetTexture(textureId);
    if (weaponTexture == nullptr) {
        std::cout << "[EQUIPMENT RENDER] No existe textura de arma: "
                  << textureId << std::endl;
        return;
    }

    auto& sprite = player->getComponent<SpriteComponent>();
    const SDL_Rect& playerSrc  = sprite.getSrcRect();
    const SDL_Rect& playerDest = sprite.getDestRect();

    SDL_Rect weaponSrc = {
        playerSrc.x - sprite.getStartX(),
        playerSrc.y - sprite.getStartY(),
        playerSrc.w,
        playerSrc.h
    };

    // El destRect debe tener el tamaño del frame escalado — no el del personaje.
    // playerDest.w/h heredan el tamaño del body (54x94 con scale 2), que es
    // el mismo que queremos para la espada.
    const SpriteSheetConfig cfg = armorSpriteConfigForCurrentRace();
    SDL_Point offset = visualOffsetForCurrentRace(weapon);

    SDL_Rect weaponDest = {
        playerDest.x + offset.x,
        playerDest.y + offset.y,
        playerSrc.w * cfg.scale,
        playerSrc.h * cfg.scale
    };

    //SDL_RenderCopy(renderer, weaponTexture, &weaponSrc, &weaponDest);
    // Usamos el mismo flip que el cuerpo del personaje para que
    // el arma acompañe la orientación y quede siempre en la mano derecha.
    SDL_RenderCopyEx(renderer, weaponTexture, &weaponSrc, &weaponDest,
                     0, nullptr, sprite.spriteFlip);
}

void Game::renderEquippedShield() {
    if (isLocalPlayerDead()) {
        showStatusMessage("No puedes usar objetos estando muerto");
        return;
    }
    if (!equipmentState.shield.has_value()) {
        return;
    }

    const ItemView& shield = equipmentState.shield.value();

    const std::string& textureId = shield.visualTextureId;
    if (textureId.empty()) {
        return;
    }

    SDL_Texture* shieldTexture = assets->GetTexture(textureId);
    if (shieldTexture == nullptr) {
        std::cout << "[EQUIPMENT RENDER] No existe textura de escudo: "
                  << textureId << std::endl;
        return;
    }

    auto& sprite = player->getComponent<SpriteComponent>();
    const SDL_Rect& playerSrc  = sprite.getSrcRect();
    const SDL_Rect& playerDest = sprite.getDestRect();

    SDL_Rect shieldSrc = {
        playerSrc.x - sprite.getStartX(),
        playerSrc.y - sprite.getStartY(),
        playerSrc.w,
        playerSrc.h
    };

    const SpriteSheetConfig cfg = armorSpriteConfigForCurrentRace();
    SDL_Point offset = visualOffsetForCurrentRace(shield);

    SDL_Rect shieldDest = {
        playerDest.x + offset.x,
        playerDest.y + offset.y,
        playerSrc.w * cfg.scale,
        playerSrc.h * cfg.scale
    };

    //SDL_RenderCopy(renderer, shieldTexture, &shieldSrc, &shieldDest);
    SDL_RenderCopyEx(renderer, shieldTexture, &shieldSrc, &shieldDest,
                     0, nullptr, sprite.spriteFlip);
}

void Game::renderEnemyHealthBars() {
    for (const auto& [enemyId, enemy] : enemies) {
        if (enemy == nullptr) {
            continue;
        }

        if (attackSystem.isEnemyDead(enemyId)) {
            continue;
        }

        auto& transform = enemy->getComponent<TransformComponent>();

        int currentHp = attackSystem.getEnemyHealth(enemyId);
        int maxHp = attackSystem.getEnemyMaxHealth(enemyId);

        if (maxHp <= 0) {
            continue;
        }

        float hpRatio = static_cast<float>(currentHp) / static_cast<float>(maxHp);

        if (hpRatio < 0.0f) {
            hpRatio = 0.0f;
        }

        if (hpRatio > 1.0f) {
            hpRatio = 1.0f;
        }

        int screenX = static_cast<int>(transform.position.x) - camera.x;
        int screenY = static_cast<int>(transform.position.y) - camera.y + 133;

        const int barWidth = 50;
        const int barHeight = 6;

        // Ajuste vertical de la barra.
        // Más negativo = más arriba. Más positivo = más abajo.
        const int barOffsetY = -4;

        SDL_Rect backgroundBar{
            screenX,
            screenY + barOffsetY,
            barWidth,
            barHeight
        };

        SDL_Rect healthBar{
            screenX,
            screenY + barOffsetY,
            static_cast<int>(barWidth * hpRatio),
            barHeight
        };

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(renderer, &backgroundBar);

        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        SDL_RenderFillRect(renderer, &healthBar);

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(renderer, &backgroundBar);
    }
}
bool Game::isLocalPlayerDead() const {
    // Si el servidor ya marcó al jugador como fantasma, está muerto.
    if (playerState.isDead) {
        return true;
    }

    // Si todavía no recibimos una vida válida, no podemos asumir muerte
    // solo porque hp sea 0.
    if (!hasReceivedValidPlayerStats) {
        return false;
    }

    // Luego de recibir stats válidas, hp <= 0 sí representa muerte.
    return playerState.hp <= 0;
}

void Game::applyLocalPlayerGhostState() {
    // Evita repetir esta lógica todos los frames.
    if (localGhostStateApplied) {
        return;
    }

    localGhostStateApplied = true;

    // Marcamos el estado visual del jugador como muerto/fantasma.
    playerState.isDead = true;

    // La vida visual queda en cero.
    playerState.hp = 0;

    assets->applyGhostAppearance(*player);

    // Cortamos persecución de enemigos.
    attackSystem.clearEnemyAggro();

    // Mensaje temporal para confirmar el estado.
    showStatusMessage("Has muerto");

    std::cout << "[PLAYER] Jugador pasó a fantasma. HP=0, ataque bloqueado."
              << std::endl;

    // Próximo paso:
    // cambiar sprite/animación a fantasma.
}

void Game::reviveLocalPlayer(int newHp) {
    // El jugador vuelve a estar vivo.
    playerState.isDead = false;

    // Permitimos que, si muere otra vez, se pueda aplicar de nuevo
    // la transición a fantasma.
    localGhostStateApplied = false;
    playerState.hp = newHp;

    // Mensaje visual temporal.
    showStatusMessage("Has revivido");

    std::cout << "[PLAYER] Revivió. HP=" << playerState.hp << std::endl;
    assets->applyPlayerAppearance(*player, playerState);
    refreshPlayerEquipmentVisuals();
}

bool Game::sameColor(SDL_Color a, SDL_Color b) const {
    // Compara color completo, incluido alpha.
    return a.r == b.r &&
           a.g == b.g &&
           a.b == b.b &&
           a.a == b.a;
}

SDL_Texture* Game::getOrCreateTextTexture(
    const std::string& key,
    const std::string& text,
    TTF_Font* font,
    SDL_Color color,
    int& outW,
    int& outH
) {
    // Buscamos si ya existe una textura cacheada para esta key lógica.
    auto it = textCache.find(key);

    if (it != textCache.end()) {
        CachedText& cached = it->second;

        // Si texto, fuente y color siguen iguales, reutilizamos la textura.
        if (cached.texture != nullptr &&
            cached.text == text &&
            cached.font == font &&
            sameColor(cached.color, color)) {
            outW = cached.w;
            outH = cached.h;
            return cached.texture;
        }

        // Si cambió algo, destruimos la textura anterior.
        if (cached.texture != nullptr) {
            SDL_DestroyTexture(cached.texture);
            cached.texture = nullptr;
        }
    }

    // Si el texto está vacío, no generamos textura.
    if (text.empty() || font == nullptr) {
        outW = 0;
        outH = 0;
        return nullptr;
    }

    // Creamos surface nueva solo cuando el texto realmente cambió.
    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), color);
    if (surface == nullptr) {
        outW = 0;
        outH = 0;
        return nullptr;
    }

    // Convertimos surface a texture.
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture == nullptr) {
        SDL_FreeSurface(surface);
        outW = 0;
        outH = 0;
        return nullptr;
    }

    CachedText cached;
    cached.text = text;
    cached.color = color;
    cached.font = font;
    cached.texture = texture;
    cached.w = surface->w;
    cached.h = surface->h;

    SDL_FreeSurface(surface);

    // Guardamos o reemplazamos la entrada cacheada.
    textCache[key] = cached;

    outW = cached.w;
    outH = cached.h;

    return texture;
}

void Game::clearTextCache() {
    // Destruimos todas las texturas cacheadas.
    for (auto& [key, cached] : textCache) {
        if (cached.texture != nullptr) {
            SDL_DestroyTexture(cached.texture);
            cached.texture = nullptr;
        }
    }

    textCache.clear();
}

void Game::applyInventoryUpdate(const InventoryUpdateMessage& msg) {
    // Guardamos el estado visual anterior para mantener posiciones.
    const auto previousSlots = inventoryState.slots;

    // Limpiamos inventario visual actual.
    for (auto& slot : inventoryState.slots) {
        slot.reset();
    }

    // Limpiamos equipamiento visual actual.
    equipmentState.weapon.reset();
    equipmentState.helmet.reset();
    equipmentState.armor.reset();
    equipmentState.shield.reset();

    const auto& serverItems = msg.getItems();
    const auto& equipped = msg.getEquipped();

    // Armamos set de items equipados.
    std::unordered_set<uint32_t> equippedIds;

    for (uint32_t equippedId : equipped) {
        if (equippedId != 0) {
            equippedIds.insert(equippedId);
        }
    }

    // Mapa de items NO equipados por instanceId.
    std::unordered_map<uint32_t, const Item*> availableItems;

    for (const Item& serverItem : serverItems) {
        if (equippedIds.find(serverItem.instanceId) != equippedIds.end()) {
            continue;
        }

        availableItems[serverItem.instanceId] = &serverItem;
    }

    // Items ya colocados visualmente.
    std::unordered_set<uint32_t> placedIds;

    auto makeItemView = [&](const Item& serverItem) -> std::optional<ItemView> {
        try {
            ItemView view = itemCatalog.requireById(
                static_cast<int>(serverItem.catalogId)
            );

            view.instanceId = serverItem.instanceId;
            return view;

        } catch (const std::exception& e) {
            std::cerr << "[CLIENT][INV] catalogId desconocido="
                      << serverItem.catalogId
                      << " instanceId="
                      << serverItem.instanceId
                      << " typeName="
                      << serverItem.typeName
                      << " error="
                      << e.what()
                      << std::endl;

            return std::nullopt;
        }
    };

    // 1. Primero mantenemos en su lugar los items que ya estaban visibles.
    for (std::size_t i = 0; i < previousSlots.size() && i < inventoryState.slots.size(); ++i) {
        if (!previousSlots[i].has_value()) {
            continue;
        }

        const uint32_t previousInstanceId = previousSlots[i]->instanceId;

        auto it = availableItems.find(previousInstanceId);

        if (it == availableItems.end()) {
            continue;
        }

        std::optional<ItemView> view = makeItemView(*it->second);

        if (!view.has_value()) {
            continue;
        }

        inventoryState.slots[i] = view.value();
        placedIds.insert(previousInstanceId);
    }

    // 2. Después colocamos items nuevos o recién desequipados en el primer slot libre.
    for (const Item& serverItem : serverItems) {
        if (equippedIds.find(serverItem.instanceId) != equippedIds.end()) {
            continue;
        }

        if (placedIds.find(serverItem.instanceId) != placedIds.end()) {
            continue;
        }

        std::optional<ItemView> view = makeItemView(serverItem);

        if (!view.has_value()) {
            continue;
        }

        auto freeSlot = std::find_if(
            inventoryState.slots.begin(),
            inventoryState.slots.end(),
            [](const std::optional<ItemView>& slot) {
                return !slot.has_value();
            }
        );

        if (freeSlot == inventoryState.slots.end()) {
            std::cerr << "[CLIENT][INV] no hay slot libre para instanceId="
                      << serverItem.instanceId
                      << std::endl;
            continue;
        }

        *freeSlot = view.value();
        placedIds.insert(serverItem.instanceId);
    }

    auto applyEquipped = [&](EquipSlot slot, std::optional<ItemView>& target) {
        const auto index = static_cast<std::size_t>(slot);

        if (index >= equipped.size()) {
            return;
        }

        const uint32_t equippedInstanceId = equipped[index];

        if (equippedInstanceId == 0) {
            target.reset();
            return;
        }

        auto it = std::find_if(
            serverItems.begin(),
            serverItems.end(),
            [equippedInstanceId](const Item& item) {
                return item.instanceId == equippedInstanceId;
            }
        );

        if (it == serverItems.end()) {
            target.reset();

            std::cerr << "[CLIENT][EQUIP] instanceId equipado no vino en items. id="
                      << equippedInstanceId
                      << std::endl;
            return;
        }

        std::optional<ItemView> view = makeItemView(*it);

        if (!view.has_value()) {
            target.reset();
            return;
        }

        target = view.value();
    };

    applyEquipped(EquipSlot::HAND,   equipmentState.weapon);
    applyEquipped(EquipSlot::HELMET, equipmentState.helmet);
    applyEquipped(EquipSlot::ARMOR,  equipmentState.armor);
    applyEquipped(EquipSlot::SHIELD, equipmentState.shield);

    refreshPlayerEquipmentVisuals();
    refreshPlayerBodySprite();
}


void Game::handleEntityMove(const EntityMoveMessage& moveMsg) {
    const uint32_t entityId = static_cast<uint32_t>(moveMsg.getId());
    const float serverX = static_cast<float>(moveMsg.getX());
    const float serverY = static_cast<float>(moveMsg.getY());
    const Direction direction = moveMsg.getDirection();
    const bool moving = moveMsg.isMoving();

    auto enemyIt = enemies.find(entityId);
    if (enemyIt != enemies.end() && enemyIt->second != nullptr) {
        auto& enemyTransform = enemyIt->second->getComponent<TransformComponent>();

        enemyTransform.position.x = serverX;
        enemyTransform.position.y = serverY;

        std::cout << "[sync enemy] id="
                  << entityId
                  << " server=(" << serverX << ", " << serverY << ")"
                  << std::endl;

        return;
    }

    if (clientWorld != nullptr) {
        clientWorld->updatePlayerPosition(entityId, serverX, serverY,direction,moving);
    }

    // Log opcional para verificar que llegan posiciones pequeñas.
    std::cout << "[sync] server=("<< serverX << ", " << serverY<< ")" << std::endl;

}
void Game::handlePlayerDied(const PlayerDiedMessage& diedMsg) {

    // Leemos el id del jugador muerto enviado por el server.
    const uint32_t deadPlayerId = diedMsg.getId();

    std::cout << "[SERVER] MSG_PLAYER_DIED recibido. playerId="
              << deadPlayerId
              << std::endl;

    // mas adelante varios players visibles, acá deberías comparar:
    // if (deadPlayerId == playerDto.id) { ... }
    playerState.hp = 0;

    // Aplica el estado muerto/fantasma en el cliente.
    applyLocalPlayerGhostState();
}
void Game::handlePlayerStats(const PlayerStatsMessage& stats) {


    const int serverHp = stats.getHp();

    if (serverHp > 0) {
        hasReceivedValidPlayerStats = true;
    }

    // Si estoy muerto y el server manda HP positivo,
    // significa que el server aceptó la resurrección.
    if (playerState.isDead && serverHp > 0) {
        reviveLocalPlayer(serverHp);
    } else if (!playerState.isDead) {
        playerState.hp = serverHp;
    } else {
        playerState.hp = 0;
    }

    playerState.maxHp = stats.getMaxHp();
    playerState.mana = stats.getMana();
    playerState.maxMana = stats.getMaxMana();
    playerState.exp = stats.getExp();
    playerState.expToNextLevel = stats.getExpLimit();
    playerState.level = stats.getLevel();
    playerState.gold = stats.getGold();
}
void Game::handleEntitySpawn(const EntitySpawnMessage& spawnMsg) {


    const PlayerDto& dto = spawnMsg.getPlayerDto();

    std::cout << "[CLIENT] MSG_ENTITY_SPAWN recibido. playerID="
              << static_cast<int>(dto.playerID)
              << " localID="
              << static_cast<int>(playerDto.playerID)
              << " pos=(" << dto.xpos << ", " << dto.ypos << ")"
              << std::endl;

    if (clientWorld != nullptr) {
        clientWorld->spawnRemotePlayer(dto);
    }
}
void Game::handleInventoryUpdate(const InventoryUpdateMessage& inventoryMsg) {

    applyInventoryUpdate(inventoryMsg);

    std::cout << "[CLIENT] MSG_INVENTORY_UPDATE recibido. items="
              << inventoryMsg.getItems().size()
              << std::endl;
}

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

std::optional<ClientEquipmentSlot> Game::toClientEquipmentSlot(int index) const {
    switch (index) {
        case static_cast<int>(ClientEquipmentSlot::Weapon):
            return ClientEquipmentSlot::Weapon;

        case static_cast<int>(ClientEquipmentSlot::Helmet):
            return ClientEquipmentSlot::Helmet;

        case static_cast<int>(ClientEquipmentSlot::Armor):
            return ClientEquipmentSlot::Armor;

        case static_cast<int>(ClientEquipmentSlot::Shield):
            return ClientEquipmentSlot::Shield;

        default:
            return std::nullopt;
    }
}

EquipSlot Game::toServerEquipSlot(ClientEquipmentSlot slot) const {
    switch (slot) {
        case ClientEquipmentSlot::Weapon:
            return EquipSlot::HAND;

        case ClientEquipmentSlot::Helmet:
            return EquipSlot::HELMET;

        case ClientEquipmentSlot::Armor:
            return EquipSlot::ARMOR;

        case ClientEquipmentSlot::Shield:
            return EquipSlot::SHIELD;
    }

    return EquipSlot::HAND;
}