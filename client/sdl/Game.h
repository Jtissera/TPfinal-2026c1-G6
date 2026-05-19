
#ifndef PRUEBA_SDL_GAME_H
#define PRUEBA_SDL_GAME_H
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include "ECS/ECS.h"
#include "AssetManager.h"
#include "Collision.h"
#include <vector>

#include "Map.h"


class Game {
public:
    Game();
    ~Game();

    void init(const char* title, int width, int height, bool fullscreen);
    void handleEvents();
    void update();
    void render();
    void clean();
    bool running() const;

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
    Entity*     enemy  = nullptr;  // ← nuevo
};


#endif //PRUEBA_SDL_GAME_H
