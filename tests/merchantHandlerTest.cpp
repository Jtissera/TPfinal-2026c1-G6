#include <gtest/gtest.h>
#include "server/city/merchantHandler.h"
#include "helpers/playerBuilder.h"
#include "helpers/tomlBuilder.h"

class MerchantHandlerTest : public ::testing::Test
{
protected:
    toml::table config{TomlBuilder::withCityPrices()};
    ItemRepository itemRepo{config};
    MerchantHandler handler{itemRepo, config};
};

TEST_F(MerchantHandlerTest, CompraArma)
{
    Player player = PlayerBuilder().withId(1).withGold(500).build();
    auto result = handler.handleBuy(player, "espada");
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(player.getInventory().hasItem("espada"));
    EXPECT_EQ(player.getGold(), 350u); // 500 - 150
}

TEST_F(MerchantHandlerTest, CompraConOroInsuficienteFalla)
{
    Player player = PlayerBuilder().withId(1).withGold(10).build();
    auto result = handler.handleBuy(player, "espada"); // cuesta 150
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(player.getGold(), 10u);
}

TEST_F(MerchantHandlerTest, VendeItemDelInventario)
{
    Player player = PlayerBuilder().withId(1).withItem("espada").build();
    auto result = handler.handleSell(player, "espada");
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(player.getInventory().hasItem("espada"));
    EXPECT_EQ(player.getGold(), 75u); // 150 / 2
}

TEST_F(MerchantHandlerTest, VenderItemInexistenteFalla)
{
    Player player = PlayerBuilder().withId(1).build();
    auto result = handler.handleSell(player, "espada");
    EXPECT_FALSE(result.ok);
}

TEST_F(MerchantHandlerTest, CompraPocion)
{
    Player player = PlayerBuilder().withId(1).withGold(100).build();
    auto result = handler.handleBuy(player, "pocion_vida");
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(player.getInventory().hasItem("pocion_vida"));
}