#pragma once
#include "../game/player/Player.h"
#include "../game/items/itemRepository.h"
#include "../resurrection/priestLocator.h"
#include "../resurrection/resurrectionSystem.h"
#include "cityResult.h"
#include "../../editor/map/mapData.h"
#include <toml++/toml.hpp>
#include <string>

class PriestHandler
{
public:
    PriestHandler(ItemRepository &itemRepo,
                  ResurrectionSystem &resSystem,
                  const MapData &mapData,
                  const toml::table &config);

    CityResult handleResurrect(Player &player);       // junto al sacerdote: instantáneo
    CityResult handleRemoteResurrect(Player &player); // fantasma lejos: diferido
    CityResult handleHeal(Player &player);
    CityResult handleBuy(Player &player, const std::string &itemName);

private:
    ItemRepository &itemRepo;
    ResurrectionSystem &resSystem;
    const MapData &mapData;
    uint32_t msPerTile;

    uint32_t priceOf(const std::string &itemName) const;
    bool isSellable(const std::string &itemName) const;

    const toml::table &config;
};