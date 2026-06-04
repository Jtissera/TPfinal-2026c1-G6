

#ifndef TALLER_TP_EQUIPMENTCOMPONENT_H
#define TALLER_TP_EQUIPMENTCOMPONENT_H

#pragma once

#include <optional>
#include <string>

#include "client/sdl/ECS/Components.h"
#include "../items/ItemCatalog.h"
#include "../state/ItemView.h"
#include "../../../common/dtos/equipmentDto.h"

class AssetManager;

class EquipmentComponent : public Component {
private:
    AssetManager& assets;
    std::string race;

    std::optional<ItemView> weapon;
    std::optional<ItemView> armor;
    std::optional<ItemView> helmet;
    std::optional<ItemView> shield;

    std::string visualTextureForRace(const ItemView& item) const;
    SDL_Point visualOffsetForRace(const ItemView& item) const;

    void applyArmorToSprite();
    void applyHelmetToSprite();

    void drawEquipmentLayer(RenderContext& context, const ItemView& item);

    void drawFront(RenderContext& context);




public:
    EquipmentComponent(AssetManager& assets, std::string race);

    void setWeapon(std::optional<ItemView> item);
    void setArmor(std::optional<ItemView> item);
    void setHelmet(std::optional<ItemView> item);
    void setShield(std::optional<ItemView> item);

    void clear();

    void setFromDto(
        const EquipmentDto& dto,
        const ItemCatalog& itemCatalog
    );

    const std::optional<ItemView>& getWeapon() const;
    const std::optional<ItemView>& getArmor() const;
    const std::optional<ItemView>& getHelmet() const;
    const std::optional<ItemView>& getShield() const;
    bool shouldDrawWeaponBehind() const;
    bool shouldDrawShieldBehind() const;

    void draw(RenderContext& context) override;
    bool isShortRace() const;
    SpriteSheetConfig armorSpriteConfig() const;

    void drawBehind(RenderContext& context);
};



#endif //TALLER_TP_EQUIPMENTCOMPONENT_H
