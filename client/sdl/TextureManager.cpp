
#include "TextureManager.h"
#include "../Game.h"
#include <iostream>

std::map<std::string, SDL_Texture*> TextureManager::textureCache;

SDL_Texture* TextureManager::loadTexture(const char* path) {
    std::string key(path);
    
    auto it = textureCache.find(key);
    if (it != textureCache.end()) {
        return it->second;
    }

    SDL_Surface* surface = IMG_Load(path);
    if (!surface) {
        std::cout << "ERROR IMG_Load: " << path << " - " << IMG_GetError() << std::endl;
        return nullptr;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(Game::renderer, surface);
    SDL_FreeSurface(surface);

    if (!tex) {
        std::cout << "ERROR CreateTexture: " << SDL_GetError() << std::endl;
        return nullptr;
    }

    textureCache[key] = tex;
    std::cout << "Textura cargada: " << path << std::endl;
    return tex;
}

void TextureManager::Draw(SDL_Texture* tex, SDL_Rect src, SDL_Rect dest, SDL_RendererFlip flip) {
    SDL_RenderCopyEx(Game::renderer, tex, &src, &dest, 0.0, nullptr, flip);
}


