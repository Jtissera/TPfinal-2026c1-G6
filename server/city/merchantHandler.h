#pragma once
#include "../game/player/Player.h"
#include "../game/items/itemRepository.h"
#include "cityResult.h"
#include <toml++/toml.hpp>
#include <string>
#include <vector>

class MerchantHandler
{
public:
    MerchantHandler(ItemRepository &itemRepo, const toml::table &config);

    CityResult handleBuy(Player &player, const std::string &itemName);
    CityResult handleSell(Player &player, const std::string &itemName);
    CityResult handleList() const;

private:
    ItemRepository &itemRepo;
    const toml::table &config;

    bool isSellable(const std::string &itemName) const;
    uint32_t priceOf(const std::string &itemName) const;
    uint32_t sellPriceOf(const std::string &itemName) const; // 50% del precio base
};