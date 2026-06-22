#include "priestHandler.h"
#include <cmath>

PriestHandler::PriestHandler(ItemRepository &itemRepo,
                             ResurrectionSystem &resSystem,
                             const MapData &mapData,
                             const toml::table &config)
    : itemRepo(itemRepo),
      resSystem(resSystem),
      mapData(mapData),
      config(config),
      msPerTile(config["city"]["ms_per_tile_resurrection"].value_or(500u))
{
    const toml::array *arr = config["city"]["priest_catalog"]["items"].as_array();
    if (arr)
    {
        for (const toml::node &node : *arr)
        {
            std::optional<std::string> val = node.value<std::string>();
            if (val.has_value())
                catalog.push_back(val.value());
        }
    }
}

CityResult PriestHandler::handleResurrect(Player &player)
{
    if (!player.isGhost())
        return {false, "No estás muerto."};
    player.resurrect(player.getTileX(), player.getTileY());
    return {true, "Has resucitado."};
}

CityResult PriestHandler::handleRemoteResurrect(Player &player)
{
    if (!player.isGhost())
        return {false, "No estás muerto."};
    if (resSystem.isPending(player.getId()))
        return {false, "Ya estás siendo resucitado."};

    std::optional<std::pair<int, int>> nearest = PriestLocator::findNearest(
        mapData, player.getTileX(), player.getTileY());
    if (!nearest)
        return {false, "No hay sacerdote en este mundo."};

    float dx = static_cast<float>(nearest->first - player.getTileX());
    float dy = static_cast<float>(nearest->second - player.getTileY());
    float dist = std::sqrt(dx * dx + dy * dy);
    float delayMs = dist * static_cast<float>(msPerTile);

    resSystem.enqueue(player.getId(), nearest->first, nearest->second, delayMs);
    player.startResurrection();

    return {true, "Estás siendo llevado ante el sacerdote...",
            static_cast<uint32_t>(delayMs)};
}

CityResult PriestHandler::handleHeal(Player &player)
{
    if (player.isGhost())
        return {false, "Un fantasma no puede ser curado."};
    player.restoreFullHpAndMana();
    return {true, "Has sido curado."};
}

CityResult PriestHandler::handleBuy(Player &player, const std::string &itemName)
{
    if (!isSellable(itemName))
        return {false, "El sacerdote no vende '" + itemName + "'."};

    uint32_t price = priceOf(itemName);
    if (player.getGold() < price)
        return {false, "No tenés suficiente oro."};

    Item item = itemRepo.createItem(itemName);
    if (!player.getInventory().addItem(std::move(item)))
        return {false, "Inventario lleno."};

    player.spendGold(price);
    return {true, "Compraste " + itemName + "."};
}

uint32_t PriestHandler::priceOf(const std::string &itemName) const
{
    return config["city"]["prices"][itemName].value_or(0u);
}

bool PriestHandler::isSellable(const std::string &itemName) const
{
    for (const std::string &name : catalog)
        if (name == itemName)
            return true;
    return false;
}

CityResult PriestHandler::handleList() const
{
    std::string msg =
        "=== Sacerdote ===\n"
        "/curar         — Restaura vida y mana (gratis)\n"
        "/resucitar     — Resurrección remota (gratis)\n"
        "--- A la venta ---\n";

    for (const std::string &itemName : catalog)
    {
        uint32_t price = priceOf(itemName);
        if (price > 0)
            msg += "  /comprar " + itemName + "  (" + std::to_string(price) + " oro)\n";
    }

    return {true, msg};
}