
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
SDL_Rect     Game::camera{0, 0, 800, 640};
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

    loadAssets();
    map = new Map(manager, "terrain", 3, 32);
    map->LoadMap("assets/sprites/MapAssets/map.map", 25, 20);

    player = assets->CreatePlayer(playerDto);

    NPCData goblin;
    goblin.x     = 1800.0f;
    goblin.y     = 1200.0f;
    goblin.hp    = 50;
    goblin.hpMax = 50;
    goblin.type  = NpcType::SKELETON;
    enemy = assets->CreateEnemy(goblin);

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

    for (auto& e : manager.getGroup(groupEnemies)) {
        SDL_Rect eCol = e->getComponent<ColliderComponent>().collider;
        if (Collision::AABB(playerCol, eCol))
            std::cout << "Colision con enemigo!" << std::endl;
    }

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
    if (camera.y > 20 * 96 - 507) camera.y = 20 * 96 - 507;
}

void Game::render() {
    SDL_RenderClear(renderer);
    for (auto& t : manager.getGroup(groupMap))         t->draw();
    //for (auto& c : manager.getGroup(groupColliders))   c->draw();
    for (auto& p : manager.getGroup(groupPlayers))     p->draw();
    for (auto& p : manager.getGroup(groupProjectiles)) p->draw();
    //for (auto& e : manager.getGroup(groupEnemies))     e->draw();
    label->draw();
    renderHUD();
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

    // === BARRA TOP (full ancho) ===
    SDL_Texture* texTop = assets->GetTexture("hud_top");
    SDL_Rect topDest = {0, 0, 1080, 33};
    SDL_RenderCopy(renderer, texTop, nullptr, &topDest);

    // === LOGO (sobre la barra top) ===
    SDL_Texture* texLogo = assets->GetTexture("hud_logo");
    SDL_Rect logoDest = {5, 0, 177, 33};
    SDL_RenderCopy(renderer, texLogo, nullptr, &logoDest);

    // === CHAT ===
    SDL_Texture* texChat = assets->GetTexture("hud_chat");
    SDL_Rect chatDest = {0, 33, 800, 100};
    SDL_RenderCopy(renderer, texChat, nullptr, &chatDest);

    // === PANEL DERECHO - PJ INFO ===
    SDL_Texture* texPjInfo = assets->GetTexture("hud_pj_info");
    SDL_Rect pjInfoDest = {800, 33, 280, 100};
    SDL_RenderCopy(renderer, texPjInfo, nullptr, &pjInfoDest);

    // === PANEL DERECHO - INVENTARIO ===
    SDL_Texture* texInv = assets->GetTexture("hud_inv");
    SDL_Rect invDest = {800, 133, 280, 267};
    SDL_RenderCopy(renderer, texInv, nullptr, &invDest);

    // === PANEL DERECHO - STATS ===
    SDL_Texture* texStats = assets->GetTexture("hud_stats");
    SDL_Rect statsDest = {800, 400, 280, 240};
    SDL_RenderCopy(renderer, texStats, nullptr, &statsDest);

    // === BARRAS DE VIDA MANA EXP ===
    int hpActual = 75,   hpMax   = 100;
    int manaActual = 40, manaMax = 100;
    int expActual = 300, expMax  = 1000;

    SDL_Texture* texVida = assets->GetTexture("barra_vida");
    SDL_Rect vidaDest = {810, 510, 216, 16};
    SDL_RenderCopy(renderer, texVida, nullptr, &vidaDest);
    int vidaAncho = (216 * hpActual) / hpMax;
    SDL_Rect vidaSrc  = {0, 0, vidaAncho, 16};
    SDL_Rect vidaFill = {810, 510, vidaAncho, 16};
    SDL_RenderCopy(renderer, texVida, &vidaSrc, &vidaFill);

    SDL_Texture* texMana = assets->GetTexture("barra_mana");
    SDL_Rect manaDest = {810, 545, 216, 16};
    SDL_RenderCopy(renderer, texMana, nullptr, &manaDest);
    int manaAncho = (216 * manaActual) / manaMax;
    SDL_Rect manaSrc  = {0, 0, manaAncho, 16};
    SDL_Rect manaFill = {810, 545, manaAncho, 16};
    SDL_RenderCopy(renderer, texMana, &manaSrc, &manaFill);

    SDL_Texture* texExp = assets->GetTexture("barra_exp");
    SDL_Rect expDest = {810, 580, 216, 16};
    SDL_RenderCopy(renderer, texExp, nullptr, &expDest);
    int expAncho = (216 * expActual) / expMax;
    SDL_Rect expSrc  = {0, 0, expAncho, 16};
    SDL_Rect expFill = {810, 580, expAncho, 16};
    SDL_RenderCopy(renderer, texExp, &expSrc, &expFill);
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