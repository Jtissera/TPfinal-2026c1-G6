
#ifndef PRUEBA_SDL_GAME_H
#define PRUEBA_SDL_GAME_H
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include "sdl/ECS/ECS.h"
#include "sdl/AssetManager.h"
#include "sdl/Collision.h"
#include <vector>

#include "sdl/Map.h"
#include "common/queue.h"


class Game {
public:
    Game();
    ~Game();

    void init(const char* title, int width, int height, bool fullscreen,Queue<std::shared_ptr<const Message>>& sendQueue,
              Queue<std::shared_ptr<const Message>>& receiveQueue,
              const PlayerDto& playerDto);
    void handleEvents();
    void update();
    void render();
    void clean();
    bool running() const;
    void renderHUD();



    // Estáticos — accedidos por los componentes
    static bool         isRunning;
    static SDL_Renderer* renderer;
    static SDL_Event     event;
    static SDL_Rect      camera;
    static AssetManager* assets;

    enum groupLabels : std::size_t {
        groupMap,
        groupPlayers,
        groupColliders,
        groupProjectiles,
        groupNPC,
        groupEnemies,
    };

private:
    SDL_Window* window = nullptr;
    Manager     manager;
    Map*        map    = nullptr;
    Entity* player = nullptr;
    Entity* label  = nullptr;
    // Entity* labelName = nullptr;
    // Entity* labelLevel= nullptr;
    // Entity* labelClas = nullptr;
    // Entity* labelHP = nullptr;
    // Entity* labelMana = nullptr;
    // Entity* labelXP = nullptr;
    // Entity* labelGold = nullptr;
    Entity*     enemy  = nullptr;  // ← nuevo
    Queue<std::shared_ptr<const Message>>* sendQueue    = nullptr;
    Queue<std::shared_ptr<const Message>>* receiveQueue = nullptr;
    PlayerDto playerDto;
    void loadText();
    void loadAssets();

};


#endif //PRUEBA_SDL_GAME_H
