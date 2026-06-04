#include <gtest/gtest.h>
#include "helpers/playerBuilder.h"
#include "helpers/tomlBuilder.h"
#include "server/game/items/itemRepository.h"
#include "server/world/gameWorld.h"
#include "server/npc/npcRepository.h"
#include "server/npc/npcFactory.h"
#include "editor/map/mapData.h"
#include "editor/map/tile.h"
#include "common/npcType.h"

// ---------------------------------------------------------------------------
// Fixture: mundo 10x10 todo walkable, zona SAFE, con NPCs de ciudad pintados
// ---------------------------------------------------------------------------

class CityIntegrationTest : public ::testing::Test
{
protected:
    toml::table config{TomlBuilder::withCityPrices()};
    ItemRepository itemRepo{config};
    NpcRepository npcRepo{config};
    NpcFactory npcFactory{npcRepo};

    // Construimos el mundo con un MapData que tiene los NPCs de ciudad
    GameWorld makeWorld()
    {
        MapData map(10, 10);
        for (uint16_t y = 0; y < 10; y++)
            for (uint16_t x = 0; x < 10; x++)
            {
                map.at(x, y).walkable = true;
                map.at(x, y).zone = ZoneType::SAFE;
            }

        // Sacerdote en (5,5), comerciante en (5,6), banquero en (5,7)
        map.at(5, 5).npc = NpcType::PRIEST;
        map.at(5, 6).npc = NpcType::MERCHANT;
        map.at(5, 7).npc = NpcType::BANKER;

        return GameWorld(std::move(map), npcFactory, itemRepo, config);
    }
};

// ---------------------------------------------------------------------------
// Sacerdote
// ---------------------------------------------------------------------------

TEST_F(CityIntegrationTest, SacerdoteCuraJugadorAdyacente)
{
    auto world = makeWorld();
    // Jugador en (4,5): adyacente al sacerdote en (5,5)
    Player p = PlayerBuilder().withId(1).withDamagedHp().atTile(4, 5).build();
    ASSERT_LT(p.getHp(), p.getMaxHp());
    world.addPlayer(std::move(p));

    auto result = world.handleCityInteraction(
        1, NpcType::PRIEST,
        {CityCommand::Type::HEAL, "", 0, false});

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(world.getPlayer(1).getHp(), world.getPlayer(1).getMaxHp());
}

TEST_F(CityIntegrationTest, SacerdoteVendeStaffAdyacente)
{
    auto world = makeWorld();
    Player p = PlayerBuilder().withId(1).withGold(500).atTile(4, 5).build();
    world.addPlayer(std::move(p));

    auto result = world.handleCityInteraction(
        1, NpcType::PRIEST,
        {CityCommand::Type::BUY, "vara_fresno", 0, false});

    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(world.getPlayer(1).getInventory().hasItem("vara_fresno"));
}

TEST_F(CityIntegrationTest, SacerdoteNoVendeArmas)
{
    auto world = makeWorld();
    Player p = PlayerBuilder().withId(1).withGold(500).atTile(4, 5).build();
    world.addPlayer(std::move(p));

    auto result = world.handleCityInteraction(
        1, NpcType::PRIEST,
        {CityCommand::Type::BUY, "espada", 0, false});

    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(world.getPlayer(1).getInventory().hasItem("espada"));
}

// ---------------------------------------------------------------------------
// Comerciante
// ---------------------------------------------------------------------------

TEST_F(CityIntegrationTest, ComercianteVendeArma)
{
    auto world = makeWorld();
    Player p = PlayerBuilder().withId(1).withGold(500).atTile(4, 6).build();
    world.addPlayer(std::move(p));

    auto result = world.handleCityInteraction(
        1, NpcType::MERCHANT,
        {CityCommand::Type::BUY, "espada", 0, false});

    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(world.getPlayer(1).getInventory().hasItem("espada"));
}

