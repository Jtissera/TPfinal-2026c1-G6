#pragma once
#include "../server/game/player/Player.h"
#include "../server/game/stats/raceRepository.h"
#include "../server/game/stats/classRepository.h"
#include "../server/game/items/item.h"
#include "../server/game/items/itemSlot.h"
#include <toml++/toml.h>

class PlayerBuilder
{
public:
    PlayerBuilder &asMage()
    {
        _mage = true;
        return *this;
    }
    PlayerBuilder &withId(uint32_t id)
    {
        _id = id;
        return *this;
    }
    PlayerBuilder &withGold(uint32_t g)
    {
        _gold = g;
        return *this;
    }
    PlayerBuilder &atTile(int x, int y)
    {
        _tx = x;
        _ty = y;
        return *this;
    }

    PlayerBuilder &asGhost()
    {
        _ghost = true;
        return *this;
    }

    PlayerBuilder &withDamagedHp()
    {
        _damaged = true;
        return *this;
    }

    PlayerBuilder &withItem(const std::string &typeName)
    {
        _items.push_back(typeName);
        return *this;
    }

    Player build() const
    {
        static toml::table config = makeConfig();
        RaceRepository raceRepo(config);
        ClassRepository clsRepo(config);

        RaceStats race = raceRepo.get("human");
        ClassStats cls = _mage ? clsRepo.get("mage") : clsRepo.get("warrior");
        int16_t maxMana = _mage ? 100 : 0;
        Player p(_id, "test_" + std::to_string(_id), race, cls, 100, maxMana, config);
        p.setTilePos(_tx, _ty);

        if (_gold > 0)
            p.addGold(_gold);

        for (const auto &typeName : _items)
        {
            Item item;
            item.instanceId = nextItemId++;
            item.typeName = typeName;
            item.slot = ItemSlot::WEAPON;
            p.getInventory().addItem(std::move(item));
        }

        if (_damaged)
            p.takeDamage(50);

        if (_ghost)
            p.die(0); // mata al jugador para que sea ghost

        return p;
    }

private:
    uint32_t _id = 1;
    bool _mage = false;
    uint32_t _gold = 0;
    int _tx = 0;
    int _ty = 0;
    bool _ghost = false;
    bool _damaged = false;
    std::vector<std::string> _items;

    mutable uint32_t nextItemId = 100;

    static toml::table makeConfig()
    {
        return toml::parse(R"(
        [races.human]
        health       = 1.0
        mana         = 1.0
        recovery     = 1.0
        constitution = 10
        intelligence = 10
        strength     = 10
        agility      = 10

        [classes.warrior]
        health        = 1.5
        mana          = 0.0
        meditation    = 0.0
        can_use_magic = false

        [classes.mage]
        health        = 0.8
        mana          = 1.5
        meditation    = 2.0
        can_use_magic = true

        [combat]
        attack_range        = 1
        max_level_diff      = 10
        ranged_attack_range = 10

        [player]
        newbie_max_level    = 12
        max_inventory_items = 20

        [gold]
        gold_level_base     = 100
        gold_level_exponent = 1.1
        overflow_drop       = 0.20

        [initial_inventory.warrior]
        items = []

        [initial_inventory.mage]
        items = []
    )");
    }
};