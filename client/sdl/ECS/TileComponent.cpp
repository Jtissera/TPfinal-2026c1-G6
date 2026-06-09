#include "TileComponent.h"
#include "../TextureManager.h"
#include "../../Game.h"

TileComponent::TileComponent(AssetManager &assets, int srcX, int srcY, int xpos, int ypos,
                             int srcW, int srcH, int tscale, const std::string &id)
{
    texture = assets.GetTexture(id);

    // Ahora guardamos el ancho y alto real del recorte de origen
    srcRect = {srcX, srcY, srcW, srcH};

    position.x = static_cast<float>(xpos);
    position.y = static_cast<float>(ypos);

    // Escalamos el ancho y el alto respetando sus proporciones
    destRect.w = srcW * tscale;
    destRect.h = srcH * tscale;
}

TileComponent::~TileComponent()
{
    // La textura la administra AssetManager, no la destruimos acá
}

void TileComponent::update(UpdateContext &context)
{
    // los tiles son estáticos, solo necesitan recalcular destRect
    // cuando la cámara se movió. En frames donde el jugador no se mueve,
    // esto evita 300 operaciones aritméticas innecesarias.
    if (!context.cameraMoved)
        return;

    destRect.x = static_cast<int>(position.x - context.camera.x);
    destRect.y = static_cast<int>(position.y - context.camera.y) + 133;
}

void TileComponent::draw(RenderContext &context)
{
    const int visibleLeft = context.viewport.x;
    const int visibleRight = context.viewport.x + context.viewport.w;
    const int visibleTop = context.viewport.y;
    const int visibleBottom = context.viewport.y + context.viewport.h;

    const bool outsideScreen =
        destRect.x + destRect.w < visibleLeft ||
        destRect.x > visibleRight ||
        destRect.y + destRect.h < visibleTop ||
        destRect.y > visibleBottom;

    if (outsideScreen)
    {
        return;
    }

    context.textureManager.Draw(texture, srcRect, destRect, SDL_FLIP_NONE);
}
