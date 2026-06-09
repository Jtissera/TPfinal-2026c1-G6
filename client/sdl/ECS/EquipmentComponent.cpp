#include "EquipmentComponent.h"

#include <iostream>
#include <utility>

#include "SpriteComponent.h"
#include "client/sdl/AssetManager.h"
#include "client/sdl/RenderContext.h"

EquipmentComponent::EquipmentComponent(AssetManager &assets, std::string race)
    : assets(assets),
      race(std::move(race))
{
}

void EquipmentComponent::setWeapon(std::optional<ItemView> item)
{
    weapon = std::move(item);
}

void EquipmentComponent::setArmor(std::optional<ItemView> item)
{
    armor = std::move(item);
    applyArmorToSprite();
}

void EquipmentComponent::setHelmet(std::optional<ItemView> item)
{
    helmet = std::move(item);
    applyHelmetToSprite();
}

void EquipmentComponent::setShield(std::optional<ItemView> item)
{
    shield = std::move(item);
}

void EquipmentComponent::clear()
{
    weapon.reset();
    armor.reset();
    helmet.reset();
    shield.reset();

    applyArmorToSprite();
    applyHelmetToSprite();
}

void EquipmentComponent::setFromDto(const EquipmentDto &dto, const ItemCatalog &itemCatalog)
{
    std::optional<ItemView> newWeapon;
    std::optional<ItemView> newArmor;
    std::optional<ItemView> newHelmet;
    std::optional<ItemView> newShield;

    try
    {
        if (dto.weaponCatalogId != 0)
        {
            newWeapon = itemCatalog.requireById(static_cast<int>(dto.weaponCatalogId));
        }

        if (dto.armorCatalogId != 0)
        {
            newArmor = itemCatalog.requireById(static_cast<int>(dto.armorCatalogId));
        }

        if (dto.helmetCatalogId != 0)
        {
            newHelmet = itemCatalog.requireById(static_cast<int>(dto.helmetCatalogId));
        }

        if (dto.shieldCatalogId != 0)
        {
            newShield = itemCatalog.requireById(static_cast<int>(dto.shieldCatalogId));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "[EQUIPMENT_COMPONENT] Error cargando equipo: "
                  << e.what()
                  << std::endl;
        return;
    }

    weapon = std::move(newWeapon);
    armor = std::move(newArmor);
    helmet = std::move(newHelmet);
    shield = std::move(newShield);

    applyArmorToSprite();
    applyHelmetToSprite();
}

const std::optional<ItemView> &EquipmentComponent::getWeapon() const
{
    return weapon;
}

const std::optional<ItemView> &EquipmentComponent::getArmor() const
{
    return armor;
}

const std::optional<ItemView> &EquipmentComponent::getHelmet() const
{
    return helmet;
}

const std::optional<ItemView> &EquipmentComponent::getShield() const
{
    return shield;
}

std::string EquipmentComponent::visualTextureForRace(const ItemView &item) const
{

    if (isShortRace() && !item.visualTextureIdShort.empty())
    {
        return item.visualTextureIdShort;
    }

    if (!isShortRace() && !item.visualTextureIdTall.empty())
    {
        return item.visualTextureIdTall;
    }

    return item.visualTextureId;
}

SDL_Point EquipmentComponent::visualOffsetForRace(const ItemView &item) const
{
    if (isShortRace())
    {
        return SDL_Point{item.visualShortOffsetX, item.visualShortOffsetY};
    }

    return SDL_Point{item.visualTallOffsetX, item.visualTallOffsetY};
}

void EquipmentComponent::applyArmorToSprite()
{
    if (entity == nullptr || !entity->hasComponent<SpriteComponent>())
    {
        return;
    }

    auto &sprite = entity->getComponent<SpriteComponent>();

    if (armor.has_value())
    {
        const ItemView &armorItem = armor.value();

        const std::string textureId = visualTextureForRace(armorItem);

        SDL_Texture *texture = assets.GetTexture(textureId);

        if (texture == nullptr)
        {
            std::cerr << "[EQUIP ARMOR] textura no cargada: "
                      << textureId
                      << " race="
                      << race
                      << std::endl;
            return;
        }

        sprite.setSpriteTextureAndConfig(textureId, armorSpriteConfig());
        return;
    }

    sprite.setSpriteTextureAndConfig("body_sheet", assets.bodyConfigForRace(race));
}

void EquipmentComponent::applyHelmetToSprite()
{
    if (entity == nullptr || !entity->hasComponent<SpriteComponent>())
    {
        return;
    }

    auto &sprite = entity->getComponent<SpriteComponent>();

    if (!helmet.has_value())
    {
        sprite.clearHelmet();
        return;
    }

    const ItemView &helmetItem = helmet.value();

    // Por ahora la cabeza/casco quedan en SpriteComponent.
    // EquipmentComponent solo decide cuándo setear o limpiar.
    sprite.setHelmetTexture(
        helmetItem.visualTextureId,
        helmetItem.visualOffsetX,
        helmetItem.visualOffsetY,
        helmetItem.iconSrcW,
        helmetItem.iconSrcH,
        helmetItem.visualDownSrcX,
        helmetItem.visualDownSrcY,
        helmetItem.visualLeftSrcX,
        helmetItem.visualLeftSrcY,
        helmetItem.visualRightSrcX,
        helmetItem.visualRightSrcY,
        helmetItem.visualUpSrcX,
        helmetItem.visualUpSrcY);
}

void EquipmentComponent::drawEquipmentLayer(
    RenderContext &context,
    const ItemView &item)
{
    if (entity == nullptr || !entity->hasComponent<SpriteComponent>())
    {
        return;
    }

    const std::string textureId = visualTextureForRace(item);

    if (textureId.empty())
    {
        return;
    }

    SDL_Texture *texture = assets.GetTexture(textureId);

    if (texture == nullptr)
    {
        std::cerr << "[EQUIPMENT_COMPONENT] Textura no encontrada: "
                  << textureId
                  << std::endl;
        return;
    }

    auto &sprite = entity->getComponent<SpriteComponent>();

    const SDL_Rect &playerSrc = sprite.getSrcRect();
    const SDL_Rect &playerDest = sprite.getDestRect();

    SDL_Rect itemSrc{
        playerSrc.x - sprite.getStartX(),
        playerSrc.y - sprite.getStartY(),
        playerSrc.w,
        playerSrc.h};

    const SpriteSheetConfig cfg = assets.bodyConfigForRace(race);
    SDL_Point offset = visualOffsetForRace(item);

    SDL_Rect itemDest{
        playerDest.x + offset.x,
        playerDest.y + offset.y,
        playerSrc.w * cfg.scale,
        playerSrc.h * cfg.scale};

    SDL_RenderCopyEx(
        context.renderer,
        texture,
        &itemSrc,
        &itemDest,
        0,
        nullptr,
        sprite.spriteFlip);
}

void EquipmentComponent::draw(RenderContext &context)
{
    drawFront(context);
}

bool EquipmentComponent::isShortRace() const
{
    return race == "Dwarf" ||
           race == "Gnome" ||
           race == "Enano" ||
           race == "Gnomo" ||
           race == "DWARF" ||
           race == "GNOME";
}

SpriteSheetConfig EquipmentComponent::armorSpriteConfig() const
{
    if (!armor.has_value())
    {
        return assets.bodyConfigForRace(race);
    }

    const ItemView &armorItem = armor.value();

    return SpriteSheetConfig{
        27, // ancho frame armadura
        47, // alto frame armadura
        2,  // escala
        0,  // startX
        0,  // startY
        isShortRace() ? armorItem.visualShortOffsetX : armorItem.visualTallOffsetX,
        isShortRace() ? armorItem.visualShortOffsetY : armorItem.visualTallOffsetY};
}
bool EquipmentComponent::shouldDrawWeaponBehind() const
{
    if (entity == nullptr || !entity->hasComponent<SpriteComponent>())
    {
        return false;
    }

    const auto &sprite = entity->getComponent<SpriteComponent>();

    return sprite.getAnimationIndex() == 1 ||
           sprite.getAnimationIndex() == 2;
}

bool EquipmentComponent::shouldDrawShieldBehind() const
{
    if (entity == nullptr || !entity->hasComponent<SpriteComponent>())
    {
        return false;
    }

    const auto &sprite = entity->getComponent<SpriteComponent>();

    return sprite.getAnimationIndex() == 1 ||
           sprite.getAnimationIndex() == 3;
}

void EquipmentComponent::drawBehind(RenderContext &context)
{
    if (shield.has_value() && shouldDrawShieldBehind())
    {
        drawEquipmentLayer(context, shield.value());
    }

    if (weapon.has_value() && shouldDrawWeaponBehind())
    {
        drawEquipmentLayer(context, weapon.value());
    }
}

void EquipmentComponent::drawFront(RenderContext &context)
{
    if (shield.has_value() && !shouldDrawShieldBehind())
    {
        drawEquipmentLayer(context, shield.value());
    }

    if (weapon.has_value() && !shouldDrawWeaponBehind())
    {
        drawEquipmentLayer(context, weapon.value());
    }
}