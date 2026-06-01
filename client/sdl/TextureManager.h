
#ifndef PRUEBA_SDL_TEXTUREMANAGER_H
#define PRUEBA_SDL_TEXTUREMANAGER_H
#pragma once
#include <SDL2/SDL_image.h>
#include <SDL2/SDL.h>
#include <map>
#include <string>

class TextureManager {
public:
    explicit TextureManager(SDL_Renderer* renderer);

    SDL_Texture* loadTexture(const char* path);
    void Draw(SDL_Texture* tex, const SDL_Rect& src, const SDL_Rect& dest, SDL_RendererFlip flip);
    ~TextureManager();
    
private:
    SDL_Renderer* renderer;
     std::map<std::string, SDL_Texture*> textureCache;
};




#endif //PRUEBA_SDL_TEXTUREMANAGER_H
