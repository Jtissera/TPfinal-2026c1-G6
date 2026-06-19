#include "NameplateComponent.h"

#include "client/sdl/AssetManager.h"
#include "client/sdl/RenderContext.h"

#include <iostream>
#include <utility>

NameplateComponent::NameplateComponent(std::string name,
                                       std::string className,
                                       uint32_t level,
                                       std::string clan,
                                       NameplateType type)
    // Movemos strings para evitar copias innecesarias.
    : name(std::move(name)),
      className(std::move(className)),
      level(level),
      clan(std::move(clan)),
      type(type) {}

NameplateComponent::~NameplateComponent()
{
    // Liberamos la textura cacheada al destruir el componente.
    destroyTexture();
}

void NameplateComponent::init()
{
    // Este componente necesita TransformComponent para ubicarse.
    if (entity->hasComponent<TransformComponent>()) {
        transform = &entity->getComponent<TransformComponent>();
    } else {
        std::cerr << "[NAMEPLATE] entidad sin TransformComponent" << std::endl;
    }
}

std::string NameplateComponent::buildText() const
{
    // Para enemigos no mostramos clan.
    if (type == NameplateType::Enemy) {
        if (!className.empty()) {
            return name + " | " + className + " | Nivel " + std::to_string(level);
        }

        return name + " | Nivel " + std::to_string(level);
    }

    // Para jugadores mostramos nombre, clase y nivel.
    std::string text = name + " | " + className + " | Nivel " + std::to_string(level);

    // Clan opcional.
    if (!clan.empty()) {
        text += " | " + clan;
    }

    return text;
}

SDL_Color NameplateComponent::textColor() const
{
    // Colores simples y distinguibles.
    // Local: celeste.
    // Remoto: blanco.
    // Enemigo: rojo.
    switch (type) {
    case NameplateType::LocalPlayer:
        return SDL_Color{120, 220, 255, 255};

    case NameplateType::RemotePlayer:
        return SDL_Color{255, 255, 255, 255};

    case NameplateType::Enemy:
        return SDL_Color{255, 80, 80, 255};
    }

    return SDL_Color{255, 255, 255, 255};
}

void NameplateComponent::destroyTexture()
{
    // SDL_Texture debe destruirse manualmente.
    if (textTexture != nullptr) {
        SDL_DestroyTexture(textTexture);
        textTexture = nullptr;
    }

    textWidth = 0;
    textHeight = 0;
}

void NameplateComponent::rebuildTexture(RenderContext& context)
{
    // Si no cambió el texto, no hacemos nada.
    if (!dirty) {
        return;
    }

    // Liberamos la textura anterior antes de crear una nueva.
    destroyTexture();

    // Pedimos la fuente al AssetManager desde RenderContext.
    TTF_Font* font = context.assets.GetFont("ao_regular");

    if (font == nullptr) {
        std::cerr << "[NAMEPLATE] fuente ao_regular no encontrada" << std::endl;
        dirty = false;
        return;
    }

    const std::string text = buildText();
    const SDL_Color color = textColor();

    // Creamos una superficie con SDL_ttf.
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), color);

    if (surface == nullptr) {
        std::cerr << "[NAMEPLATE] error creando surface: "
                  << TTF_GetError()
                  << std::endl;
        dirty = false;
        return;
    }

    // Convertimos la surface en textura renderizable.
    textTexture = SDL_CreateTextureFromSurface(context.renderer, surface);

    if (textTexture == nullptr) {
        std::cerr << "[NAMEPLATE] error creando texture: "
                  << SDL_GetError()
                  << std::endl;
        SDL_FreeSurface(surface);
        dirty = false;
        return;
    }

    // Guardamos tamaño para centrar el texto.
    textWidth = surface->w;
    textHeight = surface->h;

    std::cout << "[NAMEPLATE TEXTURE OK] text=" << text
              << " size=(" << textWidth << "," << textHeight << ")"
              << std::endl;

    // Ya no necesitamos la surface.
    SDL_FreeSurface(surface);

    // La textura quedó actualizada.
    dirty = false;
}

void NameplateComponent::draw(RenderContext& context)
{

    std::cout << "[NAMEPLATE DRAW] name=" << name
          << " level=" << level
          << " transform=" << transform
          << std::endl;

    if (transform == nullptr) {
        return;
    }

    // Reconstruimos textura solo si el texto cambió.
    rebuildTexture(context);

    if (textTexture == nullptr) {
        return;
    }

    // Posición en pantalla.
    // Misma idea general que los sprites:
    // mundo - cámara + offset vertical del mapa.
    const int entityScreenX = static_cast<int>(transform->position.x - context.camera.x);
    const int entityScreenY = static_cast<int>(transform->position.y - context.camera.y + context.mapOffsetY);

    // Ancho visual de la entidad.
    const int entityWidth = transform->width * transform->scale;
    const int entityHeight = transform->height * transform->scale;

    // Dibujamos debajo del personaje.
    SDL_Rect dst{};
    dst.w = textWidth;
    dst.h = textHeight;

    // Centrado respecto del cuerpo.
    dst.x = entityScreenX + (entityWidth / 2) - (textWidth / 2);

    // Debajo del sprite, con pequeño margen.
    dst.y = entityScreenY + entityHeight + 4;

    SDL_RenderCopy(context.renderer, textTexture, nullptr, &dst);
}

void NameplateComponent::setLevel(uint32_t newLevel)
{
    if (level == newLevel) {
        return;
    }

    level = newLevel;
    dirty = true;
}

void NameplateComponent::setClan(const std::string& newClan)
{
    if (clan == newClan) {
        return;
    }

    clan = newClan;
    dirty = true;
}

void NameplateComponent::setName(const std::string& newName)
{
    if (name == newName) {
        return;
    }

    name = newName;
    dirty = true;
}

void NameplateComponent::setClassName(const std::string& newClassName)
{
    if (className == newClassName) {
        return;
    }

    className = newClassName;
    dirty = true;
}