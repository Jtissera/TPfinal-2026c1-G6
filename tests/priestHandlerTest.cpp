#include <gtest/gtest.h>
#include "server/city/priestHandler.h"
#include "server/game/items/itemRepository.h"
#include "server/resurrection/resurrectionSystem.h"
#include "editor/map/mapData.h"
#include "helpers/playerBuilder.h"
#include "helpers/tomlBuilder.h"
#include "common/npcType.h"

class PriestHandlerTest : public ::testing::Test
{
protected:
    toml::table config{TomlBuilder::withCityPrices()};
    ItemRepository itemRepo{config};
    MapData map{10, 10};
    ResurrectionSystem resSystem;
    PriestHandler handler{itemRepo, resSystem, map, config};
};

TEST_F(PriestHandlerTest, ResucitaGhost)
{
    Player player = PlayerBuilder().withId(1).asGhost().build();
    auto result = handler.handleResurrect(player);
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(player.isGhost());
}

TEST_F(PriestHandlerTest, ResucitarVivoFalla)
{
    Player player = PlayerBuilder().withId(1).build();
    auto result = handler.handleResurrect(player);
    EXPECT_FALSE(result.ok);
    EXPECT_TRUE(player.isAlive());
}

TEST_F(PriestHandlerTest, CuraHpYMana)
{
    Player player = PlayerBuilder().withId(1).asMage().withDamagedHp().build();
    EXPECT_LT(player.getHp(), player.getMaxHp());
    handler.handleHeal(player);
    EXPECT_EQ(player.getHp(), player.getMaxHp());
    EXPECT_EQ(player.getMana(), player.getMaxMana());
}

TEST_F(PriestHandlerTest, CurarFantasmaFalla)
{
    Player player = PlayerBuilder().withId(1).asGhost().build();
    auto result = handler.handleHeal(player);
    EXPECT_FALSE(result.ok);
}

TEST_F(PriestHandlerTest, VendeStaff)
{
    Player player = PlayerBuilder().withId(1).withGold(500).build();
    auto result = handler.handleBuy(player, "vara_fresno");
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(player.getInventory().hasItem("vara_fresno"));
}

TEST_F(PriestHandlerTest, VendePocion)
{
    Player player = PlayerBuilder().withId(1).withGold(100).build();
    auto result = handler.handleBuy(player, "pocion_vida");
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(player.getInventory().hasItem("pocion_vida"));
}

TEST_F(PriestHandlerTest, NoVendeArmas)
{
    Player player = PlayerBuilder().withId(1).withGold(500).build();
    auto result = handler.handleBuy(player, "espada");
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(player.getInventory().hasItem("espada"));
}

TEST_F(PriestHandlerTest, ResucitarRemotoEncolaConDelay)
{
    map.at(9, 9).npc = NpcType::PRIEST;
    Player player = PlayerBuilder().withId(1).asGhost().atTile(0, 0).build();
    auto result = handler.handleRemoteResurrect(player);
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(resSystem.isPending(1));
    EXPECT_TRUE(player.isResurrecting());
}

TEST_F(PriestHandlerTest, ResucitarRemotoDelayProporcionalADistancia)
{
    ItemRepository itemRepo2{config};
    ResurrectionSystem sys1, sys2;

    MapData map1(10, 10);
    map1.at(1, 1).npc = NpcType::PRIEST;
    PriestHandler h1{itemRepo2, sys1, map1, config};

    MapData map2(10, 10);
    map2.at(9, 9).npc = NpcType::PRIEST;
    PriestHandler h2{itemRepo2, sys2, map2, config};

    Player playerCerca = PlayerBuilder().withId(1).asGhost().atTile(0, 0).build();
    Player playerLejos = PlayerBuilder().withId(2).asGhost().atTile(0, 0).build();

    h1.handleRemoteResurrect(playerCerca);
    h2.handleRemoteResurrect(playerLejos);

    int completados1 = 0, completados2 = 0;
    sys1.tick(800.0f, [&](uint32_t, int, int)
              { completados1++; });
    sys2.tick(800.0f, [&](uint32_t, int, int)
              { completados2++; });

    EXPECT_EQ(completados1, 1);
    EXPECT_EQ(completados2, 0);
}

TEST_F(PriestHandlerTest, ResucitarRemotoDosVecesFalla)
{
    map.at(9, 9).npc = NpcType::PRIEST;
    Player player = PlayerBuilder().withId(1).asGhost().atTile(0, 0).build();
    handler.handleRemoteResurrect(player);
    auto result = handler.handleRemoteResurrect(player);
    EXPECT_FALSE(result.ok);
}

TEST_F(PriestHandlerTest, SinSacerdoteEnMapaFalla)
{
    Player player = PlayerBuilder().withId(1).asGhost().atTile(0, 0).build();
    auto result = handler.handleRemoteResurrect(player);
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(resSystem.isPending(1));
}