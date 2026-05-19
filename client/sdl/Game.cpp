
#include "Game.h"
#include "Map.h"
#include "TextureManager.h"
#include "ECS/Components.h"
#include "ECS/UILabel.h"
#include <sstream>
#include <iostream>

// Definicion de estaticos
bool         Game::isRunning = false;
SDL_Renderer* Game::renderer = nullptr;
SDL_Event    Game::event;
SDL_Rect     Game::camera{0, 0, 800, 640};
AssetManager* Game::assets  = nullptr;

Game::Game() {
    assets = new AssetManager(&manager);
}

Game::~Game() {
    delete assets;
    delete map;
}

void Game::init(const char* title, int width, int height, bool fullscreen) {
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

    // Cargar assets
    assets->AddTexture("terrain", "assets/sprites/MapAssets/terrain_ss.png");
    assets->AddTexture("player",  "assets/llama.png");
    assets->AddFont("arial",      "assets/sprites/MapAssets/arial.ttf", 16);

    // Mapa
    map = new Map(manager, "terrain", 3, 32);
    map->LoadMap("assets/sprites/MapAssets/map.map", 25, 20);

    // Jugador
    player = &manager.addEntity();
    player->addComponent<TransformComponent>(400.0f, 320.0f, 32, 32, 1);
    player->addComponent<SpriteComponent>("player", true);
    player->addComponent<KeyboardController>();
    player->addComponent<ColliderComponent>("player");
    player->addGroup(groupPlayers);

    // Label de debug
    label = &manager.addEntity();
    SDL_Color white = {255, 255, 255, 255};
    label->addComponent<UILabel>(10, 10, "Argentum Online", "arial", white);
}

void Game::handleEvents() {
    SDL_PollEvent(&event);
    if (event.type == SDL_QUIT) {
        isRunning = false;
    }
}

void Game::update() {
    auto& players    = manager.getGroup(groupPlayers);
    auto& colliders  = manager.getGroup(groupColliders);
    auto& projectiles = manager.getGroup(groupProjectiles);

    Vector2D playerPos = player->getComponent<TransformComponent>().position;

    // Debug label
    std::stringstream ss;
    ss << "Pos: " << playerPos;
    label->getComponent<UILabel>().SetLabelText(ss.str(), "arial");

    manager.refresh();
    manager.update();

    // Colisiones con terreno
    SDL_Rect playerCol = player->getComponent<ColliderComponent>().collider;
    for (auto& c : colliders) {
        SDL_Rect cCol = c->getComponent<ColliderComponent>().collider;
        if (Collision::AABB(cCol, playerCol)) {
            player->getComponent<TransformComponent>().position = playerPos;
        }
    }

    // Colisiones con proyectiles
    for (auto& p : projectiles) {
        if (Collision::AABB(player->getComponent<ColliderComponent>().collider,
                            p->getComponent<ColliderComponent>().collider)) {
            std::cout << "Hit player!" << std::endl;
            p->destroy();
        }
    }

    // Cámara sigue al jugador
    playerPos = player->getComponent<TransformComponent>().position;
    camera.x = static_cast<int>(playerPos.x) - 400;
    camera.y = static_cast<int>(playerPos.y) - 320;
    if (camera.x < 0) camera.x = 0;
    if (camera.y < 0) camera.y = 0;
}

void Game::render() {
    SDL_RenderClear(renderer);

    for (auto& t : manager.getGroup(groupMap))         t->draw();
    for (auto& c : manager.getGroup(groupColliders))   c->draw();
    for (auto& p : manager.getGroup(groupPlayers))     p->draw();
    for (auto& p : manager.getGroup(groupProjectiles)) p->draw();

    label->draw();

    SDL_RenderPresent(renderer);
}

void Game::clean() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    std::cout << "Game cleaned." << std::endl;
}

bool Game::running() const {
    return isRunning;
}

