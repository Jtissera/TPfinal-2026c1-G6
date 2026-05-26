#pragma once

#include <string>
#include <SDL2/SDL.h>

#include "sdl/screens/ConfigScreen.h"

class Client {
public:
    Client(const char* hostname, const char* servname,
           SDL_Renderer* renderer, SDL_Window* window,
           int windowW, int windowH);

    int run();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

private:
    std::string hostname;
    std::string servname;

    SDL_Renderer* renderer;
    SDL_Window*   window;      // necesario para fullscreen / resize
    int windowW;
    int windowH;

    ClientConfig config;       // cargado al iniciar, persistido al guardar

    static constexpr const char* FONT_PATH = "assets/sprites/MapAssets/arial.ttf";
    static constexpr uint8_t PROTOCOL_VERSION = 0x01;
};
