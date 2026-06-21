
#include "HealthBarComponent.h"


#include "HealthBarComponent.h"

#include "client/sdl/AssetManager.h"
#include "client/sdl/RenderContext.h"

#include <algorithm>
#include <iostream>

#include <SDL2/SDL_ttf.h>

HealthBarComponent::HealthBarComponent(int hp, int hpMax)
    : hp(hp), hpMax(hpMax) {}

HealthBarComponent::HealthBarComponent(int hp,
                                       int hpMax,
                                       int barWidth,
                                       int barHeight,
                                       int offsetY)
    : hp(hp),
      hpMax(hpMax),
      barWidth(barWidth),
      barHeight(barHeight),
      offsetY(offsetY) {}

void HealthBarComponent::init()
{
    // Necesitamos el Transform para tener una posición base.
    if (entity->hasComponent<TransformComponent>()) {
        transform = &entity->getComponent<TransformComponent>();
    }

    // Si existe SpriteComponent, usamos su destRect real para ubicar mejor la barra.
    if (entity->hasComponent<SpriteComponent>()) {
        sprite = &entity->getComponent<SpriteComponent>();
    }

    if (transform == nullptr) {
        std::cerr << "[HEALTH BAR] entidad sin TransformComponent" << std::endl;
    }
}

void HealthBarComponent::setHealth(int newHp, int newHpMax)
{
    // Normalizamos valores para evitar divisiones inválidas o barras fuera de rango.
    if (newHpMax < 0) {
        newHpMax = 0;
    }

    if (newHp < 0) {
        newHp = 0;
    }

    if (newHpMax > 0 && newHp > newHpMax) {
        newHp = newHpMax;
    }

    hp = newHp;
    hpMax = newHpMax;
}

void HealthBarComponent::draw(RenderContext& context)
{
    if (transform == nullptr) {
        return;
    }

    // Si no hay vida máxima válida, no dibujamos.
    if (hpMax <= 0) {
        return;
    }

    // Si está muerto, no dibujamos la barra.
    if (hp <= 0) {
        return;
    }

    float ratio = static_cast<float>(hp) / static_cast<float>(hpMax);
    ratio = std::clamp(ratio, 0.0f, 1.0f);

    int entityScreenX = static_cast<int>(transform->position.x - context.camera.x);
    int entityScreenY = static_cast<int>(transform->position.y - context.camera.y + context.mapOffsetY);
    int entityWidth = transform->width * transform->scale;

    // Preferimos el rectángulo real del sprite si está disponible.
    if (sprite != nullptr) {
        const SDL_Rect& spriteRect = sprite->getDestRect();
        entityScreenX = spriteRect.x;
        entityScreenY = spriteRect.y;
        entityWidth = spriteRect.w;
    }

    // Centramos la barra con respecto al sprite.
    SDL_Rect frameRect{};
    frameRect.w = barWidth;
    frameRect.h = barHeight;
    frameRect.x = entityScreenX + (entityWidth / 2) - (barWidth / 2);
    frameRect.y = entityScreenY + offsetY;

    // Dibujamos la textura base del HUD si existe.
    SDL_Texture* frameTexture = context.assets.GetTexture(textureId);

    if (frameTexture != nullptr) {
        SDL_RenderCopy(context.renderer, frameTexture, nullptr, &frameRect);
    } else {
        // Fallback por si la textura no está cargada.
        SDL_SetRenderDrawColor(context.renderer, 0, 0, 0, 255);
        SDL_RenderFillRect(context.renderer, &frameRect);
        SDL_SetRenderDrawColor(context.renderer, 255, 255, 255, 255);
        SDL_RenderDrawRect(context.renderer, &frameRect);
    }

    // Relleno interno proporcional.
    // Dejamos 2 píxeles de margen para no tapar el marco.
    SDL_Rect fillRect{};
    fillRect.x = frameRect.x + 2;
    fillRect.y = frameRect.y + 2;
    fillRect.w = static_cast<int>((barWidth - 4) * ratio);
    fillRect.h = barHeight - 4;

    SDL_SetRenderDrawColor(context.renderer, 200, 0, 0, 255);
    SDL_RenderFillRect(context.renderer, &fillRect);

    // Texto hp/hpMax centrado.
    TTF_Font* font = context.assets.GetFont("ao_regular");

    if (font == nullptr) {
        return;
    }

    const std::string text = std::to_string(hp) + "/" + std::to_string(hpMax);
    const SDL_Color white{255, 255, 255, 255};

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), white);

    if (surface == nullptr) {
        return;
    }

    SDL_Texture* textTexture = SDL_CreateTextureFromSurface(context.renderer, surface);

    if (textTexture == nullptr) {
        SDL_FreeSurface(surface);
        return;
    }

    SDL_Rect textRect{};
    textRect.w = surface->w;
    textRect.h = surface->h;
    textRect.x = frameRect.x + (frameRect.w / 2) - (textRect.w / 2);
    textRect.y = frameRect.y + (frameRect.h / 2) - (textRect.h / 2);

    SDL_FreeSurface(surface);

    SDL_RenderCopy(context.renderer, textTexture, nullptr, &textRect);

    SDL_DestroyTexture(textTexture);
}
