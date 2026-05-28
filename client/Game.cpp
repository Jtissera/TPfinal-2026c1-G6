
#include "Game.h"
#include "sdl/Map.h"
#include "sdl/TextureManager.h"
#include "sdl/ECS/Components.h"
#include <sstream>
#include <iostream>
#include "sdl/UpdateContext.h"
#include "sdl/RenderContext.h"

#include "common/network/messages/client/combat/attackMessage.h"
#include "common/network/messages/server/player/EntityMoveMessage.h"
#include "common/network/protocol/serverOpCode.h"
#include "sdl/state/PlayerViewStateMapper.h"
#include "sdl/GroupLabels.h"


Game::Game() {
}

void Game::init(const char* title, int width, int height, bool fullscreen,
                Queue<std::shared_ptr<const Message>>& sendQ,
                Queue<std::shared_ptr<const Message>>& receiveQ,
                const PlayerDto& pDto) {
    this->sendQueue    = &sendQ;
    this->receiveQueue = &receiveQ;
    this->playerDto    = pDto;
    this->playerState = toPlayerViewState(this->playerDto );

    int flags = fullscreen ? SDL_WINDOW_FULLSCREEN : 0;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        std::cerr << "Error SDL_Init: " << SDL_GetError() << std::endl;
        return;
    }
    if (TTF_Init() == -1) {
        std::cerr << "Error TTF_Init: " << TTF_GetError() << std::endl;
        return;
    }

    window   = SDL_CreateWindow(title,
                                SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                width, height, flags);
    if (window == nullptr) {
        std::cerr << "Error SDL_CreateWindow: " << SDL_GetError() << std::endl;
        return;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    if (renderer == nullptr) {
        std::cerr << "Error SDL_CreateRenderer: " << SDL_GetError() << std::endl;
        return;
    }

    textureManager = std::make_unique<TextureManager>(renderer);
    assets = std::make_unique<AssetManager>(&manager,*sendQueue,*textureManager);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    isRunning = true;
    std::cout << "[INIT] antes loadAssets" << std::endl;
    loadAssets();
    std::cout << "[INIT] antes itemCatalog" << std::endl;
    try {
        itemCatalog.loadFromJson("assets/items/items.json");
        std::cout << "[INIT] itemCatalog cargado" << std::endl;

        loadInitialInventoryForCurrentClass();
        std::cout << "[INIT] inventario inicial cargado" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error cargando catálogo de ítems: " << e.what() << std::endl;
        isRunning = false;
        return;
    }

    std::cout << "[INIT] antes CreatePlayer" << std::endl;
    player = assets->CreatePlayer(playerDto);
    std::cout << "[INIT] antes Map" << std::endl;


    std::cout << "[INIT] antes Map" << std::endl;
    map = new Map(manager, *assets, "terrain", 3, 32);
    map->LoadMap("assets/sprites/MapAssets/mapa.argmap");

    std::cout << "Tiles cargados como entidades: "
          << manager.getGroup(groupMap).size()
          << std::endl;

    // En Game.cpp, al final de init(), después de crear el player
    NPCData fakeEnemy;
    fakeEnemy.npcID   = 99;          // ID falso
    fakeEnemy.type = NpcType::SKELETON;
    fakeEnemy.x    = 600;         // posición en píxeles de mundo
    fakeEnemy.y    = 400;

    std::cout << "[INIT] antes Enemy" << std::endl;
    Entity* e = assets->CreateEnemy(fakeEnemy);
    enemies[fakeEnemy.npcID] = e;
    std::cout << "[INIT] fin Game::init" << std::endl;
    std::cout << "[INIT] fin Game::init" << std::endl;
}

void Game::handleEvents() {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT){
            isRunning = false;
        }

        if (event.type == SDL_KEYDOWN && event.key.repeat != 0){
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

            attackSystem.handleMouseClick(mouseX, mouseY, camera, enemies, sendQueue);
        }
    }

}