TEST_F(CityIntegrationTest, ComercianteNoVendeStaff)
{
    auto world = makeWorld();
    Player p = PlayerBuilder().withId(1).withGold(500).atTile(4, 6).build();
    world.addPlayer(std::move(p));

    auto result = world.handleCityInteraction(
        1, NpcType::MERCHANT,
        {CityCommand::Type::BUY, "vara_fresno", 0, false});

    EXPECT_FALSE(result.ok);
}

TEST_F(CityIntegrationTest, ComercianteCompraItem)
{
    auto world = makeWorld();
    Player p = PlayerBuilder().withId(1).withItem("espada").atTile(4, 6).build();
    world.addPlayer(std::move(p));

    auto result = world.handleCityInteraction(
        1, NpcType::MERCHANT,
        {CityCommand::Type::SELL, "espada", 0, false});

    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(world.getPlayer(1).getInventory().hasItem("espada"));
    EXPECT_GT(world.getPlayer(1).getGold(), 0u);
}

// ---------------------------------------------------------------------------
// Banquero
// ---------------------------------------------------------------------------

TEST_F(CityIntegrationTest, BanqueroDepositaOro)
{
    auto world = makeWorld();
    Player p = PlayerBuilder().withId(1).withGold(300).atTile(4, 7).build();
    world.addPlayer(std::move(p));

    CityCommand cmd;
    cmd.type = CityCommand::Type::DEPOSIT;
    cmd.isGold = true;
    cmd.goldAmount = 200;

    auto result = world.handleCityInteraction(1, NpcType::BANKER, cmd);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(world.getPlayer(1).getGold(), 100u);
}

TEST_F(CityIntegrationTest, BanqueroRetiraOro)
{
    auto world = makeWorld();
    Player p = PlayerBuilder().withId(1).atTile(4, 7).build();
    world.addPlayer(std::move(p));

    // Depositar primero para tener fondos
    CityCommand deposit;
    deposit.type = CityCommand::Type::DEPOSIT;
    deposit.isGold = true;
    deposit.goldAmount = 0; // sin oro, usamos handleDepositGold directo
    // Mejor: agregar oro al banco via handleCityInteraction con un jugador con oro
    Player p2 = PlayerBuilder().withId(2).withGold(500).atTile(4, 7).build();
    world.addPlayer(std::move(p2));

    CityCommand dep;
    dep.type = CityCommand::Type::DEPOSIT;
    dep.isGold = true;
    dep.goldAmount = 300;
    world.handleCityInteraction(2, NpcType::BANKER, dep);

    // Retirar con jugador 2
    CityCommand wit;
    wit.type = CityCommand::Type::WITHDRAW;
    wit.isGold = true;
    wit.goldAmount = 100;
    auto result = world.handleCityInteraction(2, NpcType::BANKER, wit);

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(world.getPlayer(2).getGold(), 300u); // 200 restante + 100 retirado
}

// ---------------------------------------------------------------------------
// Proximidad — getNpcTypeAtTile
// ---------------------------------------------------------------------------

TEST_F(CityIntegrationTest, NpcTypeAtTileDevuelvePriestCorrectamente)
{
    auto world = makeWorld();
    auto npc = world.getNpcTypeAtTile(5, 5);
    ASSERT_TRUE(npc.has_value());
    EXPECT_EQ(*npc, NpcType::PRIEST);
}

TEST_F(CityIntegrationTest, NpcTypeAtTileSinNpcDevuelveNullopt)
{
    auto world = makeWorld();
    auto npc = world.getNpcTypeAtTile(0, 0);
    EXPECT_FALSE(npc.has_value());
}

TEST_F(CityIntegrationTest, NpcTypeAtTileFueraDeMapaDevuelveNullopt)
{
    auto world = makeWorld();
    auto npc = world.getNpcTypeAtTile(99, 99);
    EXPECT_FALSE(npc.has_value());
}