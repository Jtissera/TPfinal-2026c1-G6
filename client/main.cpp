#include <iostream>
#include <exception>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

#include "client_client.h"

static constexpr int WINDOW_W = 1080;
static constexpr int WINDOW_H = 640;

int main(int argc, char* argv[])
try {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " <hostname> <servname>\n";
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL_Init: " << SDL_GetError() << "\n";
        return 1;
    }
    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init: " << TTF_GetError() << "\n";
        SDL_Quit();
        return 1;
    }

    // SDL_mixer — opcional: si falla, el juego corre sin audio
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) == -1)
        std::cerr << "[Audio] Mix_OpenAudio: " << Mix_GetError() << " (sin audio)\n";

    SDL_Window* window = SDL_CreateWindow(
        "Argentum Online",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_W, WINDOW_H,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow: " << SDL_GetError() << "\n";
        Mix_CloseAudio(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer: " << SDL_GetError() << "\n";
        SDL_DestroyWindow(window);
        Mix_CloseAudio(); TTF_Quit(); SDL_Quit();
        return 1;
    }

    int ret = 0;
    {
        Client client(argv[1], argv[2], renderer, window, WINDOW_W, WINDOW_H);
        ret = client.run();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    Mix_CloseAudio();
    TTF_Quit();
    SDL_Quit();
    return ret;
}
catch (const std::exception& e) {
    std::cerr << "[Client] Error fatal: " << e.what() << "\n";
    return 1;
}
