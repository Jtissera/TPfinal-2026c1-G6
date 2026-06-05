#include <gtest/gtest.h>
#include "server/city/bankerHandler.h"
#include "server/bank/bankRepository.h"
#include "helpers/playerBuilder.h"

class BankerHandlerTest : public ::testing::Test
{
protected:
    BankRepository bankRepo;
    BankerHandler handler{bankRepo};
};

TEST_F(BankerHandlerTest, DepositaItemDelInventario)
{
    Player player = PlayerBuilder().withId(1).withItem("espada").build();
    auto result = handler.handleDeposit(player, "espada");
    EXPECT_TRUE(result.ok);
    EXPECT_FALSE(player.getInventory().findItem(1));
    EXPECT_EQ(bankRepo.get(1).getItems().size(), 1u);
}

TEST_F(BankerHandlerTest, DepositaItemInexistenteFalla)
{
    Player player = PlayerBuilder().withId(1).build();
    auto result = handler.handleDeposit(player, "espada");
    EXPECT_FALSE(result.ok);
}

TEST_F(BankerHandlerTest, RetiraItemDelBanco)
{
    Player player = PlayerBuilder().withId(1).build();
    Item item;
    item.instanceId = 1;
    item.typeName = "espada";
    item.slot = ItemSlot::WEAPON;
    bankRepo.get(1).depositItem(std::move(item));
    auto result = handler.handleWithdraw(player, "espada");
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(player.getInventory().findItem(1));
}

TEST_F(BankerHandlerTest, RetiraItemInexistenteFalla)
{
    Player player = PlayerBuilder().withId(1).build();
    auto result = handler.handleWithdraw(player, "espada");
    EXPECT_FALSE(result.ok);
}

TEST_F(BankerHandlerTest, DepositaOro)
{
    Player player = PlayerBuilder().withId(1).withGold(500).build();
    auto result = handler.handleDepositGold(player, 200);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(player.getGold(), 300u);
    EXPECT_EQ(bankRepo.get(1).getGold(), 200u);
}

TEST_F(BankerHandlerTest, DepositaOroInsuficienteFalla)
{
    Player player = PlayerBuilder().withId(1).withGold(50).build();
    auto result = handler.handleDepositGold(player, 200);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(player.getGold(), 50u);
}

TEST_F(BankerHandlerTest, RetiraOroParcial)
{
    Player player = PlayerBuilder().withId(1).build();
    bankRepo.get(1).depositGold(300);
    auto result = handler.handleWithdrawGold(player, 100);
    EXPECT_TRUE(result.ok);
    EXPECT_EQ(player.getGold(), 100u);
    EXPECT_EQ(bankRepo.get(1).getGold(), 200u);
}

TEST_F(BankerHandlerTest, RetiraOroSinFondosFalla)
{
    Player player = PlayerBuilder().withId(1).build();
    auto result = handler.handleWithdrawGold(player, 100);
    EXPECT_FALSE(result.ok);
}

TEST_F(BankerHandlerTest, BancoIndependientePorJugador)
{
    Player p1 = PlayerBuilder().withId(1).withGold(500).build();
    Player p2 = PlayerBuilder().withId(2).build();
    handler.handleDepositGold(p1, 300);
    EXPECT_EQ(bankRepo.get(1).getGold(), 300u);
    EXPECT_EQ(bankRepo.get(2).getGold(), 0u);
}