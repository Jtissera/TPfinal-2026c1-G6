
#ifndef PRUEBA_SDL_TEXTUREMANAGER_H
#define PRUEBA_SDL_TEXTUREMANAGER_H

#include <SDL2/SDL_image.h>
#include <SDL2/SDL.h>
#include <map>
#include <string>

class TextureManager {
public:
    static SDL_Texture* loadTexture(const char* filename);
    static void Draw(SDL_Texture* tex, SDL_Rect src, SDL_Rect dest, SDL_RendererFlip flip);

private:
    static std::map<std::string, SDL_Texture*> textureCache;
};




#endif //PRUEBA_SDL_TEXTUREMANAGER_H
