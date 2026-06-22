#include "merchantHandler.h"
#include <algorithm>

MerchantHandler::MerchantHandler(ItemRepository &itemRepo,
                                 const toml::table &config)
    : itemRepo(itemRepo), config(config)
{
    const toml::array *arr = config["city"]["merchant_blacklist"]["items"].as_array();
    if (arr)
    {
        for (const toml::node &node : *arr)
        {
            std::optional<std::string> val = node.value<std::string>();
            if (val)
                blacklist.push_back(*val);
        }
    }
}

bool MerchantHandler::isBlacklisted(const std::string &itemName) const
{
    for (const std::string &name : blacklist)
        if (name == itemName)
            return true;
    return false;
}

bool MerchantHandler::isSellable(const std::string &itemName) const
{
    return !isBlacklisted(itemName) &&
           config["city"]["prices"][itemName].value_or(0u) > 0;
}

uint32_t MerchantHandler::priceOf(const std::string &itemName) const
{
    return config["city"]["prices"][itemName].value_or(0u);
}

uint32_t MerchantHandler::sellPriceOf(const std::string &itemName) const
{
    return priceOf(itemName) / 2;
}

CityResult MerchantHandler::handleBuy(Player &player, const std::string &itemName)
{
    if (!isSellable(itemName))
        return {false, "El comerciante no vende '" + itemName + "'."};

    uint32_t price = priceOf(itemName);
    if (player.getGold() < price)
        return {false, "No tenés suficiente oro."};

    Item item = itemRepo.createItem(itemName);
    if (!player.getInventory().addItem(std::move(item)))
        return {false, "Inventario lleno."};

    player.spendGold(price);
    return {true, "Compraste " + itemName + "."};
}

CityResult MerchantHandler::handleSell(Player &player, const std::string &itemName)
{
    std::optional<Item> removed = player.getInventory().removeItemByName(itemName);
    if (!removed)
        return {false, "No tenés '" + itemName + "' en el inventario."};

    uint32_t sellPrice = sellPriceOf(itemName);
    player.addGold(sellPrice);
    return {true, "Vendiste " + itemName + " por " + std::to_string(sellPrice) + " oro."};
}

CityResult MerchantHandler::handleList() const
{
    std::string msg =
        "=== Comerciante ===\n"
        "/vender <item> — Vende un item (mitad de precio)\n"
        "--- A la venta ---\n";

    const toml::table *pricesNode = config["city"]["prices"].as_table();
    if (pricesNode)
    {
        std::vector<std::pair<std::string, uint32_t>> items;
        for (const auto &entry : *pricesNode)
        {
            const std::string name = std::string(entry.first.str());
            const toml::node &node = entry.second;
            const std::optional<uint32_t> maybePrice = node.value<uint32_t>();
            if (!maybePrice.has_value() || maybePrice.value() == 0)
                continue;
            if (isSellable(name))
                items.push_back({name, maybePrice.value()});
        }
        std::sort(items.begin(), items.end());

        for (const std::pair<std::string, uint32_t> &item : items)
        {
            msg += "  /comprar " + item.first +
                   "  (" + std::to_string(item.second) + " oro" +
                   " | venta: " + std::to_string(item.second / 2) + " oro)\n";
        }
    }

    return {true, msg};
}