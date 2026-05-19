
#include "TileComponent.h"
#include "../TextureManager.h"
#include "../Game.h"

TileComponent::TileComponent(int srcX, int srcY, int xpos, int ypos,
                             int tsize, int tscale, const std::string& id) {
    texture    = Game::assets->GetTexture(id);
    srcRect    = {srcX, srcY, tsize, tsize};
    position.x = static_cast<float>(xpos);
    position.y = static_cast<float>(ypos);
    destRect.w = destRect.h = tsize * tscale;
}

TileComponent::~TileComponent() {
    // La textura la administra AssetManager, no la destruimos acá
}

void TileComponent::update() {
    destRect.x = static_cast<int>(position.x - Game::camera.x);
    destRect.y = static_cast<int>(position.y - Game::camera.y);
}

void TileComponent::draw() {
    TextureManager::Draw(texture, srcRect, destRect, SDL_FLIP_NONE);
}