void Game::update() {
    std::shared_ptr<const Message> msg;
    while (receiveQueue->try_pop(msg)) {
        if (msg->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE)) {
            const auto& moveMsg = static_cast<const EntityMoveMessage&>(*msg);

            player->getComponent<TransformComponent>().position.x =
                static_cast<float>(moveMsg.getX());

            player->getComponent<TransformComponent>().position.y =
                static_cast<float>(moveMsg.getY());

            std::cout << "[client] pos recibida del server: " 
                      << moveMsg.getX() << ", " << moveMsg.getY() << std::endl;
        } else if (msg->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_PLAYER_STATS)) {
            const auto& stats = static_cast<const PlayerStatsMessage&>(*msg);
            playerState.hp = stats.getHp();
            playerState.maxHp = stats.getMaxHp();
            playerState.mana = stats.getMana();
            playerState.maxMana = stats.getMaxMana();
            playerState.exp = stats.getExp();
            playerState.expToNextLevel = stats.getExpLimit();
            playerState.level = stats.getLevel();
            playerState.gold = stats.getGold();
        }
    }

    UpdateContext updateContext{
        SDL_GetKeyboardState(nullptr),
        sendQueue,
        camera
    };
    manager.refresh();
    manager.update(updateContext);
    attackSystem.update();

    Vector2D playerPos = player->getComponent<TransformComponent>().position;
    camera.x = static_cast<int>(playerPos.x) - 450;
    camera.y = static_cast<int>(playerPos.y) - 343;
    if (camera.x < 0) camera.x = 0;
    if (camera.y < 0) camera.y = 0;
    if (camera.x > 20 * 96 - 900) camera.x = 20 * 96 - 900;
    if (camera.y > 15 * 96 - 687) camera.y = 15 * 96 - 687;

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

    // Dibuja jugadores.
    for (auto& p : manager.getGroup(groupPlayers)) {
        p->draw(renderContext);
    }

    // Dibuja enemigos.
    for (auto& p : manager.getGroup(groupEnemies)) {
        p->draw(renderContext);
    }

    attackSystem.render(renderer, *assets, camera);

    // Importante: sacar el clip antes de dibujar el HUD.
    SDL_RenderSetClipRect(renderer, nullptr);

    // Dibuja HUD por encima del juego.
    renderHUD();

    // Presenta el frame final en pantalla.
    SDL_RenderPresent(renderer);
}

void Game::clean() {
    assets.reset();
    textureManager.reset();

    if (map != nullptr) {
        delete map;
        map = nullptr;
    }

    if (renderer != nullptr) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }

    if (window != nullptr) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }

    TTF_Quit();
    SDL_Quit();

    std::cout << "Game cleaned." << std::endl;
}

bool Game::running() const { return isRunning; }


