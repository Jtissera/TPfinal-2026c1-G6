
#include "Game.h"
#include "sdl/Map.h"
#include "sdl/TextureManager.h"
#include "sdl/ECS/Components.h"
#include "sdl/ECS/UILabel.h"
#include <sstream>
#include <iostream>

#include "common/network/messages/server/player/EntityMoveMessage.h"
#include "common/network/protocol/serverOpCode.h"

// Definicion de estaticos
bool         Game::isRunning = false;
SDL_Renderer* Game::renderer = nullptr;
SDL_Event    Game::event;
SDL_Rect     Game::camera{0, 0, 900, 720};
AssetManager* Game::assets  = nullptr;

Game::Game() {
}

Game::~Game() {
    delete assets;
    delete map;
}
void Game::init(const char* title, int width, int height, bool fullscreen,
                Queue<std::shared_ptr<const Message>>& sendQ,
                Queue<std::shared_ptr<const Message>>& receiveQ,
                const PlayerDto& pDto) {
    this->sendQueue    = &sendQ;
    this->receiveQueue = &receiveQ;
    this->playerDto    = pDto;
    this->assets       = new AssetManager(&manager, *sendQueue);

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
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    isRunning = true;

    this->playerDto = pDto;
    std::cout << "HP: " << playerDto.hp << "/" << playerDto.hpMax << std::endl;
    std::cout << "Mana: " << playerDto.mana << "/" << playerDto.manaMax << std::endl;
    std::cout << "Exp: " << playerDto.exp << "/" << playerDto.expMax << std::endl;

    loadAssets();
   // loadText();

    player = assets->CreatePlayer(playerDto);

    map = new Map(manager, "terrain", 3, 32);
    map->LoadMap("assets/sprites/MapAssets/map.map", 25, 20);

    label = &manager.addEntity();
    SDL_Color white = {255, 255, 255, 255};
    label->addComponent<UILabel>(10, 10, "Argentum Online", "arial", white);
}

void Game::handleEvents() {
    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT)
        isRunning = false;
    if (event.type == SDL_KEYDOWN && event.key.repeat != 0)
        event.type = SDL_USEREVENT;
}

void Game::update() {
    auto& colliders   = manager.getGroup(groupColliders);
    auto& projectiles = manager.getGroup(groupProjectiles);

    Vector2D playerPos = player->getComponent<TransformComponent>().position;

    // Procesar mensajes del servidor
    std::shared_ptr<const Message> msg;
    while (receiveQueue->try_pop(msg)) {
        if (msg->opCode() == static_cast<uint8_t>(ServerOpCode::MSG_ENTITY_MOVE)) {
            const auto& moveMsg = static_cast<const EntityMoveMessage&>(*msg);
            player->getComponent<TransformComponent>().position.x =
                static_cast<float>(moveMsg.getX());
            player->getComponent<TransformComponent>().position.y =
                static_cast<float>(moveMsg.getY());
        }
    }

    std::stringstream ss;
    ss << "Pos: " << playerPos;
    label->getComponent<UILabel>().SetLabelText(ss.str(), "arial");

    manager.refresh();
    manager.update();

    SDL_Rect playerCol = player->getComponent<ColliderComponent>().collider;
    for (auto& c : colliders) {
        SDL_Rect cCol = c->getComponent<ColliderComponent>().collider;
        if (Collision::AABB(cCol, playerCol))
            player->getComponent<TransformComponent>().position = playerPos;
    }

    // for (auto& e : manager.getGroup(groupEnemies)) {
    //     SDL_Rect eCol = e->getComponent<ColliderComponent>().collider;
    //     if (Collision::AABB(playerCol, eCol))
    //         std::cout << "Colision con enemigo!" << std::endl;
    // }

    for (auto& p : projectiles) {
        if (Collision::AABB(player->getComponent<ColliderComponent>().collider,
                            p->getComponent<ColliderComponent>().collider)) {
            std::cout << "Hit player!" << std::endl;
            p->destroy();
        }
    }

    playerPos = player->getComponent<TransformComponent>().position;
    camera.x  = static_cast<int>(playerPos.x) - 400;
    camera.y  = static_cast<int>(playerPos.y) - 320;
    if (camera.x < 0) camera.x = 0;
    if (camera.y < 0) camera.y = 0;
    if (camera.x > 25 * 96 - 800) camera.x = 25 * 96 - 800;
    if (camera.y > 20 * 96 - 587) camera.y = 20 * 96 - 587;
}

void Game::render() {
    SDL_RenderClear(renderer);
    for (auto& t : manager.getGroup(groupMap))         t->draw();
    //for (auto& c : manager.getGroup(groupColliders))   c->draw();
    for (auto& p : manager.getGroup(groupPlayers))     p->draw();
    for (auto& p : manager.getGroup(groupProjectiles)) p->draw();
    //for (auto& e : manager.getGroup(groupEnemies))     e->draw();

    renderHUD();

    label->draw();
    // Labels HUD


    SDL_RenderPresent(renderer);
}

void Game::clean() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    std::cout << "Game cleaned." << std::endl;
}

