//
// Created by mauro on 25/5/26.
//

#ifndef TALLER_TP_HUD_H
#define TALLER_TP_HUD_H
#include <SDL_render.h>

#include "sdl/AssetManager.h"



class HUD {



public:
    explicit HUD(SDL_Renderer* renderer,AssetManager*, PlayerDto* player );
private:
    SDL_Renderer* renderer;
    AssetManager* manager;
    PlayerDto* player;
};

#endif //TALLER_TP_HUD_H