void Game::renderHUD() {
    //1. Fondo/marco     ← primero (abajo)
    // 2. Barras          ← encima del fondo
    // 3. Slots/items     ← encima de las barras
    // 4. Textos          ← último (arriba de todo)


    //
    // // 1. Cargar la textura una vez en loadAssets()

    //
    // // 2. En renderHUD(), dibujar la imagen donde querés el fondo
    // SDL_Texture* fondo = assets->GetTexture("fondo_inventario");
    //
    // // Define dónde y qué tamaño en pantalla
    // SDL_Rect destino = {800, 0, 280, 640};  // x, y, ancho, alto
    //
    // // Dibuja la imagen estirada para llenar ese rectángulo
    // SDL_RenderCopy(renderer, fondo, nullptr, &destino);
    //
    // // Después dibujás todo lo demás ENCIMA (barras, slots, texto)

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

    SDL_RenderCopy(renderer, texTop,    nullptr, &rTop);
    SDL_RenderCopy(renderer, texLogo,   nullptr, &rLogo);
    SDL_RenderCopy(renderer, texChat,   nullptr, &rChat);
    SDL_RenderCopy(renderer, texPjInfo, nullptr, &rPjInfo);
    SDL_RenderCopy(renderer, texInv,    nullptr, &rInv);
    SDL_RenderCopy(renderer, texStats,  nullptr, &rStats);

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

    // === CAJA DE NIVEL ===
    SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
    SDL_Rect nivelBox = {908, 38, 50, 50};
    SDL_RenderFillRect(renderer, &nivelBox);
    SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
    SDL_RenderDrawRect(renderer, &nivelBox);

    // helper para texto centrado en un rect
    TTF_Font* fontBold    = assets->GetFont("ao_bold");
    TTF_Font* fontRegular = assets->GetFont("ao_regular");

    auto drawTextCentered = [&](const std::string& text, TTF_Font* font,
                                 int x, int y, int w, int h, SDL_Color color) {
        SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), color);
        if (!surf) return;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        int tw, th;
        SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
        SDL_Rect dest = {x + (w - tw) / 2, y + (h - th) / 2, tw, th};
        SDL_RenderCopy(renderer, tex, nullptr, &dest);
        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);
    };

    auto drawTextAt = [&](const std::string& text, TTF_Font* font,
                           int x, int y, SDL_Color color) {
        SDL_Surface* surf = TTF_RenderText_Blended(font, text.c_str(), color);
        if (!surf) return;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
        int tw, th;
        SDL_QueryTexture(tex, nullptr, nullptr, &tw, &th);
        SDL_Rect dest = {x, y, tw, th};
        SDL_RenderCopy(renderer, tex, nullptr, &dest);
        SDL_FreeSurface(surf);
        SDL_DestroyTexture(tex);
    };

    SDL_Color white  = {255, 255, 255, 255};
    SDL_Color yellow = {255, 215, 0,   255};

    // Nivel centrado en la caja
    drawTextCentered(std::to_string(playerState.level),
                     fontBold, 908, 38, 50, 50, yellow);

    // Nombre grande
    drawTextAt(playerState.name, fontBold, 968, 45, yellow);

    // Clase
    drawTextAt(playerClassToString(playerState.playerClass), fontRegular, 968, 75, white);

    // === EQUIPAMIENTO (4 slots con frame) ===
    drawTextCentered("Equipamiento", fontRegular, 900, 142, 380, 20, white);

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

        SDL_RenderCopy(renderer, texFrame, nullptr, &slot);
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
            eqLabels[i],
            fontRegular,
            slot.x - 8,
            slot.y + eqSlotSize + 4,
            eqSlotSize + 16,
            14,
            white
        );
    }
    // === INVENTARIO (grilla 5x4) ===
    const int inventoryTitleY = 255;
    drawTextCentered("Inventario", fontRegular, 900, inventoryTitleY, 380, 20, white);

    const int invSlotSize = 44;
    const int invGapX = 8;
    const int invGapY = 7;

    const int invStartX = 964;
    const int invStartY = 280;
    const int invCols = 5;
    const int invRows = 4;

    for (int fila = 0; fila < invRows; fila++) {
        for (int col = 0; col < invCols; col++) {
            // Calcula qué slot lógico representa esta posición visual.
            const int index = fila * invCols + col;

            // Rectángulo visual del slot.
            SDL_Rect slot = {
                invStartX + col * (invSlotSize + invGapX),
                invStartY + fila * (invSlotSize + invGapY),
                invSlotSize,
                invSlotSize
            };

            // Fondo del slot.
            SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
            SDL_RenderFillRect(renderer, &slot);

            // Borde del slot.
            SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
            SDL_RenderDrawRect(renderer, &slot);

            // Si el slot existe y contiene un ítem, lo dibujamos.
            if (index < static_cast<int>(inventoryState.slots.size()) && inventoryState.slots[index].has_value()) {
                const ItemView& item = inventoryState.slots[index].value();
                // La textura viene del textureId definido en items.json.
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
                // Si hay cantidad mayor a 1, mostramos el número.
                if (item.quantity > 1) {
                    drawTextAt(
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

    // === BARRAS CON TEXTO CENTRADO ===
    int hpActual   = playerState.hp;
    int hpMax      = playerState.maxHp;

    int manaActual = playerState.mana;
    int manaMax    = playerState.maxMana;

    int expActual  = playerState.exp;
    int expMax     = playerState.expToNextLevel;

    // Función para dibujar barra con texto encima
    auto drawBar = [&](SDL_Texture* tex, int x, int y, int w, int h,
                       int actual, int max, TTF_Font* font) {
        // Fondo oscuro (barra vacía)
        SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
        SDL_Rect bgRect = {x, y, w, h};
        SDL_RenderFillRect(renderer, &bgRect);

        // Barra coloreada solo hasta fillW
        int fillW = max > 0 ? (w * actual) / max : 0;
        if (fillW > 0) {
            SDL_Rect srcR  = {0, 0, fillW, h};
            SDL_Rect fillR = {x, y, fillW, h};
            SDL_RenderCopy(renderer, tex, &srcR, &fillR);
        }

        // Texto centrado dentro de la barra
        std::string text = std::to_string(actual) + "/" + std::to_string(max);
        drawTextCentered(text, font, x, y, w, h, {255, 255, 255, 255});
    };

    SDL_Texture* texVida = assets->GetTexture("barra_vida");
    SDL_Texture* texMana = assets->GetTexture("barra_mana");
    SDL_Texture* texExp  = assets->GetTexture("barra_exp");

    // Exp en pj_info
    drawTextCentered("Experiencia", fontRegular, 910, 100, 350, 16, white);
    drawBar(texExp, 910, 118, 350, 16, expActual, expMax, fontRegular);


    // Stats - orden: Oro, Vida, Mana
    const int statsX = 950;
    const int statsBarW = 260;
    const int statsBarH = 18;

    drawTextAt("Oro: " + std::to_string(playerState.gold), fontRegular, 915, 585, yellow);

    drawTextCentered("Vida", fontRegular, statsX, 610, statsBarW, 18, white);
    drawBar(texVida, statsX, 630, statsBarW, statsBarH, hpActual, hpMax, fontRegular);

    drawTextCentered("Mana", fontRegular, statsX, 665, statsBarW, 18, white);
    drawBar(texMana, statsX, 685, statsBarW, statsBarH, manaActual, manaMax, fontRegular);
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

void Game::loadInitialInventoryForCurrentClass() {
    for (auto& slot : inventoryState.slots) {
        slot = std::nullopt;
    }

    switch (playerState.playerClass) {

        case PlayerClass::Cleric:
            inventoryState.slots[0] = itemCatalog.requireById(2); // Báculo temporal
            inventoryState.slots[1] = itemCatalog.requireById(4); // Capucha
            inventoryState.slots[2] = itemCatalog.requireById(6); // Poción vida
            inventoryState.slots[3] = itemCatalog.requireById(7); // Poción maná

            break;
        case PlayerClass::Mage:
            inventoryState.slots[0] = itemCatalog.requireById(2); // Báculo
            inventoryState.slots[1] = itemCatalog.requireById(4); // Capucha
            inventoryState.slots[2] = itemCatalog.requireById(7); // Poción maná
            inventoryState.slots[3] = itemCatalog.requireById(6); // Poción vida
            break;
        case PlayerClass::Paladin:
            inventoryState.slots[0] = itemCatalog.requireById(1); // Espada
            inventoryState.slots[1] = itemCatalog.requireById(3); // Armadura
            inventoryState.slots[2] = itemCatalog.requireById(5); // Escudo
            inventoryState.slots[3] = itemCatalog.requireById(6); // Poción vida

            break;
        case PlayerClass::Warrior:
            inventoryState.slots[0] = itemCatalog.requireById(1); // Espada
            inventoryState.slots[1] = itemCatalog.requireById(3); // Armadura de cuero
            inventoryState.slots[2] = itemCatalog.requireById(5); // Escudo
            inventoryState.slots[3] = itemCatalog.requireById(6); // Poción vida

            break;
        default:
            inventoryState.slots[0] = itemCatalog.requireById(1);
            inventoryState.slots[1] = itemCatalog.requireById(6);
            break;

    }




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
              << std::endl;

    // Si es poción, todavía no la consumimos en este paso.
    if (item.type == ClientItemType::HealthPotion ||
        item.type == ClientItemType::ManaPotion) {
        std::cout << "[INVENTORY] poción seleccionada, consumo pendiente"
                  << std::endl;
        consumePotion(slotIndex);
        return;
        }

    // Si no es poción, intentamos equiparlo.
    equipItemFromInventory(slotIndex);
}

void Game::equipItemFromInventory(int slotIndex) {
    std::cout << "[DEBUG] entro a equipItemFromInventory slot="
          << slotIndex
          << std::endl;

    if (slotIndex < 0 ||
        slotIndex >= static_cast<int>(inventoryState.slots.size())) {
        return;
        }

    if (!inventoryState.slots[slotIndex].has_value()) {
        return;
    }

    ItemView itemToEquip = inventoryState.slots[slotIndex].value();

    std::optional<ItemView>* targetSlot = nullptr;

    if (itemToEquip.type == ClientItemType::MeleeWeapon ||
        itemToEquip.type == ClientItemType::RangedWeapon ||
        itemToEquip.type == ClientItemType::MagicWeapon) {

        if (itemToEquip.type == ClientItemType::MagicWeapon &&
            playerState.playerClass == PlayerClass::Warrior) {
            std::cout << "[EQUIPMENT] Guerrero no puede equipar arma mágica"
                      << std::endl;
            return;
            }

        targetSlot = &equipmentState.weapon;
        } else if (itemToEquip.type == ClientItemType::Armor) {
            targetSlot = &equipmentState.armor;
        } else if (itemToEquip.type == ClientItemType::Helmet) {
            targetSlot = &equipmentState.helmet;
        } else if (itemToEquip.type == ClientItemType::Shield) {
            targetSlot = &equipmentState.shield;
        } else {
            std::cout << "[EQUIPMENT] ítem no equipable: "
                      << itemToEquip.itemName
                      << std::endl;
            return;
        }
    // Si  había algo equipado, vuelve al slot del inventario.
    if (targetSlot->has_value()) {
        inventoryState.slots[slotIndex] = targetSlot->value();
    } else {
        inventoryState.slots[slotIndex] = std::nullopt;
    }

    *targetSlot = itemToEquip;

    std::cout << "[EQUIPMENT] equipado: "
              << itemToEquip.itemName
              << std::endl;
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
    std::optional<ItemView>* selectedSlot = nullptr;

    if (equipmentSlotIndex == 0) {
        selectedSlot = &equipmentState.weapon;
    } else if (equipmentSlotIndex == 1) {
        selectedSlot = &equipmentState.helmet;
    } else if (equipmentSlotIndex == 2) {
        selectedSlot = &equipmentState.armor;
    } else if (equipmentSlotIndex == 3) {
        selectedSlot = &equipmentState.shield;
    } else {
        return;
    }

    if (!selectedSlot->has_value()) {
        std::cout << "[EQUIPMENT] slot vacío" << std::endl;
        return;
    }

    ItemView itemToUnequip = selectedSlot->value();

    if (!addItemToFirstFreeInventorySlot(itemToUnequip)) {
        std::cout << "[EQUIPMENT] no se puede desequipar: inventario lleno"
                  << std::endl;
        return;
    }

    selectedSlot->reset();

    std::cout << "[EQUIPMENT] desequipado: "
              << itemToUnequip.itemName
              << std::endl;
}
void Game::consumePotion(int slotIndex) {
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

