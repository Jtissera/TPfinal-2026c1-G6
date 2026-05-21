
#ifndef PRUEBA_SDL_GAMEOBJECT_H
#define PRUEBA_SDL_GAMEOBJECT_H
#include <SDL2/SDL.h>

#include "../Game.h"

class GameObject {

private:
    int xpos;
    int ypos;
    SDL_Texture* objTexture;
    SDL_Rect srcRect,destRect;
    SDL_Renderer* renderer;

public:
    GameObject(const char* textureSheet,int x,int y);
    ~GameObject();

    void Update();
    void Render();

};



#endif //PRUEBA_SDL_GAMEOBJECT_H
