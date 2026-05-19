
#include "GameObject.h"
#include "TextureManager.h"
#include <iostream>

#include "Game.h"

GameObject::GameObject(const char *textureSheet,int x, int y) {

    objTexture = TextureManager::loadTexture(textureSheet);
    xpos = x;
    ypos = y;

}

GameObject::~GameObject() {
}

void GameObject::Update() {
    // Movemos el objeto en diagonal.
    xpos++;
    ypos++;

    // srcRect: recorta el frame real del sprite sheet (no tocar)
    srcRect.x = 0;
    srcRect.y = 0;
    srcRect.w = 156;
    srcRect.h = 208;

    // destRect: acá controlás el tamaño visible en pantalla
    destRect.x = xpos;
    destRect.y = ypos;
    destRect.w = 48;  // ← tamaño en pantalla (escalado a ~1.5 tiles)
    destRect.h = 64;  // ← 2 tiles de alto
}

void GameObject::Render() {
    // Si no hay textura cargada, no hay nada que dibujar.
    if (objTexture == nullptr) {
        std::cout << "No se renderiza porque objTexture es nullptr" << std::endl;
        return;
    }

    // Dibujamos toda la textura, sin recorte.
    int result = SDL_RenderCopy(Game::renderer, objTexture, &srcRect, &destRect);

    // Si SDL_RenderCopy falla, mostramos el error.
    if (result != 0) {
        std::cout << "Error en SDL_RenderCopy: "
                  << SDL_GetError() << std::endl;
    }
}