bool Game::running() const { return isRunning; }

// void Game::loadText() {
//     SDL_Color white  = {255, 255, 255, 255};
//     SDL_Color yellow = {255, 215, 0,   255};
//     SDL_Color red    = {220, 50,  50,  255};
//     SDL_Color blue   = {50,  100, 220, 255};
//     SDL_Color green  = {50,  200, 50,  255};
//
//     labelName = &manager.addEntity();
//     labelName->addComponent<UILabel>(910, 45, playerDto.nombre, "ao_bold", yellow);
//
//     labelLevel = &manager.addEntity();
//     labelLevel->addComponent<UILabel>(910, 65, "Nivel: " + std::to_string(playerDto.level), "ao_regular", white);
//
//     labelClas = &manager.addEntity();
//     labelClas->addComponent<UILabel>(910, 85, "Guerrero", "ao_regular", white);
//
//     labelHP = &manager.addEntity();
//     labelHP->addComponent<UILabel>(910, 520, std::to_string(playerDto.hp) + "/" + std::to_string(playerDto.hpMax), "ao_regular", red);
//
//     labelMana = &manager.addEntity();
//     labelMana->addComponent<UILabel>(910, 557, std::to_string(playerDto.mana) + "/" + std::to_string(playerDto.manaMax), "ao_regular", blue);
//
//     labelXP = &manager.addEntity();
//     labelXP->addComponent<UILabel>(910, 592, std::to_string(playerDto.exp) + "/" + std::to_string(playerDto.expMax), "ao_regular", green);
//
//     labelGold = &manager.addEntity();
//     labelGold->addComponent<UILabel>(910, 445, "Oro: " + std::to_string(playerDto.oro), "ao_regular", yellow);
//
//
// }

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
    SDL_Rect rInv    = {900, 133, 380,  300};
    SDL_Rect rStats  = {900, 433, 380,  287};

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
    SDL_RenderDrawLine(renderer, 900, 433, 1280, 433);
    SDL_RenderDrawLine(renderer, 900, 434, 1280, 434);

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
    drawTextAt("Guerrero", fontRegular, 968, 75, white);

    // === EQUIPAMIENTO (4 slots: Arma, Casco, Armadura, Escudo) ===
    drawTextCentered("Equipamiento", fontRegular, 900, 140, 380, 20, white);

    int eqSlotSize = 55;
    int eqY = 165;
    int eqStartX = 915;
    std::string eqLabels[] = {"Arma", "Casco", "Armadura", "Escudo"};

    for (int i = 0; i < 4; i++) {
        SDL_Rect slot = {eqStartX + i * (eqSlotSize + 10), eqY, eqSlotSize, eqSlotSize};
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
        SDL_RenderFillRect(renderer, &slot);
        SDL_SetRenderDrawColor(renderer, 100, 80, 40, 255);
        SDL_RenderDrawRect(renderer, &slot);
        // Label debajo del slot
        drawTextCentered(eqLabels[i], fontRegular,
                         slot.x, slot.y + eqSlotSize + 2,
                         eqSlotSize, 14, white);
    }

    // === INVENTARIO (grilla 4x3) ===
    drawTextCentered("Inventario", fontRegular, 900, 245, 380, 20, white);

    int invSlotSize = 50;
    int invStartX   = 915;
    int invStartY   = 268;

    for (int fila = 0; fila < 3; fila++) {
        for (int col = 0; col < 4; col++) {
            SDL_Rect slot = {
                invStartX + col * (invSlotSize + 8),
                invStartY + fila * (invSlotSize + 5),
                invSlotSize, invSlotSize
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
    drawTextAt("Oro: " + std::to_string(playerDto.oro), fontRegular, 910, 445, yellow);

    drawTextCentered("Vida", fontRegular, 910, 460, 350, 20, white);
    drawBar(texVida, 910, 480, 350, 20, hpActual, hpMax, fontRegular);

    drawTextCentered("Mana", fontRegular, 910, 510, 350, 20, white);
    drawBar(texMana, 910, 528, 350, 20, manaActual, manaMax, fontRegular);
}
void Game::loadAssets() {

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
    // Mapa
    assets->AddTexture("terrain", "assets/sprites/MapAssets/terrain_ss.png");

    // Personaje
    assets->AddTexture("player", "assets/sprites/spritesprueba/PNG/Vampires1/Without_shadow/Vampires1_Walk_without_shadow.png");

    // Enemigos — por ahora todos usan el mismo sprite
    assets->AddTexture("skeleton", "assets/sprites/llama.png");
    assets->AddTexture("goblin",   "assets/sprites/llama.png");
    assets->AddTexture("zombie",   "assets/sprites/llama.png");

    // HUD
    assets->AddTexture("barra_vida", "assets/Recursos/interface/es_barradevida.bmp");
    assets->AddTexture("barra_mana", "assets/Recursos/interface/es_barrademana.bmp");
    assets->AddTexture("barra_exp",  "assets/Recursos/interface/es_barraexperiencia.bmp");

    // Fuentes
    assets->AddFont("arial", "assets/sprites/MapAssets/arial.ttf", 16);
}