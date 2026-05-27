

#include "TextureManager.h"
#include "../Game.h"
#include <iostream>

TextureManager::TextureManager(SDL_Renderer *renderer) : renderer(renderer){}

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

    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);

    if (tex == nullptr) {
        std::cerr << "ERROR SDL_CreateTextureFromSurface: "
                  << SDL_GetError() << std::endl;
        return nullptr;
    }

    textureCache.emplace(key,tex);
    std::cout << "Textura cargada: " << path << std::endl;
    return tex;
}

void TextureManager::Draw(SDL_Texture* tex,const SDL_Rect& src,const SDL_Rect& dest, SDL_RendererFlip flip) {
    SDL_RenderCopyEx(renderer, tex, &src, &dest, 0.0, nullptr, flip);
}

TextureManager::~TextureManager() {
    for (auto& [_, texture] : textureCache) {
        SDL_DestroyTexture(texture);
    }
}


