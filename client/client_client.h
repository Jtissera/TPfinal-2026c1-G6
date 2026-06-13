#pragma once

#include <atomic>
#include <string>
#include <SDL2/SDL.h>

#include "sdl/screens/ConfigScreen.h"
#include "ServerWatcher.h"

class Client
{
public:
    Client(const char *hostname, const char *servname,
           SDL_Renderer *renderer, SDL_Window *window,
           int windowW, int windowH);

    ~Client();

    int run();

    Client(const Client &) = delete;
    Client &operator=(const Client &) = delete;

private:
    std::string hostname;
    std::string servname;

    SDL_Renderer *renderer;
    SDL_Window *window;
    int windowW;
    int windowH;

    ClientConfig config;

    std::atomic<bool> serverShutdownDetected{false};

    ServerWatcher serverWatcher;

    static constexpr const char *FONT_PATH = "assets/sprites/MapAssets/arial.ttf";
    static constexpr uint8_t PROTOCOL_VERSION = 0x01;
};