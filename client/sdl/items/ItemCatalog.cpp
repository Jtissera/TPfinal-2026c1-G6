
#include "ItemCatalog.h"
#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>
#include <stdexcept>

void ItemCatalog::loadFromJson(const std::string& path) {
    std::ifstream file(path);

    if (!file.is_open()) {
        throw std::runtime_error("ItemCatalog: no se pudo abrir el archivo: " + path);
    }

    nlohmann::json data;
    file >> data;

    if (!data.contains("items") || !data["items"].is_array()) {
        throw std::runtime_error("ItemCatalog: JSON invalido, falta array 'items'");
    }

    for (const auto& itemJson : data["items"]) {
        ItemView item;

        item.itemId = itemJson.value("id", 0);
        item.itemName = itemJson.value("name", "");

        item.textureId = itemJson.value("textureId", "");
        item.visualTextureId = itemJson.value("visualTextureId", "");

        item.type = parseItemType(itemJson.value("type", "other"));

        item.quantity = itemJson.value("quantity", 1);

        item.damageMin = itemJson.value("damageMin", 0);
        item.damageMax = itemJson.value("damageMax", 0);

        item.defenseMin = itemJson.value("defenseMin", 0);
        item.defenseMax = itemJson.value("defenseMax", 0);

        item.manaCost = itemJson.value("manaCost", 0);

        item.healAmount = itemJson.value("healAmount", 0);
        item.manaAmount = itemJson.value("manaAmount", 0);

        item.ranged = itemJson.value("ranged", false);

        item.soundId = itemJson.value("soundId", "");
        item.iconSrcX = itemJson.value("iconSrcX", 0);
        item.iconSrcY = itemJson.value("iconSrcY", 1);
        item.iconSrcW = itemJson.value("iconSrcW", 32);
        item.iconSrcH = itemJson.value("iconSrcH", 32);

        if (item.itemId <= 0) {
            throw std::runtime_error("ItemCatalog: item con id invalido en " + path);
        }

        addItem(item);
    }
}

void ItemCatalog::addItem(const ItemView& item) {
    itemsById[item.itemId] = item;
}

bool ItemCatalog::contains(int itemId) const {
    return itemsById.find(itemId) != itemsById.end();
}

const ItemView* ItemCatalog::getById(int itemId) const {
    auto it = itemsById.find(itemId);

    if (it == itemsById.end()) {
        return nullptr;
    }

    return &it->second;
}

const ItemView& ItemCatalog::requireById(int itemId) const {
    const ItemView* item = getById(itemId);

    if (item == nullptr) {
        throw std::runtime_error("ItemCatalog: item id not found: " + std::to_string(itemId));
    }

    return *item;
}

ClientItemType ItemCatalog::parseItemType(const std::string& type) const {
    if (type == "melee_weapon") {
        return ClientItemType::MeleeWeapon;
    }

    if (type == "ranged_weapon") {
        return ClientItemType::RangedWeapon;
    }

    if (type == "magic_weapon") {
        return ClientItemType::MagicWeapon;
    }

    if (type == "armor") {
        return ClientItemType::Armor;
    }

    if (type == "helmet") {
        return ClientItemType::Helmet;
    }

    if (type == "shield") {
        return ClientItemType::Shield;
    }

    if (type == "health_potion") {
        return ClientItemType::HealthPotion;
    }

    if (type == "mana_potion") {
        return ClientItemType::ManaPotion;
    }

    return ClientItemType::Other;
}