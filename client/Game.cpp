
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

    this->playerDto = pDto;
    loadAssets();
    itemCatalog.loadFromJson("assets/items/items.json");
    player = assets->CreatePlayer(playerDto);

    map = new Map(manager,*assets, "terrain", 3, 32);
    map->LoadMap("assets/sprites/MapAssets/mapa.argmap");

    // En Game.cpp, al final de init(), después de crear el player
    NPCData fakeEnemy;
    fakeEnemy.npcID   = 99;          // ID falso
    fakeEnemy.type = NpcType::SKELETON;
    fakeEnemy.x    = 600;         // posición en píxeles de mundo
    fakeEnemy.y    = 400;

    Entity* e = assets->CreateEnemy(fakeEnemy);
    enemies[fakeEnemy.npcID] = e;    // guardás el puntero en el mapa

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
            attackSystem.handleMouseClick(event.button.x,event.button.y,camera,enemies,sendQueue);
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
    playerDto.hp     = stats.getHp();
    playerDto.hpMax  = stats.getMaxHp();
    playerDto.mana   = stats.getMana();
    playerDto.manaMax= stats.getMaxMana();
    playerDto.exp    = stats.getExp();
    playerDto.level  = stats.getLevel();
    playerDto.oro    = stats.getGold();
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

    // // Dibuja proyectiles, si existen.
    // for (auto& p : manager.getGroup(groupProjectiles)) {
    //     p->draw(renderContext);
    // }
    // Renderiza efectos de ataque.
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
    drawTextCentered(std::to_string(playerDto.level),
                     fontBold, 908, 38, 50, 50, yellow);

    // Nombre grande
    drawTextAt(playerDto.nombre, fontBold, 968, 45, yellow);

    // Clase
    drawTextAt(playerDto.clase, fontRegular, 968, 75, white);

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

    // 5 columnas de 44 + 4 gaps de 8 = 252.
    // Centro: 900 + (380 - 252) / 2 = 964.
    const int invStartX = 964;
    const int invStartY = 280;

    for (int fila = 0; fila < 4; fila++) {
        for (int col = 0; col < 5; col++) {
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
        }
    }

    // === BARRAS CON TEXTO CENTRADO ===
    int hpActual   = playerDto.hp,   hpMax   = playerDto.hpMax;
    int manaActual = playerDto.mana, manaMax = playerDto.manaMax;
    int expActual  = playerDto.exp,  expMax  = playerDto.expMax;

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

    drawTextAt("Oro: " + std::to_string(playerDto.oro), fontRegular, 915, 585, yellow);

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

