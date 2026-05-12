#include <iostream>
#include <exception>

#include <SDL2pp/SDL2pp.hh>
#include <SDL2/SDL.h>

#include "client_client.h"

using namespace SDL2pp;

int main(int argc, char* argv[]) try {
    
        if (argc != 3) {
            std::cerr << "Bad program call. Expected " << argv[0] << " <hostname> <servname>\n";
            return 1;
        }

    
    std::cout << "[Client] Argentum Online client starting..." << std::endl;

    SDL sdl(SDL_INIT_VIDEO);

    Window window(
        "Argentum Online",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480,
        SDL_WINDOW_RESIZABLE
    );

    Renderer renderer(window, -1, SDL_RENDERER_ACCELERATED);

    renderer.SetDrawColor(0, 0, 0, 255);
    renderer.Clear();
    renderer.Present();

    char* hostname = argv[1];
    char* servname = argv[2];
    
    Client client(hostname, servname);
    client.run();


    std::cout << "[Client] Window open. Closing in 3 seconds..." << std::endl;
    SDL_Delay(3000);

    return 0;
} catch (std::exception& e) {
    std::cerr << "[Client] Error: " << e.what() << std::endl;
    return 1;
}

