#include "NameplateComponent.h"

#include "client/sdl/AssetManager.h"
#include "client/sdl/RenderContext.h"

#include <iostream>
#include <utility>

NameplateComponent::NameplateComponent(std::string name,
                                       std::string className,
                                       uint32_t level,
                                       std::string clan,
                                       NameplateType type,
                                       std::string fontId)
    // Movemos strings para evitar copias innecesarias.
    : name(std::move(name)),
      className(std::move(className)),
      level(level),
      clan(std::move(clan)),
      type(type),
      fontId(fontId)
{
}

NameplateComponent::~NameplateComponent()
{
    // Liberamos la textura cacheada al destruir el componente.
    destroyTexture();
}

void NameplateComponent::init()
{
    // Este componente necesita TransformComponent para ubicarse.
    if (entity->hasComponent<TransformComponent>())
    {
        transform = &entity->getComponent<TransformComponent>();
    }
    else
    {
        std::cerr << "[NAMEPLATE] entidad sin TransformComponent" << std::endl;
    }
}

std::vector<std::string> NameplateComponent::buildLines() const
{
    std::vector<std::string> lines;

    // NPC pasivo: solo nombre.
    if (type == NameplateType::PassiveNpc)
    {
        lines.push_back(name);
        return lines;
    }

    // Enemigo hostil: nombre + nivel.
    if (type == NameplateType::Enemy)
    {
        lines.push_back(name + "  Nivel - " + std::to_string(level));
        return lines;
    }

    // Jugadores: nombre + nivel.
    lines.push_back(name + "  Nivel - " + std::to_string(level));

    // Segunda línea: clase.
    if (!className.empty())
    {
        lines.push_back(className);
    }

    // Tercera línea: clan.
    if (!clan.empty())
    {
        lines.push_back(clan);
    }

    return lines;
}

SDL_Color NameplateComponent::textColor() const
{
    switch (type)
    {
    case NameplateType::LocalPlayer:
        return SDL_Color{120, 220, 255, 255};

    case NameplateType::RemotePlayer:
        return SDL_Color{255, 255, 255, 255};

    case NameplateType::Enemy:
        return SDL_Color{255, 80, 80, 255};

    case NameplateType::PassiveNpc:
        return SDL_Color{120, 180, 255, 255};
    }

    return SDL_Color{255, 255, 255, 255};
}

void NameplateComponent::destroyTexture()
{
    for (TextLine &line : textLines)
    {
        if (line.texture != nullptr)
        {
            SDL_DestroyTexture(line.texture);
            line.texture = nullptr;
        }

        line.width = 0;
        line.height = 0;
    }

    textLines.clear();
}

void NameplateComponent::rebuildTexture(RenderContext &context)
{
    if (!dirty)
    {
        return;
    }

    destroyTexture();

    TTF_Font *font = context.assets.GetFont(fontId);

    if (font == nullptr)
    {
        std::cerr << "[NAMEPLATE] fuente no encontrada id=" << fontId << std::endl;
        dirty = false;
        return;
    }
    const SDL_Color color = textColor();
    const std::vector<std::string> lines = buildLines();

    for (const std::string &lineText : lines)
    {
        if (lineText.empty())
        {
            continue;
        }

        SDL_Surface *surface = TTF_RenderUTF8_Blended(
            font,
            lineText.c_str(),
            color);

        if (surface == nullptr)
        {
            std::cerr << "[NAMEPLATE] error creando surface: "
                      << TTF_GetError()
                      << std::endl;
            continue;
        }

        SDL_Texture *texture = SDL_CreateTextureFromSurface(
            context.renderer,
            surface);

        if (texture == nullptr)
        {
            std::cerr << "[NAMEPLATE] error creando texture: "
                      << SDL_GetError()
                      << std::endl;
            SDL_FreeSurface(surface);
            continue;
        }

        TextLine line{};
        line.texture = texture;
        line.width = surface->w;
        line.height = surface->h;

        textLines.push_back(line);

        SDL_FreeSurface(surface);
    }

    dirty = false;
}

void NameplateComponent::draw(RenderContext &context)
{
    if (transform == nullptr)
        return;

    rebuildTexture(context);

    if (textLines.empty())
        return;

    int spriteLeft = static_cast<int>(transform->position.x - context.camera.x);
    int spriteTop = static_cast<int>(transform->position.y - context.camera.y + context.mapOffsetY);
    int spriteWidth = 0;

    if (entity->hasComponent<SpriteComponent>())
    {
        const SDL_Rect &dest = entity->getComponent<SpriteComponent>().getDestRect();
        if (dest.w > 0)
        {
            spriteLeft = dest.x;
            spriteTop = dest.y;
            spriteWidth = dest.w;
        }
    }

    constexpr int lineSpacing = 2;
    constexpr int marginAboveSprite = 6;

    int totalHeight = 0;
    for (const TextLine &line : textLines)
        totalHeight += line.height;
    totalHeight += static_cast<int>(textLines.size() - 1) * lineSpacing;

    int currentY = spriteTop - totalHeight - marginAboveSprite;

    for (const TextLine &line : textLines)
    {
        if (line.texture == nullptr)
            continue;

        SDL_Rect dst{};
        dst.w = line.width;
        dst.h = line.height;
        dst.x = spriteLeft + (spriteWidth / 2) - (line.width / 2);
        dst.y = currentY;

        SDL_RenderCopy(context.renderer, line.texture, nullptr, &dst);
        currentY += line.height + lineSpacing;
    }
}

void NameplateComponent::setLevel(uint32_t newLevel)
{
    // Si el nivel no cambió, no hay que reconstruir la textura.
    if (level == newLevel)
    {
        return;
    }

    // Guardamos el nuevo nivel.
    level = newLevel;

    // Marcamos la textura como sucia para que se regenere en el próximo draw().
    dirty = true;
}

void NameplateComponent::setClan(const std::string &newClan)
{
    // Si el clan no cambió, no hay que reconstruir la textura.
    if (clan == newClan)
    {
        return;
    }

    // Guardamos el nuevo clan. Si viene vacío, el componente deja de dibujarlo.
    clan = newClan;

    // Marcamos la textura como sucia para regenerar las líneas visibles.
    dirty = true;
}