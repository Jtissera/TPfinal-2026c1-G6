#pragma once
#include <toml++/toml.h>

class TomlBuilder
{
public:
    static toml::table withCityPrices()
    {
        return toml::parse(R"(
        [city]
        ms_per_tile_resurrection = 500

        [city.prices]
        vara_fresno      = 100
        flauta_elfica    = 500
        baculo_nudoso    = 300
        baculo_engarzado = 800
        pocion_vida      = 50
        pocion_mana      = 50
        espada           = 150
        hacha            = 200
        martillo         = 180
        arco_simple      = 120
        arco_compuesto   = 400
        armadura_cuero   = 200
        armadura_placas  = 600
        tunica_azul      = 250
        capucha          = 80
        casco_hierro     = 180
        escudo_tortuga   = 100
        escudo_hierro    = 220
        sombrero_magico  = 350

        [items.espada]
        slot = "WEAPON"
        is_ranged = false
        damage_min = 2
        damage_max = 5

        [items.hacha]
        slot = "WEAPON"
        is_ranged = false
        damage_min = 4
        damage_max = 5

        [items.martillo]
        slot = "WEAPON"
        is_ranged = false
        damage_min = 1
        damage_max = 9

        [items.arco_simple]
        slot = "WEAPON"
        is_ranged = true
        damage_min = 1
        damage_max = 4

        [items.arco_compuesto]
        slot = "WEAPON"
        is_ranged = true
        damage_min = 4
        damage_max = 16

        [items.vara_fresno]
        slot = "STAFF"
        is_ranged = true
        damage_min = 2
        damage_max = 4
        mana_cost = 5

        [items.flauta_elfica]
        slot = "STAFF"
        is_ranged = true
        mana_cost = 100
        effect = "heal"

        [items.baculo_nudoso]
        slot = "STAFF"
        is_ranged = true
        damage_min = 4
        damage_max = 8
        mana_cost = 15

        [items.baculo_engarzado]
        slot = "STAFF"
        is_ranged = true
        damage_min = 8
        damage_max = 20
        mana_cost = 30

        [items.armadura_cuero]
        slot = "ARMOR"
        defense_min = 2
        defense_max = 6

        [items.armadura_placas]
        slot = "ARMOR"
        defense_min = 15
        defense_max = 30

        [items.tunica_azul]
        slot = "ARMOR"
        defense_min = 6
        defense_max = 10

        [items.capucha]
        slot = "HELMET"
        defense_min = 1
        defense_max = 4

        [items.casco_hierro]
        slot = "HELMET"
        defense_min = 4
        defense_max = 8

        [items.escudo_tortuga]
        slot = "SHIELD"
        defense_min = 1
        defense_max = 2

        [items.escudo_hierro]
        slot = "SHIELD"
        defense_min = 1
        defense_max = 4

        [items.sombrero_magico]
        slot = "HELMET"
        defense_min = 4
        defense_max = 12

        [items.pocion_vida]
        slot = "CONSUMABLE"
        heal_amount = 100

        [items.pocion_mana]
        slot = "CONSUMABLE"
        mana_amount = 100

        [npcs.goblin]
        hp                 = 50
        damage_min         = 2
        damage_max         = 5
        level              = 3
        agility            = 8
        strength           = 5
        detection_range    = 5
        home_range         = 10
        attack_cooldown_ms = 1000
        move_cooldown_ms   = 500
        zones              = ["COMBAT"]

        [combat]
        attack_range        = 1
        max_level_diff      = 10
        ranged_attack_range = 10

        [player]
        newbie_max_level    = 12
        max_inventory_items = 20

        [npc]
        spawn_interval_ticks = 200
        max_population       = 20
        spawn_batch_size     = 4

        [world]
        tile_size = 96

        [cavern]
        gold_multiplier = 1.5
        xp_multiplier   = 1.5
        item_multiplier = 1.5

        [dungeon]
        gold_multiplier = 3.0
        xp_multiplier   = 2.5
        item_multiplier = 2.5

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
    )");
    }
};