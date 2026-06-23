#pragma once

#include "../game/player/Player.h"
#include "../game/items/itemRepository.h"
#include "../resurrection/priestLocator.h"
#include "../resurrection/resurrectionSystem.h"
#include "../../common/network/messages/server/player/resurrectionStartedMessage.h"
#include "cityResult.h"
#include "../../editor/map/mapData.h"
#include <toml++/toml.hpp>
#include <string>
#include <vector>

class PriestHandler
{
public:
    PriestHandler(ItemRepository &itemRepo,
                  ResurrectionSystem &resSystem,
                  const MapData &mapData,
                  const toml::table &config);

    CityResult handleResurrect(Player &player);
    CityResult handleRemoteResurrect(Player &player);
    CityResult handleHeal(Player &player);
    CityResult handleBuy(Player &player, const std::string &itemName);
    CityResult handleList() const;

private:
    ItemRepository &itemRepo;
    ResurrectionSystem &resSystem;
    const MapData &mapData;
    const toml::table &config;
    uint32_t msPerTile;
    std::vector<std::string> catalog;

    uint32_t priceOf(const std::string &itemName) const;
    bool isSellable(const std::string &itemName) const;
};