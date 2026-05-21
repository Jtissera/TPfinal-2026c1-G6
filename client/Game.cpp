
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

    assets->AddTexture("terrain",  "assets/sprites/MapAssets/terrain_ss.png");
    assets->AddTexture("player",   "assets/sprites/spritesprueba/PNG/Vampires1/Without_shadow/Vampires1_Walk_without_shadow.png");
    assets->AddTexture("skeleton", "assets/sprites/llama.png");
    assets->AddFont("arial",       "assets/sprites/MapAssets/arial.ttf", 16);

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
    if (camera.y > 20 * 96 - 640) camera.y = 20 * 96 - 640;
}

void Game::render() {
    SDL_RenderClear(renderer);
    for (auto& t : manager.getGroup(groupMap))         t->draw();
    for (auto& c : manager.getGroup(groupColliders))   c->draw();
    for (auto& p : manager.getGroup(groupPlayers))     p->draw();
    for (auto& p : manager.getGroup(groupProjectiles)) p->draw();
    for (auto& e : manager.getGroup(groupEnemies))     e->draw();
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
    // TODO: implementar HUD
}
