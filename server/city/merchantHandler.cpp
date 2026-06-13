#include "merchantHandler.h"
#include <set>
#include <algorithm>

static const std::set<std::string> MERCHANT_BLACKLIST = {
    "vara_fresno", "flauta_elfica", "baculo_nudoso", "baculo_engarzado"};

MerchantHandler::MerchantHandler(ItemRepository &itemRepo,
                                 const toml::table &config)
    : itemRepo(itemRepo), config(config) {}

CityResult MerchantHandler::handleBuy(Player &player,
                                      const std::string &itemName)
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

CityResult MerchantHandler::handleSell(Player &player,
                                       const std::string &itemName)
{
    auto removed = player.getInventory().removeItemByName(itemName);
    if (!removed)
        return {false, "No tenés '" + itemName + "' en el inventario."};

    uint32_t sellPrice = sellPriceOf(itemName);
    player.addGold(sellPrice);
    return {true, "Vendiste " + itemName + " por " + std::to_string(sellPrice) + " oro."};
}

bool MerchantHandler::isSellable(const std::string &itemName) const
{
    return MERCHANT_BLACKLIST.count(itemName) == 0 && config["city"]["prices"][itemName].value_or(0u) > 0;
}

uint32_t MerchantHandler::priceOf(const std::string &itemName) const
{
    return config["city"]["prices"][itemName].value_or(0u);
}

uint32_t MerchantHandler::sellPriceOf(const std::string &itemName) const
{
    return priceOf(itemName) / 2;
}

CityResult MerchantHandler::handleList() const
{
    std::string msg =
        "=== Comerciante ===\n"
        "/vender <item> — Vende un item (mitad de precio)\n"
        "--- A la venta ---\n";

    const auto *pricesNode = config["city"]["prices"].as_table();
    if (pricesNode)
    {
        std::vector<std::pair<std::string, uint32_t>> items;
        for (const auto &[key, val] : *pricesNode)
        {
            const std::string name = std::string(key.str());
            const uint32_t price = val.value_or(0u);
            if (price > 0 && isSellable(name))
            {
                items.push_back({name, price});
            }
        }
        std::sort(items.begin(), items.end());

        for (const auto &[name, price] : items)
        {
            msg += "  /comprar " + name + "  (" + std::to_string(price) + " oro" + " | venta: " + std::to_string(price / 2) + " oro)\n";
        }
    }

    return {true, msg};
}